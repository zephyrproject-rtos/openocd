/***************************************************************************
 *   Copyright (C) 2013-2014 Synopsys, Inc.                                *
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

/* TODO: Register definitions are a bit inconsistent in that some properties
 * are set statically in a description table, while some properties are set at
 * runtime with `if` and `switch`. Would be good to rething this when time will
 * allow. */

#if 0
/* XML feature names */
const char * const general_group_name = "general";
static const char * const float_group_name = "float";
static const char * const feature_core_v2_name = "org.gnu.gdb.arc.core.v2";
static const char * const feature_aux_minimal_name = "org.gnu.gdb.arc.aux-minimal";
static const char * const feature_aux_other_name = "org.gnu.gdb.arc.aux-other";

/* Describe all possible registers. */
static const struct arc32_reg_desc arc32_regs_descriptions[ARC_TOTAL_NUM_REGS] = {
	/* regnum, name, address, gdb_type, readonly */
	{ ARC_REG_R0,       "r0",        0, REG_TYPE_UINT32,   false },
	{ ARC_REG_R1,       "r1",        1, REG_TYPE_UINT32,   false },
	{ ARC_REG_R2,       "r2",        2, REG_TYPE_UINT32,   false },
	{ ARC_REG_R3,       "r3",        3, REG_TYPE_UINT32,   false },
	{ ARC_REG_R4,       "r4",        4, REG_TYPE_UINT32,   false },
	{ ARC_REG_R5,       "r5",        5, REG_TYPE_UINT32,   false },
	{ ARC_REG_R6,       "r6",        6, REG_TYPE_UINT32,   false },
	{ ARC_REG_R7,       "r7",        7, REG_TYPE_UINT32,   false },
	{ ARC_REG_R8,       "r8",        8, REG_TYPE_UINT32,   false },
	{ ARC_REG_R9,       "r9",        9, REG_TYPE_UINT32,   false },
	{ ARC_REG_R10,      "r10",      10, REG_TYPE_UINT32,   false },
	{ ARC_REG_R11,      "r11",      11, REG_TYPE_UINT32,   false },
	{ ARC_REG_R12,      "r12",      12, REG_TYPE_UINT32,   false },
	{ ARC_REG_R13,      "r13",      13, REG_TYPE_UINT32,   false },
	{ ARC_REG_R14,      "r14",      14, REG_TYPE_UINT32,   false },
	{ ARC_REG_R15,      "r15",      15, REG_TYPE_UINT32,   false },
	{ ARC_REG_R16,      "r16",      16, REG_TYPE_UINT32,   false },
	{ ARC_REG_R17,      "r17",      17, REG_TYPE_UINT32,   false },
	{ ARC_REG_R18,      "r18",      18, REG_TYPE_UINT32,   false },
	{ ARC_REG_R19,      "r19",      19, REG_TYPE_UINT32,   false },
	{ ARC_REG_R20,      "r20",      20, REG_TYPE_UINT32,   false },
	{ ARC_REG_R21,      "r21",      21, REG_TYPE_UINT32,   false },
	{ ARC_REG_R22,      "r22",      22, REG_TYPE_UINT32,   false },
	{ ARC_REG_R23,      "r23",      23, REG_TYPE_UINT32,   false },
	{ ARC_REG_R24,      "r24",      24, REG_TYPE_UINT32,   false },
	{ ARC_REG_R25,      "r25",      25, REG_TYPE_UINT32,   false },
	{ ARC_REG_GP,       "gp",       26, REG_TYPE_DATA_PTR, false },
	{ ARC_REG_FP,       "fp",       27, REG_TYPE_DATA_PTR, false },
	{ ARC_REG_SP,       "sp",       28, REG_TYPE_DATA_PTR, false },
	{ ARC_REG_ILINK,    "ilink",    29, REG_TYPE_CODE_PTR, false },
	{ ARC_REG_R30,      "r30",      30, REG_TYPE_UINT32,   false },
	{ ARC_REG_BLINK,    "blink",    31, REG_TYPE_CODE_PTR, false },
	{ ARC_REG_R32,      "r32",      32, REG_TYPE_UINT32,   false },
	{ ARC_REG_R33,      "r33",      33, REG_TYPE_UINT32,   false },
	{ ARC_REG_R34,      "r34",      34, REG_TYPE_UINT32,   false },
	{ ARC_REG_R35,      "r35",      35, REG_TYPE_UINT32,   false },
	{ ARC_REG_R36,      "r36",      36, REG_TYPE_UINT32,   false },
	{ ARC_REG_R37,      "r37",      37, REG_TYPE_UINT32,   false },
	{ ARC_REG_R38,      "r38",      38, REG_TYPE_UINT32,   false },
	{ ARC_REG_R39,      "r39",      39, REG_TYPE_UINT32,   false },
	{ ARC_REG_R40,      "r40",      40, REG_TYPE_UINT32,   false },
	{ ARC_REG_R41,      "r41",      41, REG_TYPE_UINT32,   false },
	{ ARC_REG_R42,      "r42",      42, REG_TYPE_UINT32,   false },
	{ ARC_REG_R43,      "r43",      43, REG_TYPE_UINT32,   false },
	{ ARC_REG_R44,      "r44",      44, REG_TYPE_UINT32,   false },
	{ ARC_REG_R45,      "r45",      45, REG_TYPE_UINT32,   false },
	{ ARC_REG_R46,      "r46",      46, REG_TYPE_UINT32,   false },
	{ ARC_REG_R47,      "r47",      47, REG_TYPE_UINT32,   false },
	{ ARC_REG_R48,      "r48",      48, REG_TYPE_UINT32,   false },
	{ ARC_REG_R49,      "r49",      49, REG_TYPE_UINT32,   false },
	{ ARC_REG_R50,      "r50",      50, REG_TYPE_UINT32,   false },
	{ ARC_REG_R51,      "r51",      51, REG_TYPE_UINT32,   false },
	{ ARC_REG_R52,      "r52",      52, REG_TYPE_UINT32,   false },
	{ ARC_REG_R53,      "r53",      53, REG_TYPE_UINT32,   false },
	{ ARC_REG_R54,      "r54",      54, REG_TYPE_UINT32,   false },
	{ ARC_REG_R55,      "r55",      55, REG_TYPE_UINT32,   false },
	{ ARC_REG_R56,      "r56",      56, REG_TYPE_UINT32,   false },
	{ ARC_REG_R57,      "r57",      57, REG_TYPE_UINT32,   false },
	{ ARC_REG_R58,      "r58",      58, REG_TYPE_UINT32,   false },
	{ ARC_REG_R59,      "r59",      59, REG_TYPE_UINT32,   false },
	{ ARC_REG_LP_COUNT, "lp_count", 60, REG_TYPE_UINT32,   false },
	{ ARC_REG_RESERVED, "reserved", 61, REG_TYPE_UINT32,    true },
	{ ARC_REG_LIMM,     "limm",     62, REG_TYPE_UINT32,    true },
	{ ARC_REG_R63,      "pcl",      63, REG_TYPE_CODE_PTR,  true },
	/* AUX */
	{ ARC_REG_PC,           "pc",		    0x6,   REG_TYPE_CODE_PTR, false },
	{ ARC_REG_LP_START,     "lp_start",     0x2,   REG_TYPE_CODE_PTR, false },
	{ ARC_REG_LP_END,       "lp_end",       0x3,   REG_TYPE_CODE_PTR, false },
	{ ARC_REG_STATUS32,     "status32",     0xA,   REG_TYPE_UINT32,   false },
	{ ARC_REG_IDENTITY,     "identity",     0x4,   REG_TYPE_UINT32,    true },
	{ ARC_REG_DEBUG,        "debug",        0x5,   REG_TYPE_UINT32,   false },
	{ ARC_REG_STATUS32_P0,  "status32_p0",  0xB,   REG_TYPE_UINT32,   false },
	{ ARC_REG_STATUS32_L2,  "status32_l2",  0xC,   REG_TYPE_UINT32,    true },
	{ ARC_REG_AUX_USER_SP,  "aux_user_sp",  0xD,   REG_TYPE_UINT32,   false },
	{ ARC_REG_AUX_IRQ_CTRL, "aux_irq_ctrl", 0xE,   REG_TYPE_UINT32,   false },
	{ ARC_REG_IC_IVIC,      "ic_ivic",      0x10,  REG_TYPE_UINT32,   false },
	{ ARC_REG_IC_CTRL,      "ic_ctrl",      0x11,  REG_TYPE_UINT32,   false },
	{ ARC_REG_IC_LIL,       "ic_lil",       0x13,  REG_TYPE_UINT32,   false },
	{ ARC_REG_AUX_DCCM,     "aux_dccm",     0x18,  REG_TYPE_UINT32,   false },
	{ ARC_REG_IC_IVIL,      "ic_ivil",      0x19,  REG_TYPE_UINT32,   false },
	{ ARC_REG_IC_RAM_ADDR,  "ic_ram_addr",  0x1A,  REG_TYPE_UINT32,   false },
	{ ARC_REG_IC_TAG,       "ic_tag",       0x1B,  REG_TYPE_UINT32,   false },
	{ ARC_REG_IC_DATA,      "ic_data",      0x1D,  REG_TYPE_UINT32,   false },
	{ ARC_REG_DEBUGI,       "debugi",       0x1F,  REG_TYPE_UINT32,   false },
	{ ARC_REG_COUNT0,       "count0",       0x21,  REG_TYPE_UINT32,   false },
	{ ARC_REG_CONTROL0,     "control0",     0x22,  REG_TYPE_UINT32,   false },
	{ ARC_REG_LIMIT0,       "limit0",       0x23,  REG_TYPE_UINT32,   false },
	{ ARC_REG_INT_VECTOR_BASE, "int_vector_base", 0x25, REG_TYPE_CODE_PTR, false },
	{ ARC_REG_AUX_IRQ_ACT,  "aux_irq_act",  0x43,  REG_TYPE_UINT32,   false },
	{ ARC_REG_DC_IVDC,      "dc_ivdc",      0x47,  REG_TYPE_UINT32,   false },
	{ ARC_REG_DC_CTRL,      "dc_ctrl",      0x48,  REG_TYPE_UINT32,   false },
	{ ARC_REG_DC_LDL,       "dc_ldl",       0x49,  REG_TYPE_UINT32,   false },
	{ ARC_REG_DC_IVDL,      "dc_ivdl",      0x4A,  REG_TYPE_UINT32,   false },
	{ ARC_REG_DC_FLSH,      "dc_flsh",      0x4B,  REG_TYPE_UINT32,   false },
	{ ARC_REG_DC_FLDL,      "dc_fldl",      0x4C,  REG_TYPE_UINT32,   false },
	{ ARC_REG_DC_RAM_ADDR,  "dc_ram_addr",  0x58,  REG_TYPE_UINT32,   false },
	{ ARC_REG_DC_TAG,       "dc_tag",       0x59,  REG_TYPE_UINT32,   false },
	{ ARC_REG_DC_DATA,      "dc_data",      0x5B,  REG_TYPE_UINT32,   false },
	{ ARC_REG_COUNT1,       "count1",       0x100, REG_TYPE_UINT32,   false },
	{ ARC_REG_CONTROL1,     "control1",     0x101, REG_TYPE_UINT32,   false },
	{ ARC_REG_LIMIT1,       "limit1",       0x102, REG_TYPE_UINT32,   false },
	{ ARC_REG_AUX_RTC_CTRL, "aux_rtc_ctrl", 0x103, REG_TYPE_UINT32,   false },
	{ ARC_REG_AUX_RTC_LOW,  "aux_rtc_low",  0x104, REG_TYPE_UINT32,    true },
	{ ARC_REG_AUX_RTC_HIGH, "aux_rtc_high", 0x105, REG_TYPE_UINT32,    true },
	{ ARC_REG_IRQ_PRIORITY_PENDING, "irq_priority_pending", 0x200, REG_TYPE_UINT32,  true },
	{ ARC_REG_AUX_IRQ_HINT, "irq_hint",     0x201, REG_TYPE_UINT32,   false },
	{ ARC_REG_IRQ_PRIORITY, "irq_priority", 0x206, REG_TYPE_UINT32,   false },
	{ ARC_REG_AUX_ICCM,     "aux_iccm",     0x208, REG_TYPE_UINT32,   false },
	{ ARC_REG_USTACK_TOP,   "ustack_top",   0x260, REG_TYPE_DATA_PTR, false },
	{ ARC_REG_USTACK_BASE,  "ustack_base",  0x261, REG_TYPE_DATA_PTR, false },
	{ ARC_REG_KSTACK_TOP,   "kstack_top",   0x264, REG_TYPE_DATA_PTR, false },
	{ ARC_REG_KSTACK_BASE,  "kstack_base",  0x265, REG_TYPE_DATA_PTR, false },
	{ ARC_REG_JLI_BASE,     "jli_base",     0x290, REG_TYPE_CODE_PTR, false },
	{ ARC_REG_LDI_BASE,     "ldi_base",     0x291, REG_TYPE_DATA_PTR, false },
	{ ARC_REG_EI_BASE,      "ei_base",      0x292, REG_TYPE_CODE_PTR, false },
	{ ARC_REG_ERET,         "eret",         0x400, REG_TYPE_CODE_PTR, false },
	{ ARC_REG_ERBTA,        "erbta",        0x401, REG_TYPE_CODE_PTR, false },
	{ ARC_REG_ERSTATUS,     "erstatus",     0x402, REG_TYPE_UINT32,   false },
	{ ARC_REG_ECR,          "ecr",          0x403, REG_TYPE_UINT32,   false },
	{ ARC_REG_EFA,          "efa",          0x404, REG_TYPE_CODE_PTR, false },
	{ ARC_REG_MPU_EN,       "mpu_en",       0x409, REG_TYPE_UINT32,   false },
	{ ARC_REG_ICAUSE,       "icause",       0x40A, REG_TYPE_UINT32,    true }, /* aka icause1 */
	{ ARC_REG_IRQ_SELECT,   "irq_select",   0x40B, REG_TYPE_UINT32,   false }, /* aka icause2 */
	{ ARC_REG_IRQ_ENABLE,   "irq_enable",   0x40C, REG_TYPE_UINT32,   false },
	{ ARC_REG_IRQ_TRIGGER,  "irq_trigger",  0x40D, REG_TYPE_UINT32,   false },
	{ ARC_REG_IRQ_STATUS,   "irq_status",   0x40F, REG_TYPE_UINT32,    true },
	{ ARC_REG_BTA,          "bta",          0x412, REG_TYPE_CODE_PTR, false },
	{ ARC_REG_BTA_L1,       "bta_l1",       0x413, REG_TYPE_CODE_PTR, false },
	{ ARC_REG_BTA_L2,       "bta_l2",       0x414, REG_TYPE_CODE_PTR, false },
	{ ARC_REG_IRQ_PULSE_CANCEL, "irq_pulse_cancel", 0x415,  REG_TYPE_UINT32, false },
	{ ARC_REG_IRQ_PENDING,  "irq_pending",  0x416, REG_TYPE_UINT32,    true },
	{ ARC_REG_MPU_ECR,      "mpu_ecr",      0x420, REG_TYPE_UINT32,    true },
	{ ARC_REG_MPU_RDB0,     "mpu_rdb0",     0x422, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDP0,     "mpu_rdp0",     0x423, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDB1,     "mpu_rdb1",     0x424, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDP1,     "mpu_rdp1",     0x425, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDB2,     "mpu_rdb2",     0x426, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDP2,     "mpu_rdp2",     0x427, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDB3,     "mpu_rdb3",     0x428, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDP3,     "mpu_rdp3",     0x429, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDB4,     "mpu_rdb4",     0x42A, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDP4,     "mpu_rdp4",     0x42B, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDB5,     "mpu_rdb5",     0x42C, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDP5,     "mpu_rdp5",     0x42D, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDB6,     "mpu_rdb6",     0x42E, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDP6,     "mpu_rdp6",     0x42F, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDB7,     "mpu_rdb7",     0x430, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDP7,     "mpu_rdp7",     0x431, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDB8,     "mpu_rdb8",     0x432, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDP8,     "mpu_rdp8",     0x433, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDB9,     "mpu_rdb9",     0x434, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDP9,     "mpu_rdp9",     0x435, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDB10,    "mpu_rdb10",    0x436, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDP10,    "mpu_rdp10",    0x437, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDB11,    "mpu_rdb11",    0x438, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDP11,    "mpu_rdp11",    0x439, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDB12,    "mpu_rdb12",    0x43A, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDP12,    "mpu_rdp12",    0x43B, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDB13,    "mpu_rdb13",    0x43C, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDP13,    "mpu_rdp13",    0x43D, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDB14,    "mpu_rdb14",    0x43E, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDP14,    "mpu_rdp14",    0x43F, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDB15,    "mpu_rdb15",    0x440, REG_TYPE_UINT32,   false },
	{ ARC_REG_MPU_RDP15,    "mpu_rdp15",    0x441, REG_TYPE_UINT32,   false },
	{ ARC_REG_SMART_CONTROL,"smart_control",0x700, REG_TYPE_UINT32,   false },
	{ ARC_REG_SMART_DATA,   "smart_data",   0x701, REG_TYPE_UINT32,    true },
	/* BCR */
	{ ARC_REG_BCR_VER,          "bcr_ver",          0x60, REG_TYPE_UINT32, true },
	{ ARC_REG_BTA_LINK_BUILD,   "bta_link_build",   0x63, REG_TYPE_UINT32, true },
	{ ARC_REG_VECBASE_AC_BUILD, "vecbase_ac_build", 0x68, REG_TYPE_UINT32, true },
	{ ARC_REG_MPU_BUILD,        "mpu_build",        0x6D, REG_TYPE_UINT32, true },
	{ ARC_REG_RF_BUILD,         "rf_build",         0x6E, REG_TYPE_UINT32, true },
	{ ARC_REG_D_CACHE_BUILD,    "d_cache_build",    0x72, REG_TYPE_UINT32, true },
	{ ARC_REG_DCCM_BUILD,       "dccm_build",       0x74, REG_TYPE_UINT32, true },
	{ ARC_REG_TIMER_BUILD,      "timer_build",      0x75, REG_TYPE_UINT32, true },
	{ ARC_REG_AP_BUILD,         "ap_build",         0x76, REG_TYPE_UINT32, true },
	{ ARC_REG_I_CACHE_BUILD,    "i_cache_build",    0x77, REG_TYPE_UINT32, true },
	{ ARC_REG_ICCM_BUILD,       "iccm_build",       0x78, REG_TYPE_UINT32, true },
	{ ARC_REG_MULTIPLY_BUILD,   "multiply_build",   0x7B, REG_TYPE_UINT32, true },
	{ ARC_REG_SWAP_BUILD,       "swap_build",       0x7C, REG_TYPE_UINT32, true },
	{ ARC_REG_NORM_BUILD,       "norm_build",       0x7D, REG_TYPE_UINT32, true },
	{ ARC_REG_MINMAX_BUILD,     "minmax_build",     0x7E, REG_TYPE_UINT32, true },
	{ ARC_REG_BARREL_BUILD,     "barrel_build",     0x7F, REG_TYPE_UINT32, true },
	{ ARC_REG_ISA_CONFIG,       "isa_config",       0xC1, REG_TYPE_UINT32, true },
	{ ARC_REG_STACK_REGION_BUILD, "stack_region_build", 0xC5, REG_TYPE_UINT32, true },
	{ ARC_REG_CPROT_BUILD,      "cprot_build",      0xC9, REG_TYPE_UINT32, true },
	{ ARC_REG_IRQ_BUILD,        "irq_build",        0xF3, REG_TYPE_UINT32, true },
	{ ARC_REG_IFQUEUE_BUILD,    "ifqueue_build",    0xFE, REG_TYPE_UINT32, true },
	{ ARC_REG_SMART_BUILD,      "smart_build",      0xFF, REG_TYPE_UINT32, true },
};
#endif

static int arc_regs_get_core_reg(struct reg *reg)
{
	assert(reg != NULL);

	struct arc_reg_t *arc_reg = reg->arch_info;
	struct target *target = arc_reg->target;
	struct arc32_common *arc32 = target_to_arc32(target);

	if (target->state != TARGET_HALTED)
		return ERROR_TARGET_NOT_HALTED;

	if (reg->valid) {
		LOG_DEBUG("Get register (cached) gdb_num=%" PRIu32 ", name=%s, value=0x%" PRIx32,
				reg->number, arc_reg->desc2->name, arc_reg->value);
		return ERROR_OK;
	}

	if (arc_reg->desc2->is_core) {
		if (arc_reg->desc2->arch_num == 61 || arc_reg->desc2->arch_num == 62) {
			LOG_ERROR("It is forbidden to read core registers 61 and 62.");
			return ERROR_FAIL;
		}
		arc_jtag_read_core_reg_one(&arc32->jtag_info, arc_reg->desc2->arch_num,
			&arc_reg->value);
	} else {
		arc_jtag_read_aux_reg_one(&arc32->jtag_info, arc_reg->desc2->arch_num,
			&arc_reg->value);
	}

	buf_set_u32(reg->value, 0, 32, arc_reg->value);
	reg->valid = true;
	reg->dirty = false;
	LOG_DEBUG("Get register gdb_num=%" PRIu32 ", name=%s, value=0x%" PRIx32,
			reg->number , arc_reg->desc2->name, arc_reg->value);

	return ERROR_OK;
}

static int arc_regs_set_core_reg(struct reg *reg, uint8_t *buf)
{
	LOG_DEBUG("-");
	struct arc_reg_t *arc_reg = reg->arch_info;
	struct target *target = arc_reg->target;
	//struct arc32_common *arc32 = target_to_arc32(target);
	uint32_t value = buf_get_u32(buf, 0, 32);

	if (target->state != TARGET_HALTED)
		return ERROR_TARGET_NOT_HALTED;
#if 0
	if (arc_reg->desc->readonly) {
		LOG_ERROR("Cannot set value to a read-only register %s.", arc_reg->desc2->name);
		return ERROR_FAIL;
	}
#endif

	if (arc_reg->desc2->is_core && (arc_reg->desc2->arch_num == 61 ||
			arc_reg->desc2->arch_num == 62)) {
		LOG_ERROR("It is forbidden to write core registers 61 and 62.");
		return ERROR_FAIL;
	}

	buf_set_u32(reg->value, 0, 32, value);

	arc_reg->value = value;

	LOG_DEBUG("Set register gdb_num=%" PRIu32 ", name=%s, value=0x%08" PRIx32,
			reg->number, arc_reg->desc2->name, value);
	reg->valid = true;
	reg->dirty = true;

	return ERROR_OK;
}

const struct reg_arch_type arc32_reg_type = {
	.get = arc_regs_get_core_reg,
	.set = arc_regs_set_core_reg,
};

/**
 * Read BCRs.
 */
int arc_regs_read_bcrs(struct target *target)
{
	LOG_DEBUG("-");
#if 0
	struct arc32_common *arc32 = target_to_arc32(target);
	uint32_t numregs = ARC_REG_AFTER_BCR - ARC_REG_FIRST_BCR;
	uint32_t *addrs = calloc(numregs, sizeof(uint32_t));
	uint32_t *values = calloc(numregs, sizeof(uint32_t));

	for (unsigned i = ARC_REG_FIRST_BCR; i < ARC_REG_AFTER_BCR; i++) {
		addrs[i - ARC_REG_FIRST_BCR]  = arc32_regs_descriptions[i].addr;
	}
	int jtag_retval = arc_jtag_read_aux_reg(&arc32->jtag_info, addrs, numregs, values);
	if (ERROR_OK != jtag_retval) {
		LOG_ERROR("Error reading BCR registers from target.");
		free(addrs);
		free(values);
		return jtag_retval;
	}

	struct bcr_set_t *bcrs = &(arc32->bcr_set);
	bcrs->bcr_ver.raw = values[ARC_REG_BCR_VER - ARC_REG_FIRST_BCR];
	bcrs->bta_link_build.raw = values[ARC_REG_BTA_LINK_BUILD - ARC_REG_FIRST_BCR];
	bcrs->vecbase_ac_build.raw = values[ARC_REG_VECBASE_AC_BUILD - ARC_REG_FIRST_BCR];
	bcrs->mpu_build.raw = values[ARC_REG_MPU_BUILD - ARC_REG_FIRST_BCR];
	bcrs->rf_build.raw = values[ARC_REG_RF_BUILD - ARC_REG_FIRST_BCR];
	bcrs->d_cache_build.raw = values[ARC_REG_D_CACHE_BUILD - ARC_REG_FIRST_BCR];

	bcrs->dccm_build.raw = values[ARC_REG_DCCM_BUILD - ARC_REG_FIRST_BCR];
	bcrs->timer_build.raw = values[ARC_REG_TIMER_BUILD - ARC_REG_FIRST_BCR];
	bcrs->ap_build.raw = values[ARC_REG_AP_BUILD - ARC_REG_FIRST_BCR];
	bcrs->i_cache_build.raw = values[ARC_REG_I_CACHE_BUILD - ARC_REG_FIRST_BCR];
	bcrs->iccm_build.raw = values[ARC_REG_ICCM_BUILD - ARC_REG_FIRST_BCR];
	bcrs->multiply_build.raw = values[ARC_REG_MULTIPLY_BUILD - ARC_REG_FIRST_BCR];
	bcrs->swap_build.raw = values[ARC_REG_SWAP_BUILD - ARC_REG_FIRST_BCR];
	bcrs->norm_build.raw = values[ARC_REG_NORM_BUILD - ARC_REG_FIRST_BCR];
	bcrs->minmax_build.raw = values[ARC_REG_MINMAX_BUILD - ARC_REG_FIRST_BCR];
	bcrs->barrel_build.raw = values[ARC_REG_BARREL_BUILD - ARC_REG_FIRST_BCR];
	bcrs->isa_config.raw = values[ARC_REG_ISA_CONFIG - ARC_REG_FIRST_BCR];
	bcrs->stack_region_build.raw = values[ARC_REG_STACK_REGION_BUILD - ARC_REG_FIRST_BCR];
	bcrs->cprot_build.raw = values[ARC_REG_CPROT_BUILD - ARC_REG_FIRST_BCR];
	bcrs->irq_build.raw = values[ARC_REG_IRQ_BUILD - ARC_REG_FIRST_BCR];
	bcrs->ifqueue_build.raw = values[ARC_REG_IFQUEUE_BUILD - ARC_REG_FIRST_BCR];
	bcrs->smart_build.raw = values[ARC_REG_SMART_BUILD - ARC_REG_FIRST_BCR];

	free(addrs);
	free(values);
#endif
	return ERROR_OK;
}

/** Configure optional registers, depending on BCR values. */
void arc_regs_build_reg_list(struct target *target)
{
	LOG_DEBUG("-");
#if 0

	struct arc32_common *arc32 = target_to_arc32(target);
	struct reg *reg_list = arc32->core_cache->reg_list;
	struct bcr_set_t *bcrs = &(arc32->bcr_set);

	/* Enable baseline registers which are always present. */
	reg_list[ARC_REG_IDENTITY].exist = true;
	reg_list[ARC_REG_PC].exist = true;
	reg_list[ARC_REG_STATUS32].exist = true;
	reg_list[ARC_REG_BTA].exist = true;
	reg_list[ARC_REG_ECR].exist = true;
	reg_list[ARC_REG_INT_VECTOR_BASE].exist = true;
	reg_list[ARC_REG_AUX_USER_SP].exist = true;
	reg_list[ARC_REG_ERET].exist = true;
	reg_list[ARC_REG_ERBTA].exist = true;
	reg_list[ARC_REG_ERSTATUS].exist = true;
	reg_list[ARC_REG_EFA].exist = true;

	/* Enable debug registers. They accompany debug host interface, so there is
	 * no way OpenOCD could communicate with target that has no such registers.
	 */
	reg_list[ARC_REG_DEBUG].exist = true;
	reg_list[ARC_REG_DEBUGI].exist = true;

	/* AUX registers are disabled by default, enable them depending on BCR
	 * contents. */
	for (unsigned regnum = 0; regnum < ARC_TOTAL_NUM_REGS; regnum++) {
		/* Some core regs are missing from cores with reduced register banks. */
		if (bcrs->rf_build.e &&
		   ((regnum > ARC_REG_R3 && regnum < ARC_REG_R10) ||
			(regnum > ARC_REG_R15 && regnum < ARC_REG_R26))) {
			reg_list[regnum].exist = false;
			continue;
		}

		/* Enable MPU regions registers */
		if (regnum >= ARC_REG_MPU_RDB0 && regnum <= ARC_REG_MPU_RDP15 &&
			(bcrs->mpu_build.version >= 2)) {
			const unsigned num_regions = bcrs->mpu_build.regions;
			/* Shift because we have two registers per region. Add  */
			const unsigned region_id = (regnum - ARC_REG_MPU_RDB0) >> 1;
			if (region_id + 1 >= num_regions) {
				reg_list[regnum].exist = true;
			}
			continue;
		}

		switch (regnum) {
			/* Enable zero-delay loop registers. */
			case ARC_REG_LP_COUNT:
			case ARC_REG_LP_START:
			case ARC_REG_LP_END:
				if (bcrs->isa_config.lpc_size)
					reg_list[regnum].exist = true;
				break;
			/* Enable code density registers (not for ISAv1). */
			case ARC_REG_JLI_BASE:
			case ARC_REG_LDI_BASE:
			case ARC_REG_EI_BASE:
				if (bcrs->isa_config.c && bcrs->isa_config.version >= 2)
					reg_list[regnum].exist = true;
				break;
			/* Timer 0 */
			case ARC_REG_COUNT0:
			case ARC_REG_CONTROL0:
			case ARC_REG_LIMIT0:
				if (bcrs->timer_build.t0)
					reg_list[regnum].exist = true;
				break;
			/* Timer 1 */
			case ARC_REG_COUNT1:
			case ARC_REG_CONTROL1:
			case ARC_REG_LIMIT1:
				if (bcrs->timer_build.t1)
					reg_list[regnum].exist = true;
				break;
			/* RTC: 64-bit timer */
			case ARC_REG_AUX_RTC_CTRL:
			case ARC_REG_AUX_RTC_LOW:
			case ARC_REG_AUX_RTC_HIGH:
				if (bcrs->timer_build.rtc)
					reg_list[regnum].exist = true;
				break;
			/* I$ with feature level 0 and up*/
			case ARC_REG_IC_IVIC:
			case ARC_REG_IC_CTRL:
				if (bcrs->i_cache_build.version >= 4)
					reg_list[regnum].exist = true;
				break;
			/* I$ with feature level 1 and up */
			case ARC_REG_IC_LIL:
			case ARC_REG_IC_IVIL:
				if (bcrs->i_cache_build.version >= 4 && bcrs->i_cache_build.fl > 0)
					reg_list[regnum].exist = true;
				break;
			/* I$ with feature level 2 */
			case ARC_REG_IC_RAM_ADDR:
			case ARC_REG_IC_TAG:
			case ARC_REG_IC_DATA:
				if (bcrs->i_cache_build.version >= 4 && bcrs->i_cache_build.fl > 1)
					reg_list[regnum].exist = true;
				break;
			/* D$ with feature level 0 and up*/
			case ARC_REG_DC_IVDC:
			case ARC_REG_DC_CTRL:
			case ARC_REG_DC_FLSH:
				if (bcrs->d_cache_build.version >= 4)
					reg_list[regnum].exist = true;
				break;
			/* D$ with feature level 1 and up */
			case ARC_REG_DC_LDL:
			case ARC_REG_DC_IVDL:
			case ARC_REG_DC_FLDL:
				if (bcrs->d_cache_build.version >= 4 && bcrs->d_cache_build.fl > 0)
					reg_list[regnum].exist = true;
				break;
			/* D$ with feature level 2 */
			case ARC_REG_DC_RAM_ADDR:
			case ARC_REG_DC_TAG:
			case ARC_REG_DC_DATA:
				if (bcrs->d_cache_build.version >= 4 && bcrs->d_cache_build.fl > 1)
					reg_list[regnum].exist = true;
				break;
			/* DCCM regs */
			case ARC_REG_AUX_DCCM:
				if (bcrs->dccm_build.version >= 3)
					reg_list[regnum].exist = true;
				break;
			/* ICCM regs */
			case ARC_REG_AUX_ICCM:
				if (bcrs->iccm_build.version >= 4)
					reg_list[regnum].exist = true;
				break;
			/* Enable MPU registers. */
			case ARC_REG_MPU_EN:
			case ARC_REG_MPU_ECR:
				if (bcrs->mpu_build.version >= 2)
					reg_list[regnum].exist = true;
				break;
			/* Enable SMART registers */
			case ARC_REG_SMART_CONTROL:
			case ARC_REG_SMART_DATA:
				if (bcrs->smart_build.version >= 3)
					reg_list[regnum].exist = true;
				break;
			/* Enable STATUS32_P0 for fast interrupts. */
			case ARC_REG_STATUS32_P0:
				if (bcrs->irq_build.version >= 2 && bcrs->irq_build.f)
					reg_list[regnum].exist = true;
				break;
			/* Enable interrupt registers  */
			case ARC_REG_AUX_IRQ_CTRL:
			case ARC_REG_AUX_IRQ_ACT:
			case ARC_REG_IRQ_SELECT:
			case ARC_REG_IRQ_PRIORITY:
			case ARC_REG_IRQ_ENABLE:
			case ARC_REG_IRQ_TRIGGER:
			case ARC_REG_IRQ_PENDING:
			case ARC_REG_IRQ_PULSE_CANCEL:
			case ARC_REG_IRQ_STATUS:
			case ARC_REG_IRQ_PRIORITY_PENDING:
			case ARC_REG_AUX_IRQ_HINT:
			case ARC_REG_ICAUSE:
				if (bcrs->irq_build.version >= 2 && bcrs->irq_build.irqs)
					reg_list[regnum].exist = true;
				break;
			/* Stack checking registers */
			case ARC_REG_KSTACK_BASE:
			case ARC_REG_KSTACK_TOP:
			case ARC_REG_USTACK_BASE:
			case ARC_REG_USTACK_TOP:
				if (bcrs->stack_region_build.version >= 2)
					reg_list[regnum].exist = true;
				break;
		}
	}
#endif
}
#if 0
struct reg_cache *arc_regs_build_reg_cache_deprecated(struct target *target)
{
	uint32_t i;

	/* get pointers to arch-specific information */
	struct arc32_common *arc32 = target_to_arc32(target);
	struct reg_cache **cache_p = register_get_last_cache_p(&target->reg_cache);
	struct reg_cache *cache = malloc(sizeof(struct reg_cache));
	struct reg *reg_list = calloc(ARC_TOTAL_NUM_REGS, sizeof(struct reg));
	struct arc_reg_t *arch_info = calloc(ARC_TOTAL_NUM_REGS, sizeof(struct arc_reg_t));

	/* Build the process context cache */
	cache->name = "arc32 registers";
	cache->next = NULL;
	cache->reg_list = reg_list;
	cache->num_regs = ARC_TOTAL_NUM_REGS;
	(*cache_p) = cache;
	arc32->core_cache = cache;

	// XML features
	struct reg_feature *core_feature = calloc(1, sizeof(struct reg_feature));
	core_feature->name = feature_core_v2_name;
	struct reg_feature *aux_minimal_feature = calloc(1, sizeof(struct reg_feature));
	aux_minimal_feature->name = feature_aux_minimal_name;
	struct reg_feature *aux_other_feature = calloc(1, sizeof(struct reg_feature));
	aux_other_feature->name = feature_aux_other_name;

	/* Data types */
	struct reg_data_type *uint32_data_type = calloc(sizeof(struct reg_data_type), 1);
	struct reg_data_type *code_ptr_data_type = calloc(sizeof(struct reg_data_type), 1);
	struct reg_data_type *data_ptr_data_type = calloc(sizeof(struct reg_data_type), 1);
	uint32_data_type->type = REG_TYPE_UINT32;
	uint32_data_type->id = "uint32";
	code_ptr_data_type->type = REG_TYPE_CODE_PTR;
	code_ptr_data_type->id = "code_ptr";
	data_ptr_data_type->type = REG_TYPE_DATA_PTR;
	data_ptr_data_type->id = "data_ptr";
	/* Only three data types are used by ARC */
	struct reg_data_type *data_types[13] = { NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, uint32_data_type, NULL, NULL, code_ptr_data_type,
		data_ptr_data_type };

	for (i = 0; i < ARC_TOTAL_NUM_REGS; i++) {
		arch_info[i].desc = arc32_regs_descriptions + i;
		arch_info[i].target = target;
		arch_info[i].arc32_common = arc32;
		arch_info[i].dummy = false;
		reg_list[i].name = arc32_regs_descriptions[i].name;
		reg_list[i].size = 32;
		reg_list[i].value = calloc(1, 4);
		reg_list[i].dirty = 0;
		reg_list[i].valid = 0;
		reg_list[i].type = &arc32_reg_type;
		reg_list[i].arch_info = &arch_info[i];

		reg_list[i].number = arc32_regs_descriptions[i].regnum;
		/* By default only core regs and BCRs are enabled. */
		reg_list[i].exist = (i < ARC_REG_AFTER_CORE || i == ARC_REG_PCL ||
				(i >= ARC_REG_FIRST_BCR && i < ARC_REG_AFTER_BCR));

		reg_list[i].group = general_group_name;
		reg_list[i].caller_save = true;
		reg_list[i].reg_data_type = data_types[ arch_info[i].desc->gdb_type ];

		if (i <= ARC_REG_PCL) {
			reg_list[i].feature = core_feature;
		} else if (i > ARC_REG_PCL && i <= ARC_REG_STATUS32) {
			reg_list[i].feature = aux_minimal_feature;
		} else {
			reg_list[i].feature = aux_other_feature;
		}

		LOG_DEBUG("reg n=%3i name=%3s group=%s feature=%s", i,
			reg_list[i].name, reg_list[i].group,
			reg_list[i].feature->name);
	}

	return cache;
}
#endif

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

