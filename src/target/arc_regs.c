/*
 * Copyright (C) 2012 Synopsys, Inc. (www.synopsys.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *   Maintainer: frank.dols@synopsys.com
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "arc.h"

/* ----- Supporting functions ---------------------------------------------- */

static char *arc_gdb_reg_names_list[] = {
	/* core regs */
	"r0",   "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
	"r8",   "r9", "r10", "r11", "r12", "r13", "r14", "r15",
	"r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
	"r24", "r25",  "gp",  "fp",  "sp", "ilink1", "ilink2", "blink",
	"r32", "r33", "r34", "r35", "r36", "r37", "r38", "r39",
	"r40", "r41", "r42", "r43", "r44", "r45", "r46", "r47",
	"r48", "r49", "r50", "r51", "r52", "r53", "r54", "r55",
	"r56", "r57", "r58", "r59", "lp_count", "pcl",
	/* aux regs */
	"pc",           "lp_start",     "lp_end",       "status32",
	"status32_l1",  "status32_l2",  "aux_irq_lv12", "aux_irq_lev",
	"aux_irq_hint", "eret",         "erbta",        "erstatus",
	"ecr",          "efa",          "icause1",      "icause2",
	"aux_ienable",  "aux_itrigger", "bta",          "bta_l1",
	"bta_l2",  "aux_irq_pulse_cancel",  "aux_irq_pending"
};

static struct arc32_core_reg
	arc_gdb_reg_list_arch_info[ARC32_NUM_GDB_REGS] = { // + 3
	/* ARC core regs R0 - R26 */
	 {0, NULL, NULL},  {1, NULL, NULL},  {2, NULL, NULL},  {3, NULL, NULL},
	 {4, NULL, NULL},  {5, NULL, NULL},  {6, NULL, NULL},  {7, NULL, NULL},
	 {8, NULL, NULL},  {9, NULL, NULL}, {10, NULL, NULL}, {11, NULL, NULL},
	{12, NULL, NULL}, {13, NULL, NULL}, {14, NULL, NULL}, {15, NULL, NULL},
	{16, NULL, NULL}, {17, NULL, NULL}, {18, NULL, NULL}, {19, NULL, NULL},
	{20, NULL, NULL}, {21, NULL, NULL}, {22, NULL, NULL}, {23, NULL, NULL},
	{24, NULL, NULL}, {25, NULL, NULL}, {26, NULL, NULL}, {27, NULL, NULL},
	{28, NULL, NULL}, {29, NULL, NULL}, {30, NULL, NULL}, {31, NULL, NULL},
	{32, NULL, NULL}, {33, NULL, NULL}, {34, NULL, NULL}, {35, NULL, NULL},
	{36, NULL, NULL}, {37, NULL, NULL}, {38, NULL, NULL}, {39, NULL, NULL},
	{40, NULL, NULL}, {41, NULL, NULL}, {42, NULL, NULL}, {43, NULL, NULL},
	{44, NULL, NULL}, {45, NULL, NULL}, {46, NULL, NULL}, {47, NULL, NULL},
	{48, NULL, NULL}, {49, NULL, NULL}, {50, NULL, NULL}, {51, NULL, NULL},
	{52, NULL, NULL}, {53, NULL, NULL}, {54, NULL, NULL}, {55, NULL, NULL},
	{56, NULL, NULL}, {57, NULL, NULL}, {58, NULL, NULL}, {59, NULL, NULL},
	{60, NULL, NULL}, {61, NULL, NULL},
	/* selection of ARC aux regs */
	{62, NULL, NULL}, {63, NULL, NULL}, {64, NULL, NULL}, {65, NULL, NULL},
	{66, NULL, NULL}, {67, NULL, NULL}, {68, NULL, NULL}, {69, NULL, NULL},
	{70, NULL, NULL}, {71, NULL, NULL}, {72, NULL, NULL}, {73, NULL, NULL},
	{74, NULL, NULL}, {75, NULL, NULL}, {76, NULL, NULL}, {77, NULL, NULL},
	{78, NULL, NULL}, {79, NULL, NULL}, {80, NULL, NULL}, {81, NULL, NULL},
	{82, NULL, NULL}, {83, NULL, NULL}, {84, NULL, NULL}, {85, NULL, NULL},
	{86, NULL, NULL} //, {87, NULL, NULL}
};

static int arc_regs_get_core_reg(struct reg *reg)
{
	int retval = ERROR_OK;

	LOG_DEBUG("  >>> Calling into <<<");

	struct arc32_core_reg *arc32_reg = reg->arch_info;
	struct target *target = arc32_reg->target;
	struct arc32_common *arc32_target = target_to_arc32(target);

	if (target->state != TARGET_HALTED)
		return ERROR_TARGET_NOT_HALTED;

	retval = arc32_target->read_core_reg(target, arc32_reg->num);

	return retval;
}

static int arc_regs_set_core_reg(struct reg *reg, uint8_t *buf)
{
	int retval = ERROR_OK;

	LOG_DEBUG("  >>> Calling into <<<");

	struct arc32_core_reg *arc32_reg = reg->arch_info;
	struct target *target = arc32_reg->target;
	uint32_t value = buf_get_u32(buf, 0, 32);

	if (target->state != TARGET_HALTED)
		return ERROR_TARGET_NOT_HALTED;

	buf_set_u32(reg->value, 0, 32, value);
	reg->dirty = 1;
	reg->valid = 1;

	return retval;
}

static const struct reg_arch_type arc32_reg_type = {
	.get = arc_regs_get_core_reg,
	.set = arc_regs_set_core_reg,
};

/* ----- Exported functions ------------------------------------------------ */

struct reg_cache *arc_regs_build_reg_cache(struct target *target)
{
	int num_regs = ARC32_NUM_GDB_REGS;
	int i;

	LOG_DEBUG(">> Entering <<");

	/* get pointers to arch-specific information */
	struct arc32_common *arc32 = target_to_arc32(target);
	struct reg_cache **cache_p = register_get_last_cache_p(&target->reg_cache);
	struct reg_cache *cache = malloc(sizeof(struct reg_cache));
	struct reg *reg_list = malloc(sizeof(struct reg) * num_regs);
	struct arc32_core_reg *arch_info = 
		malloc(sizeof(struct arc32_core_reg) * num_regs);

	/* Build the process context cache */
	cache->name = "arc32 registers";
	cache->next = NULL;
	cache->reg_list = reg_list;
	cache->num_regs = num_regs;
	(*cache_p) = cache;
	arc32->core_cache = cache;

	for (i = 0; i < num_regs; i++) {
		arch_info[i] = arc_gdb_reg_list_arch_info[i];
		arch_info[i].target = target;
		arch_info[i].arc32_common = arc32;
		reg_list[i].name = arc_gdb_reg_names_list[i];
		reg_list[i].size = 32;
		reg_list[i].value = calloc(1, 4);
		reg_list[i].dirty = 0;
		reg_list[i].valid = 0;
		reg_list[i].type = &arc32_reg_type;
		reg_list[i].arch_info = &arch_info[i];
	}

	return cache;
}

int arc_regs_read_core_reg(struct target *target, int num)
{
	int retval = ERROR_OK;
	uint32_t reg_value;

	LOG_DEBUG("  >>> Calling into <<<");

	/* get pointers to arch-specific information */
	struct arc32_common *arc32 = target_to_arc32(target);

	if ((num < 0) || (num >= ARC32_NUM_GDB_REGS))
		return ERROR_COMMAND_SYNTAX_ERROR;

	reg_value = arc32->core_regs[num];
	buf_set_u32(arc32->core_cache->reg_list[num].value, 0, 32, reg_value);
	LOG_DEBUG("read core reg %02i value 0x%" PRIx32 "", num , reg_value);
	arc32->core_cache->reg_list[num].valid = 1;
	arc32->core_cache->reg_list[num].dirty = 0;

	return retval;
}

int arc_regs_write_core_reg(struct target *target, int num)
{
	int retval = ERROR_OK;
	uint32_t reg_value;

	LOG_DEBUG("  >>> Calling into <<<");

	/* get pointers to arch-specific information */
	struct arc32_common *arc32 = target_to_arc32(target);

	if ((num < 0) || (num >= ARC32_NUM_GDB_REGS))
		return ERROR_COMMAND_SYNTAX_ERROR;

	reg_value = buf_get_u32(arc32->core_cache->reg_list[num].value, 0, 32);
	arc32->core_regs[num] = reg_value;
	LOG_DEBUG("write core reg %i value 0x%" PRIx32 "", num , reg_value);
	arc32->core_cache->reg_list[num].valid = 1;
	arc32->core_cache->reg_list[num].dirty = 0;

	return retval;
}

int arc_regs_read_registers(struct target *target, uint32_t *regs)
{
	int retval = ERROR_OK;
	int i;

	LOG_DEBUG(">> Entering <<");

	struct arc32_common *arc32 = target_to_arc32(target);

	/*
	 * read core registers R0-R31 + R32-R60 for high-end core
	 * gdb requests:
	 *       regmap[0]  = (&(pt_buf.scratch.r0)
	 *       ...
	 *       regmap[60] = (&(pt_buf.scratch.r60)
	 */
	for (i = 0; i < ARC32_NUM_CORE_REGS; i++)
		arc_jtag_read_core_reg(&arc32->jtag_info, i, regs + i);

	/* do not read extension+ registers for low-end cores */
	if (arc32->processor_type != ARCEM_NUM) {
		for (i = ARC32_NUM_CORE_REGS; i <= ARC32_LAST_EXTENSION_REG; i++)
			arc_jtag_read_core_reg(&arc32->jtag_info, i, regs + i);

		arc_jtag_read_core_reg(&arc32->jtag_info, LP_COUNT_REG, regs + LP_COUNT_REG);
		arc_jtag_read_core_reg(&arc32->jtag_info, LIDI_REG,     regs + LIDI_REG);
	}

	arc_jtag_read_core_reg(&arc32->jtag_info, PCL_REG, regs + PCL_REG);

	/*
	 * we have here a hole of 2 regs, they are 0 on the other end (GDB)
	 *   aux-status & aux-semaphore are not transfered
	 */

	/* read auxilary registers */
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_PC_REG,            regs + 64);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_LP_START_REG,      regs + 65);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_LP_END_REG,        regs + 66);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_STATUS32_REG,      regs + 67);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_STATUS32_L1_REG,   regs + 68);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_STATUS32_L2_REG,   regs + 69);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_IRQ_LV12_REG,      regs + 70);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_IRQ_LEV_REG,       regs + 71);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_IRQ_HINT_REG,      regs + 72);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_ERET_REG,          regs + 73);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_ERBTA_REG,         regs + 74);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_ERSTATUS_REG,      regs + 75);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_ECR_REG,           regs + 76);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_EFA_REG,           regs + 77);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_ICAUSE1_REG,       regs + 78);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_ICAUSE2_REG,       regs + 79);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_IENABLE_REG,       regs + 80);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_ITRIGGER_REG,      regs + 81);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_BTA_REG,           regs + 82);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_BTA_L1_REG,        regs + 83);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_BTA_L2_REG,        regs + 84);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_IRQ_PULSE_CAN_REG, regs + 85);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_IRQ_PENDING_REG,   regs + 86);

	return retval;
}

int arc_regs_write_registers(struct target *target, uint32_t *regs)
{
	int retval = ERROR_OK;
	int i;

	LOG_DEBUG(">> Entering <<");

	struct arc32_common *arc32 = target_to_arc32(target);

	/*
	 * write core registers R0-Rx (see above read_registers() !)
	 */
	for (i = 0; i < ARC32_NUM_CORE_REGS; i++)
		arc_jtag_write_core_reg(&arc32->jtag_info, i, regs + i);

	/* do not write extension+ registers for low-end cores */
	if (arc32->processor_type != ARCEM_NUM) {
		for (i = ARC32_NUM_CORE_REGS; i <= ARC32_LAST_EXTENSION_REG; i++)
			arc_jtag_write_core_reg(&arc32->jtag_info, i, regs + i);

		arc_jtag_write_core_reg(&arc32->jtag_info, LP_COUNT_REG, regs + LP_COUNT_REG);
		arc_jtag_write_core_reg(&arc32->jtag_info, LIDI_REG,     regs + LIDI_REG);
	}

	arc_jtag_write_core_reg(&arc32->jtag_info, PCL_REG, regs + PCL_REG);

	/* write auxilary registers */
	arc_jtag_write_core_reg(&arc32->jtag_info, AUX_PC_REG,            regs + 64);
	arc_jtag_write_core_reg(&arc32->jtag_info, AUX_LP_START_REG,      regs + 65);
	arc_jtag_write_core_reg(&arc32->jtag_info, AUX_LP_END_REG,        regs + 66);
	arc_jtag_write_core_reg(&arc32->jtag_info, AUX_STATUS32_REG,      regs + 67);
	arc_jtag_write_core_reg(&arc32->jtag_info, AUX_STATUS32_L1_REG,   regs + 68);
	arc_jtag_write_core_reg(&arc32->jtag_info, AUX_STATUS32_L2_REG,   regs + 69);
	arc_jtag_write_core_reg(&arc32->jtag_info, AUX_IRQ_LV12_REG,      regs + 70);
	arc_jtag_write_core_reg(&arc32->jtag_info, AUX_IRQ_LEV_REG,       regs + 71);
	arc_jtag_write_core_reg(&arc32->jtag_info, AUX_IRQ_HINT_REG,      regs + 72);
	arc_jtag_write_core_reg(&arc32->jtag_info, AUX_ERET_REG,          regs + 73);
	arc_jtag_write_core_reg(&arc32->jtag_info, AUX_ERBTA_REG,         regs + 74);
	arc_jtag_write_core_reg(&arc32->jtag_info, AUX_ERSTATUS_REG,      regs + 75);
	arc_jtag_write_core_reg(&arc32->jtag_info, AUX_ECR_REG,           regs + 76);
	arc_jtag_write_core_reg(&arc32->jtag_info, AUX_EFA_REG,           regs + 77);
	arc_jtag_write_core_reg(&arc32->jtag_info, AUX_ICAUSE1_REG,       regs + 78);
	arc_jtag_write_core_reg(&arc32->jtag_info, AUX_ICAUSE2_REG,       regs + 79);
	arc_jtag_write_core_reg(&arc32->jtag_info, AUX_IENABLE_REG,       regs + 80);
	arc_jtag_write_core_reg(&arc32->jtag_info, AUX_ITRIGGER_REG,      regs + 81);
	arc_jtag_write_core_reg(&arc32->jtag_info, AUX_BTA_REG,           regs + 82);
	arc_jtag_write_core_reg(&arc32->jtag_info, AUX_BTA_L1_REG,        regs + 83);
	arc_jtag_write_core_reg(&arc32->jtag_info, AUX_BTA_L2_REG,        regs + 84);
	arc_jtag_write_core_reg(&arc32->jtag_info, AUX_IRQ_PULSE_CAN_REG, regs + 85);
	arc_jtag_write_core_reg(&arc32->jtag_info, AUX_IRQ_PENDING_REG,   regs + 86);

	return retval;
}

int arc_regs_get_gdb_reg_list(struct target *target, struct reg **reg_list[],
	int *reg_list_size)
{
	int retval = ERROR_OK;
	int i;

	struct arc32_common *arc32 = target_to_arc32(target);

	LOG_INFO(" arc-elf32-gdb <= register-cache => openocd");

	LOG_DEBUG(">> Entering <<");

	/* get pointers to arch-specific information storage */
	*reg_list_size = ARC32_NUM_GDB_REGS;
	*reg_list = malloc(sizeof(struct reg *) * (*reg_list_size));

	/* build the ARC core reg list */
	for (i = 0; i < ARC32_NUM_GDB_REGS; i++)
		(*reg_list)[i] = &arc32->core_cache->reg_list[i];

	return retval;
}

int arc_regs_print_core_registers(struct target *target)
{
	int retval = ERROR_OK;
	int reg_nbr;
	uint32_t value;

	LOG_DEBUG(">> Entering <<");

	struct arc32_common *arc32 = target_to_arc32(target);

	LOG_USER("\n ARC core register display.\n");

	for(reg_nbr = 0; reg_nbr < ARC32_NUM_CORE_REGS; reg_nbr++) {
		arc_jtag_read_core_reg(&arc32->jtag_info, reg_nbr, &value);
		LOG_USER_N(" R%02d: 0x%08X", reg_nbr, value);

		if(reg_nbr ==  3 || reg_nbr ==  7 || reg_nbr == 11 || reg_nbr == 15 ||
		   reg_nbr == 19 || reg_nbr == 23 || reg_nbr == 27 || reg_nbr == 31 )
			LOG_USER(" ");
	}

	/* do not read extension+ registers for low-end cores */
	if (arc32->processor_type != ARCEM_NUM) {
		for(reg_nbr = ARC32_NUM_CORE_REGS; reg_nbr <= PCL_REG; reg_nbr++) {
			arc_jtag_read_core_reg(&arc32->jtag_info, reg_nbr, &value);
			LOG_USER_N(" R%02d: 0x%08X", reg_nbr, value);

			if(reg_nbr == 35 || reg_nbr == 39 || reg_nbr == 43 || reg_nbr == 47 ||
			   reg_nbr == 51 || reg_nbr == 55 || reg_nbr == 59 || reg_nbr == 63 )
				LOG_USER(" ");
		}
	}

	LOG_USER(" ");

	return retval;
}

int arc_regs_print_aux_registers(struct arc_jtag *jtag_info)
{
	int retval = ERROR_OK;
	uint32_t value;

	LOG_DEBUG(">> Entering <<");

	LOG_USER("\n ARC auxiliary register display.\n");

	arc_jtag_read_aux_reg(jtag_info, AUX_STATUS_REG, &value);
	LOG_USER(" STATUS:           0x%08X\t(@:0x%03X)", value, AUX_STATUS_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_SEMAPHORE_REG, &value);
	LOG_USER(" SEMAPHORE:        0x%08X\t(@:0x%03X)", value, AUX_SEMAPHORE_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_LP_START_REG, &value);
	LOG_USER(" LP START:         0x%08X\t(@:0x%03X)", value, AUX_LP_START_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_LP_END_REG, &value);
	LOG_USER(" LP END:           0x%08X\t(@:0x%03X)", value, AUX_LP_END_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_IDENTITY_REG, &value);
	LOG_USER(" IDENTITY:         0x%08X\t(@:0x%03X)", value, AUX_IDENTITY_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_DEBUG_REG, &value);
	LOG_USER(" DEBUG:            0x%08X\t(@:0x%03X)", value, AUX_DEBUG_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_PC_REG, &value);
	LOG_USER(" PC:               0x%08X\t(@:0x%03X)", value, AUX_PC_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_STATUS32_REG, &value);
	LOG_USER(" STATUS32:         0x%08X\t(@:0x%03X)", value, AUX_STATUS32_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_STATUS32_L1_REG, &value);
	LOG_USER(" STATUS32 L1:      0x%08X\t(@:0x%03X)", value, AUX_STATUS32_L1_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_STATUS32_L2_REG, &value);
	LOG_USER(" STATUS32 L2:      0x%08X\t(@:0x%03X)", value, AUX_STATUS32_L2_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_COUNT0_REG, &value);
	LOG_USER(" COUNT0:           0x%08X\t(@:0x%03X)", value, AUX_COUNT0_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_CONTROL0_REG, &value);
	LOG_USER(" CONTROL0:         0x%08X\t(@:0x%03X)", value, AUX_CONTROL0_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_LIMIT0_REG, &value);
	LOG_USER(" LIMIT0:           0x%08X\t(@:0x%03X)", value, AUX_LIMIT0_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_INT_VECTOR_BASE_REG, &value);
	LOG_USER(" INT VECTOR BASE:  0x%08X\t(@:0x%03X)", value, AUX_INT_VECTOR_BASE_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_MACMODE_REG, &value);
	LOG_USER(" MACMODE:          0x%08X\t(@:0x%03X)", value, AUX_MACMODE_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_IRQ_LV12_REG, &value);
	LOG_USER(" IRQ LV12:         0x%08X\t(@:0x%03X)", value, AUX_IRQ_LV12_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_COUNT1_REG, &value);
	LOG_USER(" COUNT1:           0x%08X\t(@:0x%03X)", value, AUX_COUNT1_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_CONTROL1_REG, &value);
	LOG_USER(" CONTROL1:         0x%08X\t(@:0x%03X)", value, AUX_CONTROL1_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_LIMIT1_REG, &value);
	LOG_USER(" LIMIT1:           0x%08X\t(@:0x%03X)", value, AUX_LIMIT1_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_IRQ_LEV_REG, &value);
	LOG_USER(" IRQ LEV:          0x%08X\t(@:0x%03X)", value, AUX_IRQ_LEV_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_IRQ_HINT_REG, &value);
	LOG_USER(" IRQ HINT:         0x%08X\t(@:0x%03X)", value, AUX_IRQ_HINT_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_ERET_REG, &value);
	LOG_USER(" ERET:             0x%08X\t(@:0x%03X)", value, AUX_ERET_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_ERBTA_REG, &value);
	LOG_USER(" ERBTA:            0x%08X\t(@:0x%03X)", value, AUX_ERBTA_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_ERSTATUS_REG, &value);
	LOG_USER(" ERSTATUS:         0x%08X\t(@:0x%03X)", value, AUX_ERSTATUS_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_ECR_REG, &value);
	LOG_USER(" ECR:              0x%08X\t(@:0x%03X)", value, AUX_ECR_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_EFA_REG, &value);
	LOG_USER(" EFA:              0x%08X\t(@:0x%03X)", value, AUX_EFA_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_ICAUSE1_REG, &value);
	LOG_USER(" ICAUSE1:          0x%08X\t(@:0x%03X)", value, AUX_ICAUSE1_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_ICAUSE2_REG, &value);
	LOG_USER(" ICAUSE2:          0x%08X\t(@:0x%03X)", value, AUX_ICAUSE2_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_IENABLE_REG, &value);
	LOG_USER(" IENABLE:          0x%08X\t(@:0x%03X)", value, AUX_IENABLE_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_ITRIGGER_REG, &value);
	LOG_USER(" ITRIGGER:         0x%08X\t(@:0x%03X)", value, AUX_ITRIGGER_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_XPU_REG, &value);
	LOG_USER(" XPU:              0x%08X\t(@:0x%03X)", value, AUX_XPU_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_BTA_REG, &value);
	LOG_USER(" BTA:              0x%08X\t(@:0x%03X)", value, AUX_BTA_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_BTA_L1_REG, &value);
	LOG_USER(" BTA L1:           0x%08X\t(@:0x%03X)", value, AUX_BTA_L1_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_BTA_L2_REG, &value);
	LOG_USER(" BTA L2:           0x%08X\t(@:0x%03X)", value, AUX_BTA_L2_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_IRQ_PULSE_CAN_REG, &value);
	LOG_USER(" IRQ PULSE CAN:    0x%08X\t(@:0x%03X)", value, AUX_IRQ_PULSE_CAN_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_IRQ_PENDING_REG, &value);
	LOG_USER(" IRQ PENDING:      0x%08X\t(@:0x%03X)", value, AUX_IRQ_PENDING_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_XFLAGS_REG, &value);
	LOG_USER(" XFLAGS:           0x%08X\t(@:0x%03X)", value, AUX_XFLAGS_REG);
	LOG_USER(" ");

	return retval;
}
