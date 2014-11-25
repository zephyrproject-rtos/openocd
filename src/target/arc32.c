/***************************************************************************
 *   Copyright (C) 2013-2014 Synopsys, Inc.                                *
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

#include "arc.h"

/* ----- Supporting functions ---------------------------------------------- */

static const char *arc_isa_strings[] = {
	"ARC32", "ARC16"
};

/* ----- Exported functions ------------------------------------------------ */

int arc32_init_arch_info(struct target *target, struct arc32_common *arc32,
	struct jtag_tap *tap)
{
	arc32->common_magic = ARC32_COMMON_MAGIC;
	target->arch_info = arc32;

	arc32->fast_data_area = NULL;

	arc32->jtag_info.tap = tap;
	arc32->jtag_info.scann_size = 4;

	/* has breakpoint/watchpoint unit been scanned */
	arc32->bp_scanned = 0;
	arc32->data_break_list = NULL;

	/* Flush D$ by default. It is safe to assume that D$ is present,
	 * because if it isn't, there will be no error, just a slight
	 * performance penalty from unnecessary JTAG operations. */
	arc32->has_dcache = true;
	arc32_reset_caches_states(target);

	arc32->gdb_compatibility_mode = true;

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
	const uint32_t core_regs_size = ARC_REG_FIRST_AUX * sizeof(uint32_t);
	const uint32_t regs_to_scan = (arc32->gdb_compatibility_mode ?
			ARC_TOTAL_NUM_REGS : ARC_REG_AFTER_GDB_GENERAL);
	const uint32_t aux_regs_size = (regs_to_scan - ARC_REG_FIRST_AUX) *
		sizeof(uint32_t);
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

	for (i = 0; i < regs_to_scan; i++) {
		struct reg *reg = &(reg_list[i]);
		struct arc_reg_t *arc_reg = reg->arch_info;
		if (!reg->valid && reg->exist && !arc_reg->dummy) {
			if (arc_reg->desc->regnum < ARC_REG_FIRST_AUX) {
				/* core reg */
				core_addrs[core_cnt] = arc_reg->desc->addr;
				core_cnt += 1;
			} else {
				/* aux reg */
				aux_addrs[aux_cnt] = arc_reg->desc->addr;
				aux_cnt += 1;
			}
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
	for (i = 0; i < ARC_REG_FIRST_AUX; i++) {
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
	for (i = ARC_REG_FIRST_AUX; i < regs_to_scan; i++) {
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
			LOG_DEBUG("Get aux register regnum=%" PRIu32 ", name=%s, value=0x%" PRIx32,
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
	const uint32_t core_regs_size = ARC_REG_AFTER_CORE_EXT * sizeof(uint32_t);
	const uint32_t aux_regs_size = (ARC_REG_AFTER_AUX - ARC_REG_FIRST_AUX) *
		sizeof(uint32_t);
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

	for (i = 0; i < ARC_REG_AFTER_AUX; i++) {
		struct reg *reg = &(reg_list[i]);
		struct arc_reg_t *arc_reg = reg->arch_info;
		if (reg->valid && reg->exist && reg->dirty) {
			LOG_DEBUG("Will write regnum=%u", i);
			if (arc_reg->desc->regnum < ARC_REG_FIRST_AUX) {
				/* core reg */
				core_addrs[core_cnt] = arc_reg->desc->addr;
				core_values[core_cnt] = arc_reg->value;
				core_cnt += 1;
			} else {
				/* aux reg */
				aux_addrs[aux_cnt] = arc_reg->desc->addr;
				aux_values[aux_cnt] = arc_reg->value;
				aux_cnt += 1;
			}
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

		//retval = arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_DEBUG_REG, &value);
		value = SET_CORE_SINGLE_INSTR_STEP; /* set the IS bit */

		if (arc32->processor_type == ARC600_NUM) {
			value |= SET_CORE_SINGLE_STEP;  /* set the SS bit */
			LOG_DEBUG("ARC600 extra single step bit to set.");
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
	CHECK_RETVAL(arc_jtag_read_aux_reg_one(&arc32->jtag_info, AUX_STATUS_REG, &value));
	LOG_DEBUG("          [STATUS]: 0x%08" PRIx32, value);
	CHECK_RETVAL(arc_jtag_read_aux_reg_one(&arc32->jtag_info, AUX_PC_REG, &value));
	LOG_DEBUG("              [PC]: 0x%08" PRIx32, value);

	return ERROR_OK;
}

int arc32_arch_state(struct target *target)
{
	struct arc32_common *arc32 = target_to_arc32(target);

	LOG_DEBUG("target state: %s in: %s mode, PC at: 0x%08" PRIx32,
		target_state_name(target),
		arc_isa_strings[arc32->isa_mode],
		buf_get_u32(arc32->core_cache->reg_list[ARC_REG_PC].value, 0, 32));


	return ERROR_OK;
}

int arc32_get_current_pc(struct target *target)
{
	uint32_t dpc;
	struct arc32_common *arc32 = target_to_arc32(target);

	/* read current PC */
	CHECK_RETVAL(arc_jtag_read_aux_reg_one(&arc32->jtag_info, AUX_PC_REG, &dpc));

	/* save current PC */
	buf_set_u32(arc32->core_cache->reg_list[ARC_REG_PC].value, 0, 32, dpc);

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


/* Configure some core features, depending on BCRs. */
int arc32_configure(struct target *target)
{
	LOG_DEBUG("-");
	struct arc32_common *arc32 = target_to_arc32(target);
	struct bcr_set_t *bcrs = &(arc32->bcr_set);

	/* DCCM */
	if (bcrs->dccm_build.version >= 3 && bcrs->dccm_build.size0 > 0) {
		CHECK_RETVAL(arc_jtag_read_aux_reg_one(&arc32->jtag_info, AUX_DCCM, &(arc32->dccm_start)));
		arc32_address_t dccm_size = 0x100;
		dccm_size <<= bcrs->dccm_build.size0;
		if (bcrs->dccm_build.size0 == 0xF)
			dccm_size <<= bcrs->dccm_build.size1;
		arc32->dccm_end = arc32->dccm_start + dccm_size;
		LOG_DEBUG("DCCM detected start=0x%" PRIx32 " end=0x%" PRIx32, arc32->dccm_start, arc32->dccm_end);
	} else {
		arc32->dccm_start = 0;
		arc32->dccm_end = 0;
	}

	/* ICCM0 */
	arc32_address_t aux_iccm = 0;
	if (bcrs->iccm_build.version >= 4 && bcrs->iccm_build.iccm0_size0 > 0) {
		CHECK_RETVAL(arc_jtag_read_aux_reg_one(&arc32->jtag_info, AUX_ICCM, &aux_iccm));
		arc32_address_t iccm0_size = 0x100;
		iccm0_size <<= bcrs->iccm_build.iccm0_size0;
		if (bcrs->iccm_build.iccm0_size0 == 0xF)
			iccm0_size <<= bcrs->iccm_build.iccm0_size1;
		arc32->iccm0_start = aux_iccm & (0xF0000000 >> (32 - arc_regs_addr_size_bits(bcrs)));
		arc32->iccm0_end = arc32->iccm0_start + iccm0_size;
		LOG_DEBUG("ICCM0 detected start=0x%" PRIx32 " end=0x%" PRIx32, arc32->iccm0_start, arc32->iccm0_end);
	} else {
		arc32->iccm0_start = 0;
		arc32->iccm0_end = 0;
	}

	/* ICCM1 */
	if (bcrs->iccm_build.version >= 4 && bcrs->iccm_build.iccm1_size0 > 0) {
		/* Use value read for ICCM0 */
		if (!aux_iccm)
			CHECK_RETVAL(arc_jtag_read_aux_reg_one(&arc32->jtag_info, AUX_ICCM, &aux_iccm));
		arc32_address_t iccm1_size = 0x100;
		iccm1_size <<= bcrs->iccm_build.iccm1_size0;
		if (bcrs->iccm_build.iccm1_size0 == 0xF)
			iccm1_size <<= bcrs->iccm_build.iccm1_size1;
		arc32->iccm1_start = aux_iccm & (0x0F000000 >> (32 - arc_regs_addr_size_bits(bcrs)));
		arc32->iccm1_end = arc32->iccm1_start + iccm1_size;
		LOG_DEBUG("ICCM1 detected start=0x%" PRIx32 " end=0x%" PRIx32, arc32->iccm1_start, arc32->iccm1_end);
	} else {
		arc32->iccm1_start = 0;
		arc32->iccm1_end = 0;
	}

	return ERROR_OK;
}

