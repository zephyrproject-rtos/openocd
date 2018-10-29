/***************************************************************************
 *   Copyright (C) 2011 by Julius Baxter                                   *
 *   julius@opencores.org                                                  *
 *                                                                         *
 *   Copyright (C) 2013 by Marek Czerski                                   *
 *   ma.czerski@gmail.com                                                  *
 *                                                                         *
 *   Copyright (C) 2013 by Franck Jullien                                  *
 *   elec4fun@gmail.com                                                    *
 *                                                                         *
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

#ifndef OPENOCD_TARGET_RV32M1_H
#define OPENOCD_TARGET_RV32M1_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <target/target.h>

#ifndef offsetof
#define offsetof(st, m) __builtin_offsetof(st, m)
#endif

/*
 * Support two cores:
 * Debug unit 0 for core 0, debug unit 1 for core 1.
 */
#define CORE_NUM 2
#define DEBUG_UNIT0_BASE        (0xF9000000)
#define DEBUG_UNIT1_BASE        (0xF9008000)
#define EVENT_UNIT0_BASE        (0xE0041000)
#define EVENT_UNIT1_BASE        (0x4101F000)

#define DEBUG_GPR_OFFSET 0x400
#define DEBUG_FPR_OFFSET 0x500
#define DEBUG_REG_OFFSET 0x2000
#define DEBUG_CSR_OFFSET 0x4000

/* RV32M1 registers */
enum rv32m1_reg_nums {
    RV32M1_REG_R0 = 0,
    RV32M1_REG_R31 = 31,
    RV32M1_REG_NPC = 32,
    RV32M1_REG_PPC = 33,
    RV32M1_CORE_REG_NUM,
    RV32M1_REG_CSR0 = 65,
	RV32M1_REG_CSR4095 = 4160,
    RV32M1_ALL_REG_NUM
};

struct rv32m1_debug_unit {
    volatile uint32_t DBG_CTRL;      /* Offset 0x0000. */
    volatile uint32_t DBG_HIT;       /* Offset 0x0004. */
    volatile uint32_t DBG_IE;        /* Offset 0x0008. */
    volatile uint32_t DBG_CAUSE;     /* Offset 0x000c. */
    uint8_t reserved0[0x30];
    struct rv32m1_debug_bp {
        volatile uint32_t CTRL;
        volatile uint32_t DATA;
    } DBG_BP[8];                     /* Offset 0x0040. */
    uint8_t reserved1[0x380];
    volatile uint32_t GPR[32];       /* Offset 0x0400. */
    uint8_t reserved2[0x80];
    volatile uint32_t FPR_LSP[32];   /* Offset 0x0500. */
    volatile uint32_t FPR_MSP[32];   /* Offset 0x0580. */
    uint8_t reserved3[0x1A00];
    volatile uint32_t DBG_NPC;       /* Offset 0x2000. */
    volatile uint32_t DBG_PPC;       /* Offset 0x2004. */
    uint8_t reserved4[0x1FF8];
    volatile uint32_t CSR[4906];     /* Offset 0x4000. */
};

extern int coreIdx;
extern uint32_t rv32m1_debug_unit_reg_addr[];
extern uint32_t rv32m1_event_unit_reg_addr[];

struct rv32m1_jtag {
    struct jtag_tap *tap;
    int rv32m1_jtag_inited;
    int rv32m1_jtag_module_selected;
    uint8_t *current_reg_idx;
    struct rv32m1_tap_ip *tap_ip;
    struct rv32m1_du *du_core;
    struct target *target;
};

struct rv32m1_info {
    struct rv32m1_jtag jtag;
    uint32_t* reg_cache_mem;
    char * reg_name_mem;
};

static inline struct rv32m1_info *
target_to_rv32m1(struct target *target)
{
    return (struct rv32m1_info *)target->arch_info;
}

#define NO_SINGLE_STEP     0
#define SINGLE_STEP        1

/* RV32M1 Debug registers and bits needed for resuming */
#define RV32M1_DEBUG_CTRL_HALT   (1<<16) /* HALT. */
#define RV32M1_DEBUG_CTRL_SSTE   (1<<0)  /* SSTE, single step enable. */
#define RV32M1_DEBUG_IE_ECALL (1 << 11) // Environment call from M-Mode
#define RV32M1_DEBUG_IE_SAF   (1 << 7 ) // Store Access Fault (together with LAF)
#define RV32M1_DEBUG_IE_SAM   (1 << 6 ) // Store Address Misaligned (never traps)
#define RV32M1_DEBUG_IE_LAF   (1 << 5 ) // Load Access Fault (together with SAF)
#define RV32M1_DEBUG_IE_LAM   (1 << 4 ) // Load Address Misaligned (never traps)
#define RV32M1_DEBUG_IE_BP    (1 << 3 ) // EBREAK instruction causes trap
#define RV32M1_DEBUG_IE_ILL   (1 << 2 ) // Illegal Instruction
#define RV32M1_DEBUG_IE_IAF   (1 << 1 ) // Instruction Access Fault (not implemented)
#define RV32M1_DEBUG_IE_IAM   (1 << 0 ) // Instruction Address Misaligned (never traps)

#define RV32M1_DEBUG_REG_ADDR(coreIdx, reg) ((uint32_t)(rv32m1_debug_unit_reg_addr[coreIdx] + \
            offsetof(struct rv32m1_debug_unit, reg)))

#endif /* OPENOCD_TARGET_RV32M1_H */
