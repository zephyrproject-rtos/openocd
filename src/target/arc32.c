/***************************************************************************
 *   Copyright (C) 2013-2015 Synopsys, Inc.                                *
 *   Frank Dols <frank.dols@synopsys.com>                                  *
 *   Mischa Jonker <mischa.jonker@synopsys.com>                            *
 *   Anton Kolesov <anton.kolesov@synopsys.com>                            *
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
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "arc32.h"


/* ----- Public data  ------------------------------------------------------ */
const char * const arc_reg_debug = "debug";
const char * const arc_reg_isa_config = "isa_config";
const char * const arc_reg_isa_config_addr_size = "addr_size";


/* ----- Private data  ----------------------------------------------------- */

static const char *arc_isa_strings[] = {
	"ARC32", "ARC16"
};

/* Standard GDB register types */
static const struct reg_data_type standard_gdb_types[] = {
	{ .type = REG_TYPE_INT,         .id = "int" },
	{ .type = REG_TYPE_INT8,        .id = "int8" },
	{ .type = REG_TYPE_INT16,       .id = "int16" },
	{ .type = REG_TYPE_INT32,       .id = "int32" },
	{ .type = REG_TYPE_INT64,       .id = "int64" },
	{ .type = REG_TYPE_INT128,      .id = "int128" },
	{ .type = REG_TYPE_UINT8,       .id = "uint8" },
	{ .type = REG_TYPE_UINT16,      .id = "uint16" },
	{ .type = REG_TYPE_UINT32,      .id = "uint32" },
	{ .type = REG_TYPE_UINT64,      .id = "uint64" },
	{ .type = REG_TYPE_UINT128,     .id = "uint128" },
	{ .type = REG_TYPE_CODE_PTR,    .id = "code_ptr" },
	{ .type = REG_TYPE_DATA_PTR,    .id = "data_ptr" },
	{ .type = REG_TYPE_FLOAT,       .id = "float" },
	{ .type = REG_TYPE_IEEE_SINGLE, .id = "ieee_single" },
	{ .type = REG_TYPE_IEEE_DOUBLE, .id = "ieee_double" },
};

/* GDB register groups. For now we suport only general and "empty" */
static const char * const reg_group_general = "general";
static const char * const reg_group_other = "";


/* ----- Exported functions ------------------------------------------------ */

int arc32_init_arch_info(struct target *target, struct arc32_common *arc32,
	struct jtag_tap *tap)
{
	arc32->common_magic = ARC32_COMMON_MAGIC;
	target->arch_info = arc32;

	arc32->fast_data_area = NULL;

	arc32->jtag_info.tap = tap;
	arc32->jtag_info.scann_size = 4;
	arc32->jtag_info.always_check_status_rd = false;
	arc32->jtag_info.check_status_fl = false;
	arc32->jtag_info.wait_until_write_finished = false;

	/* has breakpoint/watchpoint unit been scanned */
	arc32->bp_scanned = 0;

	/* We don't know how many actionpoints are in the core yet. */
	arc32->actionpoints_num_avail = 0;
	arc32->actionpoints_num = 0;
	arc32->actionpoints_list = NULL;

	/* Flush D$ by default. It is safe to assume that D$ is present,
	 * because if it isn't, there will be no error, just a slight
	 * performance penalty from unnecessary JTAG operations. */
	arc32->has_dcache = true;
	arc32_reset_caches_states(target);

	/* Add standard GDB data types */
	INIT_LIST_HEAD(&arc32->reg_data_types.list);
	struct arc_reg_data_type *std_types = calloc(ARRAY_SIZE(standard_gdb_types),
			sizeof(struct arc_reg_data_type));
	if (!std_types) {
		LOG_ERROR("Cannot allocate memory");
		return ERROR_FAIL;
	}
	for (unsigned int i = 0; i < ARRAY_SIZE(standard_gdb_types); i++) {
		std_types[i].data_type.type = standard_gdb_types[i].type;
		std_types[i].data_type.id = standard_gdb_types[i].id;
		arc32_add_reg_data_type(target, &(std_types[i]));
	}

	/* Fields related to target descriptions */
	INIT_LIST_HEAD(&arc32->core_reg_descriptions.list);
	INIT_LIST_HEAD(&arc32->aux_reg_descriptions.list);
	INIT_LIST_HEAD(&arc32->bcr_reg_descriptions.list);
	arc32->num_regs = 0;
	arc32->num_core_regs = 0;
	arc32->num_aux_regs = 0;
	arc32->num_bcr_regs = 0;
	arc32->last_general_reg = ULONG_MAX;
	arc32->pc_index_in_cache = ULONG_MAX;
	arc32->debug_index_in_cache = ULONG_MAX;

	return ERROR_OK;
}

/**
 * Read register that are used in GDB g-packet. We don't read them one-by-one,
 * but do that in one batch operation to improve speed. Calls to JTAG layer are
 * expensive so it is better to make one big call that reads all necessary
 * registers, instead of many calls, one for one register.
 */
int arc32_save_context(struct target *target)
{
	int retval = ERROR_OK;
	unsigned int i;
	struct arc32_common *arc32 = target_to_arc32(target);
	struct reg *reg_list = arc32->core_cache->reg_list;

	LOG_DEBUG("-");
	assert(reg_list);

	/* It is assumed that there is at least one AUX register in the list, for
	 * example PC. */
	const uint32_t core_regs_size = arc32->num_core_regs * sizeof(uint32_t);
	/* last_general_reg is inclusive number. To get count of registers it is
	 * required to do +1. */
	const uint32_t regs_to_scan =
		MIN(arc32->last_general_reg + 1, arc32->num_regs);
	const uint32_t aux_regs_size = arc32->num_aux_regs * sizeof(uint32_t);
	uint32_t *core_values = malloc(core_regs_size);
	uint32_t *aux_values = malloc(aux_regs_size);
	uint32_t *core_addrs = malloc(core_regs_size);
	uint32_t *aux_addrs = malloc(aux_regs_size);
	unsigned int core_cnt = 0;
	unsigned int aux_cnt = 0;

	if (!core_values || !core_addrs || !aux_values || !aux_addrs)  {
		LOG_ERROR("Not enough memory");
		retval = ERROR_FAIL;
		goto exit;
	}

	memset(core_values, 0xdeadbeef, core_regs_size);
	memset(core_addrs, 0xdeadbeef, core_regs_size);
	memset(aux_values, 0xdeadbeef, aux_regs_size);
	memset(aux_addrs, 0xdeadbeef, aux_regs_size);

	for (i = 0; i < MIN(arc32->num_core_regs, regs_to_scan); i++) {
		struct reg *reg = &(reg_list[i]);
		struct arc_reg_t *arc_reg = reg->arch_info;
		if (!reg->valid && reg->exist && !arc_reg->dummy) {
			core_addrs[core_cnt] = arc_reg->desc->arch_num;
			core_cnt += 1;
		}
	}

	for (i = arc32->num_core_regs; i < regs_to_scan; i++) {
		struct reg *reg = &(reg_list[i]);
		struct arc_reg_t *arc_reg = reg->arch_info;
		if (!reg->valid && reg->exist && !arc_reg->dummy) {
			aux_addrs[aux_cnt] = arc_reg->desc->arch_num;
			aux_cnt += 1;
		}
	}

	/* Read data from target. */
	retval = arc_jtag_read_core_reg(&arc32->jtag_info, core_addrs, core_cnt, core_values);
	if (ERROR_OK != retval) {
		LOG_ERROR("Attempt to read core registers failed.");
		retval = ERROR_FAIL;
		goto exit;
	}
	retval = arc_jtag_read_aux_reg(&arc32->jtag_info, aux_addrs, aux_cnt, aux_values);
	if (ERROR_OK != retval) {
		LOG_ERROR("Attempt to read aux registers failed.");
		retval = ERROR_FAIL;
		goto exit;
	}

	/* Parse core regs */
	core_cnt = 0;
	for (i = 0; i < MIN(arc32->num_core_regs, regs_to_scan); i++) {
		struct reg *reg = &(reg_list[i]);
		struct arc_reg_t *arc_reg = reg->arch_info;
		if (!reg->valid && reg->exist) {
			if (!arc_reg->dummy) {
				arc_reg->value = core_values[core_cnt];
				core_cnt += 1;
			} else {
				arc_reg->value = 0;
			}
			buf_set_u32(reg->value, 0, 32, arc_reg->value);
			reg->valid = true;
			reg->dirty = false;
			LOG_DEBUG("Get core register regnum=%" PRIu32 ", name=%s, value=0x%08" PRIx32,
				i , arc_reg->desc->name, arc_reg->value);
		}
	}

	/* Parse aux regs */
	aux_cnt = 0;
	for (i = arc32->num_core_regs; i < regs_to_scan; i++) {
		struct reg *reg = &(reg_list[i]);
		struct arc_reg_t *arc_reg = reg->arch_info;
		if (!reg->valid && reg->exist) {
			if (!arc_reg->dummy) {
				arc_reg->value = aux_values[aux_cnt];
				aux_cnt += 1;
			} else {
				arc_reg->value = 0;
			}
			buf_set_u32(reg->value, 0, 32, arc_reg->value);
			reg->valid = true;
			reg->dirty = false;
			LOG_DEBUG("Get aux register regnum=%" PRIu32 ", name=%s, value=0x%08" PRIx32,
				i , arc_reg->desc->name, arc_reg->value);
		}
	}

exit:
	free(core_values);
	free(core_addrs);
	free(aux_values);
	free(aux_addrs);

	return retval;
}

/**
 * See arc32_save_context() for reason why we want to dump all regs at once.
 * This however means that if there are dependencies between registers they
 * will not be observable until target will be resumed.
 */
int arc32_restore_context(struct target *target)
{
	int retval = ERROR_OK;
	unsigned int i;
	struct arc32_common *arc32 = target_to_arc32(target);
	struct reg *reg_list = arc32->core_cache->reg_list;

	LOG_DEBUG("-");
	assert(reg_list);

	/* It is assumed that there is at least one AUX register in the list. */
	const uint32_t core_regs_size = arc32->num_core_regs  * sizeof(uint32_t);
	const uint32_t aux_regs_size =  arc32->num_aux_regs * sizeof(uint32_t);
	uint32_t *core_values = malloc(core_regs_size);
	uint32_t *aux_values = malloc(aux_regs_size);
	uint32_t *core_addrs = malloc(core_regs_size);
	uint32_t *aux_addrs = malloc(aux_regs_size);
	unsigned int core_cnt = 0;
	unsigned int aux_cnt = 0;

	if (!core_values || !core_addrs || !aux_values || !aux_addrs)  {
		LOG_ERROR("Not enough memory");
		retval = ERROR_FAIL;
		goto exit;
	}

	memset(core_values, 0xdeadbeef, core_regs_size);
	memset(core_addrs, 0xdeadbeef, core_regs_size);
	memset(aux_values, 0xdeadbeef, aux_regs_size);
	memset(aux_addrs, 0xdeadbeef, aux_regs_size);

	for (i = 0; i < arc32->num_core_regs; i++) {
		struct reg *reg = &(reg_list[i]);
		struct arc_reg_t *arc_reg = reg->arch_info;
		if (reg->valid && reg->exist && reg->dirty) {
			LOG_DEBUG("Will write regnum=%u", i);
			core_addrs[core_cnt] = arc_reg->desc->arch_num;
			core_values[core_cnt] = arc_reg->value;
			core_cnt += 1;
		}
	}

	for (i = 0; i < arc32->num_aux_regs; i++) {
		struct reg *reg = &(reg_list[arc32->num_core_regs + i]);
		struct arc_reg_t *arc_reg = reg->arch_info;
		if (reg->valid && reg->exist && reg->dirty) {
			LOG_DEBUG("Will write regnum=%lu", arc32->num_core_regs + i);
			aux_addrs[aux_cnt] = arc_reg->desc->arch_num;
			aux_values[aux_cnt] = arc_reg->value;
			aux_cnt += 1;
		}
	}

	/* Write data to target. */
	/* JTAG layer will return quickly if count == 0. */
	retval = arc_jtag_write_core_reg(&arc32->jtag_info, core_addrs, core_cnt, core_values);
	if (ERROR_OK != retval) {
		LOG_ERROR("Attempt to write to core registers failed.");
		retval = ERROR_FAIL;
		goto exit;
	}
	retval = arc_jtag_write_aux_reg(&arc32->jtag_info, aux_addrs, aux_cnt, aux_values);
	if (ERROR_OK != retval) {
		LOG_ERROR("Attempt to write to aux registers failed.");
		retval = ERROR_FAIL;
		goto exit;
	}

exit:
	free(core_values);
	free(core_addrs);
	free(aux_values);
	free(aux_addrs);

	return retval;
}

int arc32_enable_interrupts(struct target *target, int enable)
{
	uint32_t value;

	struct arc32_common *arc32 = target_to_arc32(target);

	if (enable) {
		/* enable interrupts */
		value = SET_CORE_ENABLE_INTERRUPTS;
		CHECK_RETVAL(arc_jtag_write_aux_reg_one(&arc32->jtag_info, AUX_IENABLE_REG, value));
		LOG_DEBUG("interrupts enabled");
	} else {
		/* disable interrupts */
		value = SET_CORE_DISABLE_INTERRUPTS;
		CHECK_RETVAL(arc_jtag_write_aux_reg_one(&arc32->jtag_info, AUX_IENABLE_REG, value));
		LOG_DEBUG("interrupts disabled");
	}

	return ERROR_OK;
}

int arc32_start_core(struct target *target)
{
	uint32_t value;

	struct arc32_common *arc32 = target_to_arc32(target);

	target->state = TARGET_RUNNING;

	CHECK_RETVAL(arc_jtag_read_aux_reg_one(&arc32->jtag_info, AUX_STATUS32_REG, &value));
	value &= ~SET_CORE_HALT_BIT;        /* clear the HALT bit */
	CHECK_RETVAL(arc_jtag_write_aux_reg_one(&arc32->jtag_info, AUX_STATUS32_REG, value));
	LOG_DEBUG("Core started to run");

#ifdef DEBUG
	CHECK_RETVAL(arc32_print_core_state(target));
#endif
	return ERROR_OK;
}

int arc32_config_step(struct target *target, int enable_step)
{
	uint32_t value;

	struct arc32_common *arc32 = target_to_arc32(target);

	if (enable_step) {
		/* enable core debug step mode */
		CHECK_RETVAL(arc_jtag_read_aux_reg_one(&arc32->jtag_info, AUX_STATUS32_REG,
			&value));
		value &= ~SET_CORE_AE_BIT; /* clear the AE bit */
		CHECK_RETVAL(arc_jtag_write_aux_reg_one(&arc32->jtag_info, AUX_STATUS32_REG,
			value));
		LOG_DEBUG(" [status32:0x%08" PRIx32 "]", value);

		/* Doing read-modify-write, because DEBUG might contain manually set
		 * bits like UB or ED, which should be preserved.  */
		CHECK_RETVAL(arc_jtag_read_aux_reg_one(&arc32->jtag_info,
					AUX_DEBUG_REG, &value));
		value |= SET_CORE_SINGLE_INSTR_STEP; /* set the IS bit */

		if (arc32->has_debug_ss) {
			value |= SET_CORE_SINGLE_STEP;  /* set the SS bit */
			LOG_DEBUG("ARC600 extra single step bit to set.");
		}

		if (arc32->on_step_reset_debug_ra) {
			LOG_DEBUG("Resetting DEBUG.RA bit.");
			value &= ~SET_CORE_RESET_APPLIED; /* Reset the RA bit. */
		}

		CHECK_RETVAL(arc_jtag_write_aux_reg_one(&arc32->jtag_info, AUX_DEBUG_REG,
			value));
		LOG_DEBUG("core debug step mode enabled [debug-reg:0x%08" PRIx32 "]", value);
	} else {
		/* disable core debug step mode */
		CHECK_RETVAL(arc_jtag_read_aux_reg_one(&arc32->jtag_info, AUX_DEBUG_REG,
			&value));
		value &= ~SET_CORE_SINGLE_INSTR_STEP; /* clear the IS bit */
		CHECK_RETVAL(arc_jtag_write_aux_reg_one(&arc32->jtag_info, AUX_DEBUG_REG,
			value));
		LOG_DEBUG("core debug step mode disabled");
	}

#ifdef DEBUG
	CHECK_RETVAL(arc32_print_core_state(target));
#endif
	return ERROR_OK;
}

/* This function is cheap to call and returns quickly if caches already has
 * been invalidated since core had been halted. */
int arc32_cache_invalidate(struct target *target)
{
	uint32_t value, backup;

	struct arc32_common *arc32 = target_to_arc32(target);

	/* Don't waste time if already done. */
	if (arc32->cache_invalidated)
	    return ERROR_OK;

	LOG_DEBUG("Invalidating I$ & D$.");

	value = IC_IVIC_INVALIDATE;	/* invalidate I$ */
	CHECK_RETVAL(arc_jtag_write_aux_reg_one(&arc32->jtag_info, AUX_IC_IVIC_REG, value));
	CHECK_RETVAL(arc_jtag_read_aux_reg_one(&arc32->jtag_info, AUX_DC_CTRL_REG, &value));

	backup = value;
	value = value & ~DC_CTRL_IM;

	/* set DC_CTRL invalidate mode to invalidate-only (no flushing!!) */
	CHECK_RETVAL(arc_jtag_write_aux_reg_one(&arc32->jtag_info, AUX_DC_CTRL_REG, value));
	value = DC_IVDC_INVALIDATE;	/* invalidate D$ */
	CHECK_RETVAL(arc_jtag_write_aux_reg_one(&arc32->jtag_info, AUX_DC_IVDC_REG, value));

	/* restore DC_CTRL invalidate mode */
	CHECK_RETVAL(arc_jtag_write_aux_reg_one(&arc32->jtag_info, AUX_DC_CTRL_REG, backup));

	arc32->cache_invalidated = true;

	return ERROR_OK;
}

/* Flush data cache. This function is cheap to call and return quickly if D$
 * already has been flushed since target had been halted. JTAG debugger reads
 * values directly from memory, bypassing cache, so if there are unflushed
 * lines debugger will read invalid values, which will cause a lot of troubles.
 * */
int arc32_dcache_flush(struct target *target)
{
	uint32_t value, dc_ctrl_value;
	bool has_to_set_dc_ctrl_im;

	struct arc32_common *arc32 = target_to_arc32(target);

	/* Don't waste time if already done. */
	if (!arc32->has_dcache || arc32->dcache_flushed)
	    return ERROR_OK;

	LOG_DEBUG("Flushing D$.");

	/* Store current value of DC_CTRL */
	CHECK_RETVAL(arc_jtag_read_aux_reg_one(&arc32->jtag_info, AUX_DC_CTRL_REG, &dc_ctrl_value));

	/* Set DC_CTRL invalidate mode to flush (if not already set) */
	has_to_set_dc_ctrl_im = (dc_ctrl_value & DC_CTRL_IM) == 0;
	if (has_to_set_dc_ctrl_im) {
		value = dc_ctrl_value | DC_CTRL_IM;
		CHECK_RETVAL(arc_jtag_write_aux_reg_one(&arc32->jtag_info, AUX_DC_CTRL_REG, value));
	}

	/* Flush D$ */
	value = DC_IVDC_INVALIDATE;
	CHECK_RETVAL(arc_jtag_write_aux_reg_one(&arc32->jtag_info, AUX_DC_IVDC_REG, value));

	/* Restore DC_CTRL invalidate mode (even of flush failed) */
	if (has_to_set_dc_ctrl_im) {
	    CHECK_RETVAL(arc_jtag_write_aux_reg_one(&arc32->jtag_info, AUX_DC_CTRL_REG, dc_ctrl_value));
	}

	arc32->dcache_flushed = true;

	return ERROR_OK;
}

int arc32_print_core_state(struct target *target)
{
	struct arc32_common *arc32 = target_to_arc32(target);
	uint32_t value;

	CHECK_RETVAL(arc_jtag_read_aux_reg_one(&arc32->jtag_info, AUX_DEBUG_REG, &value));
	LOG_DEBUG("  AUX REG  [DEBUG]: 0x%08" PRIx32, value);
	CHECK_RETVAL(arc_jtag_read_aux_reg_one(&arc32->jtag_info, AUX_STATUS32_REG, &value));
	LOG_DEBUG("        [STATUS32]: 0x%08" PRIx32, value);
	CHECK_RETVAL(arc_jtag_read_aux_reg_one(&arc32->jtag_info, AUX_PC_REG, &value));
	LOG_DEBUG("              [PC]: 0x%08" PRIx32, value);

	return ERROR_OK;
}

int arc32_arch_state(struct target *target)
{
	if (debug_level < LOG_LVL_DEBUG)
		return ERROR_OK;

	struct arc32_common *arc32 = target_to_arc32(target);

	uint32_t pc_value;
	CHECK_RETVAL(arc32_get_register_value_u32(target, "pc", &pc_value));
	LOG_DEBUG("target state: %s in: %s mode, PC at: 0x%08" PRIx32,
		target_state_name(target),
		arc_isa_strings[arc32->isa_mode],
		pc_value);

	return ERROR_OK;
}

int arc32_get_current_pc(struct target *target)
{
	uint32_t dpc;
	struct arc32_common *arc32 = target_to_arc32(target);

	/* read current PC */
	CHECK_RETVAL(arc_jtag_read_aux_reg_one(&arc32->jtag_info, AUX_PC_REG, &dpc));

	/* save current PC */
	buf_set_u32(arc32->core_cache->reg_list[arc32->pc_index_in_cache].value,
			0, 32, dpc);

	return ERROR_OK;
}

/**
 * Reset internal states of caches. Must be called when entering debugging.
 *
 * @param target Target for which to reset caches states.
 */
int arc32_reset_caches_states(struct target *target)
{
	struct arc32_common *arc32 = target_to_arc32(target);

	LOG_DEBUG("Resetting internal variables of caches states");

	/* Reset caches states. */
	arc32->dcache_flushed = false;
	arc32->cache_invalidated = false;

	return ERROR_OK;
}

/**
 * Write 4-byte instruction to memory. This is like target_write_u32, however
 * in case of little endian ARC instructions are in middle endian format, not
 * little endian, so different type of conversion should be done.
 */
int arc32_write_instruction_u32(struct target *target, uint32_t address,
	uint32_t instr)
{
    uint8_t value_buf[4];
    if (!target_was_examined(target)) {
        LOG_ERROR("Target not examined yet");
        return ERROR_FAIL;
    }

    LOG_DEBUG("Address: 0x%08" PRIx32 ", value: 0x%08" PRIx32, address,
		instr);

    if (target->endianness == TARGET_LITTLE_ENDIAN) {
        arc32_h_u32_to_me(value_buf, instr);
    } else{
        h_u32_to_be(value_buf, instr);
    }

    CHECK_RETVAL(target_write_buffer(target, address, 4, value_buf));

    return ERROR_OK;
}

/**
 * Read 32-bit instruction from memory. It is like target_read_u32, however in
 * case of little endian ARC instructions are in middle endian format, so
 * different type of conversion should be done.
 */
int arc32_read_instruction_u32(struct target *target, uint32_t address,
		uint32_t *value)
{
    uint8_t value_buf[4];
    if (!target_was_examined(target)) {
        LOG_ERROR("Target not examined yet");
        return ERROR_FAIL;
    }

    *value = 0;
    CHECK_RETVAL(target_read_buffer(target, address, 4, value_buf));

    if (target->endianness == TARGET_LITTLE_ENDIAN)
        *value = arc32_me_to_h_u32(value_buf);
    else
        *value = be_to_h_u32(value_buf);
    LOG_DEBUG("Address: 0x%08" PRIx32 ", value: 0x%08" PRIx32, address,
        *value);

    return ERROR_OK;
}

/**
 * Evaluate size of address: from 16 to 32 bits.
 */
static int arc_regs_addr_size_bits(struct target *target,
		uint32_t *addr_size_bits)
{
	assert(target);
	assert(addr_size_bits);

	uint32_t addr_size = 32;
	CHECK_RETVAL(arc32_get_register_field(target, arc_reg_isa_config,
				arc_reg_isa_config_addr_size, &addr_size));

	/* addr_size_bits = (addr_size << 2) + 0x10  */
	switch (addr_size) {
		case 0:
			*addr_size_bits = 16;
			return ERROR_OK;
		case 1:
			*addr_size_bits = 20;
			return ERROR_OK;
		case 2:
			*addr_size_bits = 24;
			return ERROR_OK;
		case 3:
			*addr_size_bits = 28;
			return ERROR_OK;
		case 4:
			*addr_size_bits = 32;
			return ERROR_OK;
		default:
			LOG_ERROR("isa_config.addr_size value %" PRIu32 " is invalid.", addr_size);
			/* This value is read from the core - it is not a user input, hence
			 * default case is not really possible, unless format of register
			 * changes.  */
			*addr_size_bits = 32;
			return ERROR_OK;
	}
}

/* Configure some core features, depending on BCRs. */
int arc32_configure(struct target *target)
{
	LOG_DEBUG("-");
	struct arc32_common *arc32 = target_to_arc32(target);

	/* DCCM. But only if DCCM_BUILD and AUX_DCCM are known registers. */
	arc32->dccm_start = 0;
	arc32->dccm_end = 0;
	if (register_get_by_name(target->reg_cache, "dccm_build", true) &&
	    register_get_by_name(target->reg_cache, "aux_dccm", true)) {

		uint32_t dccm_build_version, dccm_build_size0, dccm_build_size1;
		CHECK_RETVAL(arc32_get_register_field(target, "dccm_build", "version",
			&dccm_build_version));
		CHECK_RETVAL(arc32_get_register_field(target, "dccm_build", "size0",
			&dccm_build_size0));
		CHECK_RETVAL(arc32_get_register_field(target, "dccm_build", "size1",
			&dccm_build_size1));
		if (dccm_build_version == 3 && dccm_build_size0 > 0) {
			CHECK_RETVAL(arc32_get_register_value_u32(target, "aux_dccm", &(arc32->dccm_start)));
			arc32_address_t dccm_size = 0x100;
			dccm_size <<= dccm_build_size0;
			if (dccm_build_size0 == 0xF)
				dccm_size <<= dccm_build_size1;
			arc32->dccm_end = arc32->dccm_start + dccm_size;
			LOG_DEBUG("DCCM detected start=0x%" PRIx32 " end=0x%" PRIx32,
					arc32->dccm_start, arc32->dccm_end);
		}
	}

	/* Only if ICCM_BUILD and AUX_ICCM are known registers. */
	arc32->iccm0_start = 0;
	arc32->iccm0_end = 0;
	if (register_get_by_name(target->reg_cache, "iccm_build", true) &&
	    register_get_by_name(target->reg_cache, "aux_iccm", true)) {
		/* Common for both ICCMx  */
		uint32_t addr_size_bits;
		CHECK_RETVAL(arc_regs_addr_size_bits(target, &addr_size_bits));

		/* ICCM0 */
		uint32_t iccm_build_version, iccm_build_size00, iccm_build_size01;
		arc32_address_t aux_iccm = 0;
		CHECK_RETVAL(arc32_get_register_field(target, "iccm_build", "version",
			&iccm_build_version));
		CHECK_RETVAL(arc32_get_register_field(target, "iccm_build", "iccm0_size0",
			&iccm_build_size00));
		CHECK_RETVAL(arc32_get_register_field(target, "iccm_build", "iccm0_size1",
			&iccm_build_size01));
		if (iccm_build_version == 4 && iccm_build_size00 > 0) {
			CHECK_RETVAL(arc32_get_register_value_u32(target, "aux_iccm", &aux_iccm));
			arc32_address_t iccm0_size = 0x100;
			iccm0_size <<= iccm_build_size00;
			if (iccm_build_size00 == 0xF)
				iccm0_size <<= iccm_build_size01;
			arc32->iccm0_start = aux_iccm & (0xF0000000 >> (32 - addr_size_bits));
			arc32->iccm0_end = arc32->iccm0_start + iccm0_size;
			LOG_DEBUG("ICCM0 detected start=0x%" PRIx32 " end=0x%" PRIx32,
					arc32->iccm0_start, arc32->iccm0_end);
		}

		/* ICCM1 */
		uint32_t iccm_build_size10, iccm_build_size11;
		CHECK_RETVAL(arc32_get_register_field(target, "iccm_build", "iccm1_size0",
			&iccm_build_size10));
		CHECK_RETVAL(arc32_get_register_field(target, "iccm_build", "iccm1_size1",
			&iccm_build_size11));
		if (iccm_build_version == 4 && iccm_build_size10 > 0) {
			/* Use value read for ICCM0 */
			if (!aux_iccm)
				CHECK_RETVAL(arc32_get_register_value_u32(target, "aux_iccm", &aux_iccm));
			arc32_address_t iccm1_size = 0x100;
			iccm1_size <<= iccm_build_size10;
			if (iccm_build_size10 == 0xF)
				iccm1_size <<= iccm_build_size11;
			arc32->iccm1_start = aux_iccm & (0x0F000000 >> (32 - addr_size_bits));
			arc32->iccm1_end = arc32->iccm1_start + iccm1_size;
			LOG_DEBUG("ICCM1 detected start=0x%" PRIx32 " end=0x%" PRIx32,
					arc32->iccm1_start, arc32->iccm1_end);
		}
	}

	return ERROR_OK;
}

void arc32_set_actionpoints_num(struct target *target, unsigned ap_num)
{
	LOG_DEBUG("target=%s actionpoints=%u", target_name(target), ap_num);
	struct arc32_common *arc32 = target_to_arc32(target);

	/* Make sure that there are no enabled actionpoints in target. */
	arc_dbg_reset_actionpoints(target);

	/* Assume that all actionpoints have been removed from target.  */
	free(arc32->actionpoints_list);

	arc32->actionpoints_num_avail = ap_num;
	arc32->actionpoints_num = ap_num;
	/* calloc can be safely called when ncount == 0.  */
	arc32->actionpoints_list = calloc(ap_num, sizeof(struct arc32_comparator));
}

void arc32_add_reg_data_type(struct target *target,
		struct arc_reg_data_type *data_type)
{
	LOG_DEBUG("-");
	struct arc32_common *arc = target_to_arc32(target);
	assert(arc);

	list_add_tail(&data_type->list, &arc->reg_data_types.list);
}

int arc32_add_reg(struct target *target, struct arc_reg_desc *arc_reg,
		const char * const type_name, const size_t type_name_len)
{
	assert(target);
	assert(arc_reg);

	struct arc32_common *arc32 = target_to_arc32(target);
	assert(arc32);

	/* Find register type */
	{
		struct arc_reg_data_type *type;
		list_for_each_entry(type, &arc32->reg_data_types.list, list) {
			if (strncmp(type->data_type.id, type_name, type_name_len) == 0) {
				arc_reg->data_type = &(type->data_type);
				break;
			}
		}
		if (!arc_reg->data_type) {
			return ERROR_ARC_REGTYPE_NOT_FOUND;
		}
	}

	if (arc_reg->is_core) {
		list_add_tail(&arc_reg->list, &arc32->core_reg_descriptions.list);
		arc32->num_core_regs += 1;
	} else if (arc_reg->is_bcr) {
		list_add_tail(&arc_reg->list, &arc32->bcr_reg_descriptions.list);
		arc32->num_bcr_regs += 1;
	} else {
		list_add_tail(&arc_reg->list, &arc32->aux_reg_descriptions.list);
		arc32->num_aux_regs += 1;
	}
	arc32->num_regs += 1;

	LOG_DEBUG(
			"added register {name=%s, num=0x%x, type=%s%s%s%s}",
			arc_reg->name, arc_reg->arch_num, arc_reg->data_type->id,
			arc_reg->is_core ? ", core" : "",  arc_reg->is_bcr ? ", bcr" : "",
			arc_reg->is_general ? ", general" : ""
		);

	return ERROR_OK;
}

/* Common code to initialize `struct reg` for different registers: core, aux, bcr. */
static void init_reg(
		struct target *target,
		struct reg *reg,
		struct arc_reg_t *arc_reg,
		struct arc_reg_desc *reg_desc,
		unsigned long number)
{
	assert(target);
	assert(reg);
	assert(arc_reg);
	assert(reg_desc);

	struct arc32_common *arc32 = target_to_arc32(target);

	/* Initialize struct arc_reg_t */
	arc_reg->desc = reg_desc;
	arc_reg->target = target;
	arc_reg->arc32_common = arc32;
	arc_reg->dummy = false; /* @todo deprecated. */

	/* Initialize struct reg */
	reg->name = reg_desc->name;
	reg->size = 32; /* All register in ARC are 32-bit */
	reg->value = calloc(1, 4);
	reg->dirty = 0;
	reg->valid = 0;
	reg->type = &arc32_reg_type;
	reg->arch_info = arc_reg;
	reg->exist = false;
	reg->caller_save = true; /* @todo should be configurable. */
	reg->reg_data_type = reg_desc->data_type;

	reg->feature = calloc(1, sizeof(struct reg_feature));
	reg->feature->name = reg_desc->gdb_xml_feature;

	/* reg->number is used by OpenOCD as value for @regnum. Thus when setting
	 * value of a register GDB will use it as a number of register in
	 * P-packet. OpenOCD gdbserver will then use number of register in
	 * P-packet as an array index in the reg_list returned by
	 * arc_regs_get_gdb_reg_list. So to ensure that registers are assigned
	 * correctly it would be required to either sort registers in
	 * arc_regs_get_gdb_reg_list or to assign numbers sequentially here and
	 * according to how registers will be sorted in
	 * arc_regs_get_gdb_reg_list. Second options is much more simpler. */
	reg->number = number;

	if (reg_desc->is_general) {
		arc32->last_general_reg = reg->number;
		reg->group = reg_group_general;
	} else {
		reg->group = reg_group_other;
	}
}

int arc32_build_reg_cache(struct target *target)
{
	/* get pointers to arch-specific information */
	struct arc32_common *arc32 = target_to_arc32(target);
	const unsigned long num_regs = arc32->num_core_regs + arc32->num_aux_regs;
	struct reg_cache **cache_p = register_get_last_cache_p(&target->reg_cache);
	struct reg_cache *cache = calloc(1, sizeof(struct reg_cache));
	struct reg *reg_list = calloc(num_regs, sizeof(struct reg));
	struct arc_reg_t *arch_info = calloc(num_regs, sizeof(struct arc_reg_t));

	/* Build the process context cache */
	cache->name = "arc32 registers";
	cache->next = NULL;
	cache->reg_list = reg_list;
	cache->num_regs = num_regs;
	(*cache_p) = cache;
	arc32->core_cache = cache;

	struct arc_reg_desc *reg_desc;
	unsigned long i = 0;
	list_for_each_entry(reg_desc, &arc32->core_reg_descriptions.list, list) {
		init_reg(target, &reg_list[i], &arch_info[i], reg_desc, i);

		LOG_DEBUG("reg n=%3li name=%3s group=%s feature=%s", i,
			reg_list[i].name, reg_list[i].group,
			reg_list[i].feature->name);

		i += 1;
	}

	list_for_each_entry(reg_desc, &arc32->aux_reg_descriptions.list, list) {
		init_reg(target, &reg_list[i], &arch_info[i], reg_desc, i);

		LOG_DEBUG("reg n=%3li name=%3s group=%s feature=%s", i,
			reg_list[i].name, reg_list[i].group,
			reg_list[i].feature->name);

		/* PC and DEBUG are essential so we search for them. */
		if (arc32->pc_index_in_cache == ULONG_MAX && strcmp("pc", reg_desc->name) == 0)
			arc32->pc_index_in_cache = i;
		else if (arc32->debug_index_in_cache == ULONG_MAX
				&& strcmp("debug", reg_desc->name) == 0)
			arc32->debug_index_in_cache = i;

		i += 1;
	}

	if (arc32->pc_index_in_cache == ULONG_MAX
			|| arc32->debug_index_in_cache == ULONG_MAX) {
		LOG_ERROR("`pc' and `debug' registers must be present in target description.");
		return ERROR_FAIL;
	}

	assert(i == (arc32->num_core_regs + arc32->num_aux_regs));

	return ERROR_OK;
}

/* This function must be called only after arc32_build_reg_cache */
int arc32_build_bcr_reg_cache(struct target *target)
{
	/* get pointers to arch-specific information */
	struct arc32_common *arc32 = target_to_arc32(target);
	const unsigned long num_regs = arc32->num_bcr_regs;
	struct reg_cache **cache_p = register_get_last_cache_p(&target->reg_cache);
	struct reg_cache *cache = malloc(sizeof(struct reg_cache));
	struct reg *reg_list = calloc(num_regs, sizeof(struct reg));
	struct arc_reg_t *arch_info = calloc(num_regs, sizeof(struct arc_reg_t));

	/* Build the process context cache */
	cache->name = "arc.bcr";
	cache->next = NULL;
	cache->reg_list = reg_list;
	cache->num_regs = num_regs;
	(*cache_p) = cache;

	struct arc_reg_desc *reg_desc;
	unsigned long i = 0;
	unsigned long gdb_regnum = arc32->core_cache->num_regs;

	list_for_each_entry(reg_desc, &arc32->bcr_reg_descriptions.list, list) {
		init_reg(target, &reg_list[i], &arch_info[i], reg_desc, gdb_regnum);
		/* BCRs always semantically, they are just read-as-zero, if there is
		 * not real register. */
		reg_list[i].exist = true;

		LOG_DEBUG("reg n=%3li name=%3s group=%s feature=%s", i,
			reg_list[i].name, reg_list[i].group,
			reg_list[i].feature->name);
		i += 1;
		gdb_regnum += 1;
	}

	assert(i == arc32->num_bcr_regs);

	return ERROR_OK;
}

int arc32_get_register_value_u32(struct target *target, const char *reg_name,
		uint32_t *value_ptr)
{
	LOG_DEBUG("reg_name=%s", reg_name);

	if (!(target && reg_name && value_ptr)) {
		LOG_ERROR("Arguments cannot be NULL.");
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	struct reg *reg = register_get_by_name(target->reg_cache, reg_name, true);

	if (!reg)
		return ERROR_ARC_REGISTER_NOT_FOUND;

	if (!reg->valid)
		CHECK_RETVAL(reg->type->get(reg));

	const struct arc_reg_t * const arc_r = reg->arch_info;
	*value_ptr = arc_r->value;

	LOG_DEBUG("return %s=0x%08" PRIx32, reg_name, *value_ptr);

	return ERROR_OK;
}

/* Set value of 32-bit register. */
int arc32_set_register_value_u32(struct target *target, const char *reg_name,
		uint32_t value)
{
	LOG_DEBUG("reg_name=%s value=0x%08" PRIx32, reg_name, value);

	if (!(target && reg_name)) {
		LOG_ERROR("Arguments cannot be NULL.");
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	struct reg *reg = register_get_by_name(target->reg_cache, reg_name, true);

	if (!reg)
		return ERROR_ARC_REGISTER_NOT_FOUND;

	uint8_t value_buf[4];
	buf_set_u32(value_buf, 0, 32, value);
	CHECK_RETVAL(reg->type->set(reg, value_buf));

	return ERROR_OK;
}

int arc32_get_register_field(struct target *target, const char *reg_name,
		const char *field_name, uint32_t *value_ptr)
{
	LOG_DEBUG("getting register field (reg_name=%s, field_name=%s)", reg_name, field_name);

	if (!(reg_name && field_name && value_ptr)) {
		LOG_ERROR("Arguments cannot be NULL.");
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	/* Get register */
	struct reg *reg = register_get_by_name(target->reg_cache, reg_name, true);

	if (!reg) {
		LOG_ERROR("Requested register `%s' doens't exist.", reg_name);
		return ERROR_ARC_REGISTER_NOT_FOUND;
	}

	if (reg->reg_data_type->type != REG_TYPE_ARCH_DEFINED
	    || reg->reg_data_type->type_class != REG_TYPE_CLASS_STRUCT)
		return ERROR_ARC_REGISTER_IS_NOT_STRUCT;

	/* Get field in a register */
	struct reg_data_type_struct *reg_struct =
		reg->reg_data_type->reg_type_struct;
	struct reg_data_type_struct_field *field;
	for (field = reg_struct->fields;
	     field != NULL;
	     field = field->next) {
		if (strcmp(field->name, field_name) == 0)
			break;
	}

	if (!field)
		return ERROR_ARC_REGISTER_FIELD_NOT_FOUND;

	if (!field->use_bitfields)
		return ERROR_ARC_FIELD_IS_NOT_BITFIELD;

	if (!reg->valid)
		CHECK_RETVAL(reg->type->get(reg));

	*value_ptr = buf_get_u32(reg->value, field->bitfield->start,
			field->bitfield->end - field->bitfield->start + 1);

	LOG_DEBUG("return (value=0x%" PRIx32 ")", *value_ptr);

	return ERROR_OK;
}

/* ARC 600 target. The only difference from others is in special target_create */
struct target_type arc600_target = {
	.name = "arc600",

	.poll =	arc_ocd_poll,

	.arch_state = arc32_arch_state,

	.target_request_data = NULL,

	.halt = arc_dbg_halt,
	.resume = arc_dbg_resume,
	.step = arc_dbg_step,

	.assert_reset = arc_ocd_assert_reset,
	.deassert_reset = arc_ocd_deassert_reset,

	.soft_reset_halt = NULL,

	.get_gdb_reg_list = arc_regs_get_gdb_reg_list,

	.read_memory = arc_mem_read,
	.write_memory = arc_mem_write,
	.checksum_memory = arc_mem_checksum,
	.blank_check_memory = arc_mem_blank_check,

	.add_breakpoint = arc_dbg_add_breakpoint,
	.add_context_breakpoint = arc_dbg_add_context_breakpoint,
	.add_hybrid_breakpoint = arc_dbg_add_hybrid_breakpoint,
	.remove_breakpoint = arc_dbg_remove_breakpoint,
	.add_watchpoint = arc_dbg_add_watchpoint,
	.remove_watchpoint = arc_dbg_remove_watchpoint,

	.run_algorithm = arc_mem_run_algorithm,
	.start_algorithm = arc_mem_start_algorithm,
	.wait_algorithm = arc_mem_wait_algorithm,

	.commands = arc_monitor_command_handlers, /* see: arc_mntr.c|.h */

	.target_create = arc_ocd_arc600_target_create,
	.init_target = arc_ocd_init_target,
	.examine = arc_ocd_examine,

	.virt2phys = arc_mem_virt2phys,
	.read_phys_memory = arc_mem_read_phys_memory,
	.write_phys_memory = arc_mem_write_phys_memory,
	.mmu = arc_mem_mmu,
};

/* ARC 700 target. Same as v2, but semantically different. */
struct target_type arc700_target = {
	.name = "arc700",

	.poll =	arc_ocd_poll,

	.arch_state = arc32_arch_state,

	.target_request_data = NULL,

	.halt = arc_dbg_halt,
	.resume = arc_dbg_resume,
	.step = arc_dbg_step,

	.assert_reset = arc_ocd_assert_reset,
	.deassert_reset = arc_ocd_deassert_reset,

	.soft_reset_halt = NULL,

	.get_gdb_reg_list = arc_regs_get_gdb_reg_list,

	.read_memory = arc_mem_read,
	.write_memory = arc_mem_write,
	.checksum_memory = arc_mem_checksum,
	.blank_check_memory = arc_mem_blank_check,

	.add_breakpoint = arc_dbg_add_breakpoint,
	.add_context_breakpoint = arc_dbg_add_context_breakpoint,
	.add_hybrid_breakpoint = arc_dbg_add_hybrid_breakpoint,
	.remove_breakpoint = arc_dbg_remove_breakpoint,
	.add_watchpoint = arc_dbg_add_watchpoint,
	.remove_watchpoint = arc_dbg_remove_watchpoint,

	.run_algorithm = arc_mem_run_algorithm,
	.start_algorithm = arc_mem_start_algorithm,
	.wait_algorithm = arc_mem_wait_algorithm,

	.commands = arc_monitor_command_handlers, /* see: arc_mntr.c|.h */

	.target_create = arc_ocd_target_create,
	.init_target = arc_ocd_init_target,
	.examine = arc_ocd_examine,

	.virt2phys = arc_mem_virt2phys,
	.read_phys_memory = arc_mem_read_phys_memory,
	.write_phys_memory = arc_mem_write_phys_memory,
	.mmu = arc_mem_mmu,
};

/* ARC v2 target */
struct target_type arcv2_target = {
	.name = "arcv2",

	.poll =	arc_ocd_poll,

	.arch_state = arc32_arch_state,

	/* TODO That seems like something similiar to metaware hostlink, so perhaps
	 * we can exploit this in the future. */
	.target_request_data = NULL,

	.halt = arc_dbg_halt,
	.resume = arc_dbg_resume,
	.step = arc_dbg_step,

	.assert_reset = arc_ocd_assert_reset,
	.deassert_reset = arc_ocd_deassert_reset,

	/* TODO Implement soft_reset_halt */
	.soft_reset_halt = NULL,

	.get_gdb_reg_list = arc_regs_get_gdb_reg_list,

	.read_memory = arc_mem_read,
	.write_memory = arc_mem_write,
	.checksum_memory = arc_mem_checksum,
	.blank_check_memory = arc_mem_blank_check,

	.add_breakpoint = arc_dbg_add_breakpoint,
	.add_context_breakpoint = arc_dbg_add_context_breakpoint,
	.add_hybrid_breakpoint = arc_dbg_add_hybrid_breakpoint,
	.remove_breakpoint = arc_dbg_remove_breakpoint,
	.add_watchpoint = arc_dbg_add_watchpoint,
	.remove_watchpoint = arc_dbg_remove_watchpoint,
	.hit_watchpoint = arc_hit_watchpoint,

	.run_algorithm = arc_mem_run_algorithm,
	.start_algorithm = arc_mem_start_algorithm,
	.wait_algorithm = arc_mem_wait_algorithm,

	.commands = arc_monitor_command_handlers, /* see: arc_mntr.c|.h */

	.target_create = arc_ocd_target_create,
	.init_target = arc_ocd_init_target,
	.examine = arc_ocd_examine,

	.virt2phys = arc_mem_virt2phys,
	.read_phys_memory = arc_mem_read_phys_memory,
	.write_phys_memory = arc_mem_write_phys_memory,
	.mmu = arc_mem_mmu,
};

