/***************************************************************************
 *   Copyright (C) 2013 Franck Jullien                                     *
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

#ifndef OPENOCD_TARGET_RV32M1_DU_H
#define OPENOCD_TARGET_RV32M1_DU_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define CPU_STALL	0
#define CPU_UNSTALL	1

#define CPU_RESET	0
#define CPU_NOT_RESET	1

int rv32m1_du_adv_register(void);

/* Linear list over all available rv32m1 debug unit */
extern struct list_head rv32m1_du_list;

struct rv32m1_du {
	const char *name;
	struct list_head list;
	int options;

	int (*rv32m1_jtag_init)(struct rv32m1_jtag *jtag_info);

	int (*rv32m1_is_cpu_running)(struct rv32m1_jtag *jtag_info, int *running);

	int (*rv32m1_cpu_stall)(struct rv32m1_jtag *jtag_info, int action);

	int (*rv32m1_cpu_reset)(struct rv32m1_jtag *jtag_info, int action);

	int (*rv32m1_jtag_read_cpu)(struct rv32m1_jtag *jtag_info,
				  uint32_t addr, int count, uint32_t *value);

	int (*rv32m1_jtag_write_cpu)(struct rv32m1_jtag *jtag_info,
				   uint32_t addr, int count, const uint32_t *value);

	int (*rv32m1_jtag_read_memory)(struct rv32m1_jtag *jtag_info, uint32_t addr, uint32_t size,
					int count, uint8_t *buffer);

	int (*rv32m1_jtag_write_memory)(struct rv32m1_jtag *jtag_info, uint32_t addr, uint32_t size,
					 int count, const uint8_t *buffer);
};

static inline struct rv32m1_du *rv32m1_jtag_to_du(struct rv32m1_jtag *jtag_info)
{
	return (struct rv32m1_du *)jtag_info->du_core;
}

static inline struct rv32m1_du *rv32m1_to_du(struct rv32m1_info *rv32m1)
{
	struct rv32m1_jtag *jtag = &rv32m1->jtag;
	return (struct rv32m1_du *)jtag->du_core;
}
#endif /* OPENOCD_TARGET_RV32M1_DU_H */
