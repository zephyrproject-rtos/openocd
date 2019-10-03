/***************************************************************************
 *   Copyright (C) 2017 by Intel Corporation                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <helper/time_support.h>
#include <jtag/jtag.h>

#include "helper/log.h"
#include "helper/types.h"
#include "rtos.h"
#include "rtos_standard_stackings.h"
#include "target/target.h"
#include "target/target_type.h"

struct Zephyr_thread {
	uint32_t ptr, next_ptr;
	uint32_t entry;
	uint32_t stack_pointer;
	uint8_t state;
	uint8_t user_options;
	int8_t prio;
};

enum Zephyr_offsets {
       OFFSET_VERSION,
       OFFSET_K_CURR_THREAD,
       OFFSET_K_THREADS,
       OFFSET_T_ENTRY,
       OFFSET_T_NEXT_THREAD,
       OFFSET_T_STATE,
       OFFSET_T_USER_OPTIONS,
       OFFSET_T_PRIO,
       OFFSET_T_STACK_POINTER,
       OFFSET_MAX
};

struct Zephyr_params {
	const char *target_name;
	uint8_t size_width;
	uint8_t pointer_width;
	uint32_t offsets[OFFSET_MAX];
	const struct rtos_register_stacking *reg_stacking;
};

static struct Zephyr_params Zephyr_params_list[] = {
	{
		.target_name = "cortex_m",
		.pointer_width = 4,
		.reg_stacking = &rtos_standard_Cortex_M4F_FPU_stacking,
	},
	{
		.target_name = NULL
	}
};

static bool Zephyr_detect_rtos(struct target *target);
static int Zephyr_create(struct target *target);
static int Zephyr_update_threads(struct rtos *rtos);
static int Zephyr_get_thread_reg_list(struct rtos *rtos, int64_t thread_id,
		struct rtos_reg **reg_list, int *num_regs);
static int Zephyr_get_symbol_list_to_lookup(symbol_table_elem_t *symbol_list[]);

struct rtos_type Zephyr_rtos = {
	.name = "Zephyr",

	.detect_rtos = Zephyr_detect_rtos,
	.create = Zephyr_create,
	.update_threads = Zephyr_update_threads,
	.get_thread_reg_list = Zephyr_get_thread_reg_list,
	.get_symbol_list_to_lookup = Zephyr_get_symbol_list_to_lookup,
};

enum Zephyr_symbol_values {
	Zephyr_VAL__kernel,
	Zephyr_VAL__kernel_openocd_offsets,
	Zephyr_VAL__kernel_openocd_size_t_size,
	Zephyr_VAL_COUNT
};

static const symbol_table_elem_t Zephyr_symbol_list[] = {
	{ .symbol_name = "_kernel", .optional = false },
	{ .symbol_name = "_kernel_openocd_offsets", .optional = false },
	{ .symbol_name = "_kernel_openocd_size_t_size", .optional = false },
	{ }
};

struct array {
	void *ptr;
	size_t elements;
};

static void array_init(struct array *array)
{
	array->ptr = NULL;
	array->elements = 0;
}

static void array_free(struct array *array)
{
	free(array->ptr);
	array_init(array);
}

static void *array_append(struct array *array, size_t size)
{
	if (!(array->elements % 16)) {
		void *ptr = realloc(array->ptr, (array->elements + 16) * size);

		if (!ptr)
			return NULL;

		array->ptr = ptr;
	}

	return (unsigned char *)array->ptr + (array->elements++) * size;
}

static void *array_detach_ptr(struct array *array)
{
	void *ptr = array->ptr;

	array_init(array);

	return ptr;
}

static uint32_t Zephyr_kptr(const struct rtos *rtos, enum Zephyr_offsets off)
{
	const struct Zephyr_params *params = rtos->rtos_specific_params;

	return rtos->symbols[Zephyr_VAL__kernel].address + params->offsets[off];
}

static int Zephyr_fetch_thread(const struct rtos *rtos, struct Zephyr_thread *thread,
	uint32_t ptr)
{
	const struct Zephyr_params *param = rtos->rtos_specific_params;
	int retval;

	thread->ptr = ptr;

	retval = target_read_u32(rtos->target, ptr + param->offsets[OFFSET_T_ENTRY],
		&thread->entry);
	if (retval != ERROR_OK)
		return retval;

	retval = target_read_u32(rtos->target, ptr + param->offsets[OFFSET_T_NEXT_THREAD],
		&thread->next_ptr);
	if (retval != ERROR_OK)
		return retval;

	retval = target_read_u32(rtos->target, ptr + param->offsets[OFFSET_T_STACK_POINTER],
		&thread->stack_pointer);
	if (retval != ERROR_OK)
		return retval;

	retval = target_read_u8(rtos->target, ptr + param->offsets[OFFSET_T_STATE],
		&thread->state);
	if (retval != ERROR_OK)
		return retval;

	retval = target_read_u8(rtos->target, ptr + param->offsets[OFFSET_T_USER_OPTIONS],
		&thread->user_options);
	if (retval != ERROR_OK)
		return retval;

	uint8_t prio;
	retval = target_read_u8(rtos->target, ptr + param->offsets[OFFSET_T_PRIO], &prio);
	if (retval != ERROR_OK)
		return retval;
	thread->prio = (int8_t)prio;

	LOG_DEBUG("Fetched thread%x: {entry@0x%x, state=%d, useropts=%d, prio=%d}",
		ptr, thread->entry, thread->state, thread->user_options, thread->prio);

	return 0;
}

static int Zephyr_fetch_thread_list(struct rtos *rtos, uint32_t current_thread)
{
	struct array thread_array;
	struct Zephyr_thread thread;
	int64_t curr_id;
	uint32_t curr;
	int retval;

	retval = target_read_u32(rtos->target, Zephyr_kptr(rtos, OFFSET_K_THREADS),
		&curr);
	if (retval != ERROR_OK) {
		LOG_ERROR("Could not fetch current thread pointer");
		return -1;
	}

	array_init(&thread_array);

	for (curr_id = -1; curr; curr = thread.next_ptr) {
		retval = Zephyr_fetch_thread(rtos, &thread, curr);
		if (retval < 0)
			goto error;

		struct thread_detail *td = array_append(&thread_array, sizeof(*td));
		if (!td)
			goto error;

		td->threadid = thread.ptr;
		td->exists = true;

		if (asprintf(&td->thread_name_str, "thr_%x_%x", thread.entry, thread.ptr) < 0)
			goto error;
		if (asprintf(&td->extra_info_str, "prio:%d,useropts:%d", thread.prio, thread.user_options) < 0)
			goto error;

		if (td->threadid == current_thread)
			curr_id = (int64_t)thread_array.elements - 1;
	}

	LOG_DEBUG("Got information for %zu threads", thread_array.elements);

	rtos_free_threadlist(rtos);

	rtos->thread_count = (int)thread_array.elements;
	rtos->thread_details = array_detach_ptr(&thread_array);

	rtos->current_threadid = curr_id;
	rtos->current_thread = current_thread;

	return 0;

error:
	{
		struct thread_detail *td = thread_array.ptr;
		for (size_t i = 0; i < thread_array.elements; i++) {
			free(td[i].thread_name_str);
			free(td[i].extra_info_str);
		}
	}
	array_free(&thread_array);

	return -1;
}

static int Zephyr_update_threads(struct rtos *rtos)
{
	struct Zephyr_params *param;
	int retval;

	if (rtos->rtos_specific_params == NULL)
		return ERROR_FAIL;

	param = (struct Zephyr_params *)rtos->rtos_specific_params;

	if (rtos->symbols == NULL) {
		LOG_ERROR("No symbols for Zephyr");
		return ERROR_FAIL;
	}

	if (rtos->symbols[Zephyr_VAL__kernel].address == 0) {
		LOG_ERROR("Can't obtain kernel struct from Zephyr");
		return ERROR_FAIL;
	}
	if (rtos->symbols[Zephyr_VAL__kernel_openocd_offsets].address == 0) {
		LOG_ERROR("Please build Zephyr with CONFIG_OPENOCD option set");
		return ERROR_FAIL;
	}

	retval = target_read_u8(rtos->target,
		rtos->symbols[Zephyr_VAL__kernel_openocd_size_t_size].address,
		&param->size_width);
	if (retval != ERROR_OK) {
		LOG_ERROR("Couldn't determine size of size_t from host");
		return retval;
	}
	if (param->size_width != 4) {
		LOG_ERROR("Only size_t of 4 bytes are supported");
		return ERROR_FAIL;
	}

	/* We can fetch the whole array for version 0, as they're supposed
	 * to grow only */
	uint32_t address = rtos->symbols[Zephyr_VAL__kernel_openocd_offsets].address;
	for (size_t i = 0; i < OFFSET_MAX; i++, address += param->size_width) {
		retval = target_read_u32(rtos->target, address, &param->offsets[i]);
		if (retval != ERROR_OK) {
			LOG_ERROR("Could not fetch offsets from Zephyr");
			return ERROR_FAIL;
		}
	}
	if (param->offsets[OFFSET_VERSION] != 0) {
		LOG_ERROR("Expecting OpenOCD support version 0, got %d instead",
			param->offsets[OFFSET_VERSION]);
		return ERROR_FAIL;
	}

	LOG_DEBUG("Zephyr OpenOCD support version %d", param->offsets[OFFSET_VERSION]);

	uint32_t current_thread;
	retval = target_read_u32(rtos->target,
		Zephyr_kptr(rtos, OFFSET_K_CURR_THREAD), &current_thread);
	if (retval != ERROR_OK) {
		LOG_ERROR("Could not obtain current thread ID");
		return retval;
	}

	retval = Zephyr_fetch_thread_list(rtos, current_thread);
	if (retval < 0) {
		LOG_ERROR("Could not obtain thread list");
		return retval;
	}

	return ERROR_OK;
}

static int Zephyr_get_thread_reg_list(struct rtos *rtos, int64_t thread_id,
		struct rtos_reg **reg_list, int *num_regs)
{
	struct Zephyr_thread thr;
	struct Zephyr_params *params;
	int retval;

	LOG_INFO("Getting thread %ld reg list", thread_id);

	if (rtos == NULL)
		return ERROR_FAIL;

	if (thread_id == 0)
		return ERROR_FAIL;

	params = rtos->rtos_specific_params;
	if (params == NULL)
		return ERROR_FAIL;

	retval = Zephyr_fetch_thread(rtos, &thr, thread_id);
	if (retval < 0)
		return retval;

	return rtos_generic_stack_read(rtos->target, params->reg_stacking,
		thr.stack_pointer, reg_list, num_regs);
}

static int Zephyr_get_symbol_list_to_lookup(symbol_table_elem_t **symbol_list)
{
	*symbol_list = malloc(sizeof(Zephyr_symbol_list));
	if (!*symbol_list)
		return ERROR_FAIL;

	memcpy(*symbol_list, Zephyr_symbol_list, sizeof(Zephyr_symbol_list));
	return ERROR_OK;
}

static bool Zephyr_detect_rtos(struct target *target)
{
	if (target->rtos->symbols == NULL) {
		LOG_INFO("Zephyr: no symbols while detecting RTOS");
		return false;
	}

	for (enum Zephyr_symbol_values symbol = Zephyr_VAL__kernel;
					symbol != Zephyr_VAL_COUNT; symbol++) {
		LOG_INFO("Zephyr: does it have symbol %d (%s)?", symbol,
			target->rtos->symbols[symbol].optional?"optional":"mandatory");

		if (target->rtos->symbols[symbol].optional)
			continue;
		if (target->rtos->symbols[symbol].address == 0)
			return false;
	}

	LOG_INFO("Zephyr: all mandatory symbols found");

	return true;
}

static int Zephyr_create(struct target *target)
{
	struct Zephyr_params *p;

	LOG_INFO("Zephyr: looking for target: %s", target->type->name);

	for (p = Zephyr_params_list; p->target_name; p++) {
		if (!strcmp(p->target_name, target->type->name)) {
			LOG_INFO("Zephyr: target known, params at %p", p);
			target->rtos->rtos_specific_params = p;
			return ERROR_OK;
		}
	}

	LOG_ERROR("Could not find target in Zephyr compatibility list");
	return ERROR_FAIL;
}
