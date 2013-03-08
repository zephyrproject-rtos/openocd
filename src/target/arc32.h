/*
 * Copyright (C) 2012 Synopsys, Inc. (www.synopsys.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *   Maintainer: frank.dols@synopsys.com
 */

#ifndef ARC32_H
#define ARC32_H

#include "target.h"
#include "arc_jtag.h"


#define ARC32_COMMON_MAGIC	0xB32EB324  /* just a unique number */

/* ARC core ARCompatISA register set */

enum {
	ARC32_R0		= 0,
	ARC32_CORE_REGS	= 27,
	ARC32_GDB_PC 	= 38,
	ARC32_NUM_GDB_REGS
};

enum arc32_isa_mode {
	ARC32_ISA_ARC32 = 0,
	ARC32_ISA_ARC16 = 1,
};


/* offsets into arc32 core register cache */
struct arc32_comparator {
	int used;
	uint32_t bp_value;
	uint32_t reg_address;
};

struct arc32_common {
	uint32_t common_magic;
	void *arch_info;

	struct arc_jtag jtag_info;

	uint32_t core_regs[ARC32_NUM_GDB_REGS];
	struct reg_cache *core_cache;

	enum arc32_isa_mode isa_mode;

	/* working area for fastdata access */
	struct working_area *fast_data_area;

	int bp_scanned;
	int num_inst_bpoints;
	int num_data_bpoints;
	int num_inst_bpoints_avail;
	int num_data_bpoints_avail;
	struct arc32_comparator *inst_break_list;
	struct arc32_comparator *data_break_list;

	/* register cache to processor synchronization */
	int (*read_core_reg)(struct target *target, int num);
	int (*write_core_reg)(struct target *target, int num);
};

#define ARC32_FASTDATA_HANDLER_SIZE	0x80000


struct arc32_core_reg {
	uint32_t num;
	struct target *target;
	struct arc32_common *arc32_common;
};

#define ARC32_NUM_CORE_REGS 64


/* --------------------------------------------------------------------------
 * ARC core Auxiliary register set
 *      name:					id:		bitfield:	comment:
 *      ------                  ----    ----------  ---------
 */
#define AUX_STATUS_REG			0x0					/* LEGACY, IS OBSOLETE */
#define AUX_SEMAPHORE_REG	 	0x1
#define AUX_LP_START_REG		0x2
#define AUX_LP_END_REG			0x3
#define AUX_IDENTITY_REG		0x4
#define AUX_DEBUG_REG			0x5
#define SET_CORE_SINGLE_STEP			(1 << 1)
#define SET_CORE_FORCE_HALT				(1 << 2)
#define SET_CORE_SINGLE_INSTR_STEP		(1 << 12)
#define SET_CORE_RESET_APPLIED			(1 << 23)
#define SET_CORE_SLEEP_MODE				(1 << 24)
#define SET_CORE_USER_BREAKPOINT		(1 << 29)
#define SET_CORE_BREAKPOINT_HALT		(1 << 30)
#define SET_CORE_SELF_HALT				(1 << 31)
#define SET_CORE_LOAD_PENDING			(1 << 32)
#define AUX_PC_REG				0x6
#define AUX_STATUS32_REG		0xA
#define AUX_STATUS32_L1_REG		0xB
#define AUX_STATUS32_L2_REG		0xC
#define AUX_COUNT0_REG			0x21
#define AUX_CONTROL0_REG		0x22
#define AUX_LIMIT0_REG			0x23
#define AUX_INT_VECTOR_BASE_REG	0x25
#define AUX_MACMODE_REG			0x41
#define AUX_IRQ_LV12_REG		0x43
													/* 0x60 - 0x7F RESERVED */
													/* 0xC0 - 0xFF RESERVED */
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
#define AUX_ITRIGGER_REG		0x40D
#define AUX_XPU_REG				0x410
#define AUX_BTA_REG				0x412
#define AUX_BTA_L1_REG			0x413
#define AUX_BTA_L2_REG			0x414
#define AUX_IRQ_PULSE_CAN_REG	0x415
#define AUX_IRQ_PENDING_REG		0x416
#define AUX_XFLAGS_REG			0x44F

#define ARC32_NUM_AUX_REGS  37

struct arc32_aux_reg {
	uint32_t num;
	struct target *target;
	struct arc32_common *arc32_common;
};






extern const struct command_registration arc32_command_handlers[];


/* ----- Inlined functions ------------------------------------------------- */

static inline struct arc32_common * target_to_arc32(struct target *target)
{
	return target->arch_info;
}




/* ----- Exported functions ------------------------------------------------ */

int arc32_init_arch_info(struct target *target, struct arc32_common *arc32,
	struct jtag_tap *tap);

struct reg_cache *arc32_build_reg_cache(struct target *target);

int arc32_debug_entry(struct target *target);

int arc32_get_gdb_reg_list(struct target *target, struct reg **reg_list[],
	int *reg_list_size);

int arc32_arch_state(struct target *target);

int arc32_pracc_read_mem(struct arc_jtag *jtag_info, uint32_t addr, int size,
	int count, void *buf);
int arc32_pracc_write_mem(struct arc_jtag *jtag_info, uint32_t addr, int size,
	int count, void *buf);





int arc32_print_core_registers(struct arc_jtag *jtag_info);
int arc32_print_aux_registers(struct arc_jtag *jtag_info);

#endif /* ARC32_H */
