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






static int PRINT = 1;



/* ----- Supporting functions ---------------------------------------------- */

/*
 * ARCompatISA core register set.
 */


static const char *arc_isa_strings[] = {
	"ARC32", "ARC16"
};


/* ......................................................................... */


/* ......................................................................... */





/* ----- Exported functions ------------------------------------------------ */


int arc32_init_arch_info(struct target *target, struct arc32_common *arc32,
	struct jtag_tap *tap)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	arc32->common_magic = ARC32_COMMON_MAGIC;
	target->arch_info = arc32;

	arc32->fast_data_area = NULL;

	arc32->jtag_info.tap = tap;
	arc32->jtag_info.scann_size = 4;

	/* has breakpoint/watchpoint unit been scanned */
	arc32->bp_scanned = 0;
	arc32->data_break_list = NULL;

	arc32->read_core_reg = arc_regs_read_core_reg;
	arc32->write_core_reg = arc_regs_write_core_reg;

	return retval;
}




int arc32_save_context(struct target *target)
{
	int retval = ERROR_OK;
	int i;
	struct arc32_common *arc32 = target_to_arc32(target);

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	retval = arc_regs_read_registers(&arc32->jtag_info, arc32->core_regs);
	if (retval != ERROR_OK)
		return retval;

	for (i = 0; i < ARC32_NUM_GDB_REGS; i++) {
//	for (i = 0; i < ARC32_NUM_CORE_REGS; i++) {
		if (!arc32->core_cache->reg_list[i].valid)
			//arc_regs_read_core_reg(target, i);
			arc32->read_core_reg(target, i);
	}

	return ERROR_OK;
}

int arc32_restore_context(struct target *target)
{
	int retval = ERROR_OK;
	int i;
	struct arc32_common *arc32 = target_to_arc32(target);

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	for (i = 0; i < ARC32_NUM_GDB_REGS; i++) {
		if (!arc32->core_cache->reg_list[i].valid)
			//arc_regs_write_core_reg(target, i);
			arc32->write_core_reg(target, i);
	}

	retval = arc_regs_write_registers(&arc32->jtag_info, arc32->core_regs);
	if (retval != ERROR_OK)
		return retval;

	return ERROR_OK;
}

int arc32_enter_debug(struct target *target)
{
	int retval = ERROR_OK;
	uint32_t value;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	struct arc32_common *arc32 = target_to_arc32(target);

	target->state = TARGET_DEBUG_RUNNING;

	//retval = arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_DEBUG_REG, &value);
	//value |= SET_CORE_FORCE_HALT; /* set the HALT bit */
	value = SET_CORE_FORCE_HALT; /* set the HALT bit */
	retval = arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_DEBUG_REG, &value);
	sleep(1);

//#ifdef DEBUG
	printf("core stopped (halted) DEGUB-REG: 0x%x\n",value);
	retval = arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_STATUS32_REG, &value);
	printf("core STATUS32: 0x%x\n",value);
//#endif

	return retval;
}

int arc32_debug_entry(struct target *target)
{
	int retval = ERROR_OK;
	uint32_t dpc;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	struct arc32_common *arc32 = target_to_arc32(target);

	/* save current PC */
	retval = arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_PC_REG, &dpc);
	if (retval != ERROR_OK)
		return retval;

	arc32->jtag_info.dpc = dpc;

	arc32_save_context(target);

	return ERROR_OK;
}

int arc32_exit_debug(struct target *target)
{
	int retval = ERROR_OK;
	uint32_t value;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	struct arc32_common *arc32 = target_to_arc32(target);

	target->state = TARGET_RUNNING;

	/* raise the Reset Applied bit flag */
	retval = arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_DEBUG_REG, &value);
	value |= SET_CORE_RESET_APPLIED; /* set the RA bit */
	retval = arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_DEBUG_REG, &value);

	/* now we can set our PC where to continue from */
	value = buf_get_u32(arc32->core_cache->reg_list[PC_REG].value, 0, 32);
	printf("resume Core (when given start-core) with PC @:0x%x\n",value);
	arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_PC_REG, &value);

arc32_print_debug_state(target);
	return retval;
}

int arc32_enable_interrupts(struct target *target, int enable)
{
	int retval = ERROR_OK;
	uint32_t value;

	LOG_DEBUG(">> Entering <<");

	struct arc32_common *arc32 = target_to_arc32(target);

	if (enable) {
		/* enable interrupts */
		value = SET_CORE_ENABLE_INTERRUPTS;
		retval = arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_IENABLE_REG, &value);
		printf("interrupts enabled [stat:%d]\n",retval);
	} else {
		/* disable interrupts */
		value = SET_CORE_DISABLE_INTERRUPTS;
		retval = arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_IENABLE_REG, &value);
		printf("interrupts disabled [stat:%d]\n",retval);
	}

	return retval;
}

int arc32_start_core(struct target *target)
{
	int retval = ERROR_OK;
	uint32_t value;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	struct arc32_common *arc32 = target_to_arc32(target);

	target->state = TARGET_RUNNING;

	arc32_print_debug_state(target);

	retval = arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_STATUS32_REG, &value);
	value &= ~SET_CORE_HALT_BIT;        /* clear the HALT bit */
	retval = arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_STATUS32_REG, &value);
	LOG_USER(" ARC Core Started Again (eating instructions)");

	arc32_print_debug_state(target);

	return retval;
}

int arc32_config_step(struct target *target, int enable_step)
{
	int retval = ERROR_OK;
	uint32_t value;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	struct arc32_common *arc32 = target_to_arc32(target);

	if (enable_step) {
		/* enable core debug step mode */
		retval = arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_STATUS32_REG, &value);
		value &= ~SET_CORE_AE_BIT;        /* clear the AE bit */
		retval = arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_STATUS32_REG, &value);
		printf("                             [status32:0x%x] [stat:%d]\n", value, retval);

		//retval = arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_DEBUG_REG, &value);
		value = SET_CORE_SINGLE_INSTR_STEP; /* set the IS bit */
		value |= SET_CORE_SINGLE_STEP;       /* set the SS bit */
		retval = arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_DEBUG_REG, &value);
		printf("core debug step mode enabled [debug-reg:0x%x] [stat:%d]\n", value, retval);
	} else {
		/* disable core debug step mode */
		retval = arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_DEBUG_REG, &value);
		value &= ~SET_CORE_SINGLE_INSTR_STEP; /* clear the IS bit */
		retval = arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_DEBUG_REG, &value);
		printf("core debug step mode disabled [stat:%d]\n",retval);
	}

	return retval;
}

int arc32_cache_invalidate(struct target *target)
{
	int retval = ERROR_OK;
	uint32_t value;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	struct arc32_common *arc32 = target_to_arc32(target);

	value = IC_IVIC_FLUSH;        /* just flush all instructions */
	retval = arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_IC_IVIC_REG, &value);
	//value = DC_IVDC_FLUSH;        /* just flush all data */
	//retval = arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_DC_IVDC_REG, &value);

	return retval;
}



int arc32_print_debug_state(struct target *target)
{
	int retval = ERROR_OK;
	struct arc32_common *arc32 = target_to_arc32(target);
	uint32_t value;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_DEBUG_REG, &value);
	LOG_USER("     AUX DEBUG REG:    0x%x", value);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_STATUS32_REG, &value);
	LOG_USER("     AUX STATUS32 REG: 0x%x", value);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_STATUS_REG, &value);
	LOG_USER("     AUX STATUS REG:   0x%x", value);
	arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_PC_REG, &value);
	LOG_USER("     AUX PC REG:       0x%x", value);

	return retval;
}




int arc32_arch_state(struct target *target)
{
	int retval = ERROR_OK;
	struct arc32_common *arc32 = target_to_arc32(target);

//	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	if (PRINT) {
		LOG_USER("target state: %s in: %s mode, PC at: 0x%8.8" PRIx32 "",
			target_state_name(target),
			arc_isa_strings[arc32->isa_mode],
			buf_get_u32(arc32->core_cache->reg_list[PC_REG].value, 0, 32));
		PRINT = 0; /* due to endless looping through ;-) */
	}

	return retval;
}

int arc32_get_current_pc(struct target *target)
{
	int retval = ERROR_OK;
	uint32_t dpc;
	struct arc32_common *arc32 = target_to_arc32(target);

//	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	/* read current PC */
	retval = arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_PC_REG, &dpc);

//	printf(" $PC: 0x%x\n",dpc);

	/* save current PC */
	buf_set_u32(arc32->core_cache->reg_list[PC_REG].value, 0, 32, dpc);

	return retval;
}

int arc32_read_mem_block(struct arc_jtag *jtag_info, uint32_t addr, int size,
	int count, void *buf)
{
	int retval = ERROR_OK;
	int i;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	assert(size <= 4);

	if (size <= 4) { /* means 4 * 2 bytes */
		for(i = 0; i < count; i++) {
			retval = arc_jtag_read_memory(jtag_info, addr + (i * 4),
				buf + (i * 4));
//#ifdef DEBUG
			uint32_t buffer;
			retval = arc_jtag_read_memory(jtag_info, addr + (i * 4), &buffer);
			printf(" > value (size:%d): 0x%x @: 0x%x\n", size, buffer, addr + (i * 4));
//#endif
		}
	} else
		retval = ERROR_FAIL;

	return retval;
}

int arc32_write_mem_block(struct arc_jtag *jtag_info, uint32_t addr, int size,
	int count, void *buf)
{
	int retval = ERROR_OK;
	int i;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	assert(size <= 4);

	if (size == 4) {
		for(i = 0; i < count; i++) {
			LOG_DEBUG(" >> write: 0x%x @ 0x%x", *(uint32_t *)(buf + (i * size)),
				addr + (i * size));
			retval = arc_jtag_write_memory(jtag_info, (addr + (i * size)),
				(buf + (i * size)));
#ifdef DEBUG
			uint32_t buffer;
			retval = arc_jtag_read_memory(jtag_info, addr + (i * size), &buffer);
			printf(" >         0x%x @: 0x%x\n", buffer, addr + (i * 4));
#endif
		}
	} else {
		/*
		 * we have to handle the sw-breakpoint here.
		 * size 2  = half word
		 */
		uint32_t buffer;

		retval = arc_jtag_read_memory(jtag_info, addr, &buffer);
		printf(" > read:  0x%x @: 0x%x\n", buffer, addr);
		printf("   need to write(16): 0x%x\n", *(uint16_t *)buf);
		//printf("   need to write(32): 0x%x\n", *(uint32_t *)buf);

		memcpy(&buffer, buf, sizeof(uint16_t));
		printf(" >> have to write: 0x%x\n",buffer);
		retval = arc_jtag_write_memory(jtag_info, addr, &buffer);

		arc_jtag_read_memory(jtag_info, addr, &buffer);
		printf(" > read:  0x%x @: 0x%x\n", buffer, addr);
	}

	return retval;
}











