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

#ifndef ARC_H
#define ARC_H

#include <helper/time_support.h>
#include <jtag/jtag.h>

#include "algorithm.h"
#include "breakpoints.h"
#include "jtag/interface.h"
#include "register.h"
#include "target.h"
#include "target_request.h"
#include "target_type.h"

#include "arc32.h"
#include "arc_core.h"
#include "arc_dbg.h"
#include "arc_jtag.h"
#include "arc_mem.h"
#include "arc_mntr.h"
#include "arc_ocd.h"
#include "arc_regs.h"
#include "arc_trgt.h"

#if defined _WIN32 || defined __CYGWIN__
#include <windows.h>
#define sleep(x) Sleep(x)
#endif

#define ARC_COMMON_MAGIC 0x1A471AC5  /* just a unique number */

struct arc_common {
	int common_magic;
	bool is_4wire;
	struct arc32_common arc32;
};

#endif /* ARC_H */
