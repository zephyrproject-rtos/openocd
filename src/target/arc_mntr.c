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

COMMAND_HANDLER(arc_handle_has_dcache)
{
	struct target *target = get_current_target(CMD_CTX);
	struct arc32_common *arc32 = target_to_arc32(target);
	return CALL_COMMAND_HANDLER(handle_command_parse_bool,
		&arc32->has_dcache, "target has data-cache");
}

/* Add register data type */
enum add_reg_type {
	CFG_ADD_REG_TYPE_NAME,
	CFG_ADD_REG_TYPE_FLAG,
};

static Jim_Nvp nvp_add_reg_type_opts[] = {
	{ .name = "-name",  .value = CFG_ADD_REG_TYPE_NAME },
	{ .name = "-flag",  .value = CFG_ADD_REG_TYPE_FLAG },
	{ .name = NULL,     .value = -1 }
};

/* This function supports only single-bit flag fields. Target description
 * format allows for multi-bit fields in <flags> but those are not supported by
 * GDB, so there is little reason to support that here as well. */
int jim_arc_add_reg_type_flags(Jim_Interp *interp, int argc,
	Jim_Obj * const *argv)
{
	Jim_GetOptInfo goi;
	Jim_GetOpt_Setup(&goi, interp, argc-1, argv+1);

	LOG_DEBUG("-");

	int e = JIM_OK;

	/* Estimate number of registers as (argc - 2)/3 as each -field option has 2
	 * arguments while -name is required. */
	unsigned int fields_sz = (goi.argc - 2) / 3;
	unsigned int cur_field = 0;

	struct arc_reg_data_type *type = calloc(1, sizeof(struct arc_reg_data_type));
	struct reg_data_type_flags *flags =
		calloc(1, sizeof(struct reg_data_type_flags));
	struct reg_data_type_flags_field *fields = calloc(fields_sz,
			sizeof(struct reg_data_type_flags_field));
	struct reg_data_type_bitfield *bitfields = calloc(fields_sz,
			sizeof(struct reg_data_type_bitfield));

	if (!(type && flags && fields && bitfields)) {
		free(type);
		free(flags);
		free(fields);
		free(bitfields);
		Jim_SetResultFormatted(goi.interp, "Failed to allocate memory.");
		return JIM_ERR;
	}

	/* Initialize type */
	type->data_type.type = REG_TYPE_ARCH_DEFINED;
	type->data_type.type_class = REG_TYPE_CLASS_FLAGS;
	flags->size = 32; /* For now ARC has only 32-bit registers */

	while (goi.argc > 0 && e == JIM_OK) {
		Jim_Nvp *n;
		e = Jim_GetOpt_Nvp(&goi, nvp_add_reg_type_opts, &n);
		if (e != JIM_OK) {
			Jim_GetOpt_NvpUnknown(&goi, nvp_add_reg_type_opts, 0);
			continue;
		}

		switch (n->value) {
			case CFG_ADD_REG_TYPE_NAME:
			{
				char *name;
				int name_len;
				if (goi.argc == 0) {
					Jim_WrongNumArgs(interp, goi.argc, goi.argv, "-name ?name? ...");
					e = JIM_ERR;
					break;
				}

				e = Jim_GetOpt_String(&goi, &name, &name_len);
				if (e == JIM_OK) {
					type->data_type.id = strndup(name, name_len);
					if (!type->data_type.id)
						e = JIM_ERR;
				}

				break;
			}
			case CFG_ADD_REG_TYPE_FLAG:
			{
				char *field_name;
				int field_name_len;
				jim_wide position;

				if (cur_field == fields_sz) {
					/* If there are more fields than estimated, then -name
					 * hasn't been specified, but this options is required. */
					Jim_SetResultFormatted(goi.interp, "-name is a required option");
					e = JIM_ERR;
					break;
				}

				if (goi.argc < 2) {
					Jim_WrongNumArgs(interp, goi.argc, goi.argv,
						"-flag ?name? ?position? ...");
					e = JIM_ERR;
					break;
				}

				/* Field name */
				e = Jim_GetOpt_String(&goi, &field_name, &field_name_len);
				if (e != JIM_OK)
					break;

				/* Field position. start == end, because flags
				 * are one-bit fields.  */
				e = Jim_GetOpt_Wide(&goi, &position);
				if (e != JIM_OK)
					break;

				fields[cur_field].name = strndup(field_name, field_name_len);
				if (!fields[cur_field].name) {
					e = JIM_ERR;
					break;
				}
				bitfields[cur_field].start = position;
				bitfields[cur_field].end = position;
				fields[cur_field].bitfield = &(bitfields[cur_field]);
				if (cur_field > 0)
					fields[cur_field - 1].next = &(fields[cur_field]);
				else
					flags->fields = fields;

				cur_field += 1;

				break;
			}
		}
	}

	if (!type->data_type.id) {
		Jim_SetResultFormatted(goi.interp, "-name is a required option");
		e = JIM_ERR;
	}

	if (e == JIM_OK) {
		struct command_context *ctx;
		struct target *target;

		ctx = current_command_context(interp);
		assert(ctx);
		target = get_current_target(ctx);
		if (!target) {
			Jim_SetResultFormatted(goi.interp, "No current target");
			e = JIM_ERR;
		} else {
			arc32_add_reg_data_type(target, type);
		}
	}

	if (e != JIM_OK) {
		free((void*)type->data_type.id);
		free(type);
		free(flags);
		/* `fields` is zeroed, so for uninitialized fields "name" is NULL. */
		for (unsigned int i = 0; i < fields_sz; i++)
			free((void*)fields[i].name);
		free(fields);
		free(bitfields);
		return e;
	}

	LOG_INFO("added type {name=%s}", type->data_type.id);

	return JIM_OK;
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
		.name = "has-dcache",
		.handler = arc_handle_has_dcache,
		.mode = COMMAND_ANY,
		.usage = "True or false",
		.help = "Does target has D$? If yes it will be flushed before memory reads.",
	},
	{
		.name = "add-reg-type-flags",
		.jim_handler = jim_arc_add_reg_type_flags,
		.mode = COMMAND_CONFIG,
		.usage = "-name ?string? (-flag ?name? ?position?)+",
		.help = "Add new 'flags' register data type. Only single bit flags "
			"are supported. Type name is global. Bitsize of register is fixed "
			"at 32 bits.",
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
