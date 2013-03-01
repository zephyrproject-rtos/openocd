/*
 * Copyright (C) 2012 Synopsys, Inc. (www.synopsys.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *   Maintainer: frank.dols@synopsys.com
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "jtag/interface.h"
#include "breakpoints.h"
#include "algorithm.h"
#include "target_request.h"
#include "target_type.h"
#include "register.h"
#include <helper/time_support.h>

#include "arc32.h"
#include "arc700.h"
#include "arc_jtag.h"

static int PRINT = 1;


/* ----- Supporting functions ---------------------------------------------- */

static int arc700_init_arch_info(struct target *target,
	struct arc700_common *arc700, struct jtag_tap *tap)
{
	int retval = ERROR_OK;
	struct arc32_common *arc32 = &arc700->arc32;

	LOG_DEBUG("  >>> Calling into <<<");

	arc700->common_magic = ARC700_COMMON_MAGIC;

	/* initialize arc700 specific info */
	retval = arc32_init_arch_info(target, arc32, tap);
	arc32->arch_info = arc700;

	return retval;
}



/* ----- Target command handler functions ---------------------------------- */

int arc700_poll(struct target *target)
{
	int retval = ERROR_OK;
	uint32_t status;
	struct arc32_common *arc32 = target_to_arc32(target);

	LOG_DEBUG(">> Entering <<");

	retval = arc_jtag_status(&arc32->jtag_info, &status);
	if (retval != ERROR_OK)
		return retval;

	/* check for processor halted */
	if (status & ARC_JTAG_STAT_RU) {
		printf("target is still running !!\n");
		target->state = TARGET_RUNNING;
	} else {
		if ((target->state == TARGET_RUNNING) ||
			(target->state == TARGET_RESET)) {

			target->state = TARGET_HALTED;
			printf("ARC core is halted or in reset.\n");

			retval = arc32_debug_entry(target);
			if (retval != ERROR_OK)
				return retval;

			target_call_event_callbacks(target, TARGET_EVENT_HALTED);
		} else if (target->state == TARGET_DEBUG_RUNNING) {

			target->state = TARGET_HALTED;
			printf("ARC core is in debug running.\n");

			retval = arc32_debug_entry(target);
			if (retval != ERROR_OK)
				return retval;

			target_call_event_callbacks(target, TARGET_EVENT_DEBUG_HALTED);
		} else {
			if (PRINT) {
				printf(" OpenOCD for ARC is ready to accept:"
					" (gdb) target remote localhost:3333\n");
				PRINT = 0;
			}
		}
	}

	return retval;
}

int arc700_arch_state(struct target *target)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	return retval;
}

int arc700_target_request_data(struct target *target,
	uint32_t size, uint8_t *buffer)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	return retval;
}

int arc700_halt(struct target *target)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	return retval;
}

int arc700_resume(struct target *target, int current,
	uint32_t address, int handle_breakpoints, int debug_execution)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	return retval;
}

int arc700_step(struct target *target, int current,
	uint32_t address, int handle_breakpoints)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	return retval;
}

int arc700_assert_reset(struct target *target)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	return retval;
}


int arc700_deassert_reset(struct target *target)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	return retval;
}

int arc700_soft_reset_halt(struct target *target)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	return retval;
}

int arc700_get_gdb_reg_list(struct target *target, struct reg **reg_list[],
	int *reg_list_size)
{
	int retval = ERROR_OK;
	int i;
	struct arc32_common *arc32 = target_to_arc32(target);

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	/* get pointers to arch-specific information */
	*reg_list_size = ARC32_NUM_CORE_REGS + ARC32_NUM_FP_REGS;
printf(" @ln:%d)\n",__LINE__);
	*reg_list = malloc(sizeof(struct reg *) * (*reg_list_size));
printf(" @ln:%d)\n",__LINE__);

	for (i = 0; i < ARC32_NUM_CORE_REGS; i++)
		(*reg_list)[i] = &arc32->core_cache->reg_list[i];
printf(" @ln:%d)\n",__LINE__);

	/* add dummy floating points regs ?? arc32.c build-reg-chache() */
	for (i = ARC32_NUM_CORE_REGS; i < (ARC32_NUM_CORE_REGS + ARC32_NUM_FP_REGS); i++)
		(*reg_list)[i] = &arc32_gdb_dummy_fp_reg;
printf(" @ln:%d)\n",__LINE__);

	return retval;
}

int arc700_read_memory(struct target *target, uint32_t address,
	uint32_t size, uint32_t count, uint8_t *buffer)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	return retval;
}

int arc700_write_memory(struct target *target, uint32_t address,
	uint32_t size, uint32_t count, const uint8_t *buffer)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	return retval;
}

int arc700_bulk_write_memory(struct target *target, uint32_t address,
	uint32_t count, const uint8_t *buffer)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	return retval;
}

int arc700_checksum_memory(struct target *target,
	uint32_t address, uint32_t count, uint32_t *checksum)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	return retval;
}

int arc700_blank_check_memory(struct target *target,
	uint32_t address, uint32_t count, uint32_t *blank)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	return retval;
}

int arc700_run_algorithm(struct target *target,
	int num_mem_params, struct mem_param *mem_params,
	int num_reg_params, struct reg_param *reg_params,
	uint32_t entry_point, uint32_t exit_point,
	int timeout_ms, void *arch_info)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	return retval;
}

int arc700_start_algorithm(struct target *target,
	int num_mem_params, struct mem_param *mem_params,
	int num_reg_params, struct reg_param *reg_params,
	uint32_t entry_point, uint32_t exit_point,
	void *arch_info)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	return retval;
}

int arc700_wait_algorithm(struct target *target,
	int num_mem_params, struct mem_param *mem_params,
	int num_reg_params, struct reg_param *reg_params,
	uint32_t exit_point, int timeout_ms,
	void *arch_info)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	return retval;
}

int arc700_add_breakpoint(struct target *target, struct breakpoint *breakpoint)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	return retval;
}

int arc700_remove_breakpoint(struct target *target,
	struct breakpoint *breakpoint)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	return retval;
}

int arc700_add_watchpoint(struct target *target, struct watchpoint *watchpoint)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	return retval;
}

int arc700_remove_watchpoint(struct target *target,
	struct watchpoint *watchpoint)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	return retval;
}

int arc700_target_create(struct target *target, Jim_Interp *interp)
{
	int retval = ERROR_OK;

	LOG_DEBUG(">> Entering <<");

	struct arc700_common *arc700 = calloc(1, sizeof(struct arc700_common));

	retval = arc700_init_arch_info(target, arc700, target->tap);

	return retval;
}

int arc700_init_target(struct command_context *cmd_ctx,
	struct target *target)
{
	int retval = ERROR_OK;

	LOG_DEBUG(">> Entering <<");

	arc32_build_reg_cache(target);

	return retval;
}

int arc700_examine(struct target *target)
{
	int retval = ERROR_OK;
	uint32_t value, status;
	struct arc32_common *arc32 = target_to_arc32(target);

	LOG_DEBUG(">> Entering <<");

	retval = arc_jtag_startup(&arc32->jtag_info);

	if (!target_was_examined(target)) {

		/* read JTAG info */
		arc_jtag_idcode(&arc32->jtag_info, &value);
		printf("JTAG ID: 0x%x\n", value);
		arc_jtag_status(&arc32->jtag_info, &status);
		printf("JTAG status: 0x%x\n", status);

		/* bring processor into HALT */
		printf("bring ARC core into HALT state.\n");
		value = SET_CORE_FORCE_HALT;
		arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_DEBUG_REG, &value);
		value = SET_CORE_FORCE_HALT;
		arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_DEBUG_REG, &value);
		sleep(2); /* just give us once some time */
		arc_jtag_status(&arc32->jtag_info, &status);
		printf("JTAG status: 0x%x\n", status);

		/* read ARC core info */
		arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_IDENTITY_REG, &value);
		printf("CPU ID: 0x%x\n", value);
		arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_PC_REG, &value);
		printf("current PC: 0x%x\n", value);

		arc32_print_core_registers(&arc32->jtag_info);
		arc32_print_aux_registers(&arc32->jtag_info);

		arc_jtag_status(&arc32->jtag_info, &status);
		if (status & ARC_JTAG_STAT_RU) {
			printf("target is still running !!\n");
			target->state = TARGET_RUNNING;
		} else {
			printf("target is halted.\n");
			target->state = TARGET_RESET; /* means HALTED after restart */
		}

		target_set_examined(target);
	}

	return retval;
}

/* ----- Command handlers -------------------------------------------------- */


COMMAND_HANDLER(handle_arc700_smp_gdb_command)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	struct target *target = get_current_target(CMD_CTX);

	struct target_list *head;
	head = target->head;

	if (head != (struct target_list *)NULL) {
		if (CMD_ARGC == 1) {
			int coreid = 0;
			COMMAND_PARSE_NUMBER(int, CMD_ARGV[0], coreid);
			if (ERROR_OK != retval)
				return retval;
			target->gdb_service->core[1] = coreid;
		}
		command_print(CMD_CTX, "gdb coreid  %d -> %d",
			target->gdb_service->core[0], target->gdb_service->core[1]);
	}

	return retval;
}

/* ----- Supported commands ------------------------------------------------ */

static const struct command_registration arc700_exec_command_handlers[] = {
	/* See: src/target/cortex_m3.c  ..._exec_command_handlers[] */
	{
		.name = "maskisr",
		.handler = NULL, //handle_arc700_mask_interrupts_command,
		.mode = COMMAND_EXEC,
		.help = "mask arc700 interrupts",
		.usage = "['auto'|'on'|'off']",
	},
	{
		.name = "vector_catch",
		.handler = NULL, //handle_arc700_vector_catch_command,
		.mode = COMMAND_EXEC,
		.help = "configure hardware vectors to trigger debug entry",
		.usage = "['all'|'none'|('bus_err'|'chk_err'|...)*]",
	},
	{
		.name = "reset_config",
		.handler = NULL, //handle_arc700_reset_config_command,
		.mode = COMMAND_ANY,
		.help = "configure software reset handling",
		.usage = "['srst'|'sysresetreq'|'vectreset']",
	},
	/* See: src/target/mips_m4k.c  mips_m4k_exec_command_handlers[] */
	{
		.name = "smp_gdb",
		.handler = handle_arc700_smp_gdb_command,
		.mode = COMMAND_EXEC,
		.help = "display/fix current core played to gdb",
		.usage = "",
	},
	COMMAND_REGISTRATION_DONE
};

const struct command_registration arc700_command_handlers[] = {
	{
		.chain = arc32_command_handlers,
	},
	{
		.name = "arc700",
		.mode = COMMAND_ANY,
		.help = "ARC700 command group",
		.usage = "Help info ...",
		.chain = arc700_exec_command_handlers,
	},
	COMMAND_REGISTRATION_DONE
};

/* ----- Target commands structure ----------------------------------------- */

struct target_type arc700_target = {
	.name = "arc700",

	.poll =	arc700_poll,
	.arch_state = arc700_arch_state,

	.target_request_data = arc700_target_request_data,

	.halt = arc700_halt,
	.resume = arc700_resume,
	.step = arc700_step,

	.assert_reset = arc700_assert_reset,
	.deassert_reset = arc700_deassert_reset,
	.soft_reset_halt = arc700_soft_reset_halt,

	.get_gdb_reg_list = arc700_get_gdb_reg_list,

	.read_memory = arc700_read_memory,
	.write_memory = arc700_write_memory,
	.bulk_write_memory = arc700_bulk_write_memory,
	.checksum_memory = arc700_checksum_memory,
	.blank_check_memory = arc700_blank_check_memory,

	.run_algorithm = arc700_run_algorithm,
	.start_algorithm = arc700_start_algorithm,
	.wait_algorithm = arc700_wait_algorithm,

	.add_breakpoint = arc700_add_breakpoint,
	.remove_breakpoint = arc700_remove_breakpoint,
	.add_watchpoint = arc700_add_watchpoint,
	.remove_watchpoint = arc700_remove_watchpoint,

	/* openocd infrastructure initialization */
	.commands = arc700_command_handlers,
	.target_create = arc700_target_create,
	.init_target = arc700_init_target,
	.examine = arc700_examine,
};
