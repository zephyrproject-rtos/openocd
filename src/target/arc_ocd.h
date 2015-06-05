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

#ifndef ARC_OCD_H
#define ARC_OCD_H

/* ----- Exported functions ------------------------------------------------ */

int arc_ocd_poll(struct target *target);

/* ......................................................................... */

int arc_ocd_assert_reset(struct target *target);
int arc_ocd_deassert_reset(struct target *target);

/* ......................................................................... */

int arc_ocd_target_create(struct target *target, Jim_Interp *interp);
int arc_ocd_arc600_target_create(struct target *target, Jim_Interp *interp);
int arc_ocd_init_target(struct command_context *cmd_ctx, struct target *target);
int arc_ocd_examine(struct target *target);

#endif /* ARC_OCD_H */
