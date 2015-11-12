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

#ifndef ARC_DBG_H
#define ARC_DBG_H

/* ----- Supporting functions ---------------------------------------------- */

int arc_dbg_enter_debug(struct target *target);
int arc_dbg_debug_entry(struct target *target);
int arc_dbg_exit_debug(struct target *target);

/* ----- Exported functions ------------------------------------------------ */

int arc_dbg_halt(struct target *target);
int arc_dbg_resume(struct target *target, int current, uint32_t address,
	int handle_breakpoints, int debug_execution);
int arc_dbg_step(struct target *target, int current, uint32_t address,
	int handle_breakpoints);

/* ......................................................................... */

int arc_dbg_add_breakpoint(struct target *target,
	struct breakpoint *breakpoint);
int arc_dbg_add_context_breakpoint(struct target *target,
	struct breakpoint *breakpoint);
int arc_dbg_add_hybrid_breakpoint(struct target *target,
	struct breakpoint *breakpoint);
int arc_dbg_remove_breakpoint(struct target *target,
	struct breakpoint *breakpoint);

int arc_dbg_add_watchpoint(struct target *target,
	struct watchpoint *watchpoint);
int arc_dbg_remove_watchpoint(struct target *target,
	struct watchpoint *watchpoint);
int arc_hit_watchpoint(struct target *target,
	struct watchpoint **hit_watchpoint);

int arc_dbg_add_auxreg_actionpoint(struct target *target,
	uint32_t auxreg_addr, uint32_t transaction);
int arc_dbg_remove_auxreg_actionpoint(struct target *target,
	uint32_t auxreg_addr);

void arc_dbg_reset_actionpoints(struct target *target);

#endif /* ARC_DBG_H */
