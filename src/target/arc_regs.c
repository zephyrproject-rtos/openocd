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
	"r56", "r57", "r58", "r59", "lp_count", "reserved", "limm", "pcl",
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
	{60, NULL, NULL}, {61, NULL, NULL}, {62, NULL, NULL}, {63, NULL, NULL},
	/* selection of ARC aux regs */
	{64, NULL, NULL}, {65, NULL, NULL}, {66, NULL, NULL}, {67, NULL, NULL},
	{68, NULL, NULL}, {69, NULL, NULL}, {70, NULL, NULL}, {71, NULL, NULL},
	{72, NULL, NULL}, {73, NULL, NULL}, {74, NULL, NULL}, {75, NULL, NULL},
	{76, NULL, NULL}, {77, NULL, NULL}, {78, NULL, NULL}, {79, NULL, NULL},
	{80, NULL, NULL}, {81, NULL, NULL}, {82, NULL, NULL}, {83, NULL, NULL},
	{84, NULL, NULL}, {85, NULL, NULL}, {86, NULL, NULL}
};

/* arc_regs_(read|write)_registers use this array as a list of AUX registers to
 * perform action on. */
static uint32_t aux_regs_addresses[] = { 
	AUX_PC_REG,
	AUX_LP_START_REG,
	AUX_LP_END_REG,
	AUX_STATUS32_REG,
	AUX_STATUS32_L1_REG,
	AUX_STATUS32_L2_REG,
	AUX_IRQ_LV12_REG,
	AUX_IRQ_LEV_REG,
	AUX_IRQ_HINT_REG,
	AUX_ERET_REG,
	AUX_ERBTA_REG,
	AUX_ERSTATUS_REG,
	AUX_ECR_REG,
	AUX_EFA_REG,
	AUX_ICAUSE1_REG,
	AUX_ICAUSE2_REG,
	AUX_IENABLE_REG,
	AUX_ITRIGGER_REG,
	AUX_BTA_REG,
	AUX_BTA_L1_REG,
	AUX_BTA_L2_REG,
	AUX_IRQ_PULSE_CAN_REG,
	AUX_IRQ_PENDING_REG,
};
static unsigned int aux_regs_addresses_count = (sizeof(aux_regs_addresses) / sizeof(uint32_t));

static int arc_regs_get_core_reg(struct reg *reg)
{
	int retval = ERROR_OK;

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

	/* get pointers to arch-specific information */
	struct arc32_common *arc32 = target_to_arc32(target);
	struct reg_cache **cache_p = register_get_last_cache_p(&target->reg_cache);
	struct reg_cache *cache = malloc(sizeof(struct reg_cache));
	struct reg *reg_list = calloc(num_regs, sizeof(struct reg));
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

	/* get pointers to arch-specific information */
	struct arc32_common *arc32 = target_to_arc32(target);

	if ((num < 0) || (num >= ARC32_NUM_GDB_REGS))
		return ERROR_COMMAND_SYNTAX_ERROR;

	reg_value = arc32->core_regs[num];
	buf_set_u32(arc32->core_cache->reg_list[num].value, 0, 32, reg_value);
	LOG_DEBUG("read core reg %02i value 0x%08" PRIx32, num , reg_value);
	arc32->core_cache->reg_list[num].valid = 1;
	arc32->core_cache->reg_list[num].dirty = 0;

	return retval;
}

int arc_regs_write_core_reg(struct target *target, int num)
{
	int retval = ERROR_OK;
	uint32_t reg_value;

	/* get pointers to arch-specific information */
	struct arc32_common *arc32 = target_to_arc32(target);

	if ((num < 0) || (num >= ARC32_NUM_GDB_REGS))
		return ERROR_COMMAND_SYNTAX_ERROR;

	reg_value = buf_get_u32(arc32->core_cache->reg_list[num].value, 0, 32);
	arc32->core_regs[num] = reg_value;
	LOG_DEBUG("write core reg %02i value 0x%08" PRIx32, num , reg_value);
	arc32->core_cache->reg_list[num].valid = 1;
	arc32->core_cache->reg_list[num].dirty = 0;

	return retval;
}

int arc_regs_read_registers(struct target *target, uint32_t *regs)
{
	int retval = ERROR_OK;
	struct arc32_common *arc32 = target_to_arc32(target);

	/* This will take a while, so calsl to keep_alive() are required. */
	keep_alive();

	/*
	 * read core registers:
	 *	R0-R31 for low-end cores
	 *	R0-R60 for high-end cores
	 * gdb requests:
	 *       regmap[0]  = (&(pt_buf.scratch.r0)
	 *       ...
	 *       regmap[60] = (&(pt_buf.scratch.r60)
	 */
	if (arc32->processor_type == ARCEM_NUM) {
		arc_jtag_read_core_reg(&arc32->jtag_info, 0, ARC32_NUM_CORE_REGS, regs);
	} else {
		/* Note that NUM_CORE_REGS is a number of registers but LAST_EXTENSION_REG
		 * is a _register number_ so it have to be incremented. */
		arc_jtag_read_core_reg(&arc32->jtag_info, 0, ARC32_LAST_EXTENSION_REG + 1, regs);
		arc_jtag_read_core_reg(&arc32->jtag_info, LP_COUNT_REG, 1, regs + LP_COUNT_REG);
		arc_jtag_read_core_reg(&arc32->jtag_info, LIDI_REG, 1, regs + LIDI_REG);
	}
	arc_jtag_read_core_reg(&arc32->jtag_info, PCL_REG, 1, regs + PCL_REG);

	/*
	 * we have here a hole of 2 regs, they are 0 on the other end (GDB)
	 *   aux-status & aux-semaphore are not transfered
	 */

	/* read auxiliary registers */
	keep_alive();
	arc_jtag_read_aux_reg(&arc32->jtag_info, aux_regs_addresses, aux_regs_addresses_count, regs + 64);

	keep_alive();
	return retval;
}

int arc_regs_write_registers(struct target *target, uint32_t *regs)
{
	/* This function takes around 2 seconds to finish in wall clock time.
	 * To avoid annoing OpenOCD warnings about uncalled keep_alive() we need
	 * to insert calls to it. I'm wondering whether there is no problems
	 * with JTAG implementation here, because really it would be much better
	 * to write registers faster, instead of adding calls to keep_alive().
	 * Writing one register takes around 40-50ms. May be they can be written
	 * in a batch?
	 *
	 * I would like to see real resoultion of this problem instead of
	 * inserted calls to keep_alive just to keep OpenOCD happy.  */
	int retval = ERROR_OK;
	struct arc32_common *arc32 = target_to_arc32(target);

	/* This will take a while, so calsl to keep_alive() are required. */
	keep_alive();

	/*
	 * write core registers R0-Rx (see above read_registers() !)
	 */
	if (arc32->processor_type == ARCEM_NUM) {
		arc_jtag_write_core_reg(&arc32->jtag_info, 0, ARC32_NUM_CORE_REGS, regs);
	} else {
		arc_jtag_write_core_reg(&arc32->jtag_info, 0, ARC32_LAST_EXTENSION_REG + 1, regs);
		arc_jtag_write_core_reg(&arc32->jtag_info, LP_COUNT_REG, 1, regs + LP_COUNT_REG);
		arc_jtag_write_core_reg(&arc32->jtag_info, LIDI_REG,     1, regs + LIDI_REG);
	}
	arc_jtag_write_core_reg(&arc32->jtag_info, PCL_REG, 1, regs + PCL_REG);

	/* Write auxiliary registers */
	keep_alive();
	arc_jtag_write_aux_reg(&arc32->jtag_info, aux_regs_addresses, aux_regs_addresses_count, regs + 64);

	keep_alive();
	return retval;
}

int arc_regs_get_gdb_reg_list(struct target *target, struct reg **reg_list[],
	int *reg_list_size, enum target_register_class reg_class)
{
	int retval = ERROR_OK;
	int i;

	struct arc32_common *arc32 = target_to_arc32(target);

	/* get pointers to arch-specific information storage */
	*reg_list_size = ARC32_NUM_GDB_REGS;
	*reg_list = malloc(sizeof(struct reg *) * (*reg_list_size));

	/* build the ARC core reg list */
	for (i = 0; i < ARC32_NUM_GDB_REGS; i++)
		(*reg_list)[i] = &arc32->core_cache->reg_list[i];

	return retval;
}

