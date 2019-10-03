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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <jtag/jtag.h>
#include <target/register.h>
#include <target/target.h>
#include <target/breakpoints.h>
#include <target/target_type.h>
#include <helper/time_support.h>
#include <helper/fileio.h>
#include "rv32m1_tap.h"
#include "rv32m1.h"
#include "rv32m1_du.h"
#include "encoding.h"

#define MATCH_EBREAK 0x100073
#define MATCH_C_EBREAK 0x9002

#define MAX_REG_NAME_LEN 12

LIST_HEAD(rv32m1_tap_list);
LIST_HEAD(rv32m1_du_list);

struct rv32m1_csr_name_map
{
    uint16_t index;
    char * name;
};

static int rv32m1_remove_breakpoint(struct target *target,
				  struct breakpoint *breakpoint);

static int rv32m1_write_memory(struct target *target, target_addr_t address,
		uint32_t size, uint32_t count, const uint8_t *buffer);
static int rv32m1_read_memory(struct target *target, target_addr_t address,
		uint32_t size, uint32_t count, uint8_t *buffer);

int coreIdx = 0;

uint32_t rv32m1_debug_unit_reg_addr[CORE_NUM] =
{
    DEBUG_UNIT0_BASE,
    DEBUG_UNIT1_BASE,
};

uint32_t rv32m1_event_unit_reg_addr[CORE_NUM] =
{
    EVENT_UNIT0_BASE,
    EVENT_UNIT1_BASE,
};

/* Hard breakpoint count. */
uint32_t rv32m1_hard_bp_count[CORE_NUM] =
{
    4, /* RI5CY has 4 breakpoints. */
    2, /* ZERO-RISCY has 2 breakpoints. */
};

#define RV32M1_CORE_REG_NUM 34

static const char * core_reg_name[] =
{
    "zero",
    "ra",
    "sp",
    "gp",
    "tp",
    "t0",
    "t1",
    "t2",
    "s0",
    "s1",
    "a0",
    "a1",
    "a2",
    "a3",
    "a4",
    "a5",
    "a6",
    "a7",
    "s2",
    "s3",
    "s4",
    "s5",
    "s6",
    "s7",
    "s8",
    "s9",
    "s10",
    "s11",
    "t3",
    "t4",
    "t5",
    "t6",
    "npc",
    "ppc",
};

const struct rv32m1_csr_name_map rv32m1_csr_names[] =
{
    {0x0000, "ustatus"},
    {0x0005, "utvec"},
    {0x0014, "uhartid"},
    {0x0041, "uepc"},
    {0x0042, "ucause"},
    {0x0300, "mstatus"},
    {0x0305, "mtvec"},
    {0x0341, "mepc"},
    {0x0342, "mcause"},
    {0x0f14, "mhartid"},
    {0x0c10, "privlv"},
};

static int rv32m1_save_context(struct target *target)
{
	struct rv32m1_info *rv32m1 = target_to_rv32m1(target);
	struct rv32m1_du *du_core = rv32m1_to_du(rv32m1);
	uint32_t dbg_cause;
	uint32_t next_pc;
	struct breakpoint *breakpoint;
	int retval;

	LOG_DEBUG("-");

    /* Only handle GPR, NPC, PPC. */
    /* Read GPR. */
	for (int i = RV32M1_REG_R0; i <= RV32M1_REG_R31; i++) {
        retval = du_core->rv32m1_jtag_read_cpu(&rv32m1->jtag,
                RV32M1_DEBUG_REG_ADDR(coreIdx, GPR[i-RV32M1_REG_R0]), 1,
                target->reg_cache->reg_list[i].value);
        if (retval != ERROR_OK)
            return retval;

        target->reg_cache->reg_list[i].valid = true;
        target->reg_cache->reg_list[i].dirty = false;
    }

    /* Read NPC, PPC. */
	for (int i = RV32M1_REG_NPC; i <= RV32M1_REG_PPC; i++) {
        retval = du_core->rv32m1_jtag_read_cpu(&rv32m1->jtag,
                RV32M1_DEBUG_REG_ADDR(coreIdx, DBG_NPC) + (i-RV32M1_REG_NPC) * 4, 1,
                target->reg_cache->reg_list[i].value);
        if (retval != ERROR_OK)
            return retval;

        target->reg_cache->reg_list[i].valid = true;
        target->reg_cache->reg_list[i].dirty = false;
    }

	/* Check if halt because of software break point. */
	retval = du_core->rv32m1_jtag_read_cpu(&rv32m1->jtag,
			RV32M1_DEBUG_REG_ADDR(coreIdx, DBG_CAUSE), 1,
			&dbg_cause);

	if (retval != ERROR_OK)
		return retval;

	/*
	 * If the core is halt because of software break point. Then the next PC
	 * should be set to previous PC. Otherwise the original instruction is skipped.
	 */
	if (RV32M1_DEBUG_CAUSE_BP == (dbg_cause & RV32M1_DEBUG_CAUSE_MASK))
	{
		next_pc = buf_get_u32(target->reg_cache->reg_list[RV32M1_REG_PPC].value, 0, 32);
		breakpoint = breakpoint_find(target, next_pc);

		if (breakpoint && (BKPT_SOFT == breakpoint->type))
		{
			buf_set_u32(target->reg_cache->reg_list[RV32M1_REG_NPC].value, 0, 32, next_pc);
			target->reg_cache->reg_list[RV32M1_REG_NPC].dirty = true;
		}
	}

	return ERROR_OK;
}

static int rv32m1_restore_context(struct target *target)
{
	struct rv32m1_info *rv32m1 = target_to_rv32m1(target);
	struct rv32m1_du *du_core = rv32m1_to_du(rv32m1);
	int retval;

	LOG_DEBUG("-");

    /* Only handle GPR, NPC, PPC. */
    /* Write GPR. */
	for (int i = RV32M1_REG_R0; i <= RV32M1_REG_R31; i++) {
        if (target->reg_cache->reg_list[i].valid && target->reg_cache->reg_list[i].dirty)
        {
            LOG_DEBUG("%s: 0x%08x", target->reg_cache->reg_list[i].name, *(uint32_t*)(target->reg_cache->reg_list[i].value));
            retval = du_core->rv32m1_jtag_write_cpu(&rv32m1->jtag,
                    RV32M1_DEBUG_REG_ADDR(coreIdx, GPR[i-RV32M1_REG_R0]), 1,
                    target->reg_cache->reg_list[i].value);
            if (retval != ERROR_OK)
                return retval;

            target->reg_cache->reg_list[i].valid = false;
            target->reg_cache->reg_list[i].dirty = false;
        }
    }

    /* Write NPC, PPC. */
	for (int i = RV32M1_REG_NPC; i <= RV32M1_REG_PPC; i++) {
        if (target->reg_cache->reg_list[i].valid && target->reg_cache->reg_list[i].dirty)
        {
            retval = du_core->rv32m1_jtag_write_cpu(&rv32m1->jtag,
                    RV32M1_DEBUG_REG_ADDR(coreIdx, DBG_NPC) + (i-RV32M1_REG_NPC) * 4, 1,
                    target->reg_cache->reg_list[i].value);
            if (retval != ERROR_OK)
                return retval;

            target->reg_cache->reg_list[i].valid = false;
            target->reg_cache->reg_list[i].dirty = false;
        }
    }

	return ERROR_OK;
}

static int rv32m1_get_core_reg(struct reg *reg)
{
	struct target *target = (struct target *) reg->arch_info;
	struct rv32m1_info *rv32m1 = target_to_rv32m1(target);
	struct rv32m1_du *du_core = rv32m1_to_du(rv32m1);

	LOG_DEBUG("-");

	if (target->state != TARGET_HALTED)
		return ERROR_TARGET_NOT_HALTED;

    /* For GPR, PPC, and NPC, read from cache. For CSR, read from HW reg directly. */
	if (reg->number < RV32M1_CORE_REG_NUM) {
		buf_set_u32(reg->value, 0, 32, *((uint32_t*)target->reg_cache->reg_list[reg->number].value));
	} else if (reg->number < RV32M1_REG_CSR0)
    {
        /* No such registers. */
        return ERROR_FAIL;
    }
    else
    {
        /* Read CSR HW register. */
		int retval = du_core->rv32m1_jtag_read_cpu(&rv32m1->jtag,
                              RV32M1_DEBUG_REG_ADDR(coreIdx, CSR[reg->number-RV32M1_REG_CSR0]),
                              1, reg->value);
		if (retval != ERROR_OK) {
			LOG_ERROR("Error while reading CSR 0x%08" PRIx32, reg->number);
			return retval;
		}
	}

	return ERROR_OK;
}

static int rv32m1_set_core_reg(struct reg *reg, uint8_t *buf)
{
	struct target *target = (struct target *) reg->arch_info;
	struct rv32m1_info *rv32m1 = target_to_rv32m1(target);
	struct rv32m1_du *du_core = rv32m1_to_du(rv32m1);
	uint32_t value = buf_get_u32(buf, 0, 32);

	LOG_DEBUG("-");

	if (target->state != TARGET_HALTED)
		return ERROR_TARGET_NOT_HALTED;

    /* For GPR, PPC, and NPC, write to cache. For CSR, write to HW reg directly. */
	if (reg->number < RV32M1_CORE_REG_NUM) {
		buf_set_u32(target->reg_cache->reg_list[reg->number].value, 0, 32, value);
		reg->dirty = true;
		reg->valid = true;
	} else if (reg->number < RV32M1_REG_CSR0)
    {
        /* No such registers. */
        return ERROR_FAIL;
	} else {
        /* Write CSR to HW register. */
		int retval = du_core->rv32m1_jtag_write_cpu(&rv32m1->jtag,
                                  RV32M1_DEBUG_REG_ADDR(coreIdx, CSR[reg->number-RV32M1_REG_CSR0]),
                                  1, &value);
		if (retval != ERROR_OK) {
			LOG_ERROR("Error while writing CSR 0x%08" PRIx32, reg->number);
			return retval;
		}
	}

	return ERROR_OK;
}

static const struct reg_arch_type rv32m1_reg_type = {
	.get = rv32m1_get_core_reg,
	.set = rv32m1_set_core_reg,
};

static int rv32m1_build_reg_cache(struct target *target)
{
	struct reg_cache *cache = calloc(sizeof(struct reg_cache), 1);
	struct rv32m1_info *rv32m1 = target_to_rv32m1(target);

	LOG_DEBUG("-");

	/* Build the process context cache */
    if (0 == coreIdx)
    {
        cache->name = "RI5CY registers";
    }
    else
    {
        cache->name = "Zero RISCY registers";
    }

    target->reg_cache = cache;

	cache->reg_list = calloc(RV32M1_ALL_REG_NUM, sizeof(struct reg));
	cache->num_regs = RV32M1_ALL_REG_NUM;

    rv32m1->reg_name_mem = calloc(1, MAX_REG_NAME_LEN * RV32M1_ALL_REG_NUM);
    if (NULL == rv32m1->reg_name_mem)
    {
        return ERROR_FAIL;
    }

    rv32m1->reg_cache_mem = calloc(4, RV32M1_ALL_REG_NUM);
    if (NULL == rv32m1->reg_cache_mem)
    {
        free(rv32m1->reg_name_mem);
        rv32m1->reg_name_mem = NULL;;
        return ERROR_FAIL;
    }

    /*
     * Initialize the register cache and register list. The register list includs
     * all registers: GPR, PPC, NPC, and CSR, but only the GPR, PPC, and NPC
     * use this software cache mechanism, when accessing CSR, touch HW reg directly.
     */
	char *reg_name = rv32m1->reg_name_mem;
	uint32_t i = 0, j = 0;

	for (i = 0; i < RV32M1_ALL_REG_NUM; i++) {
		cache->reg_list[i].size = 32;
		cache->reg_list[i].value = &(rv32m1->reg_cache_mem[i]);
		cache->reg_list[i].dirty = false;
		cache->reg_list[i].valid = false;
		cache->reg_list[i].type = &rv32m1_reg_type;
		cache->reg_list[i].arch_info = target;
		cache->reg_list[i].number = i;
		cache->reg_list[i].exist = false;

        if (i<RV32M1_CORE_REG_NUM)
        {
			sprintf(reg_name, "%s", core_reg_name[i]);
            cache->reg_list[i].exist = true;
        }
        else if (i<RV32M1_REG_CSR0)
        {
            cache->reg_list[i].exist = false;
			sprintf(reg_name, "%s", "reserved");
        }
        else
        {
            /* Search reg name. */
            for (j=0; j<ARRAY_SIZE(rv32m1_csr_names); j++)
            {
                if (rv32m1_csr_names[j].index == (i-RV32M1_REG_CSR0))
                {
                    break;
                }
            }

            if (j >= ARRAY_SIZE(rv32m1_csr_names))
            {
                /* No register name defined. */
                sprintf(reg_name, "csr%d", i - RV32M1_REG_CSR0);
            }
            else
            {
                sprintf(reg_name, "%s", rv32m1_csr_names[j].name);
                cache->reg_list[i].exist = true;
            }
        }

		if (reg_name[0]) {
			cache->reg_list[i].name = reg_name;
		}
		reg_name += strlen(reg_name) + 1;
		assert(reg_name < rv32m1->reg_name_mem + RV32M1_ALL_REG_NUM * MAX_REG_NAME_LEN);
    }

	return ERROR_OK;;
}

static int rv32m1_debug_entry(struct target *target)
{
	LOG_DEBUG("-");

	int retval = rv32m1_save_context(target);
	if (retval != ERROR_OK) {
		LOG_ERROR("Error while calling rv32m1_save_context");
		return retval;
	}

	return retval;
}

static int rv32m1_halt(struct target *target)
{
	struct rv32m1_info *rv32m1 = target_to_rv32m1(target);
	struct rv32m1_du *du_core = rv32m1_to_du(rv32m1);

	LOG_DEBUG("target->state: %s",
		  target_state_name(target));

	if (target->state == TARGET_HALTED) {
		LOG_DEBUG("Target was already halted");
		return ERROR_OK;
	}

	if (target->state == TARGET_UNKNOWN)
		LOG_WARNING("Target was in unknown state when halt was requested");

	if (target->state == TARGET_RESET) {
		if ((jtag_get_reset_config() & RESET_SRST_PULLS_TRST) &&
		    jtag_get_srst()) {
			LOG_ERROR("Can't request a halt while in reset if nSRST pulls nTRST");
			return ERROR_TARGET_FAILURE;
		} else {
			target->debug_reason = DBG_REASON_DBGRQ;
			return ERROR_OK;
		}
	}

	int retval = du_core->rv32m1_cpu_stall(&rv32m1->jtag, CPU_STALL);
	if (retval != ERROR_OK) {
		LOG_ERROR("Impossible to stall the CPU");
		return retval;
	}

	target->debug_reason = DBG_REASON_DBGRQ;

	return ERROR_OK;
}

static int rv32m1_is_cpu_running(struct target *target, int *running)
{
	struct rv32m1_info *rv32m1 = target_to_rv32m1(target);
	struct rv32m1_du *du_core = rv32m1_to_du(rv32m1);
	int retval;
	int tries = 0;
	const int RETRIES_MAX = 5;

	/* Have a retry loop to determine of the CPU is running.
	   If target has been hard reset for any reason, it might take a couple
	   of goes before it's ready again.
	*/
	while (tries < RETRIES_MAX) {

		tries++;

		retval = du_core->rv32m1_is_cpu_running(&rv32m1->jtag, running);
		if (retval != ERROR_OK) {
			LOG_WARNING("Debug IF CPU control reg read failure.");
			/* Try once to restart the JTAG infrastructure -
			   quite possibly the board has just been reset. */
			LOG_WARNING("Resetting JTAG TAP state and reconnectiong to debug IF.");
			du_core->rv32m1_jtag_init(&rv32m1->jtag);

			LOG_WARNING("...attempt %d of %d", tries, RETRIES_MAX);

			alive_sleep(2);

			continue;
		} else
			return ERROR_OK;
	}

	LOG_ERROR("Could not re-establish communication with target");
	return retval;
}

static int rv32m1_poll(struct target *target)
{
	int retval;
	int running;

	retval = rv32m1_is_cpu_running(target, &running);
	if (retval != ERROR_OK) {
		LOG_ERROR("Error while calling rv32m1_is_cpu_running");
		return retval;
	}

	/* check for processor halted */
	if (!running) {
		/* It's actually stalled, so update our software's state */
		if ((target->state == TARGET_RUNNING) ||
		    (target->state == TARGET_RESET)) {

			target->state = TARGET_HALTED;

			retval = rv32m1_debug_entry(target);
			if (retval != ERROR_OK) {
				LOG_ERROR("Error while calling rv32m1_debug_entry");
				return retval;
			}

			target_call_event_callbacks(target,
						    TARGET_EVENT_HALTED);
		} else if (target->state == TARGET_DEBUG_RUNNING) {
			target->state = TARGET_HALTED;

			retval = rv32m1_debug_entry(target);
			if (retval != ERROR_OK) {
				LOG_ERROR("Error while calling rv32m1_debug_entry");
				return retval;
			}

			target_call_event_callbacks(target,
						    TARGET_EVENT_DEBUG_HALTED);
		}
	} else { /* ... target is running */

		target->state = TARGET_RUNNING;

	}

	return ERROR_OK;
}

static int rv32m1_assert_reset(struct target *target)
{
	struct rv32m1_info *rv32m1 = target_to_rv32m1(target);
	struct rv32m1_du *du_core = rv32m1_to_du(rv32m1);

	int retval;

	LOG_DEBUG("-");

    retval = rv32m1_halt(target);

    if (ERROR_OK != retval)
        return retval;

    target->state = TARGET_RESET;

	retval = du_core->rv32m1_cpu_reset(&rv32m1->jtag, CPU_RESET);

	if (retval != ERROR_OK) {
		LOG_ERROR("Error while asserting RESET");
		return retval;
	}

    return ERROR_OK;
}

static int rv32m1_deassert_reset(struct target *target)
{
    int retval;

	/* Registers are now invalid */
	register_cache_invalidate(target->reg_cache);

	if (!target->reset_halt) {

        uint32_t debug_ctrl = 0x0;

        retval = rv32m1_write_memory(target,
                         RV32M1_DEBUG_REG_ADDR(coreIdx, DBG_CTRL),
                         4,
                         1,
                         (const uint8_t*)&debug_ctrl);

        if (retval != ERROR_OK) {
            return retval;
        }

        target->state = TARGET_RUNNING;
    }

	return ERROR_OK;
}

static int rv32m1_soft_reset_halt(struct target *target)
{
	struct rv32m1_info *rv32m1 = target_to_rv32m1(target);
	struct rv32m1_du *du_core = rv32m1_to_du(rv32m1);

	LOG_DEBUG("-");

	int retval = du_core->rv32m1_cpu_stall(&rv32m1->jtag, CPU_STALL);
	if (retval != ERROR_OK) {
		LOG_ERROR("Error while stalling the CPU");
		return retval;
	}

	retval = rv32m1_assert_reset(target);
	if (retval != ERROR_OK)
		return retval;

	retval = rv32m1_deassert_reset(target);
	if (retval != ERROR_OK)
		return retval;

	return ERROR_OK;
}

static bool is_any_soft_breakpoint(struct target *target)
{
	struct breakpoint *breakpoint = target->breakpoints;

	LOG_DEBUG("-");

	while (breakpoint)
    {
		if (breakpoint->type == BKPT_SOFT)
			return true;

        breakpoint = breakpoint->next;
    }

	return false;
}

static int rv32m1_resume_or_step(struct target *target, int current,
			       uint32_t address, int handle_breakpoints,
			       int debug_execution, int step)
{
	struct rv32m1_info *rv32m1 = target_to_rv32m1(target);
	struct rv32m1_du *du_core = rv32m1_to_du(rv32m1);
	struct breakpoint *breakpoint = NULL;
	uint32_t resume_pc;

	LOG_DEBUG("Addr: 0x%" PRIx32 ", current: %s, stepping: %s, handle breakpoints %s\n",
		  address, current ? "yes":"no", step ? "yes" : "no", handle_breakpoints ? "yes" : "no");

	if (target->state != TARGET_HALTED) {
		LOG_ERROR("Target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}

	if (!debug_execution)
		target_free_all_working_areas(target);

	/* current ? continue on current pc : continue at <address> */
	if (!current)
    {
		buf_set_u32(target->reg_cache->reg_list[RV32M1_REG_NPC].value, 0,
			    32, address);
		target->reg_cache->reg_list[RV32M1_REG_NPC].dirty = true;
		target->reg_cache->reg_list[RV32M1_REG_NPC].valid = true;
    }

	int retval = rv32m1_restore_context(target);
	if (retval != ERROR_OK) {
		LOG_ERROR("Error while calling rv32m1_restore_context");
		return retval;
	}

	resume_pc = buf_get_u32(target->reg_cache->reg_list[RV32M1_REG_NPC].value,
				0, 32);

	/* The front-end may request us not to handle breakpoints */
	if (handle_breakpoints) {
		/* Single step past breakpoint at current address */
		breakpoint = breakpoint_find(target, resume_pc);
		if (breakpoint) {
			LOG_DEBUG("Unset breakpoint at 0x%08" TARGET_PRIxADDR, breakpoint->address);
			retval = rv32m1_remove_breakpoint(target, breakpoint);
			if (retval != ERROR_OK)
				return retval;
		}
	}

    uint32_t debug_reg;

	if (is_any_soft_breakpoint(target) == true)
    {
		retval = du_core->rv32m1_jtag_read_cpu(&rv32m1->jtag, RV32M1_DEBUG_REG_ADDR(coreIdx, DBG_IE), 1, &debug_reg);
		if (retval != ERROR_OK) {
			LOG_ERROR("Error while reading DEBUG IE");
			return retval;
		}

        debug_reg |= RV32M1_DEBUG_IE_BP;
        retval = du_core->rv32m1_jtag_write_cpu(&rv32m1->jtag, RV32M1_DEBUG_REG_ADDR(coreIdx, DBG_IE), 1, &debug_reg);
		if (retval != ERROR_OK) {
			LOG_ERROR("Error while writing DEBUG IE");
			return retval;
		}
    }

	if (step)
    {
		/* Set the single step trigger. */
        debug_reg = RV32M1_DEBUG_CTRL_SSTE;
    }
	else
    {
		/* Clear the single step trigger. */
        debug_reg = 0U;
    }

    retval = du_core->rv32m1_jtag_write_cpu(&rv32m1->jtag, RV32M1_DEBUG_REG_ADDR(coreIdx, DBG_CTRL), 1, &debug_reg);

    if (retval != ERROR_OK) {
        LOG_ERROR("Error while writing DEBUG CTRL");
        return retval;
    }

	if (step)
		target->debug_reason = DBG_REASON_SINGLESTEP;
	else
		target->debug_reason = DBG_REASON_NOTHALTED;

	/* Registers are now invalid */
	register_cache_invalidate(target->reg_cache);

	if (!debug_execution) {
		target->state = TARGET_RUNNING;
		target_call_event_callbacks(target, TARGET_EVENT_RESUMED);
		LOG_DEBUG("Target resumed at 0x%08" PRIx32, resume_pc);
	} else {
		target->state = TARGET_DEBUG_RUNNING;
		target_call_event_callbacks(target, TARGET_EVENT_DEBUG_RESUMED);
		LOG_DEBUG("Target debug resumed at 0x%08" PRIx32, resume_pc);
	}

    return ERROR_OK;
}

static int rv32m1_resume(struct target *target, int current,
		target_addr_t address, int handle_breakpoints, int debug_execution)
{
	return rv32m1_resume_or_step(target, current, address,
				   handle_breakpoints,
				   debug_execution,
				   NO_SINGLE_STEP);
}

static int rv32m1_step(struct target *target, int current,
		     target_addr_t address, int handle_breakpoints)
{
	return rv32m1_resume_or_step(target, current, address,
				   handle_breakpoints,
				   0,
				   SINGLE_STEP);
}

static int rv32m1_add_breakpoint(struct target *target,
			       struct breakpoint *breakpoint)
{
	struct rv32m1_info *rv32m1 = target_to_rv32m1(target);
	struct rv32m1_du *du_core = rv32m1_to_du(rv32m1);
	uint32_t data;
	int retval;

	LOG_DEBUG("Adding breakpoint: addr 0x%08" TARGET_PRIxADDR ", len %d, type %d, set: %d, id: %" PRId32,
		  breakpoint->address, breakpoint->length, breakpoint->type,
		  breakpoint->set, breakpoint->unique_id);

	if (breakpoint->set) {
		LOG_WARNING("breakpoint (BPID: %" PRIu32 ") already set", breakpoint->unique_id);
		return ERROR_OK;
	}

	/* Handle hard breakpoint. */
	if (breakpoint->type == BKPT_HARD)
    {
        for (uint8_t i=0; i<rv32m1_hard_bp_count[coreIdx]; i++)
        {
            retval = du_core->rv32m1_jtag_read_memory(&rv32m1->jtag,
                     RV32M1_DEBUG_REG_ADDR(coreIdx, DBG_BP[i].CTRL), 4, 1, (uint8_t*)(&data));

            if (retval != ERROR_OK) {
                LOG_ERROR("Error while checking breakpoint control register 0x%08" PRIx32,
                             RV32M1_DEBUG_REG_ADDR(coreIdx, DBG_BP[i].CTRL));
                return retval;
            }

            if ((data & 0x01) == 0x00) /* If available breakpoint. */
            {
                retval = du_core->rv32m1_jtag_write_memory(&rv32m1->jtag,
                        RV32M1_DEBUG_REG_ADDR(coreIdx, DBG_BP[i].DATA), 4, 1,
                        (uint8_t*)(&breakpoint->address));

                if (retval != ERROR_OK) {
                    LOG_ERROR("Error while set the breakpoint address.");
                    return retval;
                }

                data = 1;
                retval = du_core->rv32m1_jtag_write_memory(&rv32m1->jtag,
                        RV32M1_DEBUG_REG_ADDR(coreIdx, DBG_BP[i].CTRL), 4, 1, (uint8_t *)(&data));

                if (retval != ERROR_OK) {
                    LOG_ERROR("Error while set the breakpoint available.");
                    return retval;
                }

                breakpoint->set = true;

                return ERROR_OK;
            }
        }

        LOG_ERROR("Can not add more hardware breakpoint");
        return ERROR_TARGET_RESOURCE_NOT_AVAILABLE;
    }
    else /* Soft breakpoint. */
    {
        /* Enable the breakpoint in debug unit.*/
        retval = du_core->rv32m1_jtag_read_memory(&rv32m1->jtag,
                RV32M1_DEBUG_REG_ADDR(coreIdx, DBG_IE), 4, 1, (uint8_t*)(&data));
        if (retval != ERROR_OK) {
            LOG_ERROR("Error while reading register DBG_IE");
            return retval;
        }

        if (RV32M1_DEBUG_IE_BP != (data & RV32M1_DEBUG_IE_BP))
        {
            data |= RV32M1_DEBUG_IE_BP;

            retval = du_core->rv32m1_jtag_write_memory(&rv32m1->jtag,
                    RV32M1_DEBUG_REG_ADDR(coreIdx, DBG_IE), 4, 1, (uint8_t*)(&data));
            if (retval != ERROR_OK) {
                LOG_ERROR("Error while writing register DBG_IE");
                return retval;
            }
        }

        /* Read and save the instruction */
        retval = du_core->rv32m1_jtag_read_memory(&rv32m1->jtag, breakpoint->address,
                 breakpoint->length, 1, (uint8_t*)(&data));
        if (retval != ERROR_OK) {
            LOG_ERROR("Error while reading the instruction at 0x%08" TARGET_PRIxADDR,
                   breakpoint->address);
            return retval;
        }

        if (breakpoint->orig_instr != NULL)
            free(breakpoint->orig_instr);

        breakpoint->orig_instr = malloc(breakpoint->length);
        memcpy(breakpoint->orig_instr, &data, breakpoint->length);

        if (breakpoint->length == 4) {
            retval = target_write_u32(target, breakpoint->address, MATCH_EBREAK);
        } else {
            retval = target_write_u16(target, breakpoint->address, MATCH_C_EBREAK);
        }
        if (retval != ERROR_OK) {
            free(breakpoint->orig_instr);
            LOG_ERROR("Failed to write %d-byte breakpoint instruction at 0x%08" TARGET_PRIxADDR,
                    breakpoint->length, breakpoint->address);
            return ERROR_FAIL;
        }

        /* TODO:
         * Maintain I-Cache here.
         */
        breakpoint->set = true;

        return ERROR_OK;
    }
}

static int rv32m1_remove_breakpoint(struct target *target,
				  struct breakpoint *breakpoint)
{
	struct rv32m1_info *rv32m1 = target_to_rv32m1(target);
	struct rv32m1_du *du_core = rv32m1_to_du(rv32m1);
    uint32_t data;
    int retval;

	LOG_DEBUG("Removing breakpoint: addr 0x%08" TARGET_PRIxADDR ", len %d, type %d, set: %d, id: %" PRId32,
		  breakpoint->address, breakpoint->length, breakpoint->type,
		  breakpoint->set, breakpoint->unique_id);

	if (breakpoint->type == BKPT_HARD)
    {
        for (uint8_t i=0; i<rv32m1_hard_bp_count[coreIdx]; i++)
        {
            retval = du_core->rv32m1_jtag_read_memory(&rv32m1->jtag,
                             RV32M1_DEBUG_REG_ADDR(coreIdx, DBG_BP[i].DATA),
                             4,
                             1,
                             (uint8_t *)(&data));

            if (retval != ERROR_OK) {
                LOG_ERROR("Error while checking breakpoint data register 0x%08" PRIx32,
                             RV32M1_DEBUG_REG_ADDR(coreIdx, DBG_BP[i].DATA));
                return retval;
            }

            if (data == breakpoint->address)
            {
                /* Breakpoint found, disable it. */
                data = 0;
                retval = du_core->rv32m1_jtag_write_memory(&rv32m1->jtag,
                                 RV32M1_DEBUG_REG_ADDR(coreIdx, DBG_BP[i].CTRL),
                                 4,
                                 1,
                                 (uint8_t *)(&data));

                if (retval != ERROR_OK) {
                    LOG_ERROR("Error while disable the breakpoint.");
                    return retval;
                }

                return ERROR_OK;
            }
        }

        return ERROR_FAIL;
    }

	/* Replace the removed instruction */
	retval = du_core->rv32m1_jtag_write_memory(&rv32m1->jtag,
					  breakpoint->address,
					  breakpoint->length,
					  1,
					  breakpoint->orig_instr);

	if (retval != ERROR_OK) {
		LOG_ERROR("Error while writing back the instruction at 0x%08" TARGET_PRIxADDR,
			   breakpoint->address);
		return retval;
	}

    /* TODO:
     * Maintain I-Cache here.
     */

	return ERROR_OK;
}

static int rv32m1_add_watchpoint(struct target *target,
			       struct watchpoint *watchpoint)
{
	LOG_ERROR("%s: implement me", __func__);
	return ERROR_OK;
}

static int rv32m1_remove_watchpoint(struct target *target,
				  struct watchpoint *watchpoint)
{
	LOG_ERROR("%s: implement me", __func__);
	return ERROR_OK;
}

static int rv32m1_read_memory(struct target *target, target_addr_t address,
		uint32_t size, uint32_t count, uint8_t *buffer)
{
	struct rv32m1_info *rv32m1 = target_to_rv32m1(target);
	struct rv32m1_du *du_core = rv32m1_to_du(rv32m1);

	LOG_DEBUG("Read memory at 0x%08" TARGET_PRIxADDR ", size: %" PRIu32 ", count: 0x%08" PRIx32, address, size, count);

	/* Sanitize arguments */
	if (((size != 4) && (size != 2) && (size != 1)) || (count == 0) || !buffer) {
		LOG_ERROR("Bad arguments");
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

    /* Handle the unaligned access. */
    if (1 == (address & 0x01))
    {
        count = size * count;
        size = 1;
    }
    else if (2 == (address & 0x03))
    {
        count = size * count / 2;
        size = 2;
    }

	return du_core->rv32m1_jtag_read_memory(&rv32m1->jtag, address, size, count, buffer);
}

static int rv32m1_write_memory(struct target *target, target_addr_t address,
		uint32_t size, uint32_t count, const uint8_t *buffer)
{
	struct rv32m1_info *rv32m1 = target_to_rv32m1(target);
	struct rv32m1_du *du_core = rv32m1_to_du(rv32m1);

	LOG_DEBUG("Write memory at 0x%08" TARGET_PRIxADDR ", size: %" PRIu32 ", count: 0x%08" PRIx32, address, size, count);

	/* Sanitize arguments */
	if (((size != 4) && (size != 2) && (size != 1)) || (count == 0) || !buffer) {
		LOG_ERROR("Bad arguments");
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

    /* Handle the unaligned access. */
    if (1 == (address & 0x01))
    {
        count = size * count;
        size = 1;
    }
    else if (2 == (address & 0x03))
    {
        count = size * count / 2;
        size = 2;
    }

	return du_core->rv32m1_jtag_write_memory(&rv32m1->jtag, address, size, count, buffer);
}

static int rv32m1_init_target(struct command_context *cmd_ctx,
		struct target *target)
{
	struct rv32m1_info *rv32m1 = target_to_rv32m1(target);
	struct rv32m1_du *du_core = rv32m1_to_du(rv32m1);
	struct rv32m1_jtag *jtag = &rv32m1->jtag;

	if (du_core == NULL) {
		LOG_ERROR("No debug unit selected");
		return ERROR_FAIL;
	}

	if (jtag->tap_ip == NULL) {
		LOG_ERROR("No tap selected");
		return ERROR_FAIL;
	}

	rv32m1->jtag.tap = target->tap;
	rv32m1->jtag.rv32m1_jtag_inited = 0;
	rv32m1->jtag.rv32m1_jtag_module_selected = -1;
	rv32m1->jtag.target = target;

	return rv32m1_build_reg_cache(target);
}

static void rv32m1_deinit_target(struct target *target)
{
	struct rv32m1_info *rv32m1 = target_to_rv32m1(target);

    free(rv32m1->reg_cache_mem);

    free(rv32m1->reg_name_mem);

    free(rv32m1);
}

static int rv32m1_target_create(struct target *target, Jim_Interp *interp)
{
	if (target->tap == NULL)
		return ERROR_FAIL;

	struct rv32m1_info *rv32m1 = calloc(1, sizeof(struct rv32m1_info));

	target->arch_info = rv32m1;

	rv32m1_tap_mohor_register();

	rv32m1_du_adv_register();

	return ERROR_OK;
}

static int rv32m1_examine(struct target *target)
{
	struct rv32m1_info *rv32m1 = target_to_rv32m1(target);
	struct rv32m1_du *du_core = rv32m1_to_du(rv32m1);

	if (!target_was_examined(target)) {

		target_set_examined(target);

		int running;
		uint32_t data = 0;

		int retval = du_core->rv32m1_is_cpu_running(&rv32m1->jtag, &running);
		if (retval != ERROR_OK) {
			LOG_ERROR("Couldn't read the CPU state");
			return retval;
		} else {
			if (running)
				target->state = TARGET_RUNNING;
			else {
				LOG_DEBUG("Target is halted");

				/* This is the first time we examine the target,
				 * it is stalled and we don't know why. Let's
				 * assume this is because of a debug reason.
				 */
				if (target->state == TARGET_UNKNOWN)
					target->debug_reason = DBG_REASON_DBGRQ;

				target->state = TARGET_HALTED;
			}
		}

		/* Clear all hardware breakpoints. */
		for (uint8_t i=0; i<rv32m1_hard_bp_count[coreIdx]; i++)
		{
			retval = du_core->rv32m1_jtag_write_memory(&rv32m1->jtag,
				RV32M1_DEBUG_REG_ADDR(coreIdx, DBG_BP[i].CTRL), 4, 1, (uint8_t *)(&data));

			if (retval != ERROR_OK) {
				LOG_ERROR("Error while clear the breakpoints.");
				return retval;
			}
		}
	}

	return ERROR_OK;
}

static int rv32m1_arch_state(struct target *target)
{
	return ERROR_OK;
}

static int rv32m1_get_gdb_reg_list(struct target *target, struct reg **reg_list[],
			  int *reg_list_size, enum target_register_class reg_class)
{
	if (reg_class == REG_CLASS_GENERAL) {
		/* We will have this called whenever GDB connects. */
		*reg_list_size = RV32M1_CORE_REG_NUM;
	} else {
		*reg_list_size = RV32M1_ALL_REG_NUM;
	}

    /* this is free()'d back in gdb_server.c's gdb_get_register_packet() */
    *reg_list = malloc((*reg_list_size) * sizeof(struct reg *));

	if (*reg_list == NULL)
		return ERROR_FAIL;

    for (int i = 0; i < *reg_list_size; i++)
        (*reg_list)[i] = &target->reg_cache->reg_list[i];

    return ERROR_OK;
}

int rv32m1_get_gdb_fileio_info(struct target *target, struct gdb_fileio_info *fileio_info)
{
	return ERROR_FAIL;
}

static int rv32m1_checksum_memory(struct target *target, target_addr_t address,
		uint32_t count, uint32_t *checksum) {

	return ERROR_FAIL;
}

static int rv32m1_profiling(struct target *target, uint32_t *samples,
		uint32_t max_num_samples, uint32_t *num_samples, uint32_t seconds)
{
	struct timeval timeout, now;
	struct rv32m1_info *rv32m1 = target_to_rv32m1(target);
	struct rv32m1_du *du_core = rv32m1_to_du(rv32m1);
	int retval = ERROR_OK;

	gettimeofday(&timeout, NULL);
	timeval_add_time(&timeout, seconds, 0);

	LOG_INFO("Starting rv32m1 profiling. Sampling npc as fast as we can...");

	/* Make sure the target is running */
	target_poll(target);
	if (target->state == TARGET_HALTED)
		retval = target_resume(target, 1, 0, 0, 0);

	if (retval != ERROR_OK) {
		LOG_ERROR("Error while resuming target");
		return retval;
	}

	uint32_t sample_count = 0;

	for (;;) {
		uint32_t reg_value;
		retval = du_core->rv32m1_jtag_read_cpu(&rv32m1->jtag, RV32M1_DEBUG_REG_ADDR(coreIdx, DBG_NPC) /* NPC */, 1, &reg_value);
		if (retval != ERROR_OK) {
			LOG_ERROR("Error while reading NPC");
			return retval;
		}

		samples[sample_count++] = reg_value;

		gettimeofday(&now, NULL);
		if ((sample_count >= max_num_samples) ||
			((now.tv_sec >= timeout.tv_sec) && (now.tv_usec >= timeout.tv_usec))) {
			LOG_INFO("Profiling completed. %" PRIu32 " samples.", sample_count);
			break;
		}
	}

	*num_samples = sample_count;
	return retval;
}

COMMAND_HANDLER(rv32m1_tap_select_command_handler)
{
	struct target *target = get_current_target(CMD_CTX);
	struct rv32m1_info *rv32m1 = target_to_rv32m1(target);
	struct rv32m1_jtag *jtag = &rv32m1->jtag;
	struct rv32m1_tap_ip *rv32m1_tap;

	if (CMD_ARGC != 1)
		return ERROR_COMMAND_SYNTAX_ERROR;

	list_for_each_entry(rv32m1_tap, &rv32m1_tap_list, list) {
		if (rv32m1_tap->name) {
			if (!strcmp(CMD_ARGV[0], rv32m1_tap->name)) {
				jtag->tap_ip = rv32m1_tap;
				LOG_INFO("%s tap selected", rv32m1_tap->name);
				return ERROR_OK;
			}
		}
	}

	LOG_ERROR("%s unknown, no tap selected", CMD_ARGV[0]);
	return ERROR_COMMAND_SYNTAX_ERROR;
}

COMMAND_HANDLER(rv32m1_core_select_command_handler)
{
	if (CMD_ARGC != 1)
		return ERROR_COMMAND_SYNTAX_ERROR;

    coreIdx = atoi(CMD_ARGV[0]);

    /* Use core 0 by default. */
    if (coreIdx > CORE_NUM)
    {
        LOG_WARNING("Invalid core index, use index 0");
        coreIdx = 0;
    }

    LOG_INFO("core %d selected", coreIdx);

    return ERROR_OK;
}

COMMAND_HANDLER(rv32m1_tap_list_command_handler)
{
	struct rv32m1_tap_ip *rv32m1_tap;

	if (CMD_ARGC != 0)
		return ERROR_COMMAND_SYNTAX_ERROR;

	list_for_each_entry(rv32m1_tap, &rv32m1_tap_list, list) {
		if (rv32m1_tap->name)
			command_print(CMD, "%s", rv32m1_tap->name);
	}

	return ERROR_OK;
}

COMMAND_HANDLER(rv32m1_du_select_command_handler)
{
	struct target *target = get_current_target(CMD_CTX);
	struct rv32m1_info *rv32m1 = target_to_rv32m1(target);
	struct rv32m1_jtag *jtag = &rv32m1->jtag;
	struct rv32m1_du *rv32m1_du;

	if (CMD_ARGC > 2)
		return ERROR_COMMAND_SYNTAX_ERROR;

	list_for_each_entry(rv32m1_du, &rv32m1_du_list, list) {
		if (rv32m1_du->name) {
			if (!strcmp(CMD_ARGV[0], rv32m1_du->name)) {
				jtag->du_core = rv32m1_du;
				LOG_INFO("%s debug unit selected", rv32m1_du->name);

				if (CMD_ARGC == 2) {
					int options;
					COMMAND_PARSE_NUMBER(int, CMD_ARGV[1], options);
					rv32m1_du->options = options;
					LOG_INFO("Option %x is passed to %s debug unit"
						 , options, rv32m1_du->name);
				}

				return ERROR_OK;
			}
		}
	}

	LOG_ERROR("%s unknown, no debug unit selected", CMD_ARGV[0]);
	return ERROR_COMMAND_SYNTAX_ERROR;
}

COMMAND_HANDLER(rv32m1_du_list_command_handler)
{
	struct rv32m1_du *rv32m1_du;

	if (CMD_ARGC != 0)
		return ERROR_COMMAND_SYNTAX_ERROR;

	list_for_each_entry(rv32m1_du, &rv32m1_du_list, list) {
		if (rv32m1_du->name)
			command_print(CMD, "%s", rv32m1_du->name);
	}

	return ERROR_OK;
}

static const struct command_registration rv32m1_hw_ip_command_handlers[] = {
	{
		"tap_select",
		.handler = rv32m1_tap_select_command_handler,
		.mode = COMMAND_ANY,
		.usage = "tap_select name",
		.help = "Select the TAP core to use",
	},
	{
		"tap_list",
		.handler = rv32m1_tap_list_command_handler,
		.mode = COMMAND_ANY,
		.usage = "tap_list",
		.help = "Display available TAP core",
	},
	{
		"du_select",
		.handler = rv32m1_du_select_command_handler,
		.mode = COMMAND_ANY,
		.usage = "du_select name",
		.help = "Select the Debug Unit core to use",
	},
	{
		"du_list",
		.handler = rv32m1_du_list_command_handler,
		.mode = COMMAND_ANY,
		.usage = "select_tap name",
		.help = "Display available Debug Unit core",
	},
	{
		"core_select",
		.handler = rv32m1_core_select_command_handler,
		.mode = COMMAND_ANY,
		.usage = "core_select index",
		.help = "Select the the core, 0 or 1",
	},
	COMMAND_REGISTRATION_DONE
};

static const struct command_registration rv32m1_command_handlers[] = {
	{
		.chain = rv32m1_hw_ip_command_handlers,
	},
	COMMAND_REGISTRATION_DONE
};


struct target_type rv32m1_target = {
	.name = "rv32m1",

	.poll = rv32m1_poll,
	.arch_state = rv32m1_arch_state,

	.target_request_data = NULL,

	.halt = rv32m1_halt,
	.resume = rv32m1_resume,
	.step = rv32m1_step,

	.assert_reset = rv32m1_assert_reset,
	.deassert_reset = rv32m1_deassert_reset,
	.soft_reset_halt = rv32m1_soft_reset_halt,

	.get_gdb_reg_list = rv32m1_get_gdb_reg_list,

	.read_memory = rv32m1_read_memory,
	.write_memory = rv32m1_write_memory,
	.checksum_memory = rv32m1_checksum_memory,

	.commands = rv32m1_command_handlers,
	.add_breakpoint = rv32m1_add_breakpoint,
	.remove_breakpoint = rv32m1_remove_breakpoint,
	.add_watchpoint = rv32m1_add_watchpoint,
	.remove_watchpoint = rv32m1_remove_watchpoint,

	.target_create = rv32m1_target_create,
	.init_target = rv32m1_init_target,
	.deinit_target = rv32m1_deinit_target,
	.examine = rv32m1_examine,

	.get_gdb_fileio_info = rv32m1_get_gdb_fileio_info,

	.profiling = rv32m1_profiling,
};
