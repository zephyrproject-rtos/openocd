/*
 * Copyright (C) 2012 Synopsys, Inc. (www.synopsys.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *   Maintainer: frank.dols@synopsys.com
 */

#ifndef ARC_JTAG_H
#define ARC_JTAG_H

/* JTAG TAP instructions = IEEE 1149.1 */
#define ARC_TAP_INST_IDCODE	0x01

/* JTAG core registers plus bit fields within registers */
#define ARC_INSTRUCTION_REG 		0x0 /* Instruction register */

#define ARC_JTAG_STATUS_REG 		0x8 /* Jtag cmd transaction status */
#define ARC_JTAG_STAT_ST			(000001)
#define ARC_JTAG_STAT_FL			(000010)
#define ARC_JTAG_STAT_RD			(000100)
#define ARC_JTAG_STAT_NIU			(001000) /* Not in use */
#define ARC_JTAG_STAT_RU			(010000)
#define ARC_JTAG_STAT_RA			(100000)

#define ARC_TRANSACTION_CMD_REG 	0x9 /* Command to perform */
#define ARC_JTAG_WRITE_TO_MEMORY	0x0
#define ARC_JTAG_WRITE_TO_CORE_REG	0x1
#define ARC_JTAG_WRITE_TO_AUX_REG	0x2
#define ARC_JTAG_CMD_NOP			0x3 /* Not in use */
#define ARC_JTAG_READ_FROM_MEMORY	0x4
#define ARC_JTAG_READ_FROM_CORE_REG	0x5
#define ARC_JTAG_READ_FROM_AUX_REG	0x6

#define ARC_ADDRESS_REG				0xA /* SoC address to access */

#define ARC_DATA_REG				0xB /* Data read/written from SoC */

#define ARC_IDCODE_REG				0xC /* ARC core type information */
#define ARC_CORE_TYPE				(111111 << 12)
#define ARC_NUMBER_CORE				(1111111111 << 18)
#define ARC_JTAG_VERSION			(1111 << 28)

#define ARC_BYPASS_REG				0xF /* TDI to TDO */

struct arc_jtag {
	struct jtag_tap *tap;
	uint32_t tap_end_state;
	uint32_t intest_instr;
	uint32_t cur_trans;

	uint32_t scann_size;
	uint32_t scann_instr;
	uint32_t cur_scan_chain;

	uint32_t dpc; /* Debug PC value */

	int fast_access_save;
};

/* ----- Exported JTAG functions ------------------------------------------- */

int arc_jtag_startup(struct arc_jtag *jtag_info);
int arc_jtag_shutdown(struct arc_jtag *jtag_info);

int arc_jtag_read_memory(struct arc_jtag *jtag_info, uint32_t addr,
	uint32_t *value);
int arc_jtag_write_memory(struct arc_jtag *jtag_info, uint32_t addr,
	uint32_t *value);

int arc_jtag_read_block(struct arc_jtag *jtag_info, uint32_t addr,
	uint32_t size,  uint32_t count, uint32_t *value);
int arc_jtag_write_block(struct arc_jtag *jtag_info, uint32_t addr,
	uint32_t size,  uint32_t count, uint32_t *value);

int arc_jtag_read_core_reg(struct arc_jtag *jtag_info, uint32_t addr,
	uint32_t *value);
int arc_jtag_write_core_reg(struct arc_jtag *jtag_info, uint32_t addr,
	uint32_t *value);
int arc_jtag_read_core_reg_bulk(struct arc_jtag *jtag_info,
	uint32_t *values, unsigned int count);
int arc_jtag_write_core_reg_bulk(struct arc_jtag *jtag_info, uint32_t *value,
	unsigned int count);

int arc_jtag_read_aux_reg(struct arc_jtag *jtag_info, uint32_t addr,
	uint32_t *value);
int arc_jtag_write_aux_reg(struct arc_jtag *jtag_info, uint32_t addr,
	uint32_t *value);
int arc_jtag_read_aux_reg_bulk(struct arc_jtag *jtag_info, uint32_t *addr,
	uint32_t *value, unsigned int count);
int arc_jtag_write_aux_reg_bulk(struct arc_jtag *jtag_info, uint32_t *addr,
	uint32_t *value, unsigned int count);

int arc_jtag_status(struct arc_jtag *jtag_info, uint32_t *value);
int arc_jtag_idcode(struct arc_jtag *jtag_info, uint32_t *value);

int arc_ocd_start_test(struct arc_jtag *jtag_info, int reg, uint32_t bits);

#endif /* ARC_JTAG_H */
