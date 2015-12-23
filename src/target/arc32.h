/***************************************************************************
 *   Copyright (C) 2013-2015 Synopsys, Inc.                                *
 *   Frank Dols <frank.dols@synopsys.com>                                  *
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

#ifndef ARC32_H
#define ARC32_H

#include <helper/time_support.h>
#include <jtag/jtag.h>

#include "algorithm.h"
#include "breakpoints.h"
#include "jtag/interface.h"
#include "register.h"
#include "target.h"
#include "target_request.h"
#include "target_type.h"

#include "arc_dbg.h"
#include "arc_jtag.h"
#include "arc_mem.h"
#include "arc_mntr.h"
#include "arc_ocd.h"
#include "arc_regs.h"

#if defined _WIN32 || defined __CYGWIN__
#include <windows.h>
#define sleep(x) Sleep(x)
#endif


#define ARC32_COMMON_MAGIC	0xB32EB324  /* just a unique number */

typedef uint32_t arc32_address_t;

/* ARC core ARCompatISA register set */
enum arc32_isa_mode {
	ARC32_ISA_ARC32 = 0,
	ARC32_ISA_ARC16 = 1,
};

/* Register data type */
struct arc_reg_data_type {
	struct list_head list;
	struct reg_data_type data_type;
};

/* List of register data types. */
struct arc_reg_data_type_list {
	struct list_head list;
};

/* ARC Register description */
struct arc_reg_desc {
	/* Register name */
	char *name;

	/* GDB XML feature */
	char *gdb_xml_feature;

	/* Is this a register in g/G-packet? */
	bool is_general;

	/* Architectural number: core reg num or AUX reg num */
	uint32_t arch_num;

	/* Core or AUX register? */
	bool is_core;

	/* Build configuration register? */
	bool is_bcr;

	/* Data type */
	struct reg_data_type *data_type;

	struct list_head list;
};

struct arc_reg_desc_list {
	struct list_head list;
};

#define ARC_GDB_NUM_INVALID (UINT32_MAX)

extern int jim_arc_add_reg(Jim_Interp *interp, int argc, Jim_Obj * const *argv);

enum arc_actionpoint_type {
	ARC_AP_BREAKPOINT,
	ARC_AP_WATCHPOINT,
};

/* offsets into arc32 core register cache */
struct arc32_comparator {
	int used;
	uint32_t bp_value;
	uint32_t reg_address;
	enum arc_actionpoint_type type;
};

struct arc32_common {
	uint32_t common_magic;
	void *arch_info;

	struct arc_jtag jtag_info;

	struct reg_cache *core_cache;

	enum arc32_isa_mode     isa_mode;

	/* working area for fastdata access */
	struct working_area *fast_data_area;

	int bp_scanned;

	/* Actionpoints */
	unsigned int actionpoints_num;
	unsigned int actionpoints_num_avail;
	struct arc32_comparator *actionpoints_list;

	/* Cache control */
	bool has_dcache;
	/* If true, then D$ has been already flushed since core has been
	 * halted. */
	bool dcache_flushed;
	/* If true, then caches have been already flushed since core has been
	 * halted. */
	bool cache_invalidated;

	/* Whether DEBUG.SS bit is present. This is a unique feature of ARC 600. */
	bool has_debug_ss;

	/* Workaround for a problem with ARC 600 - writing RA | IS | SS will not
	 * step an instruction - it must be only a (IS | SS). However RA is set by
	 * the processor itself and since OpenOCD does read-modify-write of DEBUG
	 * register when stepping, it is required to explicitly disable RA before
	 * stepping. */
	bool on_step_reset_debug_ra;

	/* CCM memory regions (optional). */
	arc32_address_t iccm0_start;
	arc32_address_t iccm0_end;
	arc32_address_t iccm1_start;
	arc32_address_t iccm1_end;
	arc32_address_t dccm_start;
	arc32_address_t dccm_end;

	/* Register descriptions */
	struct arc_reg_data_type_list reg_data_types;
	struct arc_reg_desc_list core_reg_descriptions;
	struct arc_reg_desc_list aux_reg_descriptions;
	struct arc_reg_desc_list bcr_reg_descriptions;
	unsigned long num_regs;
	unsigned long num_core_regs;
	unsigned long num_aux_regs;
	unsigned long num_bcr_regs;
	unsigned long last_general_reg;

	/* PC register location in register cache. */
	unsigned long pc_index_in_cache;
	/* DEBUG register location in register cache. */
	unsigned long debug_index_in_cache;
};

//#define ARC32_FASTDATA_HANDLER_SIZE	0x8000 /* haps51 */
#define ARC32_FASTDATA_HANDLER_SIZE	0x10000  /* 64Kb */

/* ARC 32bits Compact v2 opcodes */
#define ARC32_SDBBP 0x256F003F  /* BRK */

/* ARC 16bits Compact v2 opcodes */
#define ARC16_SDBBP 0x7FFF      /* BRK_S */

/* Borrowed from nds32.h */
#define CHECK_RETVAL(action)			\
	do {					\
		int __retval = (action);	\
		if (__retval != ERROR_OK) {	\
			LOG_DEBUG("error while calling \"%s\"",	\
				# action);     \
			return __retval;	\
		}				\
	} while (0)

#define JIM_CHECK_RETVAL(action)		\
	do {					\
		int __retval = (action);	\
		if (__retval != JIM_OK) {	\
			LOG_DEBUG("error while calling \"%s\"",	\
				# action);     \
			return __retval;	\
		}				\
	} while (0)


#define ARC_COMMON_MAGIC 0x1A471AC5  /* just a unique number */

/* Error codes */
#define ERROR_ARC_REGISTER_NOT_FOUND       (-700)
#define ERROR_ARC_REGISTER_FIELD_NOT_FOUND (-701)
#define ERROR_ARC_REGISTER_IS_NOT_STRUCT   (-702)
#define ERROR_ARC_FIELD_IS_NOT_BITFIELD    (-703)
#define ERROR_ARC_REGTYPE_NOT_FOUND        (-704)

struct arc_common {
	int common_magic;
	bool is_4wire;
	struct arc32_common arc32;
};


/* ----- Public data ------------------------------------------------------- */
extern const char * const arc_reg_debug;
extern const char * const arc_reg_isa_config;
extern const char * const arc_reg_isa_config_addr_size;


/* ----- Inlined functions ------------------------------------------------- */

/**
 * Convert data in host endianness to the middle endian. This is required to
 * write 4-byte instructions.
 */
static inline void arc32_h_u32_to_me(uint8_t* buf, int val)
{
    buf[1] = (uint8_t) (val >> 24);
    buf[0] = (uint8_t) (val >> 16);
    buf[3] = (uint8_t) (val >> 8);
    buf[2] = (uint8_t) (val >> 0);
}

/**
 * Convert data in middle endian to host endian. This is required to read 32-bit
 * instruction from little endian ARCs.
 */
static inline uint32_t arc32_me_to_h_u32(const uint8_t* buf)
{
    return (uint32_t)(buf[2] | buf[3] << 8 | buf[0] << 16 | buf[1] << 24);
}

static inline struct arc32_common * target_to_arc32(struct target *target)
{
	return target->arch_info;
}

/* ----- Exported functions ------------------------------------------------ */

int arc32_init_arch_info(struct target *target, struct arc32_common *arc32,
	struct jtag_tap *tap);

int arc32_build_reg_cache(struct target *target);
int arc32_build_bcr_reg_cache(struct target *target);

int arc32_save_context(struct target *target);
int arc32_restore_context(struct target *target);

int arc32_enable_interrupts(struct target *target, int enable);

int arc32_start_core(struct target *target);

int arc32_config_step(struct target *target, int enable_step);

int arc32_cache_invalidate(struct target *target);

int arc32_wait_until_core_is_halted(struct target *target);

int arc32_print_core_state(struct target *target);
int arc32_arch_state(struct target *target);
int arc32_get_current_pc(struct target *target);
int arc32_dcache_flush(struct target *target);

int arc32_reset_caches_states(struct target *target);

int arc32_write_instruction_u32(struct target *target, uint32_t address,
		uint32_t instr);
int arc32_read_instruction_u32(struct target *target, uint32_t address,
		uint32_t *value);

int arc32_configure(struct target *target);

/* Configurable registers functions */
void arc32_add_reg_data_type(struct target *target,
		struct arc_reg_data_type *data_type);
int arc32_add_reg(struct target *target, struct arc_reg_desc *arc_reg,
		const char * const type_name, const size_t type_name_len);

/* Get value of 32-bit register. */
int arc32_get_register_value_u32(struct target *target, const char *reg_name,
		uint32_t * value_ptr);
/* Set value of 32-bit register. */
int arc32_set_register_value_u32(struct target *target, const char *reg_name,
		uint32_t value);
/* Get value of field in struct register */
int arc32_get_register_field(struct target *target, const char *reg_name,
		const char *field_name, uint32_t *value_ptr);

/* Set an amount of actionpoints in target.  This function will remove any
 * existing actionpoints from target.  */
void arc32_set_actionpoints_num(struct target *target, unsigned ap_num);

#endif /* ARC32_H */
