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


/* --------------------------------------------------------------------------
 *
 *   ARC targets expose command interface.
 *   It can be accessed via GDB through the (gdb) monitor command.
 *
 * ------------------------------------------------------------------------- */

COMMAND_HANDLER(handle_set_pc_command)
{
	int retval = ERROR_OK;
	uint32_t value;

	LOG_DEBUG(">> Entering <<");

	struct target *target = get_current_target(CMD_CTX);
	struct arc32_common *arc32 = target_to_arc32(target);

	struct target_list *head;
	head = target->head;

	if (target->state != TARGET_HALTED) {
		command_print(CMD_CTX, " NOTE: target must be HALTED for \"%s\" command",
			CMD_NAME);
		return ERROR_OK;
	}

	if (head == (struct target_list *)NULL) {

		if (CMD_ARGC >= 1) {
			COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], value);
			LOG_DEBUG("CMD_ARGC:%d  CMD_ARGV: 0x%x\n", CMD_ARGC, value);
			arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_PC_REG, &value);
			LOG_USER("Core PC @: 0x%x",value);
		} else
			LOG_USER(" > missing address to set.");
	} else
		LOG_USER(" > head list is not NULL !");

	return retval;
}

COMMAND_HANDLER(handle_print_core_registers_command)
{
	int retval = ERROR_OK;

	LOG_DEBUG(">> Entering <<");

	struct target *target = get_current_target(CMD_CTX);
	struct arc32_common *arc32 = target_to_arc32(target);

	struct target_list *head;
	head = target->head;

	if (target->state != TARGET_HALTED) {
		command_print(CMD_CTX, " NOTE: target must be HALTED for \"%s\" command",
			CMD_NAME);
		return ERROR_OK;
	}

	if (head == (struct target_list *)NULL) {
		arc_regs_print_core_registers(&arc32->jtag_info);
	} else
		LOG_USER(" > head list is not NULL !");

	return retval;
}

COMMAND_HANDLER(handle_print_aux_registers_command)
{
	int retval = ERROR_OK;

	LOG_DEBUG(">> Entering <<");

	struct target *target = get_current_target(CMD_CTX);
	struct arc32_common *arc32 = target_to_arc32(target);

	struct target_list *head;
	head = target->head;

	if (target->state != TARGET_HALTED) {
		command_print(CMD_CTX, " NOTE: target must be HALTED for \"%s\" command",
			CMD_NAME);
		return ERROR_OK;
	}

	if (head == (struct target_list *)NULL) {
		arc_regs_print_aux_registers(&arc32->jtag_info);
	} else
		LOG_USER(" > head list is not NULL !");

	return retval;
}




COMMAND_HANDLER(handle_test_gdb_command) /* originates from smp_gdb !! */
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	printf(" !! @ adapt software to get it working :-) !!\n");

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

/* ----- Exported target commands ------------------------------------------ */

static const struct command_registration arc_core_command_handlers[] = {
	{
		.name = "set-pc",
		.handler = handle_set_pc_command,
		.mode = COMMAND_EXEC,
		.usage = "<value>",
		.help = "modify the core program counter (PC) register",
	},
	{
		.name = "print-core-registers",
		.handler = handle_print_core_registers_command,
		.mode = COMMAND_EXEC,
		.usage = "has no arguments",
		.help = "list the content of all core registers",
	},
	{
		.name = "print-aux-registers",
		.handler = handle_print_aux_registers_command,
		.mode = COMMAND_EXEC,
		.usage = "has no arguments",
		.help = "list the content of all auxilary registers",
	},
	{
		.name = "test-gdb",
		.handler = handle_test_gdb_command,
		.mode = COMMAND_EXEC,
		.help = "display/fix current core gdb tests",
		.usage = "",
	},
	COMMAND_REGISTRATION_DONE
};

const struct command_registration arc_monitor_command_handlers[] = {
//	{
//		.chain = arc_registers_command_handlers,
//	},
	{
		.name = "arc",
		.mode = COMMAND_ANY,
		.help = "ARC monitor command group",
		.usage = "Help info ...",
		.chain = arc_core_command_handlers,
	},
	COMMAND_REGISTRATION_DONE
};
