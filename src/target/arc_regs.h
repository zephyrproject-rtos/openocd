/*
 * Copyright (C) 2012 Synopsys, Inc. (www.synopsys.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *   Maintainer: frank.dols@synopsys.com
 */

#ifndef ARC_REGS_H
#define ARC_REGS_H

#include "target.h"


#define ARC32_NUM_CORE_REGS 64
#define ARC32_NUM_AUX_REGS  37
#define ARC32_NUM_GDB_REGS  87


/* --------------------------------------------------------------------------
 * ARC core Auxiliary register set
 *      name:					id:		bitfield:	comment:
 *      ------                  ----    ----------  ---------
 */
#define AUX_STATUS_REG			0x0					/* LEGACY, IS OBSOLETE */
#define STAT_HALT_BIT					(1 << 25)

#define AUX_SEMAPHORE_REG	 	0x1
#define AUX_LP_START_REG		0x2
#define AUX_LP_END_REG			0x3
#define AUX_IDENTITY_REG		0x4

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

#define AUX_STATUS32_L1_REG		0xB
#define AUX_STATUS32_L2_REG		0xC

#define AUX_IC_IVIC_REG			0X10
#define IC_IVIC_FLUSH					0XFFFFFFFF

#define AUX_COUNT0_REG			0x21
#define AUX_CONTROL0_REG		0x22
#define AUX_LIMIT0_REG			0x23
#define AUX_INT_VECTOR_BASE_REG	0x25
#define AUX_MACMODE_REG			0x41
#define AUX_IRQ_LV12_REG		0x43

#define AUX_DC_IVDC_REG			0X47
#define DC_IVDC_FLUSH					(1)

#define AUX_COUNT1_REG			0x100
#define AUX_CONTROL1_REG		0x101
#define AUX_LIMIT1_REG			0x102
#define AUX_IRQ_LEV_REG			0x200
#define AUX_IRQ_HINT_REG		0x201
#define AUX_ERET_REG			0x400
#define AUX_ERBTA_REG			0x401
#define AUX_ERSTATUS_REG		0x402
#define AUX_ECR_REG				0x403
#define AUX_EFA_REG				0x404
#define AUX_ICAUSE1_REG			0x40A
#define AUX_ICAUSE2_REG			0x40B

#define AUX_IENABLE_REG			0x40C
#define SET_CORE_DISABLE_INTERRUPTS		0x00000000
#define SET_CORE_ENABLE_INTERRUPTS		0xFFFFFFFF

#define AUX_ITRIGGER_REG		0x40D
#define AUX_XPU_REG				0x410
#define AUX_BTA_REG				0x412
#define AUX_BTA_L1_REG			0x413
#define AUX_BTA_L2_REG			0x414
#define AUX_IRQ_PULSE_CAN_REG	0x415
#define AUX_IRQ_PENDING_REG		0x416
#define AUX_XFLAGS_REG			0x44F

/*
 * OpenOCD register cache registers
 *   check the arc32-read/write_registers functions
 */
#define PCL_REG             63   /* = R63 */
#define PC_REG				64







struct arc32_core_reg {
	uint32_t num;
	struct target *target;
	struct arc32_common *arc32_common;
};

struct arc32_aux_reg {
	uint32_t num;
	struct target *target;
	struct arc32_common *arc32_common;
};



/* ----- Exported functions ------------------------------------------------ */

struct reg_cache *arc_regs_build_reg_cache(struct target *target);

int arc_regs_read_core_reg(struct target *target, int num);
int arc_regs_write_core_reg(struct target *target, int num);
int arc_regs_read_registers(struct arc_jtag *jtag_info, uint32_t *regs);
int arc_regs_write_registers(struct arc_jtag *jtag_info, uint32_t *regs);




int arc_regs_get_gdb_reg_list(struct target *target, struct reg **reg_list[],
	int *reg_list_size);

int arc_regs_print_core_registers(struct arc_jtag *jtag_info);
int arc_regs_print_aux_registers(struct arc_jtag *jtag_info);

#endif /* ARC_REGS_H */
