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
			COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], value);
			LOG_DEBUG("CMD_ARGC:%u  CMD_ARGV: 0x%08" PRIx32, CMD_ARGC, value);
			arc_jtag_write_aux_reg_one(&arc32->jtag_info, AUX_PC_REG, value);
			LOG_INFO("Core PC @: 0x%08" PRIx32, value);
		} else
			LOG_ERROR(" > missing address to set.");
	} else
		LOG_ERROR(" > head list is not NULL !");

	return retval;
}

COMMAND_HANDLER(handle_set_core_into_halted_command)
{
	int retval = ERROR_OK;

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
		uint32_t value;
		arc_jtag_read_aux_reg_one(&arc32->jtag_info, AUX_DEBUG_REG, &value);
		value |= SET_CORE_FORCE_HALT; /* set the HALT bit */
		arc_jtag_write_aux_reg_one(&arc32->jtag_info, AUX_DEBUG_REG, value);
	} else
		LOG_ERROR(" > head list is not NULL !");

	return retval;
}

COMMAND_HANDLER(handle_read_core_reg_command)
{
	int retval = ERROR_OK;
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
			arc_jtag_read_core_reg(&arc32->jtag_info, reg_nbr, 1, &value);
			LOG_INFO("Core reg: 0x%" PRIx32 " contains: 0x%08" PRIx32, reg_nbr, value);
		} else
			LOG_ERROR(" > missing reg nbr to read.");
	} else
		LOG_ERROR(" > head list is not NULL !");

	return retval;
}

COMMAND_HANDLER(handle_write_core_reg_command)
{
	int retval = ERROR_OK;
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
			arc_jtag_write_core_reg(&arc32->jtag_info, reg_nbr, 1, &value);
			LOG_DEBUG("Core reg: 0x%" PRIx32 " contains: 0x%08" PRIx32, reg_nbr, value);
		} else
			LOG_ERROR(" > missing reg nbr or value to write.");
	} else
		LOG_ERROR(" > head list is not NULL !");

	return retval;
}

COMMAND_HANDLER(handle_read_aux_reg_command)
{
	int retval = ERROR_OK;
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
			arc_jtag_read_aux_reg_one(&arc32->jtag_info, reg_nbr, &value);
			LOG_ERROR("AUX reg: 0x%" PRIx32 " contains: 0x%08" PRIx32, reg_nbr, value);
		} else
			LOG_ERROR(" > missing reg nbr to read.");
	} else
		LOG_ERROR(" > head list is not NULL !");

	return retval;
}

COMMAND_HANDLER(handle_write_aux_reg_command)
{
	int retval = ERROR_OK;
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
			arc_jtag_write_aux_reg_one(&arc32->jtag_info, reg_nbr, value);
			LOG_DEBUG("AUX reg: 0x%x contains: 0x%x", reg_nbr, value);
		} else
			LOG_ERROR(" > missing reg nbr or value to write.");
	} else
		LOG_ERROR(" > head list is not NULL !");

	return retval;
}

COMMAND_HANDLER(handle_read_mem_word_command)
{
	int retval = ERROR_OK;
	uint32_t mem_addr, value;

	struct target *target = get_current_target(CMD_CTX);
	struct arc32_common *arc32 = target_to_arc32(target);

	struct target_list *head;
	head = target->head;

	if (head == (struct target_list *)NULL) {

		if (CMD_ARGC >= 1) {
			COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], mem_addr);
			LOG_DEBUG("CMD_ARGC:%u  CMD_ARGV: 0x%08" PRIx32, CMD_ARGC, mem_addr);
			arc_jtag_read_memory(&arc32->jtag_info, mem_addr, 1, &value);
			LOG_ERROR("mem addr: 0x%08" PRIx32 " contains: 0x%08" PRIx32, mem_addr, value);
		} else
			LOG_ERROR(" > missing memory address to read.");
	} else
		LOG_ERROR(" > head list is not NULL !");

	return retval;
}

COMMAND_HANDLER(handle_write_mem_word_command)
{
	int retval = ERROR_OK;
	uint32_t mem_addr, value;

	struct target *target = get_current_target(CMD_CTX);
	struct arc32_common *arc32 = target_to_arc32(target);

	struct target_list *head;
	head = target->head;

	if (head == (struct target_list *)NULL) {

		if (CMD_ARGC >= 2) {
			COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], mem_addr);
			LOG_DEBUG("CMD_ARGC:%u  CMD_ARGV: 0x%08" PRIx32, CMD_ARGC, mem_addr);
			COMMAND_PARSE_NUMBER(u32, CMD_ARGV[1], value);
			LOG_DEBUG("CMD_ARGC:%u  CMD_ARGV: 0x%08" PRIx32, CMD_ARGC, value);
			arc_jtag_write_memory(&arc32->jtag_info, mem_addr, 1, &value);
			LOG_DEBUG("mem addr: 0x%08" PRIx32 " contains: 0x%08" PRIx32, mem_addr, value);
		} else
			LOG_ERROR(" > missing memory address or value to write.");
	} else
		LOG_ERROR(" > head list is not NULL !");

	return retval;
}

COMMAND_HANDLER(handle_print_core_status_command)
{
	int retval = ERROR_OK;

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
		uint32_t value;
		arc_jtag_read_aux_reg_one(&arc32->jtag_info, AUX_DEBUG_REG, &value);
		LOG_INFO(" AUX REG    [DEBUG]: 0x%08" PRIx32, value);
		arc_jtag_read_aux_reg_one(&arc32->jtag_info, AUX_STATUS32_REG, &value);
		LOG_INFO("         [STATUS32]: 0x%08" PRIx32, value);
	} else
		LOG_ERROR(" > head list is not NULL !");

	return retval;
}

COMMAND_HANDLER(arc_handle_has_dcache)
{
	struct target *target = get_current_target(CMD_CTX);
	struct arc32_common *arc32 = target_to_arc32(target);
	return CALL_COMMAND_HANDLER(handle_command_parse_bool,
		&arc32->has_dcache, "target has data-cache");
}

/* ----- Exported target commands ------------------------------------------ */

static const struct command_registration arc_core_command_handlers[] = {
	{
		.name = "set-pc",
		.handler = handle_set_pc_command,
		.mode = COMMAND_EXEC,
		.usage = "has one argument: <value>",
		.help = "modify the ARC core program counter (PC) register",
	},
	{
		.name = "set-core-into-halted",
		.handler = handle_set_core_into_halted_command,
		.mode = COMMAND_EXEC,
		.usage = "has no arguments",
		.help = "set the ARC core into HALTED state",
	},
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
		.name = "read-mem-word",
		.handler = handle_read_mem_word_command,
		.mode = COMMAND_EXEC,
		.usage = "has one argument: <mem-addr>",
		.help = "list the content (1 word) of a particular memory location",
	},
	{
		.name = "write-mem-word",
		.handler = handle_write_mem_word_command,
		.mode = COMMAND_EXEC,
		.usage = "has two argument: <mem-addr> <value to write>",
		.help = "write value (1 word) to a particular memory location",
	},
	{
		.name = "print-core-status",
		.handler = handle_print_core_status_command,
		.mode = COMMAND_EXEC,
		.usage = "has no arguments",
		.help = "list the content of core aux debug & status32 register",
	},
	{
		.name = "has-dcache",
		.handler = arc_handle_has_dcache,
		.mode = COMMAND_ANY,
		.usage = "True or false",
		.help = "Does target has D$? If yes it will be flushed before memory reads.",
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
