/***************************************************************************
 *   Copyright (C) 2013-2015 Synopsys, Inc.                                *
 *   Frank Dols <frank.dols@synopsys.com>                                  *
 *   Mischa Jonker <mischa.jonker@synopsys.com>                            *
 *   Anton Kolesov <anton.kolesov@synopsys.com>                            *
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
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "arc32.h"

static int arc_regs_get_core_reg(struct reg *reg)
{
	assert(reg != NULL);

	struct arc_reg_t *arc_reg = reg->arch_info;
	struct target *target = arc_reg->target;
	struct arc32_common *arc32 = target_to_arc32(target);

	if (reg->valid) {
		LOG_DEBUG("Get register (cached) gdb_num=%" PRIu32 ", name=%s, value=0x%" PRIx32,
				reg->number, arc_reg->desc->name, arc_reg->value);
		return ERROR_OK;
	}

	if (arc_reg->desc->is_core) {
		if (arc_reg->desc->arch_num == 61 || arc_reg->desc->arch_num == 62) {
			LOG_ERROR("It is forbidden to read core registers 61 and 62.");
			return ERROR_FAIL;
		}
		arc_jtag_read_core_reg_one(&arc32->jtag_info, arc_reg->desc->arch_num,
			&arc_reg->value);
	} else {
		arc_jtag_read_aux_reg_one(&arc32->jtag_info, arc_reg->desc->arch_num,
			&arc_reg->value);
	}

	buf_set_u32(reg->value, 0, 32, arc_reg->value);

	/* In general it is preferable that target is halted, so its state doesn't
	 * change in ways unknown to OpenOCD, and there used to be a check in this
	 * function - it would work only if target is halted.  However there is a
	 * twist - arc32_configure is called from arc_ocd_examine_target.
	 * arc32_configure will read registers via this function, but target may be
	 * still run at this point - if it was running when OpenOCD connected to it.
	 * ARC initialization scripts would do a "force halt" of target, but that
	 * happens only after target is examined, so this function wouldn't work if
	 * it would require target to be halted.  It is possible to do a force halt
	 * of target from arc_ocd_examine_target, but then if we look at this
	 * problem longterm - this is not a solution, as it would prevent non-stop
	 * debugging.  Preferable way seems to allow register reading from nonhalted
	 * target, but those reads should be uncached.  Therefore "valid" bit is set
	 * only when target is halted.
	 *
	 * The same is not done for register setter - for now it will continue to
	 * support only halted targets, untill there will be a real need for async
	 * writes there as well.
	 */
	if (target->state == TARGET_HALTED) {
		reg->valid = true;
	} else {
		reg->valid = false;
	}
	reg->dirty = false;

	LOG_DEBUG("Get register gdb_num=%" PRIu32 ", name=%s, value=0x%" PRIx32,
			reg->number , arc_reg->desc->name, arc_reg->value);

	return ERROR_OK;
}

static int arc_regs_set_core_reg(struct reg *reg, uint8_t *buf)
{
	LOG_DEBUG("-");
	struct arc_reg_t *arc_reg = reg->arch_info;
	struct target *target = arc_reg->target;
	uint32_t value = buf_get_u32(buf, 0, 32);

	if (target->state != TARGET_HALTED)
		return ERROR_TARGET_NOT_HALTED;
#if 0
	if (arc_reg->desc->readonly) {
		LOG_ERROR("Cannot set value to a read-only register %s.", arc_reg->desc->name);
		return ERROR_FAIL;
	}
#endif

	if (arc_reg->desc->is_core && (arc_reg->desc->arch_num == 61 ||
			arc_reg->desc->arch_num == 62)) {
		LOG_ERROR("It is forbidden to write core registers 61 and 62.");
		return ERROR_FAIL;
	}

	buf_set_u32(reg->value, 0, 32, value);

	arc_reg->value = value;

	LOG_DEBUG("Set register gdb_num=%" PRIu32 ", name=%s, value=0x%08" PRIx32,
			reg->number, arc_reg->desc->name, value);
	reg->valid = true;
	reg->dirty = true;

	return ERROR_OK;
}

const struct reg_arch_type arc32_reg_type = {
	.get = arc_regs_get_core_reg,
	.set = arc_regs_set_core_reg,
};

int arc_regs_get_gdb_reg_list(struct target *target, struct reg **reg_list[],
	int *reg_list_size, enum target_register_class reg_class)
{
	assert(target->reg_cache);
	struct arc32_common *arc32 = target_to_arc32(target);

	/* get pointers to arch-specific information storage */
	*reg_list_size = arc32->num_regs;
	*reg_list = calloc(*reg_list_size, sizeof(struct reg *));

	/* OpenOCD gdb_server API seems to be inconsistent here: when it generates
	 * XML tdesc it filters out !exist registers, however when creating a
	 * g-packet it doesn't do so. REG_CLASS_ALL is used in first case, and
	 * REG_CLASS_GENERAL used in the latter one. Due to this we had to filter
	 * out !exist register for "general", but not for "all". Attempts to filter out
	 * !exist for "all" as well will cause a failed check in OpenOCD GDB
	 * server. */
	if (reg_class == REG_CLASS_ALL) {
		unsigned long i = 0;
		struct reg_cache *reg_cache = target->reg_cache;
		while (reg_cache != NULL) {
			for (unsigned j = 0; j < reg_cache->num_regs; j++, i++) {
				(*reg_list)[i] =  &reg_cache->reg_list[j];
			}
			reg_cache = reg_cache->next;
		}
		assert(i == arc32->num_regs);
		LOG_DEBUG("REG_CLASS_ALL: number of regs=%i", *reg_list_size);
	} else {
		unsigned long i = 0;
		unsigned long gdb_reg_number = 0;
		struct reg_cache *reg_cache = target->reg_cache;
		while (reg_cache != NULL) {
			for (unsigned j = 0;
				 j < reg_cache->num_regs && gdb_reg_number <= arc32->last_general_reg;
				 j++) {
				if (reg_cache->reg_list[j].exist) {
					(*reg_list)[i] =  &reg_cache->reg_list[j];
					i++;
				}
				gdb_reg_number += 1;
			}
			reg_cache = reg_cache->next;
		}
		*reg_list_size = i;
		LOG_DEBUG("REG_CLASS_GENERAL: number of regs=%i", *reg_list_size);
	}

	return ERROR_OK;
}

