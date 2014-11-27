/***************************************************************************
 *   Copyright (C) 2013-2014 Synopsys, Inc.                                *
 *   Frank Dols <frank.dols@synopsys.com>                                  *
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


/* ...........................................................................
 * -- ARC-ELF32-GDB target command handler functions -------------------------
 *  function callstack interface between OpenOCD and GDB, see target_type.h
 * ...........................................................................
 */

struct target_type arc32_target = {
	.name = "arc32",

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

	.soft_reset_halt = arc_core_soft_reset_halt,

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
