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

/* --------------------------------------------------------------------------
 *
 *   ARC targets expose command interface.
 *   It can be accessed via GDB through the (gdb) monitor command.
 *
 * ------------------------------------------------------------------------- */

/*
 * Helper functions
 */
static int arc_cmd_jim_get_uint32(Jim_GetOptInfo *goi, uint32_t *value);
static int arc_cmd_jim_get_uint(Jim_GetOptInfo *goi, unsigned *value);

/*
 * Handlers
 */

COMMAND_HANDLER(arc_handle_has_dcache)
{
	struct target *target = get_current_target(CMD_CTX);
	struct arc32_common *arc32 = target_to_arc32(target);
	return CALL_COMMAND_HANDLER(handle_command_parse_bool,
		&arc32->has_dcache, "target has data-cache");
}

/* Add flags register data type */
enum add_reg_type_flags {
	CFG_ADD_REG_TYPE_FLAGS_NAME,
	CFG_ADD_REG_TYPE_FLAGS_FLAG,
};

static Jim_Nvp nvp_add_reg_type_flags_opts[] = {
	{ .name = "-name",  .value = CFG_ADD_REG_TYPE_FLAGS_NAME },
	{ .name = "-flag",  .value = CFG_ADD_REG_TYPE_FLAGS_FLAG },
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

	/* Estimate number of registers as (argc - 2)/3 as each -flag option has 2
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
	type->data_type.reg_type_flags = flags;
	flags->size = 4; /* For now ARC has only 32-bit registers */

	while (goi.argc > 0 && e == JIM_OK) {
		Jim_Nvp *n;
		e = Jim_GetOpt_Nvp(&goi, nvp_add_reg_type_flags_opts, &n);
		if (e != JIM_OK) {
			Jim_GetOpt_NvpUnknown(&goi, nvp_add_reg_type_flags_opts, 0);
			continue;
		}

		switch (n->value) {
			case CFG_ADD_REG_TYPE_FLAGS_NAME:
			{
				const char *name;
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
			case CFG_ADD_REG_TYPE_FLAGS_FLAG:
			{
				const char *field_name;
				int field_name_len;
				jim_wide position;

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

	LOG_DEBUG("added flags type {name=%s}", type->data_type.id);

	return JIM_OK;
}

/* Add struct register data type */
enum add_reg_type_struct {
	CFG_ADD_REG_TYPE_STRUCT_NAME,
	CFG_ADD_REG_TYPE_STRUCT_BITFIELD,
};

static Jim_Nvp nvp_add_reg_type_struct_opts[] = {
	{ .name = "-name",     .value = CFG_ADD_REG_TYPE_STRUCT_NAME },
	{ .name = "-bitfield", .value = CFG_ADD_REG_TYPE_STRUCT_BITFIELD },
	{ .name = NULL,     .value = -1 }
};

/* This function supports only bitfields. */
int jim_arc_add_reg_type_struct(Jim_Interp *interp, int argc,
	Jim_Obj * const *argv)
{
	Jim_GetOptInfo goi;
	Jim_GetOpt_Setup(&goi, interp, argc-1, argv+1);

	LOG_DEBUG("-");

	int e = JIM_OK;

	/* Estimate number of registers as (argc - 2)/4 as each -bitfield option has 3
	 * arguments while -name is required. */
	unsigned int fields_sz = (goi.argc - 2) / 4;
	unsigned int cur_field = 0;

	struct arc_reg_data_type *type = calloc(1, sizeof(struct arc_reg_data_type));
	struct reg_data_type_struct *struct_type =
		calloc(1, sizeof(struct reg_data_type_struct));
	struct reg_data_type_struct_field *fields =
		calloc(fields_sz, sizeof(struct reg_data_type_struct_field));
	struct reg_data_type_bitfield *bitfields =
		calloc(fields_sz, sizeof(struct reg_data_type_bitfield));

	if (!(type && struct_type && fields && bitfields)) {
		free(type);
		free(struct_type);
		free(fields);
		free(bitfields);
		Jim_SetResultFormatted(goi.interp, "Failed to allocate memory.");
		return JIM_ERR;
	}

	/* Initialize type */
	type->data_type.type = REG_TYPE_ARCH_DEFINED;
	type->data_type.type_class = REG_TYPE_CLASS_STRUCT;
	type->data_type.reg_type_struct = struct_type;
	struct_type->size = 4; /* For now ARC has only 32-bit registers */

	while (goi.argc > 0 && e == JIM_OK) {
		Jim_Nvp *n;
		e = Jim_GetOpt_Nvp(&goi, nvp_add_reg_type_struct_opts, &n);
		if (e != JIM_OK) {
			Jim_GetOpt_NvpUnknown(&goi, nvp_add_reg_type_struct_opts, 0);
			continue;
		}

		switch (n->value) {
			case CFG_ADD_REG_TYPE_STRUCT_NAME:
			{
				const char *name;
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
			case CFG_ADD_REG_TYPE_STRUCT_BITFIELD:
			{
				const char *field_name;
				int field_name_len;
				jim_wide start, end;

				if (goi.argc < 3) {
					Jim_WrongNumArgs(interp, goi.argc, goi.argv,
						"-bitfield ?name? ?start? ?end? ...");
					e = JIM_ERR;
					break;
				}

				/* Field name */
				e = Jim_GetOpt_String(&goi, &field_name, &field_name_len);
				if (e != JIM_OK)
					break;

				/* Bit-field start */
				e = Jim_GetOpt_Wide(&goi, &start);
				if (e != JIM_OK)
					break;

				/* Bit-field end */
				e = Jim_GetOpt_Wide(&goi, &end);
				if (e != JIM_OK)
					break;

				fields[cur_field].name = strndup(field_name, field_name_len);
				if (!fields[cur_field].name) {
					e = JIM_ERR;
					break;
				}
				bitfields[cur_field].start = start;
				bitfields[cur_field].end = end;
				fields[cur_field].bitfield = &(bitfields[cur_field]);
				/* Only bitfields are supported so far. */
				fields[cur_field].use_bitfields = true;
				if (cur_field > 0)
					fields[cur_field - 1].next = &(fields[cur_field]);
				else
					struct_type->fields = fields;

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
		free(struct_type);
		/* `fields` is zeroed, so for uninitialized fields "name" is NULL. */
		for (unsigned int i = 0; i < fields_sz; i++)
			free((void*)fields[i].name);
		free(fields);
		free(bitfields);
		return e;
	}

	LOG_DEBUG("added struct type {name=%s}", type->data_type.id);

	return JIM_OK;
}

/* Add register */
enum opts_add_reg {
	CFG_ADD_REG_NAME,
	CFG_ADD_REG_ARCH_NUM,
	CFG_ADD_REG_IS_CORE,
	CFG_ADD_REG_IS_BCR,
	CFG_ADD_REG_GDB_FEATURE,
	CFG_ADD_REG_TYPE,
	CFG_ADD_REG_GENERAL,
};

static Jim_Nvp opts_nvp_add_reg[] = {
	{ .name = "-name",   .value = CFG_ADD_REG_NAME },
	{ .name = "-num",    .value = CFG_ADD_REG_ARCH_NUM },
	{ .name = "-core",   .value = CFG_ADD_REG_IS_CORE },
	{ .name = "-bcr",    .value = CFG_ADD_REG_IS_BCR },
	{ .name = "-feature",.value = CFG_ADD_REG_GDB_FEATURE },
	{ .name = "-type",   .value = CFG_ADD_REG_TYPE },
	{ .name = "-g",      .value = CFG_ADD_REG_GENERAL },
	{ .name = NULL,      .value = -1 }
};

static void free_reg_desc(struct arc_reg_desc *r) {
	if (r) {
		if (r->name)
			free(r->name);
		if (r->gdb_xml_feature)
			free(r->gdb_xml_feature);
		free(r);
	}
}

int jim_arc_add_reg(Jim_Interp *interp, int argc, Jim_Obj * const *argv)
{
	Jim_GetOptInfo goi;
	Jim_GetOpt_Setup(&goi, interp, argc-1, argv+1);

	LOG_DEBUG("-");

	struct arc_reg_desc *reg = calloc(1, sizeof(struct arc_reg_desc));
	if (!reg) {
		Jim_SetResultFormatted(goi.interp, "Failed to allocate memory.");
		return JIM_ERR;
	}

	/* Initialize */
	reg->name = NULL;
	reg->is_core = false;
	reg->is_bcr = false;
	reg->arch_num = 0;
	reg->gdb_xml_feature = NULL;
	reg->is_general = false;

	/* There is no architecture number that we could treat as invalid, so
	 * separate variable requried to ensure that arch num has been set. */
	bool arch_num_set = false;
	const char *type_name = "int"; /* Default type */
	int type_name_len = strlen(type_name);
	int e = ERROR_OK;

	/* Parse options. */
	while (goi.argc > 0) {
		Jim_Nvp *n;
		e = Jim_GetOpt_Nvp(&goi, opts_nvp_add_reg, &n);
		if (e != JIM_OK) {
			Jim_GetOpt_NvpUnknown(&goi, opts_nvp_add_reg, 0);
			free_reg_desc(reg);
			return e;
		}

		switch (n->value) {
			case CFG_ADD_REG_NAME:
			{
				const char *reg_name;
				int reg_name_len;

				if (goi.argc == 0) {
					free_reg_desc(reg);
					Jim_WrongNumArgs(interp, goi.argc, goi.argv, "-name ?name? ...");
					return JIM_ERR;
				}

				e = Jim_GetOpt_String(&goi, &reg_name, &reg_name_len);
				if (e != JIM_OK) {
					free_reg_desc(reg);
					return e;
				}

				reg->name = strndup(reg_name, reg_name_len);
				break;
			}
			case CFG_ADD_REG_IS_CORE:
			{
				reg->is_core = true;
				break;
			}
			case CFG_ADD_REG_IS_BCR:
			{
				reg->is_bcr = true;
				break;
			}
			case CFG_ADD_REG_ARCH_NUM:
			{
				jim_wide archnum;

				if (goi.argc == 0) {
					free_reg_desc(reg);
					Jim_WrongNumArgs(interp, goi.argc, goi.argv, "-num ?int? ...");
					return JIM_ERR;
				}

				e = Jim_GetOpt_Wide(&goi, &archnum);
				if (e != JIM_OK) {
					free_reg_desc(reg);
					return e;
				}

				reg->arch_num = archnum;
				arch_num_set = true;
				break;
			}
			case CFG_ADD_REG_GDB_FEATURE:
			{
				const char *feature;
				int feature_len;

				if (goi.argc == 0) {
					free_reg_desc(reg);
					Jim_WrongNumArgs(interp, goi.argc, goi.argv, "-feature ?name? ...");
					return JIM_ERR;
				}

				e = Jim_GetOpt_String(&goi, &feature, &feature_len);
				if (e != JIM_OK) {
					free_reg_desc(reg);
					return e;
				}

				reg->gdb_xml_feature = strndup(feature, feature_len);
				break;
			}
			case CFG_ADD_REG_TYPE:
			{
				if (goi.argc == 0) {
					free_reg_desc(reg);
					Jim_WrongNumArgs(interp, goi.argc, goi.argv, "-type ?type? ...");
					return JIM_ERR;
				}

				e = Jim_GetOpt_String(&goi, &type_name, &type_name_len);
				if (e != JIM_OK) {
					free_reg_desc(reg);
					return e;
				}

				break;
			}
			case CFG_ADD_REG_GENERAL:
			{
				reg->is_general = true;
				break;
			}
		}
	}

	/* Check that required fields are set */
	if (!reg->name) {
		Jim_SetResultFormatted(goi.interp, "-name option is required");
		free_reg_desc(reg);
		return JIM_ERR;
	}
	if (!reg->gdb_xml_feature) {
		Jim_SetResultFormatted(goi.interp, "-feature option is required");
		free_reg_desc(reg);
		return JIM_ERR;
	}
	if (!arch_num_set) {
		Jim_SetResultFormatted(goi.interp, "-num option is required");
		free_reg_desc(reg);
		return JIM_ERR;
	}
	if (reg->is_bcr && reg->is_core) {
		Jim_SetResultFormatted(goi.interp,
				"Register cannot be both -core and -bcr.");
		free_reg_desc(reg);
		return JIM_ERR;
	}

	/* Add new register */
	struct command_context *ctx;
	struct target *target;

	ctx = current_command_context(interp);
	assert(ctx);
	target = get_current_target(ctx);
	if (!target) {
		Jim_SetResultFormatted(goi.interp, "No current target");
		return JIM_ERR;
	}

	e = arc32_add_reg(target, reg, type_name, type_name_len);
	if (e == ERROR_ARC_REGTYPE_NOT_FOUND) {
		Jim_SetResultFormatted(goi.interp,
			"Cannot find type `%s' for register `%s'.",
			type_name, reg->name);
		free_reg_desc(reg);
		return JIM_ERR;
	}

	return e;
}

/* arc set-reg-exists ($reg_name)+
 * Accepts any amount of register names - will set them as existing in a loop.*/
COMMAND_HANDLER(arc_set_reg_exists)
{
	struct target * const target = get_current_target(CMD_CTX);

	if (CMD_ARGC == 0) {
		command_print(CMD_CTX, "At least one register name must be specified.");
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	for (unsigned int i = 0; i < CMD_ARGC; i++) {
		const char * const reg_name = CMD_ARGV[i];
		struct reg * const r = register_get_by_name(target->reg_cache, reg_name, true);

		if (!r) {
			command_print(CMD_CTX, "Register `%s' is not found.", reg_name);
			return ERROR_COMMAND_ARGUMENT_INVALID;
		}

		r->exist = true;
	}

	return JIM_OK;
}

/* arc set-reg-feature $reg_name $feature
 *
 * Rather simplistic command to update "feature" name of the register. This is
 * required for core registers, because feature name depends on whether this is
 * 32-one config or 16-reg one. Registers are created before we can decide
 * whether core is full or reduced, so as a result feature name has to be
 * updated post-factum. It would be much better to have a generic "update-reg"
 * command to support update of other registers properties, however because
 * that is not strictly require I stick with solution that will take less time
 * to implement. */
COMMAND_HANDLER(arc_set_reg_feature)
{
	struct target * const target = get_current_target(CMD_CTX);

	if (CMD_ARGC != 2) {
		command_print(CMD_CTX, "arc set-reg-feature $reg_name $feature");
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	const char * const reg_name = CMD_ARGV[0];
	struct reg * const r = register_get_by_name(target->reg_cache, reg_name, true);

	if (!r) {
		command_print(CMD_CTX, "Register `%s' is not found.", reg_name);
		return ERROR_COMMAND_ARGUMENT_INVALID;
	}

	/* Deallocate previous feature. reg_feature.name is const, so extra
	 * movements are required. For ARC feature names are always allocated
	 * in heap, so can be (and should be) free'd. */
	struct arc_reg_t * arc_r = r->arch_info;
	free(arc_r->desc->gdb_xml_feature);
	arc_r->desc->gdb_xml_feature = strdup(CMD_ARGV[1]);
	r->feature->name = arc_r->desc->gdb_xml_feature;

	return JIM_OK;
}

int jim_arc_reg(Jim_Interp *interp, int argc, Jim_Obj * const *argv)
{
	Jim_GetOptInfo goi;
	Jim_GetOpt_Setup(&goi, interp, argc-1, argv+1);

	LOG_DEBUG("-");
	if (goi.argc == 0 || goi.argc > 2 ) {
		Jim_WrongNumArgs(interp, goi.argc, goi.argv, "?regname? [?value?]");
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	const char *reg_name;
	JIM_CHECK_RETVAL(Jim_GetOpt_String(&goi, &reg_name, NULL));
	assert(reg_name);

	struct command_context * const ctx = current_command_context(interp);
	assert(ctx);
	struct target * const target = get_current_target(ctx);
	if (!target) {
		Jim_SetResultFormatted(goi.interp, "No current target");
		return JIM_ERR;
	}

	uint32_t value;
	int retval;

	/* argc decreased by one when regname has been read with JIM_GetOpt_string.  */
	if (goi.argc == 0) {
		/* Get register value.  */
		retval = arc32_get_register_value_u32(target, reg_name, &value);
	} else {
		/* Set register value.  */
		JIM_CHECK_RETVAL(arc_cmd_jim_get_uint32(&goi, &value));
		retval = arc32_set_register_value_u32(target, reg_name, value);
	}

	switch (retval) {
		case ERROR_OK:
			break;
		case ERROR_ARC_REGISTER_NOT_FOUND:
			Jim_SetResultFormatted(goi.interp,
				"Register `%s' has not been found.", reg_name);
			return ERROR_COMMAND_ARGUMENT_INVALID;
		default:
			/* Pass through other errors. */
			return retval;
	}

	Jim_SetResultInt(interp, value);

	return JIM_OK;
}

int jim_arc_reg_field(Jim_Interp *interp, int argc, Jim_Obj * const *argv)
{
	Jim_GetOptInfo goi;
	Jim_GetOpt_Setup(&goi, interp, argc-1, argv+1);

	LOG_DEBUG("-");
	if (goi.argc != 2) {
		if (goi.argc == 0)
			Jim_WrongNumArgs(interp, goi.argc, goi.argv, "?regname? ?fieldname?");
		else if (goi.argc == 1)
			Jim_WrongNumArgs(interp, goi.argc, goi.argv, "?fieldname?");
		else
			Jim_WrongNumArgs(interp, goi.argc, goi.argv, "?regname? ?fieldname?");
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	const char *reg_name, *field_name;
	JIM_CHECK_RETVAL(Jim_GetOpt_String(&goi, &reg_name, NULL));
	JIM_CHECK_RETVAL(Jim_GetOpt_String(&goi, &field_name, NULL));
	assert(reg_name);
	assert(field_name);

	struct command_context * const ctx = current_command_context(interp);
	assert(ctx);
	struct target * const target = get_current_target(ctx);
	if (!target) {
		Jim_SetResultFormatted(goi.interp, "No current target");
		return JIM_ERR;
	}

	uint32_t value;
	int retval;
	retval = arc32_get_register_field(target, reg_name, field_name, &value);

	switch (retval) {
		case ERROR_OK:
			break;
		case ERROR_ARC_REGISTER_NOT_FOUND:
			Jim_SetResultFormatted(goi.interp,
				"Register `%s' has not been found.", reg_name);
			return ERROR_COMMAND_ARGUMENT_INVALID;
		case ERROR_ARC_REGISTER_IS_NOT_STRUCT:
			Jim_SetResultFormatted(goi.interp,
				"Register `%s' must have 'struct' type.", reg_name);
			return ERROR_COMMAND_ARGUMENT_INVALID;
		case ERROR_ARC_REGISTER_FIELD_NOT_FOUND:
			Jim_SetResultFormatted(goi.interp,
				"Field `%s' has not been found in register `%s'.",
				field_name, reg_name);
			return ERROR_COMMAND_ARGUMENT_INVALID;
		case ERROR_ARC_FIELD_IS_NOT_BITFIELD:
			Jim_SetResultFormatted(goi.interp,
				"Field `%s' is not a 'bitfield' field in a structure.",
				field_name);
			return ERROR_COMMAND_ARGUMENT_INVALID;
		default:
			/* Pass through other errors. */
			return retval;
	}

	Jim_SetResultInt(interp, value);

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

COMMAND_HANDLER(arc_cmd_handle_jtag_wait_until_write_finished)
{
	struct target *target = get_current_target(CMD_CTX);
	struct arc32_common *arc32 = target_to_arc32(target);
	return CALL_COMMAND_HANDLER(handle_command_parse_bool,
		&arc32->jtag_info.wait_until_write_finished,
		"Check that current write operation initiated via JTAG has "
		"finished before resetting JTAG transaction");

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

static int arc_cmd_jim_get_uint(Jim_GetOptInfo *goi, unsigned *value)
{
	jim_wide value_wide;
	JIM_CHECK_RETVAL(Jim_GetOpt_Wide(goi, &value_wide));
	*value = (unsigned)value_wide;
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

COMMAND_HANDLER(set_actionpoint_auxreg_addr)
{
	int retval = ERROR_OK;

	struct target *target = get_current_target(CMD_CTX);
	struct target_list *head;
	head = target->head;

	if (target->state != TARGET_HALTED) {
		command_print(CMD_CTX, "NOTE: target must be HALTED for \"%s\" command",
			CMD_NAME);
		return ERROR_OK;
	}

	if (head == (struct target_list *)NULL) {
		if (CMD_ARGC == 2) {
			uint32_t auxreg_addr = 0;
			uint32_t transaction = AP_AC_TT_DISABLE;

			COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], auxreg_addr);

			switch (CMD_ARGV[1][0]) {
				case 'r':
					transaction = AP_AC_TT_READ;
					break;
				case 'w':
					transaction = AP_AC_TT_WRITE;
					break;
				case 'a':
					transaction = AP_AC_TT_READWRITE;
					break;
				default:
					LOG_ERROR("invalid watchpoint mode ('%c')", CMD_ARGV[2][0]);
					return ERROR_COMMAND_SYNTAX_ERROR;
			}

			return arc_dbg_add_auxreg_actionpoint(target, auxreg_addr, transaction);

		} else {
			return ERROR_COMMAND_SYNTAX_ERROR;
		}
	} else
		LOG_ERROR(" > head list is not NULL !");

	return retval;
}

COMMAND_HANDLER(remove_actionpoint_auxreg_addr)
{
	int retval = ERROR_OK;

	struct target *target = get_current_target(CMD_CTX);
	struct target_list *head;
	head = target->head;

	if (target->state != TARGET_HALTED) {
		command_print(CMD_CTX, "NOTE: target must be HALTED for \"%s\" command",
			CMD_NAME);
		return ERROR_OK;
	}

	if (head == (struct target_list *)NULL) {
		if (CMD_ARGC == 1) {
			uint32_t auxreg_addr = 0;
			COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], auxreg_addr);

			return arc_dbg_remove_auxreg_actionpoint(target, auxreg_addr);

		} else {
			return ERROR_COMMAND_SYNTAX_ERROR;
		}
	} else
		LOG_ERROR(" > head list is not NULL !");

	return retval;
}

static int jim_handle_actionpoints_num(Jim_Interp *interp, int argc,
	Jim_Obj * const *argv)
{
	Jim_GetOptInfo goi;
	Jim_GetOpt_Setup(&goi, interp, argc - 1, argv + 1);

	LOG_DEBUG("-");

	if (goi.argc >= 2) {
		Jim_WrongNumArgs(interp, goi.argc, goi.argv, "[?amount of actionpoints?]");
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	struct command_context *context = current_command_context(interp);
	assert(context);

	struct target *target = get_current_target(context);

	if (!target) {
		Jim_SetResultFormatted(goi.interp, "No current target");
		return JIM_ERR;
	}

	struct arc32_common *arc32 = target_to_arc32(target);
	/* It is not possible to pass &arc32->actionpoints_num directly to
	 * handle_command_parse_uint, because this value should be valid during
	 * "actionpoint reset, initiated by arc32_set_actionpoints_num.  */
	unsigned int ap_num = arc32->actionpoints_num;

	if (goi.argc == 1) {
		JIM_CHECK_RETVAL(arc_cmd_jim_get_uint(&goi, &ap_num));
		arc32_set_actionpoints_num(target, ap_num);
	}

	Jim_SetResultInt(interp, ap_num);

	return JIM_OK;
}

static const struct command_registration arc_jtag_command_group[] = {
	{
		.name = "always-check-status-rd",
		.handler = arc_cmd_handle_jtag_check_status_rd,
		.mode = COMMAND_ANY,
		.usage = "on|off",
		.help = "If true OpenOCD will check for JTAG status register and " \
			"whether 'ready' bit is set each time before doing any " \
			"JTAG operations. By default that is off.",
	},
	{
		.name = "wait-until-write-finished",
		.handler = arc_cmd_handle_jtag_wait_until_write_finished,
		.mode = COMMAND_ANY,
		.usage = "on|off",
		.help = "If true OpenOCD will poll JTAG STATUS register until "
			"'ready' bit is set after doing memory and register writes. ",
	},
	{
		.name = "check-status-fl",
		.handler = arc_cmd_handle_jtag_check_status_fl,
		.mode = COMMAND_ANY,
		.usage = "on|off",
		.help = "If true OpenOCD will check for JTAG status FL bit after all JTAG " \
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
		.name = "add-reg-type-struct",
		.jim_handler = jim_arc_add_reg_type_struct,
		.mode = COMMAND_CONFIG,
		.usage = "-name ?string? (-bitfield ?name? ?start? ?end?)+",
		.help = "Add new 'struct' register data type. Only bit-fields are "
			"supported so far, which means that for each bitfield start and end "
			"position bits must be specified. GDB also support type-fields, "
			"where common type can be used instead. Type name is global. Bitsize of "
			"register is fixed at 32 bits.",
	},
	{
		.name = "add-reg",
		.jim_handler = jim_arc_add_reg,
		.mode = COMMAND_CONFIG,
		.usage = "-name ?string? -num ?int? -feature ?string? [-gdbnum ?int?] "
			"[-core|-bcr] [-type ?type_name?] [-g]",
		.help = "Add new register. Name, architectural number and feature name "
			"are requried options. GDB regnum will default to previous register "
			"(gdbnum + 1) and shouldn't be specified in most cases. Type "
			"defaults to default GDB 'int'.",
	},
	{
		.name = "set-reg-exists",
		.handler = arc_set_reg_exists,
		.mode = COMMAND_ANY,
		.usage = "arc set-reg-exists ?register-name?+",
		.help = "Set that register exists. Accepts multiple register names as "
			"arguments.",
	},
	{
		.name = "set-reg-feature",
		.handler = arc_set_reg_feature,
		.mode = COMMAND_ANY,
		.usage = "arc set-reg-feature ?register-name? ?feature-name?",
		.help = "Set new feature name for a register.",
	},
	{
		.name = "reg",
		.jim_handler = jim_arc_reg,
		.mode = COMMAND_ANY,
		.usage = "?regname? [?value?]",
		.help = "Set or get value of register.",
	},
	{
		.name = "reg-field",
		.jim_handler = jim_arc_reg_field,
		.mode = COMMAND_ANY,
		.usage = "?regname? ?field_name?",
		.help = "Returns value of field in a register with 'struct' type.",
	},
	{
		.name = "jtag",
		.mode = COMMAND_ANY,
		.help = "ARC JTAG specific commands",
		.usage = "",
		.chain = arc_jtag_command_group,
	},
	{
		.name = "ap-auxreg-addr",
		.handler = set_actionpoint_auxreg_addr,
		.mode = COMMAND_EXEC,
		.usage = "has two argument: <auxreg-addr> <r|w|a>",
		.help = "sets break when aux register is accessed",
	},
	{
		.name = "rap-auxreg-addr",
		.handler = remove_actionpoint_auxreg_addr,
		.mode = COMMAND_EXEC,
		.usage = "has only one argument: <auxreg-addr>",
		.help = "removes break when aux register is accessed",
	},
	{
		.name = "num-actionpoints",
		.jim_handler = jim_handle_actionpoints_num,
		.mode = COMMAND_ANY,
		.usage = "[<unsigned integer>]",
		.help = "Prints or sets amount of actionpoints in the processor.",
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
