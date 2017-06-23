/*
 * Copyright(c) 2015 Intel Corporation.
 *
 * Jessica Gomez (jessica.gomez.hernandez@intel.com)
 * Ivan De Cesaris (ivan.de.cesaris@intel.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Contact Information:
 * Intel Corporation
 */

/*
 * @file
 * Debugger for Intel Quark D2000
 * TODO: update this text (Quark) with the Quark D2000 specific one
 * The CPU TAP (Lakemont TAP) is used for software debug and the CLTAP is
 * used for SoC level operations.
 * Useful docs are here: https://communities.intel.com/community/makers/documentation
 * Intel Quark SoC X1000 OpenOCD/GDB/Eclipse App Note (web search for doc num 330015)
 * Intel Quark SoC X1000 Debug Operations User Guide (web search for doc num 329866)
 * Intel Quark SoC X1000 Datasheet (web search for doc num 329676)
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <helper/log.h>

#include "target.h"
#include "target_type.h"
#include "breakpoints.h"
#include "lakemont.h"
#include "quark_d2000.h"
#include "x86_32_common.h"

extern struct command_context *global_cmd_ctx;

int quark_d2000_target_create(struct target *t, Jim_Interp *interp)
{
	struct x86_32_common *x86_32 = calloc(1, sizeof(struct x86_32_common));
	if (x86_32 == NULL) {
		LOG_ERROR("%s out of memory", __func__);
		return ERROR_FAIL;
	}
	x86_32_common_init_arch_info(t, x86_32);
	lakemont_init_arch_info(t, x86_32);
	x86_32->core_type = LMT3_5;
	return ERROR_OK;
}

int quark_d2000_init_target(struct command_context *cmd_ctx, struct target *t)
{
	return lakemont_init_target(cmd_ctx, t);
}

/*
 * issue a system reset using a mem write, preparing the CLTAP to resetbreak
 */
static int quark_d2000_target_reset(struct target *t)
{
	LOG_DEBUG("issuing target reset");
	struct x86_32_common *x86_32 = target_to_x86_32(t);
	int retval = ERROR_OK;

	/* update target state if SRST was triggered */
	lakemont_poll(t);

	/* we can't be running when writing to memory */
	if (t->state == TARGET_RUNNING) {
		retval = lakemont_halt(t);
		if (retval != ERROR_OK) {
			LOG_ERROR("%s could not halt target", __func__);
			return retval;
		}
	}

	/* remove breakpoints and watchpoints */
	/* restore flash memory in case sw breaks were set here */
	/* TODO: check if there is a better way to do it */
	x86_32_common_reset_breakpoints_watchpoints(t);

	/* set reset break */
	static struct scan_blk scan;
	struct scan_field *fields = &scan.field;
	scan.out[0] = 0x35;
	fields->out_value = ((uint8_t *)scan.out);
	fields->in_value = NULL;
	fields->num_bits = 8;
	jtag_add_ir_scan(x86_32->curr_tap, fields, TAP_IDLE);
	retval = jtag_execute_queue();
	if (retval != ERROR_OK) {
		LOG_ERROR("%s irscan failed to execute queue", __func__);
		return retval;
	}

	scan.out[0] = 0x1;
	fields->out_value = ((uint8_t *)scan.out);
	fields->num_bits = 1;
	jtag_add_dr_scan(x86_32->curr_tap, 1, fields, TAP_IDLE);
	retval = jtag_execute_queue();
	if (retval != ERROR_OK) {
		LOG_ERROR("%s drscan failed to execute queue", __func__);
		return retval;
	}

	/* write 0x2 to address 0xb0800570 to cause a warm reset */
	LOG_DEBUG("%s writing mem to reset NOW!", __func__);
	const uint8_t buf[] = { 0x2, 0x0, 0x0, 0x0 };
	retval = x86_32_common_write_memory(t, 0xb0800570, 4, 1, buf);
	if (retval != ERROR_OK) {
		LOG_ERROR("%s could not write memory", __func__);
		return retval;
	}

	/* entered PM after reset, update the state */
	t->state = TARGET_RESET;
	t->debug_reason = DBG_REASON_DBGRQ;
	retval = lakemont_update_after_probemode_entry(t);
	if (retval != ERROR_OK) {
		LOG_ERROR("%s could not update state after probemode entry", __func__);
		return retval;
	}

	/* clear reset break */
	scan.out[0] = 0x35;
	fields->out_value = ((uint8_t *)scan.out);
	fields->in_value = NULL;
	fields->num_bits = 8;
	jtag_add_ir_scan(x86_32->curr_tap, fields, TAP_IDLE);
	retval = jtag_execute_queue();
	if (retval != ERROR_OK) {
		LOG_ERROR("%s irscan failed to execute queue", __func__);
		return retval;
	}

	scan.out[0] = 0x0;
	fields->out_value = ((uint8_t *)scan.out);
	fields->num_bits = 1;
	jtag_add_dr_scan(x86_32->curr_tap, 1, fields, TAP_IDLE);
	retval = jtag_execute_queue();
	if (retval != ERROR_OK) {
		LOG_ERROR("%s drscan failed to execute queue", __func__);
		return retval;
	}

	/* resume target if reset mode is run */
	if (!t->reset_halt) {
		retval = lakemont_resume(t, 1, 0, 0, 0);
		if (retval != ERROR_OK) {
			LOG_ERROR("%s could not resume target", __func__);
			return retval;
		}
	}

	return ERROR_OK;
}

static int quark_d2000_wait_flash_mask(struct target *t, uint32_t addr, uint32_t bit_mask)
{
	int cnt = 100;
	int retval = ERROR_OK;
	uint8_t ctl_buff[4];

	do {
		retval = target_read_memory(t, addr, 4, 1, ctl_buff);
		if (retval != ERROR_OK) {
			LOG_ERROR("%s: Error reading memory addr = 0x%08" PRIx32, __func__, addr);
			return retval;
		}

		if ((uint32_t) *ctl_buff & bit_mask) {
			return ERROR_OK;
		}

		/* Give some time to bit get set */
		LOG_DEBUG("%s: Waiting for addr = 0x%08" PRIx32, __func__, addr);
		usleep(10000);
	} while (--cnt);

	return ERROR_FAIL;
}

int quark_d2000_flash_write(struct target *t, uint32_t addr,
			uint32_t size, uint32_t count, const uint8_t *buf)
{
	LOG_DEBUG("addr=0x%08" PRIx32 ", size=%" PRIu32 ", count=0x%" PRIx32 ", buf=%p",
				addr, size, count, buf);

	/* FLASH and ROM Control Registers */
	uint32_t FC_SUPPORT_ADDR; /* Register Field to specify if WR is enabled */
	uint32_t FC_SUPPORT_MASK; /* Bit mask that specifies WR enabled */
	uint32_t FC_START_ADDR;   /* Start Address in SoC memory map */
	uint32_t FC_LIMIT_ADDR;   /* End Address in SoC memory map */
	uint32_t FC_OFFSET;       /* Delta between SoC memory map and flash map */
	uint32_t FC_WR_CTL;       /* Control- Sets erase/write flag and address */
	uint32_t FC_WR_DATA;      /* 32bit register of data to be write */
	uint32_t FC_STTS;         /* Notifies when write/erase is done */

	/* OTP-ROM bit protection */
	/* by default we don't enable writing to the OTP bit */
	Jim_Obj *otp_write_enabled_obj;
	long otp_write_enabled = 0;

	/* Wait for FLASH_STTS for every word write */
	/* by default we don't need to wait, JTAG mem writes take long enough */
	Jim_Obj *flash_word_write_wait_obj;
	long flash_word_write_wait = 0;

	/* Avoid to erase page by page and wait FLASH_STTS for page delete */
	/* by default we erase page by page */
	Jim_Obj *flash_page_erase_disabled_obj;
	long flash_page_erase_disabled = 0;

	/* TODO: check if the TCL calls below are expensive, in case move the code to a better place */
	otp_write_enabled_obj = Jim_GetGlobalVariableStr(global_cmd_ctx->interp, "QUARK_D2000_OTP_WRITE_ENABLED", JIM_NONE);
	if (otp_write_enabled_obj != NULL) {
		int result = Jim_GetLong(global_cmd_ctx->interp, otp_write_enabled_obj, &otp_write_enabled);
		LOG_DEBUG("result %d, val %ld", result, otp_write_enabled);
	}
	flash_word_write_wait_obj = Jim_GetGlobalVariableStr(global_cmd_ctx->interp, "QUARK_D2000_FLASH_WORD_WRITE_WAIT", JIM_NONE);
	if (flash_word_write_wait_obj != NULL) {
		int result = Jim_GetLong(global_cmd_ctx->interp, flash_word_write_wait_obj, &flash_word_write_wait);
		LOG_DEBUG("flash_word_write_wait - result %d, val %ld", result, flash_word_write_wait);
	}
	flash_page_erase_disabled_obj = Jim_GetGlobalVariableStr(global_cmd_ctx->interp, "QUARK_D2000_FLASH_PAGE_ERASE_DISABLED", JIM_NONE);
	if (flash_page_erase_disabled_obj != NULL) {
		int result = Jim_GetLong(global_cmd_ctx->interp, flash_page_erase_disabled_obj, &flash_page_erase_disabled);
	    LOG_DEBUG("flash_page_erase_disabled - result %d, val %ld", result, flash_page_erase_disabled);
	}

	/* Select control registers based on the desired flashing region */
	/* TODO: define a function that defines these parameters in a more scalable way */
	if (addr < OTPC_LIMIT) {
		LOG_DEBUG("%s: OTPC WRITE", __func__);
		FC_SUPPORT_ADDR = FLASH_STTS;
		FC_SUPPORT_MASK = ROM_CTL_WR_DIS_MASK;
		FC_START_ADDR = OTPC_BASE_ADDR;
		FC_LIMIT_ADDR = OTPC_LIMIT;
		FC_OFFSET = OTPC_OFFSET;
		FC_WR_CTL = ROM_WR_CTL;
		FC_WR_DATA = ROM_WR_DATA;
		FC_STTS = FLASH_STTS;

		if ((addr == OTPC_BASE_ADDR) && ((buf[0] & 0x1) == 0)) {
			LOG_USER("Trying to clear the OTP bit at address 0xFFFFE000, "
					"this will lock further writes to the flash ROM after reset.");
			if (otp_write_enabled != 1) {
				LOG_ERROR("The QUARK_D2000_OTP_WRITE_ENABLED variable isn't set to 1 "
						"so the operation wasn't performed.");
				LOG_ERROR("The following command will allow it: "
						"set QUARK_D2000_OTP_WRITE_ENABLED 1");
				return ERROR_FAIL;
			}
		}
	} else if ((addr >= FLASH_BASE_ADDR) && (addr <= FLASH_LIMT)) {
		LOG_DEBUG("%s: FLASH WRITE", __func__);
		FC_SUPPORT_ADDR = FLASH_CTL;
		FC_SUPPORT_MASK = FLASH_CTL_WR_DIS_MASK;
		FC_START_ADDR = FLASH_BASE_ADDR;
		FC_LIMIT_ADDR = FLASH_LIMT;
		FC_OFFSET = FLASH_OFFSET;
		FC_WR_CTL = FLASH_WR_CTL;
		FC_WR_DATA = FLASH_WR_DATA;
		FC_STTS = FLASH_STTS;

	} else if ((addr >= OTPD_BASE_ADDR) && (addr <= OTPD_LIMT)) {
		LOG_DEBUG("%s: OTPD WRITE", __func__);
		FC_SUPPORT_ADDR = FLASH_CTL;
		FC_SUPPORT_MASK = FLASH_CTL_WR_DIS_MASK;
		FC_START_ADDR = OTPD_BASE_ADDR;
		FC_LIMIT_ADDR = OTPD_LIMT;
		FC_OFFSET = OTPD_OFFSET;
		FC_WR_CTL = FLASH_WR_CTL;
		FC_WR_DATA = FLASH_WR_DATA;
		FC_STTS = FLASH_STTS;

	} else {
		LOG_ERROR("%s: Invalid address=0x%08" PRIx32, __func__, addr);
		return ERROR_COMMAND_ARGUMENT_INVALID;
	}

	uint8_t ctl_buff[4]; /* temp location we use to write/read memory */

	/* Check that write is enabled*/
	if ((target_read_memory(t, FC_SUPPORT_ADDR, 4, 1, ctl_buff) != ERROR_OK)
			|| ((uint32_t) *ctl_buff & FC_SUPPORT_MASK)) {
		LOG_ERROR("%s: Write Flash Disabled", __func__);
		return ERROR_FAIL;
	}

	/* clear flags */
	if (target_read_memory(t, FC_STTS, 1, 1, ctl_buff) != ERROR_OK) {
		LOG_ERROR("%s: Couldn't clear WR/ER flags", __func__);
		return ERROR_FAIL;
	}

	/* If SW breakpoints are overwritten by the debugger then t->running_alg=1 */
	/* if the user is just rewriting memory - notify that SW breakpoints will be overwritten */
	if  (t->running_alg == 0) {
		struct breakpoint *iter = t->breakpoints;
		while (iter != NULL) {
			if ((iter->set != 0) && (iter->type == BKPT_SOFT) &&
					((iter->address >= FC_START_ADDR) && (iter->address <= FC_LIMIT_ADDR)) &&
					((iter->address >= addr) && (iter->address <= addr + (size * count)))) {
				LOG_USER("Breakpoint at address 0x%08" TARGET_PRIxADDR " will be overwritten!!", iter->address);
				iter->set = 0;
			}
			iter = iter->next;
		}
	}

	/* Flash's offset address */
	uint32_t flash_addr = addr - FC_START_ADDR;
	/* Page's number ID */
	uint32_t flash_page_num = (flash_addr >> 11);
	/* Page's first address */
	uint32_t flash_page_start = FC_START_ADDR + ((flash_page_num) * FC_BYTE_PAGE_SIZE);
	/* Page's last address */
	uint32_t flash_page_limit = flash_page_start + FC_BYTE_PAGE_SIZE - 1;
	/* Page's offset to be modified */
	uint32_t flash_page_offset = addr - flash_page_start;
	/* Page's address counter */
	uint32_t copy_addr = flash_page_start - FC_START_ADDR;
	/* Array to shadow page content and modify desired data */
	uint8_t data_buff[FC_BYTE_PAGE_SIZE];
	/* Amount of bytes pending to be written */
	uint32_t rest_count = count * size;
	/* Amount of bytes to be modified in the page */
	uint32_t page_count;

	/* Loop until necessary pages are saved, cleared and modified */
	while (rest_count) {

		LOG_DEBUG("pagelimit=%" PRIx32 ", pagestart=%" PRIx32 ", flashaddr=%" PRIx32 ", restcount=%" PRIx32,
				flash_page_limit, flash_page_start, flash_addr, rest_count);

		/* if the requested write overlaps two pages, write to this page
		 * page_count bytes and leave rest_count for the next page write
		 */
		if ((rest_count > flash_page_limit) || (flash_addr + FC_START_ADDR > flash_page_limit - rest_count)) {
			page_count = (flash_page_limit - flash_addr - FC_START_ADDR) +1;
			rest_count = rest_count - page_count;
			LOG_USER_N(".");
		} else {
			page_count = rest_count;
			rest_count = 0;
		}

		/* Save Page */
		if (page_count != FC_BYTE_PAGE_SIZE) {
			/* Do not waste time if all page will be overwritten */
			if (target_read_memory(t, flash_page_start, 4, 512, &data_buff[0]) != ERROR_OK) {
				LOG_ERROR("%s: Couldn't save content of page #%d. Request failed!", __func__, flash_page_num);
				return ERROR_FAIL;
			}
		}

		if (!flash_page_erase_disabled) {
			/* Erase Page */
			buf_set_u32(ctl_buff, 0, 32, (((flash_addr + FC_OFFSET) << FC_WR_CTL_ADDR) | FC_WR_CTR_DREQ));
			if (target_write_memory(t, FC_WR_CTL, 4, 1, ctl_buff) != ERROR_OK) {
				LOG_ERROR("%s: Couldn't delete page #%d. Request failed!", __func__, flash_page_num);
				return ERROR_FAIL;
			}

			/* Check for FLASH_STTS */
			if (quark_d2000_wait_flash_mask(t, FC_STTS, FC_STTS_ER_DONE) != ERROR_OK) {
				LOG_ERROR("%s: Bit ER_DONE in FLASH_STTS Timeout!", __func__);
				return ERROR_FAIL;
			}
		}

		/* Write Page */
		memcpy(&data_buff[0] + flash_page_offset, buf, page_count);
		for (uint32_t i = 0; i < 512; i++) {

			/* Write word data */
			if (target_write_memory(t, FC_WR_DATA, 4, 1, &data_buff[i*4]) != ERROR_OK) {
				LOG_ERROR("%s: Couldn't write WR_DATA register in SRAM", __func__);
				return ERROR_FAIL;
			}

			/* Select address and flag WR request */
			buf_set_u32(ctl_buff, 0, 32, (((copy_addr + FC_OFFSET) << FC_WR_CTL_ADDR) | FC_WR_CTL_WREQ));
			if (target_write_memory(t, FC_WR_CTL, 4, 1, ctl_buff) != ERROR_OK) {
				LOG_ERROR("%s: Couldn't write WR_CTL register in SRAM", __func__);
				return ERROR_FAIL;
			}

			/* Check for FLASH_STTS */
			if (flash_word_write_wait) {
				if (quark_d2000_wait_flash_mask(t, FC_STTS, FC_STTS_WR_DONE) != ERROR_OK) {
					LOG_ERROR("%s: Bit WR_DONE in FLASH_STTS Timeout!", __func__);
					return ERROR_FAIL;
				}
			}

			/* given that doing word writes */
			copy_addr += 4;
		}

		/* Prepare next page info */
		flash_addr = copy_addr;
		flash_page_num = (flash_addr >> 11);
		flash_page_start = FC_START_ADDR + ((flash_page_num) * FC_BYTE_PAGE_SIZE);
		flash_page_limit = flash_page_start + FC_BYTE_PAGE_SIZE - 1;
		flash_page_offset = 0;

		buf = buf + page_count;

		/* Check if new page is not in other flash region */
		if ((rest_count > 0) && (flash_page_start >= FC_LIMIT_ADDR)) {
			return target_write_memory(t, flash_page_start, 1, rest_count, buf);
		}

	}

	return ERROR_OK;
}

int quark_d2000_write_memory(struct target *t, target_addr_t addr, uint32_t size,
			uint32_t count, const uint8_t *buf)
{
	LOG_DEBUG("addr=0x%08" TARGET_PRIxADDR ", size=%" PRIu32 ", count=0x%" PRIx32 ", buf=%p",
				addr, size, count, buf);
	if (check_not_halted(t))
		return ERROR_TARGET_NOT_HALTED;
	if (!count || !buf) {
		LOG_ERROR("%s invalid params count=0x%" PRIx32 ", buf=%p, addr=0x%08" TARGET_PRIxADDR,
					__func__, count, buf, addr);
		return ERROR_COMMAND_ARGUMENT_INVALID;
	}

	/*
	 * Check if we have an overlapping write to the protected area.
	 * In case we are allowed to write to it, the usual code path will be taken,
	 * otherwise give it a special treatment using the code in this block.
	 * We switch to bytes writes, it makes the calculations simpler (no need to
	 * take care of tails). The flash algorithm works by words anyway so it
	 * won't affect performance
	 */
	if ((addr <= OTPC_DATA_LIMIT)) {
		/* OTPC DATA bit protection */
		/* by default we don't enable writing to the OTPC DATA area */
		Jim_Obj *otpc_data_write_enabled_obj;
		long otpc_data_write_enabled = 0;
		otpc_data_write_enabled_obj = Jim_GetGlobalVariableStr(global_cmd_ctx->interp, "QUARK_D2000_OTPC_DATA_WRITE_ENABLED", JIM_NONE);
		if (otpc_data_write_enabled_obj != NULL) {
			int result = Jim_GetLong(global_cmd_ctx->interp, otpc_data_write_enabled_obj, &otpc_data_write_enabled);
			LOG_DEBUG("otpc_data_write_enabled %d, val %ld", result, otpc_data_write_enabled);
		}
		if (!otpc_data_write_enabled) {
			uint32_t delta = (OTPC_DATA_LIMIT - addr + 1);
			count = size * count;
			size = 1;
			LOG_DEBUG("%s: delta 0x%08" PRIx32 " count 0x%08" PRIx32 " buf=%p - buf+delta=%p",
						__func__, delta, count, buf, buf + delta);
			if (count <= delta) {
				LOG_USER_N("The requested flash write is contained inside the OTP DATA protected region ");
				LOG_USER("0x%08" PRIx32 " - 0x%08" PRIx32 ", skipping that part", OTPC_BASE_ADDR, OTPC_DATA_LIMIT);
				LOG_USER("The following command will allow it: ");
				LOG_USER("set QUARK_D2000_OTPC_DATA_WRITE_ENABLED 1");
				return ERROR_OK;
			}
			count -= delta;
			addr = OTPC_DATA_LIMIT + 1;
			LOG_USER_N("The requested flash write overlaps the OTP DATA protected region ");
			LOG_USER("0x%08" PRIx32 " - 0x%08" PRIx32 ", skipping that part", OTPC_BASE_ADDR, OTPC_DATA_LIMIT);
			LOG_USER("The following command will allow it: ");
			LOG_USER("set QUARK_D2000_OTPC_DATA_WRITE_ENABLED 1");
			return quark_d2000_flash_write(t, addr, size, count, buf + delta);
		}
	}

	/*
	 * Default write behavior, we execute here if
	 * - write to RAM
	 * - write to normal flash
	 * - write to the protected flash area and the enabling is on
	 */
	if ((addr <= OTPC_LIMIT) || ((addr >= FLASH_BASE_ADDR) && (addr <= FLASH_LIMT))
			|| ((addr >= OTPD_BASE_ADDR) && (addr <= OTPD_LIMT))) {
		/* FLASH&ROM WRITE */
		return quark_d2000_flash_write(t, addr, size, count, buf);
	} else {
		/* SRAM WRITE */
		return x86_32_common_write_memory(t, addr, size, count, buf);
	}
}

struct target_type quark_d2000_target = {
	.name = "quark_d2000",
	/* Quark X1000 SoC */
	.target_create = quark_d2000_target_create,
	.init_target = quark_d2000_init_target,
	/* lakemont probemode specific code */
	.poll = lakemont_poll,
	.arch_state = lakemont_arch_state,
	.halt = lakemont_halt,
	.resume = lakemont_resume,
	.step = lakemont_step,
	.assert_reset = quark_d2000_target_reset,
	.deassert_reset = lakemont_reset_deassert,
	/* common x86 code */
	.commands = x86_32_command_handlers,
	.get_gdb_reg_list = x86_32_get_gdb_reg_list,
	.read_memory = x86_32_common_read_memory,
	.write_memory = quark_d2000_write_memory,
	.add_breakpoint = x86_32_common_add_breakpoint,
	.remove_breakpoint = x86_32_common_remove_breakpoint,
	.add_watchpoint = x86_32_common_add_watchpoint,
	.remove_watchpoint = x86_32_common_remove_watchpoint,
	.virt2phys = x86_32_common_virt2phys,
	.read_phys_memory = x86_32_common_read_phys_mem,
	.write_phys_memory = x86_32_common_write_phys_mem,
	.mmu = x86_32_common_mmu,
};
