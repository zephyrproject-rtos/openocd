/***************************************************************************
 *   Copyright (C) 2011 by Mathias Kuester                                 *
 *   kesmtp@freenet.de                                                     *
 *                                                                         *
 *   Copyright (C) 2011 sleep(5) ltd                                       *
 *   tomas@sleepfive.com                                                   *
 *                                                                         *
 *   Copyright (C) 2012 by Christopher D. Kilgour                          *
 *   techie at whiterocker.com                                             *
 *                                                                         *
 *   Copyright (C) 2013 Nemui Trinomius                                    *
 *   nemuisan_kawausogasuki@live.jp                                        *
 *                                                                         *
 *   Copyright (C) 2015 Tomas Vanek                                        *
 *   vanekt@fbl.cz                                                         *
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
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "jtag/interface.h"
#include "imp.h"
#include <helper/binarybuffer.h>
#include <helper/time_support.h>
#include <target/target_type.h>
#include <target/algorithm.h>
#include <target/armv7m.h>
#include <target/cortex_m.h>

#define FLEXRAM		0x48000000

#define FTFx_FSTAT	0x40023000
#define FTFx_FCNFG	0x40023001
#define FTFx_FCCOB3	0x40023004
#define FTFx_FPROT3	0x40023010
#define FTFx_FDPROT	0x40023017
#define SIM_SDID	0x40048024
#define SIM_SOPT1	0x40047000
#define SIM_FCFG1	0x4004804c
#define SIM_FCFG2	0x40048050
#define WDOG_STCTRH	0x40052000
/* Some register address are different for core 0 and core 1. */
static uint32_t SMC_PMCTRL;
static uint32_t SMC_PMSTAT;

/* Values */
#define PM_STAT_RUN		0x01
#define PM_STAT_VLPR		0x04
#define PM_CTRL_RUNM_RUN	0x00

/* Commands */
#define FTFx_CMD_BLOCKSTAT  0x00
#define FTFx_CMD_SECTSTAT   0x01
#define FTFx_CMD_LWORDPROG  0x06
#define FTFx_CMD_SECTERASE  0x09
#define FTFx_CMD_SECTWRITE  0x0b
#define FTFx_CMD_MASSERASE  0x44
#define FTFx_CMD_PGMPART    0x80
#define FTFx_CMD_SETFLEXRAM 0x81

#define RV32M1_SDID    0x04501008

struct rv32m1_flash_bank {
	bool probed;
	uint32_t sector_size;
	uint32_t max_flash_prog_size;
	uint32_t protection_size;
	uint32_t prog_base;		/* base address for FTFx operations */
					/* same as bank->base for pflash, differs for FlexNVM */
	uint32_t protection_block;	/* number of first protection block in this bank */

	uint32_t sim_sdid;
    uint32_t mcm_cpcr2;
    uint8_t flash_index;

	enum {
		FC_AUTO = 0,
		FC_PFLASH,
		FC_FLEX_NVM,
		FC_FLEX_RAM,
	} flash_class;

	enum {
		FS_PROGRAM_SECTOR = 1,
		FS_PROGRAM_LONGWORD = 2,
		FS_PROGRAM_PHRASE = 4, /* Unsupported */
		FS_INVALIDATE_CACHE = 8,
	} flash_support;
};

struct flash_driver rv32m1_flash;
static int rv32m1_write_inner(struct flash_bank *bank, const uint8_t *buffer,
			uint32_t offset, uint32_t count);
static int rv32m1_auto_probe(struct flash_bank *bank);

FLASH_BANK_COMMAND_HANDLER(rv32m1_flash_bank_command)
{
	struct rv32m1_flash_bank *bank_info;

	if (CMD_ARGC < 6)
		return ERROR_COMMAND_SYNTAX_ERROR;

	LOG_INFO("add flash_bank rv32m1 %s", bank->name);

    /*
     * RV32M1 has core0 flash (1M, base 0) and core1 flash(256K, base 0x01000000)
     */
    if ((0 != bank->base) && (0x01000000 != bank->base))
    {
        LOG_WARNING("Invalid flash base " TARGET_ADDR_FMT ", use 0 instead", bank->base);
    }

	bank_info = malloc(sizeof(struct rv32m1_flash_bank));

	memset(bank_info, 0, sizeof(struct rv32m1_flash_bank));

    /* Core 0 flash. */
    if (0 == bank->base)
    {
        SMC_PMCTRL = 0x40020010;
        SMC_PMSTAT = 0x40020018;
        bank_info->mcm_cpcr2 = 0xE0080034;
        bank_info->flash_index = 0;
    }
    else /* Core 1 flash. */
    {
        SMC_PMCTRL = 0x41020010;
        SMC_PMSTAT = 0x41020018;
        bank_info->mcm_cpcr2 = 0xF0080034;
        bank_info->flash_index = 1;
    }

	bank->driver_priv = bank_info;

	return ERROR_OK;
}


COMMAND_HANDLER(rv32m1_disable_wdog_handler)
{
    LOG_ERROR("Disable watchdog not supported");
    return ERROR_FAIL;
}

static int rv32m1_ftfx_decode_error(uint8_t fstat)
{
	if (fstat & 0x20) {
		LOG_ERROR("Flash operation failed, illegal command");
		return ERROR_FLASH_OPER_UNSUPPORTED;

	} else if (fstat & 0x10)
		LOG_ERROR("Flash operation failed, protection violated");

	else if (fstat & 0x40)
		LOG_ERROR("Flash operation failed, read collision");

	else if (fstat & 0x80)
		return ERROR_OK;

	else
		LOG_ERROR("Flash operation timed out");

	return ERROR_FLASH_OPERATION_FAILED;
}


static int rv32m1_ftfx_prepare(struct target *target)
{
	int result, i;
	uint8_t fstat;

	/* wait until busy */
	for (i = 0; i < 50; i++) {
		result = target_read_u8(target, FTFx_FSTAT, &fstat);
		if (result != ERROR_OK)
			return result;

		if (fstat & 0x80)
			break;
	}

	if ((fstat & 0x80) == 0) {
		LOG_ERROR("Flash controller is busy");
		return ERROR_FLASH_OPERATION_FAILED;
	}
	if (fstat != 0x80) {
		/* reset error flags */
		result = target_write_u8(target, FTFx_FSTAT, 0x70);
	}
	return result;
}

static int rv32m1_protect(struct flash_bank *bank, int set, unsigned int first,
		unsigned int last)
{
	unsigned int i;

	if (!bank->prot_blocks || bank->num_prot_blocks == 0) {
		LOG_ERROR("No protection possible for current bank!");
		return ERROR_FLASH_BANK_INVALID;
	}

	for (i = first; i < bank->num_prot_blocks && i <= last; i++)
		bank->prot_blocks[i].is_protected = set;

	LOG_INFO("Protection bits will be written at the next FCF sector erase or write.");
	LOG_INFO("Do not issue 'flash info' command until protection is written,");
	LOG_INFO("doing so would re-read protection status from MCU.");

	return ERROR_OK;
}

static int rv32m1_protect_check(struct flash_bank *bank)
{
	struct rv32m1_flash_bank *kinfo = bank->driver_priv;
	int result, b;
	unsigned int i;
	uint32_t fprot;

	if (kinfo->flash_class == FC_PFLASH) {

        /* TODO */
		/* read protection register */
		result = target_read_u32(bank->target, FTFx_FPROT3, &fprot);
		if (result != ERROR_OK)
			return result;

		/* Every bit protects 1/32 of the full flash (not necessarily just this bank) */

	} else if (kinfo->flash_class == FC_FLEX_NVM) {
		uint8_t fdprot;

		/* read protection register */
		result = target_read_u8(bank->target, FTFx_FDPROT, &fdprot);
		if (result != ERROR_OK)
			return result;

		fprot = fdprot;

	} else {
		LOG_ERROR("Protection checks for FlexRAM not supported");
		return ERROR_FLASH_BANK_INVALID;
	}

	b = kinfo->protection_block;
	for (i = 0; i < bank->num_prot_blocks; i++) {
		if ((fprot >> b) & 1)
			bank->prot_blocks[i].is_protected = 0;
		else
			bank->prot_blocks[i].is_protected = 1;

		b++;
	}

	return ERROR_OK;
}

static int rv32m1_ftfx_command(struct target *target, uint8_t fcmd, uint32_t faddr,
				uint8_t fccob4, uint8_t fccob5, uint8_t fccob6, uint8_t fccob7,
				uint8_t fccob8, uint8_t fccob9, uint8_t fccoba, uint8_t fccobb,
				uint8_t *ftfx_fstat)
{

    /*
     * When required by the command, address bit 23 selects between main flash memory
     * (=0) and secondary flash memory (=1).
     */
    if (faddr > 0x00FFFFFF)
    {
        faddr |= 0x00800000;
    }

	uint8_t command[12] = {faddr & 0xff, (faddr >> 8) & 0xff, (faddr >> 16) & 0xff, fcmd,
			fccob7, fccob6, fccob5, fccob4,
			fccobb, fccoba, fccob9, fccob8};
	int result;
	uint8_t fstat;
	int64_t ms_timeout = timeval_ms() + 250;

	result = target_write_memory(target, FTFx_FCCOB3, 4, 3, command);
	if (result != ERROR_OK)
		return result;

	/* start command */
	result = target_write_u8(target, FTFx_FSTAT, 0x80);
	if (result != ERROR_OK)
		return result;

	/* wait for done */
	do {
		result = target_read_u8(target, FTFx_FSTAT, &fstat);

		if (result != ERROR_OK)
			return result;

		if (fstat & 0x80)
			break;

	} while (timeval_ms() < ms_timeout);

	if (ftfx_fstat)
		*ftfx_fstat = fstat;

	if ((fstat & 0xf0) != 0x80) {
		LOG_DEBUG("ftfx command failed FSTAT: %02X FCCOB: %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X",
			 fstat, command[3], command[2], command[1], command[0],
			 command[7], command[6], command[5], command[4],
			 command[11], command[10], command[9], command[8]);

		return rv32m1_ftfx_decode_error(fstat);
	}

	return ERROR_OK;
}

static int rv32m1_check_run_mode(struct target *target)
{
    /* TODO: Add run mode check. */
    return ERROR_OK;
}

static void rv32m1_invalidate_flash_cache(struct flash_bank *bank)
{
	struct rv32m1_flash_bank *kinfo = bank->driver_priv;
	uint32_t mcm_cpcr2 = 0x31;

	if (!(kinfo->flash_support & FS_INVALIDATE_CACHE))
		return;

	target_write_memory(bank->target, kinfo->mcm_cpcr2, 4, 1, (uint8_t*)&mcm_cpcr2);

	return;
}

static int rv32m1_erase(struct flash_bank *bank, unsigned int first,
		unsigned int last)
{
	int result;
	unsigned int i;
	struct rv32m1_flash_bank *kinfo = bank->driver_priv;

	result = rv32m1_check_run_mode(bank->target);
	if (result != ERROR_OK)
		return result;

	/* reset error flags */
	result = rv32m1_ftfx_prepare(bank->target);
	if (result != ERROR_OK)
		return result;

	if ((first > bank->num_sectors) || (last > bank->num_sectors))
		return ERROR_FLASH_OPERATION_FAILED;

	/*
	 * FIXME: TODO: use the 'Erase Flash Block' command if the
	 * requested erase is PFlash or NVM and encompasses the entire
	 * block.  Should be quicker.
	 */
	for (i = first; i <= last; i++) {
		/* set command and sector address */
		result = rv32m1_ftfx_command(bank->target, FTFx_CMD_SECTERASE, kinfo->prog_base + bank->sectors[i].offset,
				0, 0, 0, 0,  0, 0, 0, 0,  NULL);

		if (result != ERROR_OK) {
			LOG_WARNING("erase sector %d failed", i);
			return ERROR_FLASH_OPERATION_FAILED;
		}
	}

	rv32m1_invalidate_flash_cache(bank);

	return ERROR_OK;
}

static int rv32m1_make_ram_ready(struct target *target)
{
	int result;
	uint8_t ftfx_fcnfg;

	/* check if ram ready */
	result = target_read_u8(target, FTFx_FCNFG, &ftfx_fcnfg);
	if (result != ERROR_OK)
		return result;

	if (ftfx_fcnfg & (1 << 1))
		return ERROR_OK;	/* ram ready */

	/* make flex ram available */
	result = rv32m1_ftfx_command(target, FTFx_CMD_SETFLEXRAM, 0x00ff0000,
				 0, 0, 0, 0,  0, 0, 0, 0,  NULL);
	if (result != ERROR_OK)
		return ERROR_FLASH_OPERATION_FAILED;

	/* check again */
	result = target_read_u8(target, FTFx_FCNFG, &ftfx_fcnfg);
	if (result != ERROR_OK)
		return result;

	if (ftfx_fcnfg & (1 << 1))
		return ERROR_OK;	/* ram ready */

	return ERROR_FLASH_OPERATION_FAILED;
}

static int rv32m1_write_sections(struct flash_bank *bank, const uint8_t *buffer,
			 uint32_t offset, uint32_t count)
{
	int result = ERROR_OK;
	struct rv32m1_flash_bank *kinfo = bank->driver_priv;
	uint8_t *buffer_aligned = NULL;

	uint32_t prog_section_chunk_bytes = kinfo->sector_size >> 8;
	uint32_t prog_size_bytes = kinfo->max_flash_prog_size;

	while (count > 0) {
		uint32_t size = prog_size_bytes - offset % prog_size_bytes;
		uint32_t align_begin = offset % prog_section_chunk_bytes;
		uint32_t align_end;
		uint32_t size_aligned;
		uint16_t chunk_count;
		uint8_t ftfx_fstat;

		if (size > count)
			size = count;

		align_end = (align_begin + size) % prog_section_chunk_bytes;
		if (align_end)
			align_end = prog_section_chunk_bytes - align_end;

		size_aligned = align_begin + size + align_end;
		chunk_count = size_aligned / prog_section_chunk_bytes;

		if (size != size_aligned) {
			/* aligned section: the first, the last or the only */
			if (!buffer_aligned)
				buffer_aligned = malloc(prog_size_bytes);

			memset(buffer_aligned, 0xff, size_aligned);
			memcpy(buffer_aligned + align_begin, buffer, size);

			result = target_write_memory(bank->target, FLEXRAM,
						4, size_aligned / 4, buffer_aligned);

			LOG_DEBUG("section @ " TARGET_ADDR_FMT " aligned begin %" PRIu32 ", end %" PRIu32,
					bank->base + offset, align_begin, align_end);
		} else
			result = target_write_memory(bank->target, FLEXRAM,
						4, size_aligned / 4, buffer);

		LOG_DEBUG("write section @ " TARGET_ADDR_FMT " with length %" PRIu32 " bytes",
			  bank->base + offset, size);

		if (result != ERROR_OK) {
			LOG_ERROR("target_write_memory failed");
			break;
		}

		/* execute section-write command */
		result = rv32m1_ftfx_command(bank->target, FTFx_CMD_SECTWRITE,
				kinfo->prog_base + offset - align_begin,
				chunk_count>>8, chunk_count, 0, 0,
				0, 0, 0, 0,  &ftfx_fstat);

		if (result != ERROR_OK) {
			LOG_ERROR("Error writing section at " TARGET_ADDR_FMT, bank->base + offset);
			break;
		}

		if (ftfx_fstat & 0x01)
			LOG_ERROR("Flash write error at " TARGET_ADDR_FMT, bank->base + offset);

		buffer += size;
		offset += size;
		count -= size;
	}

	free(buffer_aligned);
	return result;
}


static int rv32m1_write_inner(struct flash_bank *bank, const uint8_t *buffer,
			 uint32_t offset, uint32_t count)
{
	int result;

    result = rv32m1_make_ram_ready(bank->target);
    if (result != ERROR_OK) {
        LOG_ERROR("FlexRAM not ready.");
        return result;
    }

	LOG_DEBUG("flash write @" TARGET_ADDR_FMT, bank->base + offset);

    result = rv32m1_write_sections(bank, buffer, offset, count);

	rv32m1_invalidate_flash_cache(bank);

    return result;
}

static int rv32m1_write(struct flash_bank *bank, const uint8_t *buffer,
			 uint32_t offset, uint32_t count)
{
	int result;

	result = rv32m1_check_run_mode(bank->target);
	if (result != ERROR_OK)
		return result;

    return rv32m1_write_inner(bank, buffer, offset, count);
}

static int rv32m1_probe(struct flash_bank *bank)
{
	int result;
	struct target *target = bank->target;
	struct rv32m1_flash_bank *kinfo = bank->driver_priv;

	kinfo->probed = false;

	result = target_read_u32(target, SIM_SDID, &kinfo->sim_sdid);
	if (result != ERROR_OK)
		return result;

    kinfo->sim_sdid = RV32M1_SDID;

    if (kinfo->sim_sdid != RV32M1_SDID)
    {
        LOG_ERROR("Unsupported device, only support RV32M1.");
        return ERROR_FAIL;
    }

    /* Flash 0. */
    if (0 == kinfo->flash_index)
    {
        kinfo->max_flash_prog_size = 1<<10;
        kinfo->flash_support = FS_PROGRAM_SECTOR | FS_PROGRAM_PHRASE | FS_INVALIDATE_CACHE;
        kinfo->flash_class = FC_PFLASH;
        bank->size = 1024 * 1024; /* 1M. */
        bank->base = 0x00000000;
        kinfo->prog_base = bank->base;
        kinfo->sector_size = 4 * 1024; /* 4k. */
        kinfo->protection_size = 16 * 1024; /* 16K. */
        bank->num_prot_blocks = 64;
        kinfo->protection_block = 0;
        bank->num_sectors = bank->size / kinfo->sector_size;
    }
    else /* Flash 1. */
    {
        kinfo->max_flash_prog_size = 1<<10;
        kinfo->flash_support = FS_PROGRAM_SECTOR | FS_PROGRAM_PHRASE | FS_INVALIDATE_CACHE;
        kinfo->flash_class = FC_PFLASH;
        bank->size = 256 * 1024; /* 256K. */
        bank->base = 0x01000000;
        kinfo->prog_base = bank->base;
        kinfo->sector_size = 2 * 1024; /* 2k. */
        kinfo->protection_size = 16 * 1024; /* 16K. */
        bank->num_prot_blocks = 16;
        kinfo->protection_block = 0;
        bank->num_sectors = bank->size / kinfo->sector_size;
    }

	if (bank->num_sectors > 0) {
		/* FlexNVM bank can be used for EEPROM backup therefore zero sized */
		bank->sectors = alloc_block_array(0, kinfo->sector_size, bank->num_sectors);
		if (!bank->sectors)
			return ERROR_FAIL;

		bank->prot_blocks = alloc_block_array(0, kinfo->protection_size, bank->num_prot_blocks);
		if (!bank->prot_blocks)
			return ERROR_FAIL;

	} else {
		bank->num_prot_blocks = 0;
	}

	kinfo->probed = true;

	return ERROR_OK;
}

static int rv32m1_auto_probe(struct flash_bank *bank)
{
	struct rv32m1_flash_bank *kinfo = bank->driver_priv;

	if (kinfo && kinfo->probed)
		return ERROR_OK;

	return rv32m1_probe(bank);
}

static int rv32m1_info(struct flash_bank *bank, char *buf, int buf_size)
{
	const char *bank_class_names[] = {
		"(ANY)", "PFlash", "FlexNVM", "FlexRAM"
	};

	struct rv32m1_flash_bank *kinfo = bank->driver_priv;

	(void) snprintf(buf, buf_size,
			"%s driver for %s flash bank %s at " TARGET_ADDR_FMT "",
			bank->driver->name, bank_class_names[kinfo->flash_class],
			bank->name, bank->base);

	return ERROR_OK;
}

static int rv32m1_blank_check(struct flash_bank *bank)
{
	struct rv32m1_flash_bank *kinfo = bank->driver_priv;
	int result;

	/* suprisingly blank check does not work in VLPR and HSRUN modes */
	result = rv32m1_check_run_mode(bank->target);
	if (result != ERROR_OK)
		return result;

	/* reset error flags */
	result = rv32m1_ftfx_prepare(bank->target);
	if (result != ERROR_OK)
		return result;

    bool block_dirty = false;
    uint8_t ftfx_fstat;

    if (!block_dirty) {
        /* check if whole bank is blank */
        result = rv32m1_ftfx_command(bank->target, FTFx_CMD_BLOCKSTAT, kinfo->prog_base,
                         0, 0, 0, 0,  0, 0, 0, 0, &ftfx_fstat);

        if (result != ERROR_OK || (ftfx_fstat & 0x01))
            block_dirty = true;
    }

    if (block_dirty) {
        /* the whole bank is not erased, check sector-by-sector */
        unsigned int i;
        for (i = 0; i < bank->num_sectors; i++) {
            /* normal margin */
            result = rv32m1_ftfx_command(bank->target, FTFx_CMD_SECTSTAT,
                    kinfo->prog_base + bank->sectors[i].offset,
                    1, 0, 0, 0,  0, 0, 0, 0, &ftfx_fstat);

            if (result == ERROR_OK) {
                bank->sectors[i].is_erased = !(ftfx_fstat & 0x01);
            } else {
                LOG_DEBUG("Ignoring errored PFlash sector blank-check");
                bank->sectors[i].is_erased = -1;
            }
        }
    } else {
        /* the whole bank is erased, update all sectors */
        unsigned int i;
        for (i = 0; i < bank->num_sectors; i++)
            bank->sectors[i].is_erased = 1;
    }

	return ERROR_OK;
}


static const struct command_registration rv32m1_exec_command_handlers[] = {
	{
		.name = "disable_wdog",
		.mode = COMMAND_EXEC,
		.help = "Disable the watchdog timer",
		.usage = "",
		.handler = rv32m1_disable_wdog_handler,
	},
	COMMAND_REGISTRATION_DONE
};

static const struct command_registration rv32m1_command_handler[] = {
	{
		.name = "rv32m1",
		.mode = COMMAND_ANY,
		.help = "RV32M1 flash controller commands",
		.usage = "",
		.chain = rv32m1_exec_command_handlers,
	},
	COMMAND_REGISTRATION_DONE
};

struct flash_driver rv32m1_flash = {
	.name = "rv32m1",
	.commands = rv32m1_command_handler,
	.flash_bank_command = rv32m1_flash_bank_command,
	.erase = rv32m1_erase,
	.protect = rv32m1_protect,
	.write = rv32m1_write,
	.read = default_flash_read,
	.probe = rv32m1_probe,
	.auto_probe = rv32m1_auto_probe,
	.erase_check = rv32m1_blank_check,
	.protect_check = rv32m1_protect_check,
	.info = rv32m1_info,
};
