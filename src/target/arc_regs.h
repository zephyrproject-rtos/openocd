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

#ifndef ARC_REGS_H
#define ARC_REGS_H

#include "target.h"

/* Would be great to replace usage of constants with usage of arc32_reg_desc.
 * But that's quite a lot of code. */
/* --------------------------------------------------------------------------
 * ARC core Auxiliary register set
 *      name:					id:		bitfield:	comment:
 *      ------                  ----    ----------  ---------
 */
#define STAT_HALT_BIT					(1 << 25)

#define AUX_DEBUG_REG			0x5
#define SET_CORE_SINGLE_STEP			(1)
#define SET_CORE_FORCE_HALT				(1 << 1)
#define SET_CORE_SINGLE_INSTR_STEP		(1 << 11)
#define SET_CORE_RESET_APPLIED			(1 << 22)
#define SET_CORE_SLEEP_MODE				(1 << 23)
#define SET_CORE_USER_BREAKPOINT		(1 << 28)
#define SET_CORE_BREAKPOINT_HALT		(1 << 29)
#define SET_CORE_SELF_HALT				(1 << 30)
#define SET_CORE_LOAD_PENDING			(1 << 31)

#define AUX_PC_REG				0x6

#define AUX_STATUS32_REG		0xA
#define SET_CORE_HALT_BIT				(1)
#define SET_CORE_INTRP_MASK_E1			(1 << 1)
#define SET_CORE_INTRP_MASK_E2			(1 << 2)
#define SET_CORE_AE_BIT					(1 << 5)

#define AUX_IC_IVIC_REG			0X10
#define IC_IVIC_INVALIDATE		0XFFFFFFFF

#define AUX_DC_IVDC_REG			0X47
#define DC_IVDC_INVALIDATE		(1)
#define AUX_DC_CTRL_REG			0X48
#define DC_CTRL_IM			(1 << 6)

#define AUX_IENABLE_REG			0x40C
#define SET_CORE_DISABLE_INTERRUPTS		0x00000000
#define SET_CORE_ENABLE_INTERRUPTS		0xFFFFFFFF

 /* Action Point */
#define AP_AMV_BASE				0x220
#define AP_AMM_BASE				0x221
#define AP_AC_BASE				0x222
#define AP_STRUCT_LEN			0x3

#define AP_AC_AT_INST_ADDR		0x0
#define AP_AC_AT_MEMORY_ADDR	0x2
#define AP_AC_AT_AUXREG_ADDR	0x4

#define AP_AC_TT_DISABLE		0x00
#define AP_AC_TT_WRITE			0x10
#define AP_AC_TT_READ			0x20
#define AP_AC_TT_READWRITE		0x30

struct arc32_reg_desc {
	uint32_t regnum;
	char * const name;
	uint32_t addr;
	enum reg_type gdb_type;
	bool readonly;
};

#define ARC_INVALID_REGNUM (0xFFFFFFFF)

struct arc_reg_t {
	struct arc_reg_desc *desc;
	struct target *target;
	struct arc32_common *arc32_common;
	uint32_t value;
	bool dummy;
};

extern const struct reg_arch_type arc32_reg_type;

/* ----- Exported functions ------------------------------------------------ */

int arc_regs_read_core_reg(struct target *target, int num);
int arc_regs_write_core_reg(struct target *target, int num);
int arc_regs_read_registers(struct target *target, uint32_t *regs);
int arc_regs_write_registers(struct target *target, uint32_t *regs);

int arc_regs_get_gdb_reg_list(struct target *target, struct reg **reg_list[],
	int *reg_list_size, enum target_register_class reg_class);

int arc_regs_print_core_registers(struct target *target);
int arc_regs_print_aux_registers(struct arc_jtag *jtag_info);

#endif /* ARC_REGS_H */
