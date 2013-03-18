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

#include "breakpoints.h"
#include "algorithm.h"
#include "register.h"

#include "arc32.h"

/*
 * ARCompatISA core register set.
 *
 * Be aware of the fact that arc-elf32-gdb only accepts 38 regs. !!
 */

static char *arc32_gdb_reg_list[] = {
	/* 27 core regs (NOT all 63) */
	"r0",   "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
	"r8",   "r9", "r10", "r11", "r12", "r13", "r14", "r15",
	"r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
	"r24", "r25", "r26",
	/* 15 aux regs (NOT all 37) */
	"fp",        "sp",       "ilink1", "ilink2",
	"blink",     "lp_count", "bta",    "lp_start",
	"lp_end",    "efa",      "eret",   "status_l1",
	"status_l2", "erstatus", "pc"
};

static const char *arc_isa_strings[] = {
	"ARC32", "ARC16"
};

static struct arc32_core_reg
	arc32_gdb_reg_list_arch_info[ARC32_NUM_GDB_REGS + 3] = { /* 39 + 3 = 42 */
	/* ARC core regs R0 - R26 */
	 {0, NULL, NULL},  {1, NULL, NULL},  {2, NULL, NULL},  {3, NULL, NULL},
	 {4, NULL, NULL},  {5, NULL, NULL},  {6, NULL, NULL},  {7, NULL, NULL},
	 {8, NULL, NULL},  {9, NULL, NULL}, {10, NULL, NULL}, {11, NULL, NULL},
	{12, NULL, NULL}, {13, NULL, NULL}, {14, NULL, NULL}, {15, NULL, NULL},
	{16, NULL, NULL}, {17, NULL, NULL}, {18, NULL, NULL}, {19, NULL, NULL},
	{20, NULL, NULL}, {21, NULL, NULL}, {22, NULL, NULL}, {23, NULL, NULL},
	{24, NULL, NULL}, {25, NULL, NULL}, {26, NULL, NULL},
	/* selection of ARC aux regs */
	{100, NULL, NULL}, {101, NULL, NULL}, {102, NULL, NULL}, {103, NULL, NULL},
	{104, NULL, NULL}, {105, NULL, NULL}, {106, NULL, NULL}, {107, NULL, NULL},
	{108, NULL, NULL}, {109, NULL, NULL}, {110, NULL, NULL}, {111, NULL, NULL},
	{112, NULL, NULL}, {113, NULL, NULL}, {114, NULL, NULL}
};





/* ----- Supporting functions ---------------------------------------------- */


static int arc32_read_core_reg(struct target *target, int num)
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

static int arc32_write_core_reg(struct target *target, int num)
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

static int arc32_get_core_reg(struct reg *reg)
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

static int arc32_set_core_reg(struct reg *reg, uint8_t *buf)
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
	.get = arc32_get_core_reg,
	.set = arc32_set_core_reg,
};

/* ......................................................................... */

int arc32_read_registers(struct arc_jtag *jtag_info, uint32_t *regs)
{
	int retval = ERROR_OK;
	int i;

	LOG_DEBUG(">> Entering <<");

	/*
	 * read core registers R0-R26
	 * gdb requests:
	 *       regmap[0]  = (&(pt_buf.scratch.r0)
	 *       ...
	 *       regmap[26] = (&(pt_buf.scratch.r26)
	 */
	for (i = 0; i < ARC32_CORE_REGS; i++)
		arc_jtag_read_core_reg(jtag_info, i, regs + i);

	/*
	 * be carefull here, now we need to read specific registers for GDB.
	 * gdb requests:
	 *       regmap[27] = (&(pt_buf.scratch.fp)
	 *       regmap[28] = (&(pt_buf.scratch.sp)
	 *       gap of zerros
	 *       regmap[31] = (&(pt_buf.scratch.blink)
	 *       gap of zerros
	 *       regmap[60] = (&(pt_buf.scratch.lp_count)
	 *       gap of zerros
	 *       regmap[64] = (&(pt_buf.scratch.ret)
	 *       regmap[65] = (&(pt_buf.scratch.lp_start)
	 *       regmap[66] = (&(pt_buf.scratch.lp_end)
	 *       regmap[67] = (&(pt_buf.scratch.status32)
	 *       gap of zerros
	 *       regmap[77] = (&(pt_buf.efa)
	 *       gap of zerros
	 *       regmap[86] = -1
	 */
	arc_jtag_read_core_reg(jtag_info, 27, regs + 27); /* R27 = FP */
	arc_jtag_read_core_reg(jtag_info, 28, regs + 28); /* R28 = SP */
	/* gap of zerros */
	arc_jtag_read_core_reg(jtag_info, 31, regs + 31); /* R31 = BLINK */
	/* gap of zerros */
	arc_jtag_read_aux_reg(jtag_info, AUX_COUNT0_REG,   regs + 60);
	/* gap of zerros */
	arc_jtag_read_aux_reg(jtag_info, AUX_ERET_REG,     regs + 64);
	arc_jtag_read_aux_reg(jtag_info, AUX_LP_START_REG, regs + 65);
	arc_jtag_read_aux_reg(jtag_info, AUX_LP_END_REG,   regs + 66);
	arc_jtag_read_aux_reg(jtag_info, AUX_STATUS32_REG, regs + 67);
	/* gap of zerros */
	arc_jtag_read_aux_reg(jtag_info, AUX_EFA_REG,      regs + 77);
	/* gap of zerros */

	return retval;
}

int arc32_save_context(struct target *target)
{
	int retval = ERROR_OK;
	int i;
	struct arc32_common *arc32 = target_to_arc32(target);

	LOG_DEBUG(">> Entering <<");

	retval = arc32_read_registers(&arc32->jtag_info, arc32->core_regs);
	if (retval != ERROR_OK)
		return retval;

	for (i = 0; i < ARC32_NUM_GDB_REGS; i++) {
		if (!arc32->core_cache->reg_list[i].valid)
			arc32_read_core_reg(target, i);
	}

	return ERROR_OK;
}





/* ----- Command handlers -------------------------------------------------- */

/*
 * ARC32 targets expose command interface
 */

COMMAND_HANDLER(arc32_handle_cp0_command)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);

	return retval;
}




/* ----- Supported commands ------------------------------------------------ */

const struct command_registration arc32_exec_command_handlers[] = {
	{
		.name = "cp0",
		.handler = arc32_handle_cp0_command,
		.mode = COMMAND_EXEC,
		.usage = "regnum select [value]",
		.help = "display/modify cp0 register",
	},
	COMMAND_REGISTRATION_DONE
};

const struct command_registration arc32_command_handlers[] = {
	{
		.name = "arc32",
		.mode = COMMAND_ANY,
		.help = "arc32 command group",
		.usage = "",
		.chain = arc32_exec_command_handlers,
	},
	COMMAND_REGISTRATION_DONE
};




/* ----- Exported functions ------------------------------------------------ */


int arc32_init_arch_info(struct target *target, struct arc32_common *arc32,
	struct jtag_tap *tap)
{
	int retval = ERROR_OK;

	LOG_DEBUG(">> Entering <<");

	arc32->common_magic = ARC32_COMMON_MAGIC;
	target->arch_info = arc32;

	arc32->fast_data_area = NULL;

	arc32->jtag_info.tap = tap;
	arc32->jtag_info.scann_size = 4;

	/* has breakpoint/watchpoint unit been scanned */
	arc32->bp_scanned = 0;
	arc32->data_break_list = NULL;

	arc32->read_core_reg = arc32_read_core_reg;
	arc32->write_core_reg = arc32_write_core_reg;

	return retval;
}

struct reg_cache *arc32_build_reg_cache(struct target *target)
{
	LOG_DEBUG(">> Entering <<");

	/* get pointers to arch-specific information */
	struct arc32_common *arc32 = target_to_arc32(target);

	int num_regs = ARC32_NUM_GDB_REGS;
	struct reg_cache **cache_p = register_get_last_cache_p(&target->reg_cache);
	struct reg_cache *cache = malloc(sizeof(struct reg_cache));
	struct reg *reg_list = malloc(sizeof(struct reg) * num_regs);
	struct arc32_core_reg *arch_info = 
		malloc(sizeof(struct arc32_core_reg) * num_regs);
	int i;

	/* Build the process context cache */
	cache->name = "arc32 registers";
	cache->next = NULL;
	cache->reg_list = reg_list;
	cache->num_regs = num_regs;
	(*cache_p) = cache;
	arc32->core_cache = cache;

	for (i = 0; i < num_regs; i++) {
		arch_info[i] = arc32_gdb_reg_list_arch_info[i];
		arch_info[i].target = target;
		arch_info[i].arc32_common = arc32;
		reg_list[i].name = arc32_gdb_reg_list[i];
		reg_list[i].size = 32;
		reg_list[i].value = calloc(1, 4);
		reg_list[i].dirty = 0;
		reg_list[i].valid = 0;
		reg_list[i].type = &arc32_reg_type;
		reg_list[i].arch_info = &arch_info[i];
	}

	return cache;
}

int arc32_debug_entry(struct target *target)
{
	int retval = ERROR_OK;
	uint32_t dpc;
	struct arc32_common *arc32 = target_to_arc32(target);

	LOG_DEBUG(">> Entering <<");

	/* save current PC */
	retval = arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_PC_REG, &dpc);
	if (retval != ERROR_OK)
		return retval;

	arc32->jtag_info.dpc = dpc;

	arc32_save_context(target);

	return ERROR_OK;
}

int arc32_get_gdb_reg_list(struct target *target, struct reg **reg_list[],
	int *reg_list_size)
{
	int retval = ERROR_OK;
	int i;
	struct arc32_common *arc32 = target_to_arc32(target);

	LOG_DEBUG(">> Entering <<");

	/* get pointers to arch-specific information */
	*reg_list_size = ARC32_NUM_GDB_REGS;
	*reg_list = malloc(sizeof(struct reg *) * (*reg_list_size));

	/* build the ARC core reg list */
	for (i = 0; i < ARC32_NUM_GDB_REGS; i++)
		(*reg_list)[i] = &arc32->core_cache->reg_list[i];

	return retval;
}

int arc32_arch_state(struct target *target)
{
	int retval = ERROR_OK;
	struct arc32_common *arc32 = target_to_arc32(target);

	LOG_DEBUG(">> Entering <<");

	LOG_USER("target halted in %s mode due to %s, pc: 0x%8.8" PRIx32 "",
		arc_isa_strings[arc32->isa_mode],
		debug_reason_name(target),
		buf_get_u32(arc32->core_cache->reg_list[ARC32_GDB_PC].value, 0, 32));

	return retval;
}

int arc32_pracc_read_mem(struct arc_jtag *jtag_info, uint32_t addr, int size,
	int count, void *buf)
{
	int retval = ERROR_OK;
	int i;

	LOG_DEBUG(">> Entering <<");

	if (size == 4) { /* means 4 * 2 bytes */
		for(i = 0; i < count; i++) {
			retval = arc_jtag_read_memory(jtag_info, addr + (i * 4),
				buf + (i * 4));
#ifdef DEBUG
			uint32_t buffer;
			retval = arc_jtag_read_memory(jtag_info, addr + (i * 4), &buffer);
			printf(" > value: 0x%x @: 0x%x\n", buffer, addr + (i * 4));
#endif
		}
	} else
		retval = ERROR_FAIL;

	return retval;
}

int arc32_pracc_write_mem(struct arc_jtag *jtag_info, uint32_t addr, int size,
	int count, void *buf)
{
	int retval = ERROR_OK;
	int i;

	LOG_DEBUG(">> Entering <<");

	assert(size <= 4);

	for(i = 0; i < count; i++) {
#ifdef DEBUG
		printf(" >> write: 0x%x @ 0x%x\n", *(uint32_t *)(buf + (i * size)),
			addr + (i * size));
#endif
		retval = arc_jtag_write_memory(jtag_info, (addr + (i * size)),
			(buf + (i * size)));
#ifdef DEBUG
		uint32_t buffer;
		retval = arc_jtag_read_memory(jtag_info, addr + (i * size), &buffer);
		printf(" >         0x%x @: 0x%x\n", buffer, addr + (i * 4));
#endif
	}

	return retval;
}











int arc32_print_core_registers(struct arc_jtag *jtag_info)
{
	int retval = ERROR_OK;
	int reg_nbr;
	uint32_t value;

	LOG_DEBUG(">> Entering <<");

	printf("\n ARC core register display.\n\n");

	for(reg_nbr = 0; reg_nbr < ARC32_NUM_CORE_REGS; reg_nbr++) {
		arc_jtag_read_core_reg(jtag_info, reg_nbr, &value);
		printf(" R%02d: 0x%08X", reg_nbr, value);
		if(reg_nbr ==  3 || reg_nbr ==  7 || reg_nbr == 11 || reg_nbr == 15 ||
		   reg_nbr == 19 || reg_nbr == 23 || reg_nbr == 27 || reg_nbr == 31 ||
		   reg_nbr == 35 || reg_nbr == 39 || reg_nbr == 43 || reg_nbr == 47 ||
		   reg_nbr == 51 || reg_nbr == 55 || reg_nbr == 59 || reg_nbr == 63 )
			printf("\n");
	}

	return retval;
}

int arc32_print_aux_registers(struct arc_jtag *jtag_info)
{
	int retval = ERROR_OK;
	uint32_t value;

	LOG_DEBUG(">> Entering <<");

	printf("\n ARC auxiliary register display.\n\n");

	arc_jtag_read_aux_reg(jtag_info, AUX_STATUS_REG, &value);
	printf(" STATUS:           0x%08X\t(@:0x%03X)\n", value, AUX_STATUS_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_SEMAPHORE_REG, &value);
	printf(" SEMAPHORE:        0x%08X\t(@:0x%03X)\n", value, AUX_SEMAPHORE_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_LP_START_REG, &value);
	printf(" LP START:         0x%08X\t(@:0x%03X)\n", value, AUX_LP_START_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_LP_END_REG, &value);
	printf(" LP END:           0x%08X\t(@:0x%03X)\n", value, AUX_LP_END_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_IDENTITY_REG, &value);
	printf(" IDENTITY:         0x%08X\t(@:0x%03X)\n", value, AUX_IDENTITY_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_DEBUG_REG, &value);
	printf(" DEBUG:            0x%08X\t(@:0x%03X)\n", value, AUX_DEBUG_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_PC_REG, &value);
	printf(" PC:               0x%08X\t(@:0x%03X)\n", value, AUX_PC_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_STATUS32_REG, &value);
	printf(" STATUS32:         0x%08X\t(@:0x%03X)\n", value, AUX_STATUS32_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_STATUS32_L1_REG, &value);
	printf(" STATUS32 L1:      0x%08X\t(@:0x%03X)\n", value, AUX_STATUS32_L1_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_STATUS32_L2_REG, &value);
	printf(" STATUS32 L2:      0x%08X\t(@:0x%03X)\n", value, AUX_STATUS32_L2_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_COUNT0_REG, &value);
	printf(" COUNT0:           0x%08X\t(@:0x%03X)\n", value, AUX_COUNT0_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_CONTROL0_REG, &value);
	printf(" CONTROL0:         0x%08X\t(@:0x%03X)\n", value, AUX_CONTROL0_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_LIMIT0_REG, &value);
	printf(" LIMIT0:           0x%08X\t(@:0x%03X)\n", value, AUX_LIMIT0_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_INT_VECTOR_BASE_REG, &value);
	printf(" INT VECTOR BASE:  0x%08X\t(@:0x%03X)\n", value, AUX_INT_VECTOR_BASE_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_MACMODE_REG, &value);
	printf(" MACMODE:          0x%08X\t(@:0x%03X)\n", value, AUX_MACMODE_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_IRQ_LV12_REG, &value);
	printf(" IRQ LV12:         0x%08X\t(@:0x%03X)\n", value, AUX_IRQ_LV12_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_COUNT1_REG, &value);
	printf(" COUNT1:           0x%08X\t(@:0x%03X)\n", value, AUX_COUNT1_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_CONTROL1_REG, &value);
	printf(" CONTROL1:         0x%08X\t(@:0x%03X)\n", value, AUX_CONTROL1_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_LIMIT1_REG, &value);
	printf(" LIMIT1:           0x%08X\t(@:0x%03X)\n", value, AUX_LIMIT1_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_IRQ_LEV_REG, &value);
	printf(" IRQ LEV:          0x%08X\t(@:0x%03X)\n", value, AUX_IRQ_LEV_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_IRQ_HINT_REG, &value);
	printf(" IRQ HINT:         0x%08X\t(@:0x%03X)\n", value, AUX_IRQ_HINT_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_ERET_REG, &value);
	printf(" ERET:             0x%08X\t(@:0x%03X)\n", value, AUX_ERET_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_ERBTA_REG, &value);
	printf(" ERBTA:            0x%08X\t(@:0x%03X)\n", value, AUX_ERBTA_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_ERSTATUS_REG, &value);
	printf(" ERSTATUS:         0x%08X\t(@:0x%03X)\n", value, AUX_ERSTATUS_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_ECR_REG, &value);
	printf(" ECR:              0x%08X\t(@:0x%03X)\n", value, AUX_ECR_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_EFA_REG, &value);
	printf(" EFA:              0x%08X\t(@:0x%03X)\n", value, AUX_EFA_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_ICAUSE1_REG, &value);
	printf(" ICAUSE1:          0x%08X\t(@:0x%03X)\n", value, AUX_ICAUSE1_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_ICAUSE2_REG, &value);
	printf(" ICAUSE2:          0x%08X\t(@:0x%03X)\n", value, AUX_ICAUSE2_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_IENABLE_REG, &value);
	printf(" IENABLE:          0x%08X\t(@:0x%03X)\n", value, AUX_IENABLE_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_ITRIGGER_REG, &value);
	printf(" ITRIGGER:         0x%08X\t(@:0x%03X)\n", value, AUX_ITRIGGER_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_XPU_REG, &value);
	printf(" XPU:              0x%08X\t(@:0x%03X)\n", value, AUX_XPU_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_BTA_REG, &value);
	printf(" BTA:              0x%08X\t(@:0x%03X)\n", value, AUX_BTA_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_BTA_L1_REG, &value);
	printf(" BTA L1:           0x%08X\t(@:0x%03X)\n", value, AUX_BTA_L1_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_BTA_L2_REG, &value);
	printf(" BTA L2:           0x%08X\t(@:0x%03X)\n", value, AUX_BTA_L2_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_IRQ_PULSE_CAN_REG, &value);
	printf(" IRQ PULSE CAN:    0x%08X\t(@:0x%03X)\n", value, AUX_IRQ_PULSE_CAN_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_IRQ_PENDING_REG, &value);
	printf(" IRQ PENDING:      0x%08X\t(@:0x%03X)\n", value, AUX_IRQ_PENDING_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_XFLAGS_REG, &value);
	printf(" XFLAGS:           0x%08X\t(@:0x%03X)\n", value, AUX_XFLAGS_REG);
	printf("\n");

	return retval;
}
