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
 * Debugger for Intel Quark SE
 * This file implements any Quark SE SoC specific features such as resetbreak
 */

#ifndef QUARK_SE_H
#define QUARK_SE_H
#include <jtag/jtag.h>
#include <helper/types.h>

/* Quark SE FLASH ADDRESS */
#define FLASH0_STTS      ((uint32_t)0xB0100014)
#define FLASH0_CTL       ((uint32_t)0xB0100018)
#define FLASH0_WR_CTL    ((uint32_t)0xB010000C)
#define FLASH0_WR_DATA   ((uint32_t)0xB0100010)
#define FLASH0_BASE_ADDR ((uint32_t)0x40000000)
#define FLASH0_LIMT      ((uint32_t)0x4002FFFF)

#define FLASH1_STTS      ((uint32_t)0xB0200014)
#define FLASH1_CTL       ((uint32_t)0xB0200018)
#define FLASH1_WR_CTL    ((uint32_t)0xB020000C)
#define FLASH1_WR_DATA   ((uint32_t)0xB0200010)
#define FLASH1_BASE_ADDR ((uint32_t)0x40030000)
#define FLASH1_LIMT      ((uint32_t)0x4005FFFF)

#define ROM_WR_CTL     ((uint32_t)0xB0100004)
#define ROM_WR_DATA    ((uint32_t)0xB0100008)
#define ROM_BASE_ADDR  ((uint32_t)0xFFFFE000)
#define ROM_LIMIT      ((uint32_t)0xFFFFFFFF)

#define FLASH_CTL_WR_DIS_MASK  ((uint32_t)0x00000010) /* CTL */
#define ROM_CTL_WR_DIS_MASK    ((uint32_t)0x00000004) /* FLASH_STTS */

/* FLASH0, FLASH1 and ROM generic */
#define FC_WR_CTL_ADDR      2
#define FC_WR_CTR_DREQ    (1<<1)
#define FC_WR_CTL_WREQ    (1<<0)
#define FC_STTS_WR_DONE   (1<<1)
#define FC_STTS_ER_DONE   (1<<0)
#define FC_BYTE_PAGE_SIZE (4*512)

/* public interface */
int quark_se_flash_write(struct target *t, uint32_t addr,
			uint32_t size, uint32_t count, const uint8_t *buf);

#endif /* QUARK_SE_H */
