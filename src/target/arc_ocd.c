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

static int arc_ocd_init_arch_info(struct target *target,
	struct arc_common *arc, struct jtag_tap *tap)
{
	int retval = ERROR_OK;
	struct arc32_common *arc32 = &arc->arc32;

	LOG_DEBUG("  >>> Calling into <<<");

	arc->common_magic = ARC_COMMON_MAGIC;

	/* initialize arc specific info */
	retval = arc32_init_arch_info(target, arc32, tap);
	arc32->arch_info = arc;

	return retval;
}

/* ----- Exported functions ------------------------------------------------ */

int arc_ocd_poll(struct target *target)
{
	int retval = ERROR_OK;
	uint32_t status;
	struct arc32_common *arc32 = target_to_arc32(target);

	/* gdb calls continuously through this arc_poll() function  */
	retval = arc_jtag_status(&arc32->jtag_info, &status);
	if (retval != ERROR_OK)
		return retval;

	/* check for processor halted */
	if (status & ARC_JTAG_STAT_RU) {
		LOG_USER(" >> target is still running !!");
		target->state = TARGET_RUNNING;
	} else {
		if ((target->state == TARGET_RUNNING) ||
			(target->state == TARGET_RESET)) {

			target->state = TARGET_HALTED;
			LOG_DEBUG("ARC core is halted or in reset.\n");

			retval = arc_dbg_debug_entry(target);
			if (retval != ERROR_OK)
				return retval;

			target_call_event_callbacks(target, TARGET_EVENT_HALTED);
		} else if (target->state == TARGET_DEBUG_RUNNING) {

			target->state = TARGET_HALTED;
			LOG_USER("ARC core is in debug running mode");

			retval = arc_dbg_debug_entry(target);
			if (retval != ERROR_OK)
				return retval;

			target_call_event_callbacks(target, TARGET_EVENT_DEBUG_HALTED);
		} else {
			if (PRINT == 1) {
				LOG_USER("OpenOCD for ARC is ready to accept:"
					" (gdb) target remote <host ip address>:3333");
				PRINT = 0;
			}
		}
	}

	return retval;
}

/* ......................................................................... */

int arc_ocd_assert_reset(struct target *target)
{
	int retval = ERROR_OK;
	struct arc32_common *arc32 = target_to_arc32(target);

	LOG_DEBUG("target->state: %s", target_state_name(target));

	enum reset_types jtag_reset_config = jtag_get_reset_config();

	/* some cores support connecting while srst is asserted
	 * use that mode is it has been configured */

	bool srst_asserted = false;

	if (!(jtag_reset_config & RESET_SRST_PULLS_TRST) &&
			(jtag_reset_config & RESET_SRST_NO_GATING)) {
		jtag_add_reset(0, 1);
		srst_asserted = true;
	}

	if (jtag_reset_config & RESET_HAS_SRST) {
		/* should issue a srst only, but we may have to assert trst as well */
		if (jtag_reset_config & RESET_SRST_PULLS_TRST)
			jtag_add_reset(1, 1);
		else if (!srst_asserted)
			jtag_add_reset(0, 1);
	}

	target->state = TARGET_RESET;
	jtag_add_sleep(50000);

	register_cache_invalidate(arc32->core_cache);

	if (target->reset_halt)
		retval = target_halt(target);

	return retval;
}

int arc_ocd_deassert_reset(struct target *target)
{
	int retval = ERROR_OK;

	LOG_DEBUG("target->state: %s", target_state_name(target));

	/* deassert reset lines */
	jtag_add_reset(0, 0);

	return retval;
}

/* ......................................................................... */

int arc_ocd_target_create(struct target *target, Jim_Interp *interp)
{
	int retval = ERROR_OK;

	struct arc_common *arc = calloc(1, sizeof(struct arc_common));

	retval = arc_ocd_init_arch_info(target, arc, target->tap);

	return retval;
}

int arc_ocd_init_target(struct command_context *cmd_ctx, struct target *target)
{
	int retval = ERROR_OK;

	arc_regs_build_reg_cache(target);

	return retval;
}

int arc_ocd_examine(struct target *target)
{
	int retval = ERROR_OK;
	uint32_t value, status;
	struct arc32_common *arc32 = target_to_arc32(target);

	retval = arc_jtag_startup(&arc32->jtag_info);

	if (!target_was_examined(target)) {

		/* read JTAG info */
		arc_jtag_idcode(&arc32->jtag_info, &value);
		LOG_USER("JTAG ID: 0x%x", value);
		arc_jtag_status(&arc32->jtag_info, &status);
		LOG_USER("JTAG status: 0x%x", status);

		/* bring processor into HALT */
		LOG_USER("bring ARC core into HALT state");
		arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_DEBUG_REG, &value);
		value |= SET_CORE_FORCE_HALT;
		arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_DEBUG_REG, &value);
		sleep(1); /* just give us once some time to come to rest ;-) */

		arc_jtag_status(&arc32->jtag_info, &status);
		LOG_USER("JTAG status: 0x%x", status);

		/* read ARC core info */
		arc_core_type_info(target);

		arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_IDENTITY_REG, &value);
		LOG_USER("CPU ID: 0x%x", value);
		arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_PC_REG, &value);
		LOG_USER("current PC: 0x%x", value);

		arc_jtag_status(&arc32->jtag_info, &status);
		if (status & ARC_JTAG_STAT_RU) {
			LOG_USER("target is still running !!");
			target->state = TARGET_RUNNING;
		} else {
			LOG_DEBUG("target is halted.");
			target->state = TARGET_RESET; /* means HALTED after restart */
		}

		target_set_examined(target);
	}

	return retval;
}
