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

#include "arc32.h"

/* --------------------------------------------------------------------------
 *
 *   ARC targets expose command interface.
 *   It can be accessed via GDB through the (gdb) monitor command.
 *
 * ------------------------------------------------------------------------- */

/* TODO Use generic "reg" command to access registers where possible. This
 * command is deprecated and is left only to access extension registers. After
 * we will support flexible target configuration this command shall be removed.
 * */
COMMAND_HANDLER(handle_read_core_reg_command)
{
	uint32_t reg_nbr, value;

	struct target *target = get_current_target(CMD_CTX);
	struct arc32_common *arc32 = target_to_arc32(target);

	struct target_list *head;
	head = target->head;

	if (target->state != TARGET_HALTED) {
		command_print(CMD_CTX, "NOTE: target must be HALTED for \"%s\" command",
			CMD_NAME);
		return ERROR_OK;
	}

	if (head == (struct target_list *)NULL) {

		if (CMD_ARGC >= 1) {
			COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], reg_nbr);
			LOG_DEBUG("CMD_ARGC:%u  CMD_ARGV: 0x%" PRIx32, CMD_ARGC, reg_nbr);
			CHECK_RETVAL(arc_jtag_read_core_reg_one(&arc32->jtag_info, reg_nbr, &value));
			LOG_INFO("Core reg: 0x%" PRIx32 " contains: 0x%08" PRIx32, reg_nbr, value);
		} else
			LOG_ERROR(" > missing reg nbr to read.");
	} else
		LOG_ERROR(" > head list is not NULL !");

	return ERROR_OK;
}

/* TODO Use generic "reg" command to access registers where possible. This
 * command is deprecated and is left only to access extension registers. After
 * we will support flexible target configuration this command shall be removed.
 * */
COMMAND_HANDLER(handle_write_core_reg_command)
{
	uint32_t reg_nbr, value;

	struct target *target = get_current_target(CMD_CTX);
	struct arc32_common *arc32 = target_to_arc32(target);

	struct target_list *head;
	head = target->head;

	if (target->state != TARGET_HALTED) {
		command_print(CMD_CTX, "NOTE: target must be HALTED for \"%s\" command",
			CMD_NAME);
		return ERROR_OK;
	}

	if (head == (struct target_list *)NULL) {

		if (CMD_ARGC >= 2) {
			COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], reg_nbr);
			LOG_DEBUG("CMD_ARGC:%u  CMD_ARGV: 0x%" PRIx32, CMD_ARGC, reg_nbr);
			COMMAND_PARSE_NUMBER(u32, CMD_ARGV[1], value);
			LOG_DEBUG("CMD_ARGC:%u  CMD_ARGV: 0x%08" PRIx32, CMD_ARGC, value);
			CHECK_RETVAL(arc_jtag_write_core_reg_one(&arc32->jtag_info, reg_nbr, value));
			LOG_DEBUG("Core reg: 0x%" PRIx32 " contains: 0x%08" PRIx32, reg_nbr, value);
		} else
			LOG_ERROR(" > missing reg nbr or value to write.");
	} else
		LOG_ERROR(" > head list is not NULL !");

	return ERROR_OK;
}

/* TODO Use generic "reg" command to access registers where possible. This
 * command is deprecated and is left only to access extension registers. After
 * we will support flexible target configuration this command shall be removed.
 * */
COMMAND_HANDLER(handle_read_aux_reg_command)
{
	uint32_t reg_nbr, value;

	struct target *target = get_current_target(CMD_CTX);
	struct arc32_common *arc32 = target_to_arc32(target);

	struct target_list *head;
	head = target->head;

	if (target->state != TARGET_HALTED) {
		command_print(CMD_CTX, "NOTE: target must be HALTED for \"%s\" command",
			CMD_NAME);
		return ERROR_OK;
	}

	if (head == (struct target_list *)NULL) {

		if (CMD_ARGC >= 1) {
			COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], reg_nbr);
			LOG_DEBUG("CMD_ARGC:%u  CMD_ARGV: 0x%" PRIx32, CMD_ARGC, reg_nbr);
			CHECK_RETVAL(arc_jtag_read_aux_reg_one(&arc32->jtag_info, reg_nbr, &value));
			LOG_ERROR("AUX reg: 0x%" PRIx32 " contains: 0x%08" PRIx32, reg_nbr, value);
		} else
			LOG_ERROR(" > missing reg nbr to read.");
	} else
		LOG_ERROR(" > head list is not NULL !");

	return ERROR_OK;
}

/* TODO Use generic "reg" command to access registers where possible. This
 * command is deprecated and is left only to access extension registers. After
 * we will support flexible target configuration this command shall be removed.
 * */
COMMAND_HANDLER(handle_write_aux_reg_command)
{
	uint32_t reg_nbr, value;

	struct target *target = get_current_target(CMD_CTX);
	struct arc32_common *arc32 = target_to_arc32(target);

	struct target_list *head;
	head = target->head;

	if (target->state != TARGET_HALTED) {
		command_print(CMD_CTX, "NOTE: target must be HALTED for \"%s\" command",
			CMD_NAME);
		return ERROR_OK;
	}

	if (head == (struct target_list *)NULL) {

		if (CMD_ARGC >= 2) {
			COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], reg_nbr);
			LOG_DEBUG("CMD_ARGC:%u  CMD_ARGV: 0x%" PRIx32, CMD_ARGC, reg_nbr);
			COMMAND_PARSE_NUMBER(u32, CMD_ARGV[1], value);
			LOG_DEBUG("CMD_ARGC:%u  CMD_ARGV: 0x%08" PRIx32, CMD_ARGC, value);
			CHECK_RETVAL(arc_jtag_write_aux_reg_one(&arc32->jtag_info, reg_nbr, value));
			LOG_DEBUG("AUX reg: 0x%x contains: 0x%x", reg_nbr, value);
		} else
			LOG_ERROR(" > missing reg nbr or value to write.");
	} else
		LOG_ERROR(" > head list is not NULL !");

	return ERROR_OK;
}

COMMAND_HANDLER(arc_handle_has_dcache)
{
	struct target *target = get_current_target(CMD_CTX);
	struct arc32_common *arc32 = target_to_arc32(target);
	return CALL_COMMAND_HANDLER(handle_command_parse_bool,
		&arc32->has_dcache, "target has data-cache");
}

COMMAND_HANDLER(arc_handle_gdb_compatibility_mode)
{
	struct target *target = get_current_target(CMD_CTX);
	struct arc32_common *arc32 = target_to_arc32(target);
	return CALL_COMMAND_HANDLER(handle_command_parse_bool,
		&arc32->gdb_compatibility_mode, "GDB compatibility mode");
}


/* JTAG layer commands */
COMMAND_HANDLER(arc_cmd_handle_jtag_check_status_rd)
{
	struct target *target = get_current_target(CMD_CTX);
	struct arc32_common *arc32 = target_to_arc32(target);
	return CALL_COMMAND_HANDLER(handle_command_parse_bool,
		&arc32->jtag_info.always_check_status_rd, "Always check JTAG Status RD bit");

}

COMMAND_HANDLER(arc_cmd_handle_jtag_check_status_fl)
{
	struct target *target = get_current_target(CMD_CTX);
	struct arc32_common *arc32 = target_to_arc32(target);
	return CALL_COMMAND_HANDLER(handle_command_parse_bool,
		&arc32->jtag_info.check_status_fl, "Check JTAG Status FL bit after transaction");

}

static int arc_cmd_jim_get_uint32(Jim_GetOptInfo *goi, uint32_t *value)
{
	jim_wide value_wide;
	JIM_CHECK_RETVAL(Jim_GetOpt_Wide(goi, &value_wide));
	*value = (uint32_t)value_wide;
	return JIM_OK;
}

static int jim_arc_aux_reg(Jim_Interp *interp, int argc, Jim_Obj * const *argv)
{
	Jim_GetOptInfo goi;
	Jim_GetOpt_Setup(&goi, interp, argc-1, argv+1);

	if (goi.argc == 0 || goi.argc > 2) {
		Jim_SetResultFormatted(goi.interp,
			"usage: %s <aux_reg_num>", Jim_GetString(argv[0], NULL));
		return JIM_ERR;
	}

	struct command_context *context;
	struct target *target;

	context = current_command_context(interp);
	assert(context);

	target = get_current_target(context);
	if (!target) {
		Jim_SetResultFormatted(goi.interp, "No current target");
		return JIM_ERR;
	}

	/* Register number */
	uint32_t regnum;
	JIM_CHECK_RETVAL(arc_cmd_jim_get_uint32(&goi, &regnum));

	/* Register value */
	bool do_write = false;
	uint32_t value;
	if (goi.argc == 1) {
		do_write = true;
		JIM_CHECK_RETVAL(arc_cmd_jim_get_uint32(&goi, &value));
	}

	struct arc32_common *arc32 = target_to_arc32(target);
	assert(arc32);

	if (do_write) {
		CHECK_RETVAL(arc_jtag_write_aux_reg_one(&arc32->jtag_info, regnum, value));
	} else {
		CHECK_RETVAL(arc_jtag_read_aux_reg_one(&arc32->jtag_info, regnum, &value));
		Jim_SetResultInt(interp, value);
	}

	return ERROR_OK;
}

static int jim_arc_core_reg(Jim_Interp *interp, int argc, Jim_Obj * const *argv)
{
	Jim_GetOptInfo goi;
	Jim_GetOpt_Setup(&goi, interp, argc-1, argv+1);

	if (goi.argc == 0 || goi.argc > 2) {
		Jim_SetResultFormatted(goi.interp,
			"usage: %s <core_reg_num>", Jim_GetString(argv[0], NULL));
		return JIM_ERR;
	}

	struct command_context *context;
	struct target *target;

	context = current_command_context(interp);
	assert(context);

	target = get_current_target(context);
	if (!target) {
		Jim_SetResultFormatted(goi.interp, "No current target");
		return JIM_ERR;
	}

	/* Register number */
	uint32_t regnum;
	JIM_CHECK_RETVAL(arc_cmd_jim_get_uint32(&goi, &regnum));
	if (regnum > 63 || regnum == 61 || regnum == 62) {
		Jim_SetResultFormatted(goi.interp, "Core register number %i " \
			"is invalid. Must less then 64 and not 61 and 62.", regnum);
		return JIM_ERR;
	}

	/* Register value */
	bool do_write = false;
	uint32_t value;
	if (goi.argc == 1) {
		do_write = true;
		JIM_CHECK_RETVAL(arc_cmd_jim_get_uint32(&goi, &value));
	}

	struct arc32_common *arc32 = target_to_arc32(target);
	assert(arc32);

	if (do_write) {
		CHECK_RETVAL(arc_jtag_write_core_reg_one(&arc32->jtag_info, regnum, value));
	} else {
		CHECK_RETVAL(arc_jtag_read_core_reg_one(&arc32->jtag_info, regnum, &value));
		Jim_SetResultInt(interp, value);
	}

	return ERROR_OK;
}

static const struct command_registration arc_jtag_command_group[] = {
	{
		.name = "always-check-status-rd",
		.handler = arc_cmd_handle_jtag_check_status_rd,
		.mode = COMMAND_ANY,
		.usage = "on|off",
		.help = "If true we will check for JTAG status register and " \
			"whether 'ready' bit is set each time before doing any " \
			"JTAG operations. By default that is off.",
	},
	{
		.name = "check-status-fl",
		.handler = arc_cmd_handle_jtag_check_status_fl,
		.mode = COMMAND_ANY,
		.usage = "on|off",
		.help = "If true we will check for JTAG status FL bit after all JTAG " \
			 "transaction. This is disabled by default because it is " \
			 "known to break JTAG module in the core.",
	},
	{
		.name = "aux-reg",
		.jim_handler = jim_arc_aux_reg,
		.mode = COMMAND_EXEC,
		.help = "Get/Set AUX register by number. This command does a " \
			"raw JTAG request that bypasses OpenOCD register cache "\
			"and thus is unsafe and can have unexpected consequences. "\
			"Use at your own risk.",
		.usage = "<regnum> [<value>]"
	},
	{
		.name = "core-reg",
		.jim_handler = jim_arc_core_reg,
		.mode = COMMAND_EXEC,
		.help = "Get/Set core register by number. This command does a " \
			"raw JTAG request that bypasses OpenOCD register cache "\
			"and thus is unsafe and can have unexpected consequences. "\
			"Use at your own risk.",
		.usage = "<regnum> [<value>]"
	},
	COMMAND_REGISTRATION_DONE
};

/* ----- Exported target commands ------------------------------------------ */

static const struct command_registration arc_core_command_handlers[] = {
	{
		.name = "read-core-reg",
		.handler = handle_read_core_reg_command,
		.mode = COMMAND_EXEC,
		.usage = "has one argument: <reg-nbr>",
		.help = "list the content of a particular core registers",
	},
	{
		.name = "write-core-reg",
		.handler = handle_write_core_reg_command,
		.mode = COMMAND_EXEC,
		.usage = "has two argument: <reg-nbr> <value to write>",
		.help = "write value to a particular core registers",
	},
	{
		.name = "read-aux-reg",
		.handler = handle_read_aux_reg_command,
		.mode = COMMAND_EXEC,
		.usage = "has one argument: <reg-nbr>",
		.help = "list the content of a particular aux registers",
	},
	{
		.name = "write-aux-reg",
		.handler = handle_write_aux_reg_command,
		.mode = COMMAND_EXEC,
		.usage = "has two argument: <reg-nbr> <value to write>",
		.help = "write value to a particular aux registers",
	},
	{
		.name = "has-dcache",
		.handler = arc_handle_has_dcache,
		.mode = COMMAND_ANY,
		.usage = "True or false",
		.help = "Does target has D$? If yes it will be flushed before memory reads.",
	},
	{
		.name = "gdb-compatibility-mode",
		.handler = arc_handle_gdb_compatibility_mode,
		.mode = COMMAND_CONFIG,
		.usage = "true or false",
		.help = "GDB compatibility mode: if true OpenOCD will use register "\
			"specification compatible with old GDB for ARC that doesn't support "\
			"XML target descriptions.",
	},
	{
		.name = "jtag",
		.mode = COMMAND_ANY,
		.help = "ARC JTAG specific commands",
		.usage = "",
		.chain = arc_jtag_command_group,
	},
	COMMAND_REGISTRATION_DONE
};

const struct command_registration arc_monitor_command_handlers[] = {
	{
		.name = "arc",
		.mode = COMMAND_ANY,
		.help = "ARC monitor command group",
		.usage = "Help info ...",
		.chain = arc_core_command_handlers,
	},
	COMMAND_REGISTRATION_DONE
};
