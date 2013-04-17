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

#include "arc_jtag.h"
#include "arc_regs.h"


#define ARC32_COMMON_MAGIC	0xB32EB324  /* just a unique number */

/* supported processor types */
enum arc_processor_type {
	ARCEM_NUM	= 1,
	ARC700_NUM
};

#define ARCEM_STR	"arc-em"
#define ARC700_STR	"arc700"

/* ARC core ARCompatISA register set */
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

	enum arc_processor_type processor_type;
	enum arc32_isa_mode     isa_mode;

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

//#define ARC32_FASTDATA_HANDLER_SIZE	0x8000 /* haps51 */
#define ARC32_FASTDATA_HANDLER_SIZE	0x10000  /* 64Kb */

/* ARC 32bits Compact v2 opcodes */
#define ARC32_SDBBP 0x256F003F  /* BRK */

/* ARC 16bits Compact v2 opcodes */
#define ARC16_SDBBP 0x7FFF      /* BRK_S */

/* ----- Inlined functions ------------------------------------------------- */

static inline struct arc32_common * target_to_arc32(struct target *target)
{
	return target->arch_info;
}

/* ----- Exported functions ------------------------------------------------ */

int arc32_init_arch_info(struct target *target, struct arc32_common *arc32,
	struct jtag_tap *tap);

struct reg_cache *arc32_build_reg_cache(struct target *target);

int arc32_save_context(struct target *target);
int arc32_restore_context(struct target *target);

int arc32_enable_interrupts(struct target *target, int enable);

int arc32_start_core(struct target *target);

int arc32_config_step(struct target *target, int enable_step);

int arc32_cache_invalidate(struct target *target);

int arc32_wait_until_core_is_halted(struct target *target);

int arc32_print_core_state(struct target *target);
int arc32_arch_state(struct target *target);
int arc32_get_current_pc(struct target *target);

#endif /* ARC32_H */
