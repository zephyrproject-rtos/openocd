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

/* ARC core ARCompatISA register set */

static char *arc32_core_reg_list[] = {
	"r0",   "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
	"r8",   "r9", "r10", "r11", "r12", "r13", "r14", "r15",
	"r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
	"r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",

	"x0u0", "x0u1", "x1u0", "x1u1", "x2u0", "x2u1", "x3u0", "x3u1",
	"y0u0", "y0u1", "y1u0", "y1u1", "y2u0", "y2u1", "y3u0", "y3u1",
	"x0nu", "x1nu", "x2nu", "x3nu", "y0nu", "y1nu", "y2nu", "y3nu",

	"acc1", "acc2", "tsch", "tscl", "lpcount", "spare", "limm", "pc"
};

static const char *arc_isa_strings[] = {
	"ARC32", "ARC16"
};

static struct arc32_core_reg
	arc32_core_reg_list_arch_info[ARC32_NUM_CORE_REGS] = {
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
	{60, NULL, NULL}, {61, NULL, NULL}, {62, NULL, NULL}, {63, NULL, NULL}
};

/* Auxilary reigsters */

static char *arc32_aux_reg_list[] = {
	"status", "semaphore", "lp start", "lp end",
	"identity", "debug", "pc", "status32",
	"status32 l1", "status32 l2", "count0", "control0",
	"limit0", "int vector base", "aux macmode", "aux irq lv12",
	"count1", "control1", "limit1", "aux irq lev",
	"aux irq hint", "eret", "erbta", "erstatus",
	"ecr", "efa", "icause1", "icause2",
	"aux ienable", "aux itrigger", "xpu", "bta",
	"bta l1", "bta l2", "aux irq pulse cancel", "aux irq pending",
	"xflags"
};

static struct arc32_aux_reg
	arc32_aux_reg_list_arch_info[ARC32_NUM_AUX_REGS] = {
	{AUX_STATUS_REG,			NULL, NULL},
	{AUX_SEMAPHORE_REG,			NULL, NULL},
	{AUX_LP_START_REG,			NULL, NULL},
	{AUX_LP_END_REG,			NULL, NULL},
	{AUX_IDENTITY_REG,			NULL, NULL},
	{AUX_DEBUG_REG,				NULL, NULL},
	{AUX_PC_REG,				NULL, NULL},
	{AUX_STATUS32_REG,			NULL, NULL},
	{AUX_STATUS32_L1_REG,		NULL, NULL},
	{AUX_STATUS32_L2_REG,		NULL, NULL},
	{AUX_COUNT0_REG,			NULL, NULL},
	{AUX_CONTROL0_REG,			NULL, NULL},
	{AUX_LIMIT0_REG,			NULL, NULL},
	{AUX_INT_VECTOR_BASE_REG,	NULL, NULL},
	{AUX_MACMODE_REG,			NULL, NULL},
	{AUX_IRQ_LV12_REG,			NULL, NULL},
	{AUX_COUNT1_REG,			NULL, NULL},
	{AUX_CONTROL1_REG,			NULL, NULL},
	{AUX_LIMIT1_REG,			NULL, NULL},
	{AUX_IRQ_LEV_REG,			NULL, NULL},
	{AUX_IRQ_HINT_REG,			NULL, NULL},
	{AUX_ERET_REG,				NULL, NULL},
	{AUX_ERBTA_REG,				NULL, NULL},
	{AUX_ERSTATUS_REG,			NULL, NULL},
	{AUX_ECR_REG,				NULL, NULL},
	{AUX_EFA_REG,				NULL, NULL},
	{AUX_ICAUSE1_REG,			NULL, NULL},
	{AUX_ICAUSE2_REG,			NULL, NULL},
	{AUX_IENABLE_REG,			NULL, NULL},
	{AUX_ITRIGGER_REG,			NULL, NULL},
	{AUX_XPU_REG,				NULL, NULL},
	{AUX_BTA_REG,				NULL, NULL},
	{AUX_BTA_L1_REG,			NULL, NULL},
	{AUX_BTA_L2_REG,			NULL, NULL},
	{AUX_IRQ_PULSE_CAN_REG,		NULL, NULL},
	{AUX_IRQ_PENDING_REG,		NULL, NULL},
	{AUX_XFLAGS_REG,			NULL, NULL}
};

static uint8_t arc32_gdb_dummy_fp_value[] = {0, 0, 0, 0};

static struct reg arc32_gdb_dummy_fp_reg = {
	.name = "GDB dummy floating-point register",
	.value = arc32_gdb_dummy_fp_value,
	.dirty = 0,
	.valid = 1,
	.size = 32,
	.arch_info = NULL
};




/* ----- Supporting functions ---------------------------------------------- */


static int arc32_read_core_reg(struct target *target, int num)
{
	int retval = ERROR_OK;
	uint32_t reg_value;

	LOG_DEBUG("  >>> Calling into <<<");
	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);

	/* get pointers to arch-specific information */
	struct arc32_common *arc32 = target_to_arc32(target);

	if ((num < 0) || (num >= ARC32_NUM_CORE_REGS))
		return ERROR_COMMAND_SYNTAX_ERROR;

	reg_value = arc32->core_regs[num];
	buf_set_u32(arc32->core_cache->reg_list[num].value, 0, 32, reg_value);
	printf("read core reg %02i value 0x%" PRIx32 "\n", num , reg_value);
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
	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);

	/* get pointers to arch-specific information */
	struct arc32_common *arc32 = target_to_arc32(target);

	if ((num < 0) || (num >= ARC32_NUM_CORE_REGS))
		return ERROR_COMMAND_SYNTAX_ERROR;

	reg_value = buf_get_u32(arc32->core_cache->reg_list[num].value, 0, 32);
	arc32->core_regs[num] = reg_value;
	printf("write core reg %02i value 0x%" PRIx32 "\n", num , reg_value);
	LOG_DEBUG("write core reg %i value 0x%" PRIx32 "", num , reg_value);
	arc32->core_cache->reg_list[num].valid = 1;
	arc32->core_cache->reg_list[num].dirty = 0;

	return retval;
}

static int arc32_get_core_reg(struct reg *reg)
{
	int retval = ERROR_OK;

	LOG_DEBUG("  >>> Calling into <<<");
	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);

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
	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);

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
	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);

	/* read core registers */
	for (i = 0; i < ARC32_NUM_CORE_REGS; i++)
		arc_jtag_read_core_reg(jtag_info, i, regs + i);

	/* read status register */
//	retval = avr32_jtag_exec(jtag_info, MFSR(0, 0));
//	if (retval != ERROR_OK)
//		return retval;

//	retval = avr32_jtag_read_reg(jtag_info, 0, regs + AVR32_REG_SR);

	return retval;
}

int arc32_save_context(struct target *target)
{
	int retval = ERROR_OK;
	int i;
	struct arc32_common *arc32 = target_to_arc32(target);

	LOG_DEBUG(">> Entering <<");
	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);

	retval = arc32_read_registers(&arc32->jtag_info, arc32->core_regs);
	if (retval != ERROR_OK)
		return retval;

	for (i = 0; i < ARC32_NUM_CORE_REGS; i++) {
		if (!arc32->core_cache->reg_list[i].valid)
			arc32_read_core_reg(target, i);
	}

	return ERROR_OK;
}





/* ----- Command handlers -------------------------------------------------- */

#ifdef NEEDS_PORITNG_EFFORT

static int mips32_verify_pointer(struct command_context *cmd_ctx,
		struct mips32_common *mips32)
{
	if (mips32->common_magic != MIPS32_COMMON_MAGIC) {
		command_print(cmd_ctx, "target is not an MIPS32");
		return ERROR_TARGET_INVALID;
	}
	return ERROR_OK;
}
#endif /* NEEDS_PORITNG_EFFORT */


/*
 * ARC32 targets expose command interface
 */
COMMAND_HANDLER(arc32_handle_cp0_command)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);


#ifdef NEEDS_PORITNG_EFFORT

	struct target *target = get_current_target(CMD_CTX);
	struct mips32_common *mips32 = target_to_mips32(target);
	struct mips_ejtag *ejtag_info = &mips32->ejtag_info;


	retval = mips32_verify_pointer(CMD_CTX, mips32);
	if (retval != ERROR_OK)
		return retval;

	if (target->state != TARGET_HALTED) {
		command_print(CMD_CTX, "target must be stopped for \"%s\" command", CMD_NAME);
		return ERROR_OK;
	}

	/* two or more argument, access a single register/select (write if third argument is given) */
	if (CMD_ARGC < 2)
		return ERROR_COMMAND_SYNTAX_ERROR;
	else {
		uint32_t cp0_reg, cp0_sel;
		COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], cp0_reg);
		COMMAND_PARSE_NUMBER(u32, CMD_ARGV[1], cp0_sel);

		if (CMD_ARGC == 2) {
			uint32_t value;

			retval = mips32_cp0_read(ejtag_info, &value, cp0_reg, cp0_sel);
			if (retval != ERROR_OK) {
				command_print(CMD_CTX,
						"couldn't access reg %" PRIi32,
						cp0_reg);
				return ERROR_OK;
			}
			retval = jtag_execute_queue();
			if (retval != ERROR_OK)
				return retval;

			command_print(CMD_CTX, "cp0 reg %" PRIi32 ", select %" PRIi32 ": %8.8" PRIx32,
					cp0_reg, cp0_sel, value);
		} else if (CMD_ARGC == 3) {
			uint32_t value;
			COMMAND_PARSE_NUMBER(u32, CMD_ARGV[2], value);
			retval = mips32_cp0_write(ejtag_info, value, cp0_reg, cp0_sel);
			if (retval != ERROR_OK) {
				command_print(CMD_CTX,
						"couldn't access cp0 reg %" PRIi32 ", select %" PRIi32,
						cp0_reg,  cp0_sel);
				return ERROR_OK;
			}
			command_print(CMD_CTX, "cp0 reg %" PRIi32 ", select %" PRIi32 ": %8.8" PRIx32,
					cp0_reg, cp0_sel, value);
		}
	}
#endif /* NEEDS_PORITNG_EFFORT */

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
	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);

	arc32->common_magic = ARC32_COMMON_MAGIC;
	target->arch_info = arc32;

	arc32->fast_data_area = NULL;

	arc32->jtag_info.tap = tap;
	arc32->jtag_info.scann_size = 4;

	/* has breakpoint/watchpint unit been scanned */
	arc32->bp_scanned = 0;
	arc32->data_break_list = NULL;

	arc32->read_core_reg = arc32_read_core_reg;
	arc32->write_core_reg = arc32_write_core_reg;

	return retval;
}

struct reg_cache *arc32_build_reg_cache(struct target *target)
{
	LOG_DEBUG(">> Entering <<");
	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);

	/* get pointers to arch-specific information */
	struct arc32_common *arc32 = target_to_arc32(target);

	int num_regs = ARC32_NUM_CORE_REGS;
	struct reg_cache **cache_p = register_get_last_cache_p(&target->reg_cache);
	struct reg_cache *cache = malloc(sizeof(struct reg_cache));
	struct reg *reg_list = malloc(sizeof(struct reg) * num_regs);
	struct arc32_core_reg *arch_info = 
		malloc(sizeof(struct arc32_core_reg) * num_regs);
	int i;

	register_init_dummy(&arc32_gdb_dummy_fp_reg);

	/* Build the process context cache */
	cache->name = "arc32 registers";
	cache->next = NULL;
	cache->reg_list = reg_list;
	cache->num_regs = num_regs;
	(*cache_p) = cache;
	arc32->core_cache = cache;

	for (i = 0; i < num_regs; i++) {
		arch_info[i] = arc32_core_reg_list_arch_info[i];
		arch_info[i].target = target;
		arch_info[i].arc32_common = arc32;
		reg_list[i].name = arc32_core_reg_list[i];
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
	uint32_t dpc, dinst;
	struct arc32_common *arc32 = target_to_arc32(target);

	LOG_DEBUG(">> Entering <<");
	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);

	/* save current PC */
	retval = arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_PC_REG, &dpc);
	if (retval != ERROR_OK)
		return retval;

	arc32->jtag_info.dpc = dpc;

	arc32_save_context(target);

	return ERROR_OK;
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



