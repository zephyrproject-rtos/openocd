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

#include "jtag/interface.h"
#include "breakpoints.h"
#include "algorithm.h"
#include "target_request.h"
#include "target_type.h"
#include "register.h"
#include <helper/time_support.h>

#include "arc32.h"
#include "arc700.h"
#include "arc_jtag.h"

static int PRINT = 1;


/* ----- Supporting functions ---------------------------------------------- */

static int arc700_init_arch_info(struct target *target,
	struct arc700_common *arc700, struct jtag_tap *tap)
{
	int retval = ERROR_OK;
	struct arc32_common *arc32 = &arc700->arc32;

	LOG_DEBUG("  >>> Calling into <<<");

	arc700->common_magic = ARC700_COMMON_MAGIC;

	/* initialize arc700 specific info */
	retval = arc32_init_arch_info(target, arc32, tap);
	arc32->arch_info = arc700;

	return retval;
}

static int arc700_set_breakpoint(struct target *target,
		struct breakpoint *breakpoint)
{
	int retval = ERROR_OK;
	struct arc32_common *arc32 = target_to_arc32(target);
	struct arc32_comparator *comparator_list = arc32->inst_break_list;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	if (breakpoint->set) {
		LOG_WARNING("breakpoint already set");
		return ERROR_OK;
	}

	if (breakpoint->type == BKPT_HARD) {
		int bp_num = 0;

		while (comparator_list[bp_num].used && (bp_num < arc32->num_inst_bpoints))
			bp_num++;

		if (bp_num >= arc32->num_inst_bpoints) {
			LOG_ERROR("Can not find free FP Comparator(bpid: %d)",
					breakpoint->unique_id);
			return ERROR_TARGET_RESOURCE_NOT_AVAILABLE;
		}

		breakpoint->set = bp_num + 1;
		comparator_list[bp_num].used = 1;
		comparator_list[bp_num].bp_value = breakpoint->address;
		target_write_u32(target, comparator_list[bp_num].reg_address,
				comparator_list[bp_num].bp_value);
		target_write_u32(target, comparator_list[bp_num].reg_address + 0x08, 0x00000000);
		target_write_u32(target, comparator_list[bp_num].reg_address + 0x18, 1);

		LOG_DEBUG("bpid: %d, bp_num %i bp_value 0x%" PRIx32 "",
				  breakpoint->unique_id,
				  bp_num, comparator_list[bp_num].bp_value);
	} else if (breakpoint->type == BKPT_SOFT) {
		LOG_DEBUG("bpid: %d", breakpoint->unique_id);

		if (breakpoint->length <= 4) { /* WAS: == 4) { but we have only 32 bits access !!*/
			uint32_t verify = 0xffffffff;

			retval = target_read_memory(target, breakpoint->address, breakpoint->length, 1,
					breakpoint->orig_instr);
			if (retval != ERROR_OK)
				return retval;

			retval = target_write_u32(target, breakpoint->address, ARC32_OPC_SDBBP);
			if (retval != ERROR_OK)
				return retval;

			retval = target_read_u32(target, breakpoint->address, &verify);

			if (retval != ERROR_OK)
				return retval;
				if (verify != ARC32_OPC_SDBBP) {
				LOG_ERROR("Unable to set 32bit breakpoint at address %08" PRIx32
						" - check that memory is read/writable", breakpoint->address);
				return ERROR_OK;
			}
		} else {
			uint16_t verify = 0xffff;

			retval = target_read_memory(target, breakpoint->address, breakpoint->length, 1,
					breakpoint->orig_instr);
			if (retval != ERROR_OK)
				return retval;
			retval = target_write_u16(target, breakpoint->address, ARC16_OPC_SDBBP);
			if (retval != ERROR_OK)
				return retval;

			retval = target_read_u16(target, breakpoint->address, &verify);
			if (retval != ERROR_OK)
				return retval;
			if (verify != ARC16_OPC_SDBBP) {
				LOG_ERROR("Unable to set 16bit breakpoint at address %08" PRIx32
						" - check that memory is read/writable", breakpoint->address);
				return ERROR_OK;
			}
		}

		breakpoint->set = 64; /* Any nice value but 0 */
	}

	return retval;
}





/* ----- Target command handler functions ---------------------------------- */

int arc700_poll(struct target *target)
{
	int retval = ERROR_OK;
	uint32_t status;
	struct arc32_common *arc32 = target_to_arc32(target);

	LOG_DEBUG(">> Entering <<");

	retval = arc_jtag_status(&arc32->jtag_info, &status);
	if (retval != ERROR_OK)
		return retval;

	/* check for processor halted */
	if (status & ARC_JTAG_STAT_RU) {
		printf("target is still running !!\n");
		target->state = TARGET_RUNNING;
	} else {
		if ((target->state == TARGET_RUNNING) ||
			(target->state == TARGET_RESET)) {

			target->state = TARGET_HALTED;
			LOG_DEBUG("ARC core is halted or in reset.\n");

			retval = arc32_debug_entry(target);
			if (retval != ERROR_OK)
				return retval;

			target_call_event_callbacks(target, TARGET_EVENT_HALTED);
		} else if (target->state == TARGET_DEBUG_RUNNING) {

			target->state = TARGET_HALTED;
			printf("ARC core is in debug running.\n");

			retval = arc32_debug_entry(target);
			if (retval != ERROR_OK)
				return retval;

			target_call_event_callbacks(target, TARGET_EVENT_DEBUG_HALTED);
		} else {
			if (PRINT) {
				LOG_USER("OpenOCD for ARC is ready to accept:"
					" (gdb) target remote <host ip address>:3333");
				PRINT = 0;
			}
		}
	}

	return retval;
}

int arc700_target_request_data(struct target *target,
	uint32_t size, uint8_t *buffer)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	printf(" !! @ software to do so :-) !!\n");

	return retval;
}

int arc700_halt(struct target *target)
{
	int retval = ERROR_OK;
//	struct arc32_common *arc32 = target_to_arc32(target);
//	struct arc_jtag *jtag_info = &arc32->jtag_info;

	LOG_DEBUG(">> Entering <<");

//	printf("target->state: %s\n", target_state_name(target));
	LOG_DEBUG("target->state: %s", target_state_name(target));

	if (target->state == TARGET_HALTED) {
		LOG_DEBUG("target was already halted");
		return ERROR_OK;
	}

	if (target->state == TARGET_UNKNOWN)
		LOG_WARNING("target was in unknown state when halt was requested");

	if (target->state == TARGET_RESET) {
		if ((jtag_get_reset_config() & RESET_SRST_PULLS_TRST) && jtag_get_srst()) {
			LOG_ERROR("can't request a halt while in reset if nSRST pulls nTRST");
			return ERROR_TARGET_FAILURE;
		} else {
			/* we came here in a reset_halt or reset_init sequence
			 * debug entry was already prepared in arc700_assert_reset()
			 */
			target->debug_reason = DBG_REASON_DBGRQ;

			return ERROR_OK;
		}
	}

	/* break processor */
//	mips_ejtag_enter_debug(ejtag_info);

	target->debug_reason = DBG_REASON_DBGRQ;

	return retval;
}

int arc700_resume(struct target *target, int current,
	uint32_t address, int handle_breakpoints, int debug_execution)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	printf(" !! @ software to do so :-) !!\n");

	return retval;
}

int arc700_step(struct target *target, int current,
	uint32_t address, int handle_breakpoints)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	printf(" !! @ software to do so :-) !!\n");

	return retval;
}

int arc700_assert_reset(struct target *target)
{
	int retval = ERROR_OK;
	struct arc32_common *arc32 = target_to_arc32(target);
//	struct arc_jtag *jtag_info = &arc32->jtag_info;

	LOG_DEBUG(">> Entering <<");

	LOG_DEBUG("target->state: %s", target_state_name(target));

	enum reset_types jtag_reset_config = jtag_get_reset_config();

	/* some cores support connecting while srst is asserted
	 * use that mode is it has been configured */

	bool srst_asserted = false;

	if (!(jtag_reset_config & RESET_SRST_PULLS_TRST) &&
			(jtag_reset_config & RESET_SRST_NO_GATING)) {
		jtag_add_reset(0, 1);
		srst_asserted = true;
	}

//	if (target->reset_halt) {
		/* use hardware to catch reset */
//		mips_ejtag_set_instr(ejtag_info, EJTAG_INST_EJTAGBOOT);
//	} else
//		mips_ejtag_set_instr(ejtag_info, EJTAG_INST_NORMALBOOT);

	if (jtag_reset_config & RESET_HAS_SRST) {
		/* here we should issue a srst only, but we may have to assert trst as well */
		if (jtag_reset_config & RESET_SRST_PULLS_TRST)
			jtag_add_reset(1, 1);
		else if (!srst_asserted)
			jtag_add_reset(0, 1);
//	} else {
//		if (mips_m4k->is_pic32mx) {
//			LOG_DEBUG("Using MTAP reset to reset processor...");

			/* use microchip specific MTAP reset */
//			mips_ejtag_set_instr(ejtag_info, MTAP_SW_MTAP);
//			mips_ejtag_set_instr(ejtag_info, MTAP_COMMAND);

//			mips_ejtag_drscan_8_out(ejtag_info, MCHP_ASERT_RST);
//			mips_ejtag_drscan_8_out(ejtag_info, MCHP_DE_ASSERT_RST);
//			mips_ejtag_set_instr(ejtag_info, MTAP_SW_ETAP);
//		} else {
			/* use ejtag reset - not supported by all cores */
//			uint32_t ejtag_ctrl = ejtag_info->ejtag_ctrl | EJTAG_CTRL_PRRST | EJTAG_CTRL_PERRST;
//			LOG_DEBUG("Using EJTAG reset (PRRST) to reset processor...");
//			mips_ejtag_set_instr(ejtag_info, EJTAG_INST_CONTROL);
//			mips_ejtag_drscan_32_out(ejtag_info, ejtag_ctrl);
//		}
	}

	target->state = TARGET_RESET;
	jtag_add_sleep(50000);

	register_cache_invalidate(arc32->core_cache);

	if (target->reset_halt)
		retval = target_halt(target);

	return retval;
}

int arc700_deassert_reset(struct target *target)
{
	int retval = ERROR_OK;

	LOG_DEBUG(">> Entering <<");

	LOG_DEBUG("target->state: %s", target_state_name(target));

	/* deassert reset lines */
	jtag_add_reset(0, 0);

	return retval;
}

int arc700_soft_reset_halt(struct target *target)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	printf(" !! @ software to do so :-) !!\n");

	return retval;
}

int arc700_read_memory(struct target *target, uint32_t address,
	uint32_t size, uint32_t count, uint8_t *buffer)
{
	int retval = ERROR_OK;
	struct arc32_common *arc32 = target_to_arc32(target);
//	struct arc_jtag *jtag_info = &arc32->jtag_info;

	LOG_DEBUG(">> Entering <<");

//	printf("address: 0x%8.8" PRIx32 ", size: 0x%8.8" PRIx32 \
//		", count: 0x%8.8" PRIx32 "\n", address, size, count);
	LOG_DEBUG("address: 0x%8.8" PRIx32 ", size: 0x%8.8" PRIx32 \
		", count: 0x%8.8" PRIx32 "", address, size, count);

	if (target->state != TARGET_HALTED) {
		LOG_WARNING("target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}

	/* sanitize arguments */

	if (((size != 4) && (size != 2) && (size != 1)) || (count == 0) || !(buffer))
		return ERROR_COMMAND_SYNTAX_ERROR;

	if (((size == 4) && (address & 0x3u)) || ((size == 2) && (address & 0x1u)))
		return ERROR_TARGET_UNALIGNED_ACCESS;

	/* since we don't know if buffer is aligned, we allocate new mem that
	 * is always aligned.
	 */
	void *tunnel = NULL;

	if (size > 1) {
		tunnel = malloc(count * size * sizeof(uint8_t));
		if (tunnel == NULL) {
			LOG_ERROR("Out of memory");
			return ERROR_FAIL;
		}
	} else
		tunnel = buffer;

	/* if noDMA off, use DMAACC mode for memory read */

//	if (jtag_info->impcode & JTAG_IMP_NODMA)
		retval = arc32_pracc_read_mem(&arc32->jtag_info, address, size,
			count, tunnel);
//	else
//		retval = arc32_dmaacc_read_mem(jtag_info, address, size,
//			count, tunnel);

	/* arc32_..._read_mem with size 4/2 returns uint32_t/uint16_t in host */
	/* endianness, but byte array should represent target endianness       */
	if (ERROR_OK == retval) {
		switch (size) {
		case 4:
			target_buffer_set_u32_array(target, buffer, count, tunnel);
#ifdef DEBUG
			int i;
			for(i = 0; i < count; i++) {
				/* print byte position content of complete word */
				printf("    **> 0x%02x",buffer[3 + (4 * i)]);
				printf("%02x",buffer[2 + (4 * i)]);
				printf("%02x",buffer[1 + (4 * i)]);
				printf("%02x\n",buffer[0 + (4 * i)]);
			}
#endif
			break;
		case 2:
			target_buffer_set_u16_array(target, buffer, count, tunnel);
			break;
		}
	}

	if ((size > 1) && (tunnel != NULL))
		free(tunnel);

	return retval;
}

int arc700_write_memory(struct target *target, uint32_t address,
	uint32_t size, uint32_t count, const uint8_t *buffer)
{
	int retval = ERROR_OK;
	struct arc32_common *arc32 = target_to_arc32(target);
	struct arc_jtag *jtag_info = &arc32->jtag_info;

	LOG_DEBUG(">> Entering <<");

	printf("start writing @ address: 0x%8.8" PRIx32 " : %d bytes\n",
			address, count);
	LOG_DEBUG("address: 0x%8.8" PRIx32 ", size: 0x%8.8" PRIx32 ", count: 0x%8.8" PRIx32 "",
			address, size, count);

	if (target->state != TARGET_HALTED) {
		LOG_WARNING("target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}

	/* sanitize arguments */

	if (((size != 4) && (size != 2) && (size != 1)) || (count == 0) || !(buffer))
		return ERROR_COMMAND_SYNTAX_ERROR;

	if (((size == 4) && (address & 0x3u)) || ((size == 2) && (address & 0x1u)))
		return ERROR_TARGET_UNALIGNED_ACCESS;

	/* correct endianess if we have word or hword access */
	void *tunnel = NULL;

	if (size > 1) {
		/*
		 * arc32_..._write_mem with size 4/2 requires uint32_t/uint16_t
		 * in host endianness, but byte array represents target endianness.
		 */
		tunnel = malloc(count * size * sizeof(uint8_t));
		if (tunnel == NULL) {
			LOG_ERROR("Out of memory");
			return ERROR_FAIL;
		}

		switch (size) {
		case 4:
			target_buffer_get_u32_array(target, buffer, count,
				(uint32_t *)tunnel);
			break;
		case 2:
			target_buffer_get_u16_array(target, buffer, count,
				(uint16_t *)tunnel);
			break;
		}
		buffer = tunnel;
	}

	/*
	 * if noDMA off, use DMAACC mode for memory write,
	 * else, do direct memory transfer
	 */
	//if (jtag_info->dma_transfer & JTAG_IMP_NODMA)
		retval = arc32_pracc_write_mem(jtag_info, address, size, count,
			(void *)buffer);
	//else
		//retval = arc32_dmaacc_write_mem(jtag_info, address, size, count,
		//(void *)buffer);

	if (tunnel != NULL)
		free(tunnel);

	return retval;
}

int arc700_bulk_write_memory(struct target *target, uint32_t address,
	uint32_t count, const uint8_t *buffer)
{
	int retval = ERROR_OK;
	struct arc32_common *arc32 = target_to_arc32(target);
	struct arc_jtag *jtag_info = &arc32->jtag_info;

	LOG_DEBUG(">> Entering <<");

	LOG_DEBUG("address: 0x%8.8" PRIx32 ", count: 0x%8.8" PRIx32 "", address, count);

	if (target->state != TARGET_HALTED) {
		LOG_WARNING("target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}

	/* check alignment */
	if (address & 0x3u)
		return ERROR_TARGET_UNALIGNED_ACCESS;

	if (arc32->fast_data_area == NULL) {
		/*
		 * Get memory for block write handler
		 * we preserve this area between calls and gain a speed increase
		 * of about 3kb/sec when writing flash
		 * this will be released/nulled by the system when the target is
		 * resumed or reset.
		 */
		retval = target_alloc_working_area(target,
				ARC32_FASTDATA_HANDLER_SIZE,
				&arc32->fast_data_area);

		if (retval != ERROR_OK) {
			LOG_WARNING("No working area available, falling back to non-bulk write");
			return arc700_write_memory(target, address, 4, count, buffer);
		} else {
			LOG_WARNING(" !! ARC32_FASTDATA_HANDLER_SIZE too small !!");
			retval = ERROR_OK;
		}

		/* reset fastadata state so the algo get reloaded */
		jtag_info->fast_access_save = -1;
	}

	/* arc32_pracc_fastdata_xfer requires uint32_t in host endianness, */
	/* but byte array represents target endianness                      */
	uint32_t *tunnel = NULL;
	tunnel = malloc(count * sizeof(uint32_t));
	if (tunnel == NULL) {
		LOG_ERROR("Out of memory");
		return ERROR_FAIL;
	}

	target_buffer_get_u32_array(target, buffer, count, tunnel);

#ifdef DEBUG
	/* transfer big data block into target !! needs performance upgrade !! */
	printf(" > going to store: 0x%08x\n", (uint32_t *)tunnel[0]);
	printf(" >               : 0x%08x\n", (uint32_t *)tunnel[1]);
	printf(" >               : 0x%08x\n", (uint32_t *)tunnel[2]);
	printf(" >               : 0x%08x\n", (uint32_t *)tunnel[3]);
	printf(" >       ----->  : 0x%08x\n", (uint32_t *)tunnel[4]);
	printf(" >               : 0x%08x\n", (uint32_t *)tunnel[5]);
	printf(" >               : 0x%08x\n", (uint32_t *)tunnel[6]);
#endif
	//retval = arc700_write_memory(target, address, 4, count, (uint8_t *)tunnel);
	retval = arc_jtag_write_block(jtag_info, address, 4, count, (uint32_t *)tunnel);
	/* end of progress indication ... */
	printf("Done with download.\n");

	if (tunnel != NULL)
		free(tunnel);

	if (retval != ERROR_OK) {
		/* FASTDATA access failed, try normal memory write */
		LOG_DEBUG("Fastdata access Failed, falling back to non-bulk write");
		retval = arc700_write_memory(target, address, 4, count, buffer);
	}

	return retval;
}

int arc700_checksum_memory(struct target *target,
	uint32_t address, uint32_t count, uint32_t *checksum)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	printf(" !! @ software to do so :-) !!\n");

	return retval;
}

int arc700_blank_check_memory(struct target *target,
	uint32_t address, uint32_t count, uint32_t *blank)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	printf(" !! @ software to do so :-) !!\n");

	return retval;
}

int arc700_run_algorithm(struct target *target,
	int num_mem_params, struct mem_param *mem_params,
	int num_reg_params, struct reg_param *reg_params,
	uint32_t entry_point, uint32_t exit_point,
	int timeout_ms, void *arch_info)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	printf(" !! @ software to do so :-) !!\n");

	return retval;
}

int arc700_start_algorithm(struct target *target,
	int num_mem_params, struct mem_param *mem_params,
	int num_reg_params, struct reg_param *reg_params,
	uint32_t entry_point, uint32_t exit_point,
	void *arch_info)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	printf(" !! @ software to do so :-) !!\n");

	return retval;
}

int arc700_wait_algorithm(struct target *target,
	int num_mem_params, struct mem_param *mem_params,
	int num_reg_params, struct reg_param *reg_params,
	uint32_t exit_point, int timeout_ms,
	void *arch_info)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	printf(" !! @ software to do so :-) !!\n");

	return retval;
}

int arc700_add_breakpoint(struct target *target, struct breakpoint *breakpoint)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	struct arc32_common *arc32 = target_to_arc32(target);

	if (breakpoint->type == BKPT_HARD) {
		if (arc32->num_inst_bpoints_avail < 1) {
			printf(" >> no hardware breakpoint available yet.\n");
			LOG_INFO("no hardware breakpoint available yet.");
			return ERROR_TARGET_RESOURCE_NOT_AVAILABLE;
		}

		arc32->num_inst_bpoints_avail--;
	}

	return arc700_set_breakpoint(target, breakpoint);
}

int arc700_remove_breakpoint(struct target *target,
	struct breakpoint *breakpoint)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	printf(" !! @ software to do so :-) !!\n");

	return retval;
}

int arc700_add_watchpoint(struct target *target, struct watchpoint *watchpoint)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	printf(" !! @ software to do so :-) !!\n");

	return retval;
}

int arc700_remove_watchpoint(struct target *target,
	struct watchpoint *watchpoint)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	printf(" !! @ software to do so :-) !!\n");

	return retval;
}

int arc700_target_create(struct target *target, Jim_Interp *interp)
{
	int retval = ERROR_OK;

	LOG_DEBUG(">> Entering <<");

	struct arc700_common *arc700 = calloc(1, sizeof(struct arc700_common));

	retval = arc700_init_arch_info(target, arc700, target->tap);

	return retval;
}

int arc700_init_target(struct command_context *cmd_ctx,
	struct target *target)
{
	int retval = ERROR_OK;

	LOG_DEBUG(">> Entering <<");

	arc32_build_reg_cache(target);

	return retval;
}

int arc700_examine(struct target *target)
{
	int retval = ERROR_OK;
	uint32_t value, status;
	struct arc32_common *arc32 = target_to_arc32(target);

	LOG_DEBUG(">> Entering <<");

	retval = arc_jtag_startup(&arc32->jtag_info);

	if (!target_was_examined(target)) {

		/* read JTAG info */
		arc_jtag_idcode(&arc32->jtag_info, &value);
		printf("JTAG ID: 0x%x\n", value);
		arc_jtag_status(&arc32->jtag_info, &status);
		printf("JTAG status: 0x%x\n", status);

		/* bring processor into HALT */
		printf("bring ARC core into HALT state.\n");
		value = SET_CORE_FORCE_HALT;
		arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_DEBUG_REG, &value);
		value = SET_CORE_FORCE_HALT;
		arc_jtag_write_aux_reg(&arc32->jtag_info, AUX_DEBUG_REG, &value);
		sleep(2); /* just give us once some time */
		arc_jtag_status(&arc32->jtag_info, &status);
		printf("JTAG status: 0x%x\n", status);

		/* read ARC core info */
		arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_IDENTITY_REG, &value);
		printf("CPU ID: 0x%x\n", value);
		arc_jtag_read_aux_reg(&arc32->jtag_info, AUX_PC_REG, &value);
		printf("current PC: 0x%x\n", value);

		arc32_print_core_registers(&arc32->jtag_info);
		arc32_print_aux_registers(&arc32->jtag_info);

		arc_jtag_status(&arc32->jtag_info, &status);
		if (status & ARC_JTAG_STAT_RU) {
			printf("target is still running !!\n");
			target->state = TARGET_RUNNING;
		} else {
			LOG_DEBUG("target is halted.");
			target->state = TARGET_RESET; /* means HALTED after restart */
		}

		target_set_examined(target);
	}

	return retval;
}

/* ----- Command handlers -------------------------------------------------- */


COMMAND_HANDLER(handle_arc700_smp_gdb_command)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	printf(" !! @ adapt software to get it working :-) !!\n");

	struct target *target = get_current_target(CMD_CTX);

	struct target_list *head;
	head = target->head;

	if (head != (struct target_list *)NULL) {
		if (CMD_ARGC == 1) {
			int coreid = 0;
			COMMAND_PARSE_NUMBER(int, CMD_ARGV[0], coreid);
			if (ERROR_OK != retval)
				return retval;
			target->gdb_service->core[1] = coreid;
		}
		command_print(CMD_CTX, "gdb coreid  %d -> %d",
			target->gdb_service->core[0], target->gdb_service->core[1]);
	}

	return retval;
}

/* ----- Supported commands ------------------------------------------------ */

static const struct command_registration arc700_exec_command_handlers[] = {
	/* See: src/target/cortex_m3.c  ..._exec_command_handlers[] */
	{
		.name = "maskisr",
		.handler = NULL, //handle_arc700_mask_interrupts_command,
		.mode = COMMAND_EXEC,
		.help = "mask arc700 interrupts",
		.usage = "['auto'|'on'|'off']",
	},
	{
		.name = "vector_catch",
		.handler = NULL, //handle_arc700_vector_catch_command,
		.mode = COMMAND_EXEC,
		.help = "configure hardware vectors to trigger debug entry",
		.usage = "['all'|'none'|('bus_err'|'chk_err'|...)*]",
	},
	{
		.name = "reset_config",
		.handler = NULL, //handle_arc700_reset_config_command,
		.mode = COMMAND_ANY,
		.help = "configure software reset handling",
		.usage = "['srst'|'sysresetreq'|'vectreset']",
	},
	/* See: src/target/mips_m4k.c  mips_m4k_exec_command_handlers[] */
	{
		.name = "smp_gdb",
		.handler = handle_arc700_smp_gdb_command,
		.mode = COMMAND_EXEC,
		.help = "display/fix current core played to gdb",
		.usage = "",
	},
	COMMAND_REGISTRATION_DONE
};

const struct command_registration arc700_command_handlers[] = {
	{
		.chain = arc32_command_handlers,
	},
	{
		.name = "arc700",
		.mode = COMMAND_ANY,
		.help = "ARC700 command group",
		.usage = "Help info ...",
		.chain = arc700_exec_command_handlers,
	},
	COMMAND_REGISTRATION_DONE
};

/* ----- Target commands structure ----------------------------------------- */

struct target_type arc700_target = {
	.name = "arc700",

	.poll =	arc700_poll,
	.arch_state = arc32_arch_state,

	.target_request_data = arc700_target_request_data,

	.halt = arc700_halt,
	.resume = arc700_resume,
	.step = arc700_step,

	.assert_reset = arc700_assert_reset,
	.deassert_reset = arc700_deassert_reset,
	.soft_reset_halt = arc700_soft_reset_halt,

	.get_gdb_reg_list = (int)arc32_get_gdb_reg_list,

	.read_memory = arc700_read_memory,
	.write_memory = arc700_write_memory,
	.bulk_write_memory = arc700_bulk_write_memory,
	.checksum_memory = arc700_checksum_memory,
	.blank_check_memory = arc700_blank_check_memory,

	.run_algorithm = arc700_run_algorithm,
	.start_algorithm = arc700_start_algorithm,
	.wait_algorithm = arc700_wait_algorithm,

	.add_breakpoint = arc700_add_breakpoint,
	.remove_breakpoint = arc700_remove_breakpoint,
	.add_watchpoint = arc700_add_watchpoint,
	.remove_watchpoint = arc700_remove_watchpoint,

	/* openocd infrastructure initialization */
	.commands = arc700_command_handlers,
	.target_create = arc700_target_create,
	.init_target = arc700_init_target,
	.examine = arc700_examine,
};
