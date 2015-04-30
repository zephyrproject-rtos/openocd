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
#define AUX_USER_SP_REG         0xD

#define AUX_IC_IVIC_REG			0X10
#define IC_IVIC_INVALIDATE		0XFFFFFFFF

#define AUX_DCCM			0x18

#define AUX_COUNT0_REG			0x21
#define AUX_CONTROL0_REG		0x22
#define AUX_LIMIT0_REG			0x23
#define AUX_INT_VECTOR_BASE_REG	0x25
#define AUX_MACMODE_REG			0x41
#define AUX_IRQ_LV12_REG		0x43

#define AUX_DC_IVDC_REG			0X47
#define DC_IVDC_INVALIDATE		(1)
#define AUX_DC_CTRL_REG			0X48
#define DC_CTRL_IM			(1 << 6)

#define AUX_COUNT1_REG			0x100
#define AUX_CONTROL1_REG		0x101
#define AUX_LIMIT1_REG			0x102
#define AUX_IRQ_LEV_REG			0x200
#define AUX_IRQ_HINT_REG		0x201
#define AUX_ICCM			0x208
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

#define AUX_BCR_VER_REG			0x60
#define AUX_BTA_LINK_BUILD_REG	0x63
#define AUX_VECBASE_AC_BUILD_REG	0x68
#define AUX_RF_BUILD_REG		0x6E
#define AUX_ISA_CONFIG_REG		0xC1
#define AUX_DCCM_BUILD_REG		0x74

#define AUX_ICCM_BUILD_REG		0x78

struct arc32_reg_desc {
	uint32_t regnum;
	char * const name;
	uint32_t addr;
	enum reg_type gdb_type;
	bool readonly;
};

#define ARC_INVALID_REGNUM (0xFFFFFFFF)

struct arc_reg_t {
	/*const struct arc32_reg_desc *desc;*/
	struct arc_reg_desc *desc2;
	struct target *target;
	struct arc32_common *arc32_common;
	uint32_t value;
	bool dummy;
};
#if 0
enum arc32_reg_number {
	/* Core registers */
	ARC_REG_R0 = 0,
	ARC_REG_R1,
	ARC_REG_R2,
	ARC_REG_R3,
	ARC_REG_R4,
	ARC_REG_R5,
	ARC_REG_R6,
	ARC_REG_R7,
	ARC_REG_R8,
	ARC_REG_R9,
	ARC_REG_R10,
	ARC_REG_R11,
	ARC_REG_R12,
	ARC_REG_R13,
	ARC_REG_R14,
	ARC_REG_R15,
	ARC_REG_R16,
	ARC_REG_R17,
	ARC_REG_R18,
	ARC_REG_R19,
	ARC_REG_R20,
	ARC_REG_R21,
	ARC_REG_R22,
	ARC_REG_R23,
	ARC_REG_R24,
	ARC_REG_R25,
	ARC_REG_R26,
	ARC_REG_GP = ARC_REG_R26,
	ARC_REG_R27,
	ARC_REG_FP = ARC_REG_R27,
	ARC_REG_R28,
	ARC_REG_SP = ARC_REG_R28,
	ARC_REG_R29,
	ARC_REG_ILINK = ARC_REG_R29,
	ARC_REG_R30,
	ARC_REG_R31,
	ARC_REG_BLINK = ARC_REG_R31,

	/* Core extension registers */
	ARC_REG_AFTER_CORE,
	ARC_REG_FIRST_CORE_EXT = ARC_REG_AFTER_CORE,
	ARC_REG_R32 = ARC_REG_R31 + 1,
	ARC_REG_R33,
	ARC_REG_R34,
	ARC_REG_R35,
	ARC_REG_R36,
	ARC_REG_R37,
	ARC_REG_R38,
	ARC_REG_R39,
	ARC_REG_R40,
	ARC_REG_R41,
	ARC_REG_R42,
	ARC_REG_R43,
	ARC_REG_R44,
	ARC_REG_R45,
	ARC_REG_R46,
	ARC_REG_R47,
	ARC_REG_R48,
	ARC_REG_R49,
	ARC_REG_R50,
	ARC_REG_R51,
	ARC_REG_R52,
	ARC_REG_R53,
	ARC_REG_R54,
	ARC_REG_R55,
	ARC_REG_R56,
	ARC_REG_R57,
	/* In HS R58 and R59 are ACCL and ACCH. */
	ARC_REG_R58,
	ARC_REG_R59,
	ARC_REG_AFTER_CORE_EXT,

	/* Additional core registers. */
	ARC_REG_R60 = ARC_REG_AFTER_CORE_EXT,
	ARC_REG_LP_COUNT = ARC_REG_R60,
	ARC_REG_R61, /* Not a register: reserved address. */
	ARC_REG_RESERVED = ARC_REG_R61,
	ARC_REG_R62, /* Not a register: long immediate value. */
	ARC_REG_LIMM = ARC_REG_R62,
	ARC_REG_R63,
	ARC_REG_PCL = ARC_REG_R63,
	/* End of core register. */

	/* AUX registers */
	/* First register in this list are also "general" registers: the ones which
	 * are included in GDB g/G-packeta. General registers start from the
	 * regnum=0.  */
	ARC_REG_FIRST_AUX,
	ARC_REG_PC = ARC_REG_FIRST_AUX,
	/* ZD-loops */
	ARC_REG_LP_START,
	ARC_REG_LP_END,
	/* /ZD-loop */
	ARC_REG_STATUS32,

	ARC_REG_AFTER_GDB_GENERAL,

	/* Other baseline AUX registers. */
	ARC_REG_IDENTITY = ARC_REG_AFTER_GDB_GENERAL,
	ARC_REG_DEBUG,
	ARC_REG_STATUS32_P0,
	ARC_REG_STATUS32_L2,
	ARC_REG_AUX_USER_SP,
	ARC_REG_AUX_IRQ_CTRL,
	ARC_REG_IC_IVIC,
	ARC_REG_IC_CTRL,
	ARC_REG_IC_LIL,
	ARC_REG_AUX_DCCM,
	ARC_REG_IC_IVIL,
	ARC_REG_IC_RAM_ADDR,
	ARC_REG_IC_TAG,
	ARC_REG_IC_DATA,
	ARC_REG_DEBUGI,
	ARC_REG_COUNT0,
	ARC_REG_CONTROL0,
	ARC_REG_LIMIT0,
	ARC_REG_INT_VECTOR_BASE,
	ARC_REG_AUX_IRQ_ACT,
	ARC_REG_DC_IVDC,
	ARC_REG_DC_CTRL,
	ARC_REG_DC_LDL,
	ARC_REG_DC_IVDL,
	ARC_REG_DC_FLSH,
	ARC_REG_DC_FLDL,
	ARC_REG_DC_RAM_ADDR,
	ARC_REG_DC_TAG,
	ARC_REG_DC_DATA,
	ARC_REG_COUNT1,
	ARC_REG_CONTROL1,
	ARC_REG_LIMIT1,
	ARC_REG_AUX_RTC_CTRL,
	ARC_REG_AUX_RTC_LOW,
	ARC_REG_AUX_RTC_HIGH,
	ARC_REG_IRQ_PRIORITY_PENDING,
	ARC_REG_AUX_IRQ_HINT,
	ARC_REG_IRQ_PRIORITY,
	ARC_REG_AUX_ICCM,
	/* I haven't found a way to probe for this register */
	/*ARC_REG_AUX_CACHE_LIMIT,*/
	/* There is no way to probe this register.
	ARC_REG_DMP_PERIPHERAL,*/
	ARC_REG_USTACK_TOP,
	ARC_REG_USTACK_BASE,
	ARC_REG_KSTACK_TOP,
	ARC_REG_KSTACK_BASE,
	ARC_REG_JLI_BASE,
	ARC_REG_LDI_BASE,
	ARC_REG_EI_BASE,
	ARC_REG_ERET,
	ARC_REG_ERBTA,
	ARC_REG_ERSTATUS,
	ARC_REG_ECR,
	ARC_REG_EFA,
	ARC_REG_MPU_EN,
	ARC_REG_ICAUSE,
	ARC_REG_IRQ_SELECT,
	ARC_REG_IRQ_ENABLE,
	ARC_REG_IRQ_TRIGGER,
	ARC_REG_IRQ_STATUS,
	/* XPU and XFLAGS registers are listed in ISA, however they are present
	 * only in designes with extension and instructions and there is no way to
	 * check for their existenct at runtime. OpenOCD will not probe for them.
	 */
	/* ARC_REG_XPU, */
	ARC_REG_BTA,
	ARC_REG_BTA_L1,
	ARC_REG_BTA_L2,
	ARC_REG_IRQ_PULSE_CANCEL,
	ARC_REG_IRQ_PENDING,
	ARC_REG_MPU_ECR,
	/* Registers to describe MPU regions: RDB and RDP */
	ARC_REG_MPU_RDB0,
	ARC_REG_MPU_RDP0,
	ARC_REG_MPU_RDB1,
	ARC_REG_MPU_RDP1,
	ARC_REG_MPU_RDB2,
	ARC_REG_MPU_RDP2,
	ARC_REG_MPU_RDB3,
	ARC_REG_MPU_RDP3,
	ARC_REG_MPU_RDB4,
	ARC_REG_MPU_RDP4,
	ARC_REG_MPU_RDB5,
	ARC_REG_MPU_RDP5,
	ARC_REG_MPU_RDB6,
	ARC_REG_MPU_RDP6,
	ARC_REG_MPU_RDB7,
	ARC_REG_MPU_RDP7,
	ARC_REG_MPU_RDB8,
	ARC_REG_MPU_RDP8,
	ARC_REG_MPU_RDB9,
	ARC_REG_MPU_RDP9,
	ARC_REG_MPU_RDB10,
	ARC_REG_MPU_RDP10,
	ARC_REG_MPU_RDB11,
	ARC_REG_MPU_RDP11,
	ARC_REG_MPU_RDB12,
	ARC_REG_MPU_RDP12,
	ARC_REG_MPU_RDB13,
	ARC_REG_MPU_RDP13,
	ARC_REG_MPU_RDB14,
	ARC_REG_MPU_RDP14,
	ARC_REG_MPU_RDB15,
	ARC_REG_MPU_RDP15,
	/* See note for XPU */
	/*ARC_REG_XFLAGS,*/
	ARC_REG_SMART_CONTROL,
	ARC_REG_SMART_DATA,

	ARC_REG_AFTER_AUX,

	/* BCR - Build configuration registers */
	ARC_REG_FIRST_BCR = ARC_REG_AFTER_AUX,
	ARC_REG_BCR_VER = ARC_REG_AFTER_AUX,
	ARC_REG_BTA_LINK_BUILD,
	ARC_REG_VECBASE_AC_BUILD,
	ARC_REG_MPU_BUILD,
	ARC_REG_RF_BUILD,
	ARC_REG_D_CACHE_BUILD,
	ARC_REG_DCCM_BUILD,
	ARC_REG_TIMER_BUILD,
	ARC_REG_AP_BUILD,
	ARC_REG_I_CACHE_BUILD,
	ARC_REG_ICCM_BUILD,
	ARC_REG_MULTIPLY_BUILD,
	ARC_REG_SWAP_BUILD,
	ARC_REG_NORM_BUILD,
	ARC_REG_MINMAX_BUILD,
	ARC_REG_BARREL_BUILD,
	ARC_REG_ISA_CONFIG,
	ARC_REG_STACK_REGION_BUILD,
	ARC_REG_CPROT_BUILD,
	ARC_REG_IRQ_BUILD,
	ARC_REG_IFQUEUE_BUILD,
	ARC_REG_SMART_BUILD,

	ARC_REG_AFTER_BCR,

	ARC_TOTAL_NUM_REGS = ARC_REG_AFTER_BCR,
};
#endif
struct bcr_set_t {
	union {
		uint32_t raw;
		struct {
			uint8_t version : 8;
		};
	} bcr_ver;

	union {
		uint32_t raw;
		struct {
			bool p : 1;
		};
	} bta_link_build;

	union {
		uint32_t raw;
		struct {
			uint8_t __raz   : 2;
			uint8_t version : 8;
			uint32_t addr   : 22;
		};
	} vecbase_ac_build;

	union {
		uint32_t raw;
		struct {
			uint8_t version : 8;
			uint8_t regions : 8;
		};
	} mpu_build;

	union {
		uint32_t raw;
		struct {
			uint8_t version : 8;
			bool p    : 1;
			bool e    : 1;
			bool r    : 1;
			uint8_t b : 3;
			uint8_t d : 2;
		};
	} rf_build;

	union {
		uint32_t raw;
		struct {
			uint8_t version  : 8;
			uint8_t assoc    : 4;
			uint8_t capacity : 4;
			uint8_t bsize    : 4;
			uint8_t fl       : 2;
		};
	} d_cache_build;

	union {
		uint32_t raw;
		struct {
			uint8_t version : 8;
			uint8_t size0   : 4;
			uint8_t size1   : 4;
		};
	} dccm_build;

	union {
		uint32_t raw;
		struct {
			uint8_t version : 8;
			bool t0         : 1;
			bool t1         : 1;
			bool rtc        : 1;
			uint8_t __raz   : 5;
			uint8_t p0      : 4;
			uint8_t p1      : 4;
		};
	} timer_build;

	union {
		uint32_t raw;
		struct {
			uint8_t version : 8;
			uint8_t type    : 4;
		};
	} ap_build;

	union {
		uint32_t raw;
		struct {
			uint8_t version : 8;
			uint8_t assoc   : 4;
			uint8_t capacity: 4;
			uint8_t bsize   : 4;
			uint8_t fl      : 2;
		};
	} i_cache_build;

	union {
		uint32_t raw;
		struct {
			uint8_t version     : 8;
			uint8_t iccm0_size0 : 4;
			uint8_t iccm1_size0 : 4;
			uint8_t iccm0_size1 : 4;
			uint8_t iccm1_size1 : 4;
		};
	} iccm_build;

	union {
		uint32_t raw;
		struct {
			uint8_t version32 : 8;
			uint8_t type      : 2;
			uint8_t cyc       : 2;
			uint8_t __raz     : 4;
			uint8_t version16 : 8;
		};
	} multiply_build;

	union {
		uint32_t raw;
		struct {
			uint8_t version : 8;
		};
	} swap_build;

	union {
		uint32_t raw;
		struct {
			uint8_t version : 8;
		};
	} norm_build;

	union {
		uint32_t raw;
		struct {
			uint8_t version : 8;
		};
	} minmax_build;

	union {
		uint32_t raw;
		struct {
			uint8_t version : 8;
			uint8_t b       : 2;
		};
	} barrel_build;

	union {
		uint32_t raw;
		struct {
			uint8_t version  : 8;
			uint8_t pc_size  : 4;
			uint8_t lpc_size : 4;
			uint8_t addr_size: 4;
			bool b           : 1;
			bool a           : 1;
			uint8_t __raz    : 2;
			uint8_t c        : 4;
			uint8_t d        : 4;
		};
	} isa_config;

	union {
		uint32_t raw;
		struct {
			uint8_t version : 8;
		};
	} stack_region_build;

	union {
		uint32_t raw;
		struct {
			uint8_t version : 8;
		};
	} cprot_build;

	union {
		uint32_t raw;
		struct {
			uint8_t version : 8;
			uint8_t irqs    : 4;
			uint8_t exts    : 4;
			uint8_t p       : 4;
			bool f          : 1;
		};
	} irq_build;

	union {
		uint32_t raw;
		struct {
			uint8_t version : 8;
			uint8_t bd      : 3;
		};
	} ifqueue_build;

	union {
		uint32_t raw;
		struct {
			uint8_t version : 8;
			uint8_t __raz   : 2;
			uint32_t stack_size : 22;
		};
	} smart_build;
};

extern const struct reg_arch_type arc32_reg_type;
extern const char * const general_group_name;

/* ----- Exported functions ------------------------------------------------ */

//int arc_regs_build_reg_cache(struct target *target);

int arc_regs_read_core_reg(struct target *target, int num);
int arc_regs_write_core_reg(struct target *target, int num);
int arc_regs_read_registers(struct target *target, uint32_t *regs);
int arc_regs_write_registers(struct target *target, uint32_t *regs);

int arc_regs_get_gdb_reg_list(struct target *target, struct reg **reg_list[],
	int *reg_list_size, enum target_register_class reg_class);

int arc_regs_read_bcrs(struct target *target);
void arc_regs_build_reg_list(struct target *target);
int arc_regs_print_core_registers(struct target *target);
int arc_regs_print_aux_registers(struct arc_jtag *jtag_info);

static inline unsigned int arc_regs_addr_size_bits(struct bcr_set_t *bcrs)
{
	assert(bcrs);
	switch (bcrs->isa_config.addr_size) {
	    case 0:
		return 16;
	    case 1:
		return 20;
	    case 2:
		return 24;
	    case 3:
		return 28;
	    case 4:
		return 32;
	    default:
		LOG_ERROR("isa_config.addr_size value %i is invalid.", bcrs->isa_config.addr_size);
		assert(false);
		return 32;
	}
}

#endif /* ARC_REGS_H */
