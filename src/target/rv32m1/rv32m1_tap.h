/***************************************************************************
 *   Copyright (C) 2012 by Franck Jullien                                  *
 *   elec4fun@gmail.com                                                    *
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
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef OPENOCD_TARGET_RV32M1_TAP_H
#define OPENOCD_TARGET_RV32M1_TAP_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <helper/list.h>
#include "rv32m1.h"

int rv32m1_tap_mohor_register(void);

/* Linear list over all available rv32m1 taps */
extern struct list_head rv32m1_tap_list;

struct rv32m1_tap_ip {
	struct list_head list;
	int (*init)(struct rv32m1_jtag *jtag_info);
	const char *name;
};

#endif /* OPENOCD_TARGET_RV32M1_TAP_H */
