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

#include "arc.h"

static int PRINT = 1;

/* ----- Supporting functions ---------------------------------------------- */

static const char *arc_isa_strings[] = {
	"ARC32", "ARC16"
};

/* ----- Exported functions ------------------------------------------------ */

int arc32_init_arch_info(struct target *target, struct arc32_common *arc32,
	struct jtag_tap *tap)
{
	int retval = ERROR_OK;

	arc32->common_magic = ARC32_COMMON_MAGIC;
	target->arch_info = arc32;

	arc32->fast_data_area = NULL;

	arc32->jtag_info.tap = tap;
	arc32->jtag_info.scann_size = 4;

	/* has breakpoint/watchpoint unit been scanned */
	arc32->bp_scanned = 0;
	arc32->data_break_list = NULL;

	arc32->read_core_reg = arc_regs_read_core_reg;
	arc32->write_core_reg = arc_regs_write_core_reg;

	/* Flush D$ by default. It is safe to assume that D$ is present,
	 * because if it isn't, there will be no error, just a slight
	 * performance penalty from unnecessary JTAG operations. */
	arc32->has_dcache = true;
	arc32_reset_caches_states(target);

	return retval;
}

int arc32_save_context(struct target *target)
{
	int retval = ERROR_OK;
	int i;
	struct arc32_common *arc32 = target_to_arc32(target);

	retval = arc_regs_read_registers(target, arc32->core_regs);
	if (retval != ERROR_OK)
		return retval;

	for (i = 0; i < ARC32_NUM_GDB_REGS; i++) {
		if (!arc32->core_cache->reg_list[i].valid)
			arc32->read_core_reg(target, i);
	}

	return ERROR_OK;
}

int arc32_restore_context(struct target *target)
{
	int retval = ERROR_OK;
	int i;
	struct arc32_common *arc32 = target_to_arc32(target);

	for (i = 0; i < ARC32_NUM_GDB_REGS; i++) {
		if (arc32->core_cache->reg_list[i].dirty)
			arc32->write_core_reg(target, i);
	}

	retval = arc_regs_write_registers(target, arc32->core_regs);
	if (retval != ERROR_OK)
		return retval;

	return ERROR_OK;
}

int arc32_enable_interrupts(struct target *target, int enable)
{
	int retval = ERROR_OK;
	uint32_t value;

	struct arc32_common *arc32 = target_to_arc32(target);

	if (enable) {
		/* enable interrupts */
		value = SET_CORE_ENABLE_INTERRUPTS;
		retval = arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_IENABLE_REG, &value);
		LOG_DEBUG("interrupts enabled [stat:%d]", retval);
	} else {
		/* disable interrupts */
		value = SET_CORE_DISABLE_INTERRUPTS;
		retval = arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_IENABLE_REG, &value);
		LOG_DEBUG("interrupts disabled [stat:%d]", retval);
	}

	return retval;
}

int arc32_start_core(struct target *target)
{
	int retval = ERROR_OK;
	uint32_t value;

	struct arc32_common *arc32 = target_to_arc32(target);

	target->state = TARGET_RUNNING;

	retval = arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_STATUS32_REG, &value);
	value &= ~SET_CORE_HALT_BIT;        /* clear the HALT bit */
	retval = arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_STATUS32_REG, &value);
	LOG_DEBUG(" ARC Core Started Again (eating instructions)");

#ifdef DEBUG
	arc32_print_core_state(target);
#endif
	return retval;
}

int arc32_config_step(struct target *target, int enable_step)
{
	int retval = ERROR_OK;
	uint32_t value;

	struct arc32_common *arc32 = target_to_arc32(target);

	if (enable_step) {
		/* enable core debug step mode */
		retval = arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_STATUS32_REG,
			&value);
		value &= ~SET_CORE_AE_BIT; /* clear the AE bit */
		retval = arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_STATUS32_REG,
			&value);
		LOG_DEBUG(" [status32:0x%x] [stat:%d]", value, retval);

		//retval = arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_DEBUG_REG, &value);
		value = SET_CORE_SINGLE_INSTR_STEP; /* set the IS bit */

		if (arc32->processor_type == ARC600_NUM) {
			value |= SET_CORE_SINGLE_STEP;  /* set the SS bit */
			LOG_DEBUG("ARC600 extra single step bit to set.");
		}

		retval = arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_DEBUG_REG,
			&value);
		LOG_DEBUG("core debug step mode enabled [debug-reg:0x%x] [stat:%d]",
			value, retval);
	} else {
		/* disable core debug step mode */
		retval = arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_DEBUG_REG,
			&value);
		value &= ~SET_CORE_SINGLE_INSTR_STEP; /* clear the IS bit */
		retval = arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_DEBUG_REG,
			&value);
		LOG_DEBUG("core debug step mode disabled [stat:%d]", retval);
	}

#ifdef DEBUG
	arc32_print_core_state(target);
#endif
	return retval;
}

/* This function is cheap to call and returns quickly if caches already has
 * been invalidated since core had been halted. */
int arc32_cache_invalidate(struct target *target)
{
	int retval = ERROR_OK;
	uint32_t value, backup;

	struct arc32_common *arc32 = target_to_arc32(target);

	/* Don't waste time if already done. */
	if (arc32->cache_invalidated)
	    return ERROR_OK;

	LOG_DEBUG("Invalidating I$ & D$.");

	value = IC_IVIC_INVALIDATE;	/* invalidate I$ */
	retval = arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_IC_IVIC_REG, &value);
	retval = arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_DC_CTRL_REG, &value);

	backup = value;
	value = value & ~DC_CTRL_IM;

	/* set DC_CTRL invalidate mode to invalidate-only (no flushing!!) */
	retval = arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_DC_CTRL_REG, &value);
	value = DC_IVDC_INVALIDATE;	/* invalidate D$ */
	retval = arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_DC_IVDC_REG, &value);

	/* restore DC_CTRL invalidate mode */
	retval = arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_DC_CTRL_REG, &backup);

	arc32->cache_invalidated = true;

	return retval;
}

/* Flush data cache. This function is cheap to call and return quickly if D$
 * already has been flushed since target had been halted. JTAG debugger reads
 * values directly from memory, bypassing cache, so if there are unflushed
 * lines debugger will read invalid values, which will cause a lot of troubles.
 * */
int arc32_dcache_flush(struct target *target)
{
	int retval = ERROR_OK;
	uint32_t value, dc_ctrl_value;
	bool has_to_set_dc_ctrl_im;

	struct arc32_common *arc32 = target_to_arc32(target);

	/* Don't waste time if already done. */
	if (!arc32->has_dcache || arc32->dcache_flushed)
	    return ERROR_OK;

	LOG_DEBUG("Flushing D$.");

	/* Store current value of DC_CTRL */
	retval = arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_DC_CTRL_REG, &dc_ctrl_value);
	if (ERROR_OK != retval)
	    return retval;

	/* Set DC_CTRL invalidate mode to flush (if not already set) */
	has_to_set_dc_ctrl_im = (dc_ctrl_value & DC_CTRL_IM) == 0;
	if (has_to_set_dc_ctrl_im) {
		value = dc_ctrl_value | DC_CTRL_IM;
		retval = arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_DC_CTRL_REG, &value);
		if (ERROR_OK != retval)
		    return retval;
	}

	/* Flush D$ */
	value = DC_IVDC_INVALIDATE;
	retval = arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_DC_IVDC_REG, &value);

	/* Restore DC_CTRL invalidate mode (even of flush failed) */
	if (has_to_set_dc_ctrl_im) {
	    retval = arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_DC_CTRL_REG, &dc_ctrl_value);
	}

	arc32->dcache_flushed = true;

	return retval;
}

int arc32_wait_until_core_is_halted(struct target *target)
{
	int retval = ERROR_OK;
	uint32_t value = 1; /* get us into checking for HALT bit */

	struct arc32_common *arc32 = target_to_arc32(target);

//#ifdef DEBUG
	arc32_print_core_state(target);
//#endif

	/*
	 * check if bit N starting from 0 is set; temp & (1 << N)
	 *   here, lets check if we are HALTED
	 */
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_STATUS32_REG, &value);
	while (!(value & 1)) {
		printf(".");
		arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_STATUS32_REG, &value);
	}

#ifdef DEBUG
	arc32_print_core_state(target);
#endif

	return retval;
}

int arc32_print_core_state(struct target *target)
{
	int retval = ERROR_OK;
	struct arc32_common *arc32 = target_to_arc32(target);
	uint32_t value;

	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_DEBUG_REG, &value);
	LOG_DEBUG("  AUX REG  [DEBUG]: 0x%x", value);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_STATUS32_REG, &value);
	LOG_DEBUG("        [STATUS32]: 0x%x", value);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_STATUS_REG, &value);
	LOG_DEBUG("          [STATUS]: 0x%x", value);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_PC_REG, &value);
	LOG_DEBUG("              [PC]: 0x%x", value);

	return retval;
}

int arc32_arch_state(struct target *target)
{
	int retval = ERROR_OK;
	struct arc32_common *arc32 = target_to_arc32(target);

	if (PRINT) {
		LOG_USER("target state: %s in: %s mode, PC at: 0x%8.8" PRIx32 "",
			target_state_name(target),
			arc_isa_strings[arc32->isa_mode],
			buf_get_u32(arc32->core_cache->reg_list[PC_REG].value, 0, 32));
		PRINT = 0; /* due to endless looping through ;-) */
	}

	return retval;
}

int arc32_get_current_pc(struct target *target)
{
	int retval = ERROR_OK;
	uint32_t dpc;
	struct arc32_common *arc32 = target_to_arc32(target);

	/* read current PC */
	retval = arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_PC_REG, &dpc);

	/* save current PC */
	buf_set_u32(arc32->core_cache->reg_list[PC_REG].value, 0, 32, dpc);

	return retval;
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

