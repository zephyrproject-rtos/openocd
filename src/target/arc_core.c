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

/* ----- Exported functions ------------------------------------------------ */

int arc_core_soft_reset_halt(struct target *target)
{
	int retval = ERROR_OK;

	LOG_USER(" > NOT SUPPORTED IN THIS RELEASE.");

	return retval;
}

int arc_core_type_info(struct target *target)
{
	int retval = ERROR_OK;

	struct arc32_common *arc32 = target_to_arc32(target);

	if (strncmp(target_name(target), ARCEM_STR, 6) == 0) {
		arc32->processor_type = ARCEM_NUM;
		LOG_USER("Processor type: %s", ARCEM_STR);

	} else if (strncmp(target_name(target), ARC600_STR, 6) == 0) {
		arc32->processor_type = ARC600_NUM;
		LOG_USER("Processor type: %s", ARC600_STR);

	} else if (strncmp(target_name(target), ARC700_STR, 6) == 0) {
		arc32->processor_type = ARC700_NUM;
		LOG_USER("Processor type: %s", ARC700_STR);

	} else
	LOG_WARNING(" THIS IS A UNSUPPORTED TARGET: %s", target_name(target));

	return retval;
}
