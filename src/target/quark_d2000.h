/*
 * Copyright(c) 2015 Intel Corporation.
 *
 * Ivan De Cesaris (ivan.de.cesaris@intel.com)
 * Jessica Gomez (jessica.gomez.hernandez@intel.com)
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
 * This file implements any Quark SoC specific features such as resetbreak (TODO)
 */

#ifndef QUARK_D2000_H
#define QUARK_D2000_H
#include <jtag/jtag.h>
#include <helper/types.h>

/* QUARK D2000 FLASH ADDRESS */

/* check protection bits */
#define FLASH_STTS             ((uint32_t)0xB0100014)
#define FLASH_CTL              ((uint32_t)0xB0100018)
#define FLASH_CTL_WR_DIS_MASK  ((uint32_t)0x00000010) /* CTL */
#define ROM_CTL_WR_DIS_MASK    ((uint32_t)0x00000004) /* FLASH_STTS */

/* control flash(32kB) and OTPD(4kB) */
#define FLASH_WR_CTL           ((uint32_t)0xB010000C)
#define FLASH_WR_DATA          ((uint32_t)0xB0100010)

#define FLASH_BASE_ADDR        ((uint32_t)0x00180000)
#define FLASH_LIMT             ((uint32_t)0x00187FFF)
#define FLASH_OFFSET           ((uint32_t)0x1000)

#define OTPD_BASE_ADDR         ((uint32_t)0x00200000)
#define OTPD_LIMT              ((uint32_t)0x00200FFF)
#define OTPD_OFFSET            ((uint32_t)0x0)

/* control rom OTPC(8kB) */
#define ROM_WR_CTL             ((uint32_t)0xB0100004)
#define ROM_WR_DATA            ((uint32_t)0xB0100008)

#define OTPC_BASE_ADDR         ((uint32_t)0x00000000)
#define OTPC_LIMIT             ((uint32_t)0x00001FFF)
#define OTPC_OFFSET            ((uint32_t)0x0)

/* data area in OTPC(0x0-0x14F) */
#define OTPC_DATA_LIMIT        ((uint32_t)0x0000014F)

/* flash and rom generic */
#define FC_WR_CTL_ADDR      2
#define FC_WR_CTR_DREQ    (1<<1)
#define FC_WR_CTL_WREQ    (1<<0)
#define FC_STTS_WR_DONE   (1<<1)
#define FC_STTS_ER_DONE   (1<<0)
#define FC_BYTE_PAGE_SIZE (4*512)

/* public interface */
int quark_d2000_flash_write(struct target *t, uint32_t addr,
			uint32_t size, uint32_t count, const uint8_t *buf);

#endif /* QUARK_D2000_H */
