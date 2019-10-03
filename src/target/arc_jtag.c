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

/* TODO: This file has a lot of repetative code. Should be refactored. */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "arc.h"

/* ----- Supporting functions ---------------------------------------------- */

static int arc_jtag_set_instruction(struct arc_jtag *jtag_info, uint32_t new_instr)
{
	int retval = ERROR_OK;
	struct jtag_tap *tap;

#ifdef DEBUG
	LOG_DEBUG("Entering");
#endif

	tap = jtag_info->tap;
	if (tap == NULL)
		return ERROR_FAIL;

#ifdef DEBUG
	LOG_DEBUG(" cur_instr: 0x%08" PRIx32 " new_instr: 0x%08" PRIx32,
		buf_get_u32(tap->cur_instr, 0, tap->ir_length), new_instr);
#endif

	if (buf_get_u32(tap->cur_instr, 0, tap->ir_length) != (uint32_t)new_instr) {
		struct scan_field field;
		uint8_t instr[4];
		uint8_t ret[4];

		field.num_bits = tap->ir_length;
		field.in_value = ret;
		buf_set_u32(instr, 0, field.num_bits, new_instr);
		field.out_value = instr;

		jtag_add_ir_scan(tap, &field, jtag_info->tap_end_state);

		if (jtag_execute_queue() != ERROR_OK) {
			LOG_ERROR("setting new instruction failed");
			return ERROR_FAIL;
#ifdef DEBUG
		} else {
			LOG_DEBUG(" set JTAG instruction: 0x%08" PRIx32, new_instr);
#endif
		}
	}

	return retval;
}

static int arc_jtag_set_transaction(struct arc_jtag *jtag_info, uint32_t new_trans)
{
	int retval = ERROR_OK;
	struct jtag_tap *tap;

#ifdef DEBUG
	LOG_DEBUG("Entering");
#endif

	tap = jtag_info->tap;
	if (tap == NULL)
		return ERROR_FAIL;

#ifdef DEBUG
	LOG_DEBUG(" in:  cur_trans: 0x%08" PRIx32" new_trans: 0x%08" PRIx32,
		jtag_info->cur_trans,new_trans);
#endif

	if (jtag_info->cur_trans != (uint32_t)new_trans) {
		struct scan_field field[1];
		uint8_t trans[4] = {0};
		uint8_t ret[4];

		field[0].num_bits = 4;
		field[0].in_value = ret;
		buf_set_u32(trans, 0, field[0].num_bits, new_trans);
		field[0].out_value = trans;

		jtag_add_dr_scan(jtag_info->tap, 1, field, jtag_info->tap_end_state);

		if (jtag_execute_queue() != ERROR_OK) {
			LOG_ERROR("setting new transaction failed");
			return ERROR_FAIL;
		} else {
#ifdef DEBUG
			LOG_DEBUG("set JTAG transaction: 0x%08" PRIx32, new_trans);
#endif
			jtag_info->cur_trans = new_trans;
		}
	}

	return retval;
}

static int arc_jtag_read_data(struct arc_jtag *jtag_info, uint32_t *pdata)
{
	int retval = ERROR_OK;
	struct scan_field fields[1];
	uint8_t data_buf[4];

#ifdef DEBUG
	LOG_DEBUG("Entering");
#endif

	memset(data_buf, 0, sizeof(data_buf));

	fields[0].num_bits = 32;
	fields[0].in_value = data_buf;
	fields[0].out_value = NULL;

	jtag_add_dr_scan(jtag_info->tap, 1, fields, jtag_info->tap_end_state);

	if (jtag_execute_queue() != ERROR_OK) {
		LOG_ERROR("%s: add_dr_scan failed", __func__);
		return ERROR_FAIL;
	}

	*pdata = buf_get_u32(data_buf, 0, 32);

	return retval;
}

static int arc_jtag_write_data(struct arc_jtag *jtag_info,	uint32_t data)
{
	int retval = ERROR_OK;
	struct scan_field fields[1];
	uint8_t data_buf[4];

#ifdef DEBUG
	LOG_DEBUG("Entering");
#endif

	memset(data_buf, 0, sizeof(data_buf));

	buf_set_u32(data_buf, 0, 32, data);

	fields[0].num_bits = 32;
	fields[0].in_value = NULL;
	fields[0].out_value = data_buf;

	jtag_add_dr_scan(jtag_info->tap, 1, fields, jtag_info->tap_end_state);

	if (jtag_execute_queue() != ERROR_OK) {
		LOG_ERROR("%s: add_dr_scan failed", __func__);
		return ERROR_FAIL;
	}

	return retval;
}

void arc_jtag_transaction_reset(struct arc_jtag *jtag_info)
{
	/*
	 * run us through transaction reset.
	 * this means that none of the previous settings/commands/etc. are
	 * used anymore (of no influence)
	 */
	jtag_info->tap_end_state = TAP_IRPAUSE;
	arc_jtag_set_instruction(jtag_info, ARC_TRANSACTION_CMD_REG);
	jtag_info->tap_end_state = TAP_IDLE;
	arc_jtag_set_transaction(jtag_info, ARC_JTAG_CMD_NOP);
}

/* ----- Exported JTAG functions ------------------------------------------- */

int arc_jtag_startup(struct arc_jtag *jtag_info)
{
	int retval = ERROR_OK;

	jtag_info->tap_end_state = TAP_IDLE;

	retval = arc_jtag_set_instruction(jtag_info, ARC_TRANSACTION_CMD_REG);
	if (retval != ERROR_OK)
		return retval;
	retval = arc_jtag_set_transaction(jtag_info, ARC_JTAG_CMD_NOP);
	if (retval != ERROR_OK)
		return retval;

#ifdef DEBUG
	uint32_t status;

	retval = arc_jtag_status(jtag_info, &status);
	if (retval != ERROR_OK)
		return retval;
	LOG_USER(" JTAG status: 0x%08" PRIx32, status);
#endif

	return retval;
}

int arc_jtag_shutdown(struct arc_jtag *jtag_info)
{
	int retval = ERROR_OK;

	LOG_WARNING("arc_jtag_shutdown not implemented");

	return retval;
}

int arc_jtag_read_memory(struct arc_jtag *jtag_info, uint32_t addr,
	uint32_t *value)
{
	int retval = ERROR_OK;

	arc_jtag_transaction_reset(jtag_info);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_TRANSACTION_CMD_REG);
	jtag_info->tap_end_state = TAP_DRPAUSE;
	retval = arc_jtag_set_transaction(jtag_info, ARC_JTAG_READ_FROM_MEMORY);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_ADDRESS_REG);
	jtag_info->tap_end_state = TAP_IDLE; /* otherwise 4Byte behind */
	retval = arc_jtag_write_data(jtag_info, addr);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_DATA_REG);
	jtag_info->tap_end_state = TAP_IDLE; /* OK, give us the read */
	retval = arc_jtag_read_data(jtag_info, value);

	arc_jtag_transaction_reset(jtag_info);

	return retval;
}

int arc_jtag_write_memory(struct arc_jtag *jtag_info, uint32_t addr,
	uint32_t *value)
{
	int retval = ERROR_OK;

	arc_jtag_transaction_reset(jtag_info);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_TRANSACTION_CMD_REG);
	jtag_info->tap_end_state = TAP_DRPAUSE;
	retval = arc_jtag_set_transaction(jtag_info, ARC_JTAG_WRITE_TO_MEMORY);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_ADDRESS_REG);
	jtag_info->tap_end_state = TAP_DRPAUSE;
	retval = arc_jtag_write_data(jtag_info, addr);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_DATA_REG);
	jtag_info->tap_end_state = TAP_IDLE; /* OK, give us the write */
	retval = arc_jtag_write_data(jtag_info, *value);

	arc_jtag_transaction_reset(jtag_info);

	return retval;
}

int arc_jtag_read_block(struct arc_jtag *jtag_info, uint32_t addr,
	uint32_t size,  uint32_t count, uint32_t *value)
{
	int retval = ERROR_OK;

	LOG_ERROR("arc_jtag_read_block is not implemented");

	return retval;
}

int arc_jtag_write_block(struct arc_jtag *jtag_info, uint32_t addr,
	uint32_t size, uint32_t count, uint32_t *value)
{
	int retval = ERROR_OK;
	uint32_t i;

	assert(size <= 4); /* 4 = 32bits */

	/* we do not know where we come from */
	arc_jtag_transaction_reset(jtag_info);

	/* get us started with first word */
	jtag_info->tap_end_state = TAP_IRPAUSE;
	arc_jtag_set_instruction(jtag_info, ARC_TRANSACTION_CMD_REG);
	jtag_info->tap_end_state = TAP_DRPAUSE;
	retval = arc_jtag_set_transaction(jtag_info, ARC_JTAG_WRITE_TO_MEMORY);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	arc_jtag_set_instruction(jtag_info, ARC_ADDRESS_REG);
	jtag_info->tap_end_state = TAP_DRPAUSE;
	arc_jtag_write_data(jtag_info, addr);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	arc_jtag_set_instruction(jtag_info, ARC_DATA_REG);
	jtag_info->tap_end_state = TAP_IDLE; /* OK, give us the write */
	arc_jtag_write_data(jtag_info, *value);

	/* remaining data, address increment is taken care of by hw */
	for(i = 1; i < count; i++) {
		jtag_info->tap_end_state = TAP_IRPAUSE;
		arc_jtag_set_instruction(jtag_info, ARC_DATA_REG);
		jtag_info->tap_end_state = TAP_IDLE; /* OK, give us the write */
		arc_jtag_write_data(jtag_info, *(uint32_t *)(value + i));
	}

	arc_jtag_transaction_reset(jtag_info); /* done, cleanup behind you */

	return retval;
}

int arc_jtag_read_core_reg(struct arc_jtag *jtag_info, uint32_t addr,
	uint32_t *value)
{
	int retval = ERROR_OK;

	arc_jtag_transaction_reset(jtag_info);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_TRANSACTION_CMD_REG);
	jtag_info->tap_end_state = TAP_DRPAUSE;
	retval = arc_jtag_set_transaction(jtag_info, ARC_JTAG_READ_FROM_CORE_REG);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_ADDRESS_REG);
	jtag_info->tap_end_state = TAP_IDLE; /* otherwise 1 register behind */
	retval = arc_jtag_write_data(jtag_info, addr);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_DATA_REG);
	jtag_info->tap_end_state = TAP_IDLE; /* OK, give us the read */
	retval = arc_jtag_read_data(jtag_info, value);

	arc_jtag_transaction_reset(jtag_info);

	return retval;
}

int arc_jtag_write_core_reg(struct arc_jtag *jtag_info, uint32_t addr,
	uint32_t *value)
{
	int retval = ERROR_OK;

	arc_jtag_transaction_reset(jtag_info);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_TRANSACTION_CMD_REG);
	jtag_info->tap_end_state = TAP_DRPAUSE;
	retval = arc_jtag_set_transaction(jtag_info, ARC_JTAG_WRITE_TO_CORE_REG);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_ADDRESS_REG);
	jtag_info->tap_end_state = TAP_DRPAUSE;
	retval = arc_jtag_write_data(jtag_info, addr);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_DATA_REG);
	jtag_info->tap_end_state = TAP_IDLE; /* OK, give us the write */
	retval = arc_jtag_write_data(jtag_info, *value);

	arc_jtag_transaction_reset(jtag_info);

	return retval;
}

/**
 * Real all core registers. This is much faster then calling
 * arc_jtag_read_core_reg for each register. Core registers are sequential so
 * there is no need to set register addresses for each - hardware increments
 * them by one each time, so we need to set it only for the first one.
 *
 * @param jtag_info
 * @param values	Array of register values, values must start from R0 and be sequential.
 * @param count		Amount of registers.
 */
int arc_jtag_read_core_reg_bulk(struct arc_jtag *jtag_info, uint32_t *values,
	unsigned int count )
{
	int retval = ERROR_OK;
	unsigned int i;

	arc_jtag_transaction_reset(jtag_info);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_TRANSACTION_CMD_REG);
	jtag_info->tap_end_state = TAP_DRPAUSE;
	retval = arc_jtag_set_transaction(jtag_info, ARC_JTAG_READ_FROM_CORE_REG);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_ADDRESS_REG);
	jtag_info->tap_end_state = TAP_IDLE; /* otherwise 1 register behind */
	retval = arc_jtag_write_data(jtag_info, 0);

	for (i = 0; i < count; i++ ) {
		jtag_info->tap_end_state = TAP_IRPAUSE;
		retval = arc_jtag_set_instruction(jtag_info, ARC_DATA_REG);
		jtag_info->tap_end_state = TAP_IDLE; /* OK, give us the read */
		retval = arc_jtag_read_data(jtag_info, values + i);
	}

	arc_jtag_transaction_reset(jtag_info);

	return retval;
}

/**
 * Write all core registers. This is much faster then calling
 * arc_jtag_write_core_reg for each register. Core registers are sequential so
 * there is no need to set register addresses for each - hardware increments
 * them by one each time, so we need to set it only for the first one.
 *
 * @param jtag_info
 * @param values	Array of register values, values must start from R0 and be sequential.
 * @param count		Amount of registers.
 */
int arc_jtag_write_core_reg_bulk(struct arc_jtag *jtag_info, uint32_t *value,
	unsigned int count)
{
	int retval = ERROR_OK;
	unsigned int i;

	arc_jtag_transaction_reset(jtag_info);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_TRANSACTION_CMD_REG);
	jtag_info->tap_end_state = TAP_DRPAUSE;
	retval = arc_jtag_set_transaction(jtag_info, ARC_JTAG_WRITE_TO_CORE_REG);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_ADDRESS_REG);
	jtag_info->tap_end_state = TAP_DRPAUSE;
	retval = arc_jtag_write_data(jtag_info, 0);

	for (i = 0; i < count; i++) {
		jtag_info->tap_end_state = TAP_IRPAUSE;
		retval = arc_jtag_set_instruction(jtag_info, ARC_DATA_REG);
		jtag_info->tap_end_state = TAP_IDLE; /* OK, give us the write */
		retval = arc_jtag_write_data(jtag_info, *(value + i) );
	}

	arc_jtag_transaction_reset(jtag_info);

	return retval;
}

int arc_jtag_read_aux_reg(struct arc_jtag *jtag_info, uint32_t addr,
	uint32_t *value)
{
	int retval = ERROR_OK;

	arc_jtag_transaction_reset(jtag_info);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_TRANSACTION_CMD_REG);
	jtag_info->tap_end_state = TAP_DRPAUSE;
	retval = arc_jtag_set_transaction(jtag_info, ARC_JTAG_READ_FROM_AUX_REG);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_ADDRESS_REG);
	jtag_info->tap_end_state = TAP_IDLE; /* otherwise 1 register behind */
	retval = arc_jtag_write_data(jtag_info, addr);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_DATA_REG);
	jtag_info->tap_end_state = TAP_IDLE; /* OK, give us the read */
	retval = arc_jtag_read_data(jtag_info, value);

	arc_jtag_transaction_reset(jtag_info);

	return retval;
}

int arc_jtag_write_aux_reg(struct arc_jtag *jtag_info, uint32_t addr,
	uint32_t *value)
{
	int retval = ERROR_OK;

	arc_jtag_transaction_reset(jtag_info);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_TRANSACTION_CMD_REG);
	jtag_info->tap_end_state = TAP_DRPAUSE;
	retval = arc_jtag_set_transaction(jtag_info, ARC_JTAG_WRITE_TO_AUX_REG);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_ADDRESS_REG);
	jtag_info->tap_end_state = TAP_DRPAUSE;
	retval = arc_jtag_write_data(jtag_info, addr);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_DATA_REG);
	jtag_info->tap_end_state = TAP_IDLE; /* OK, give us the write */
	retval = arc_jtag_write_data(jtag_info, *value);

	arc_jtag_transaction_reset(jtag_info);

	return retval;
}

/**
 * Read all AUX registers. This is much faster then calling
 * arc_jtag_read_aux_reg for each register, however unlike core registers AUX
 * registers are not necessarily sequential and we have to set addresses
 * explicitly.
 *
 * @param jtag_info
 * @param addr		Array of AUX register addresses.
 * @param values	Array of register values. 
 * @param count		Amount of registers in arrays.
 */
int arc_jtag_read_aux_reg_bulk(struct arc_jtag *jtag_info, uint32_t *addr,
	uint32_t *values, unsigned int count)
{
	int retval = ERROR_OK;
	unsigned int i;

	arc_jtag_transaction_reset(jtag_info);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_TRANSACTION_CMD_REG);
	jtag_info->tap_end_state = TAP_DRPAUSE;
	retval = arc_jtag_set_transaction(jtag_info, ARC_JTAG_READ_FROM_AUX_REG);

	for (i = 0; i < count; i++ ) {
		/* Some of AUX registers are sequential, so it is possible to reduce
		 * transaction duration by exploiting this. I've measure that
		 * transaction takes around 200ms less with this patch. */
		if ( i == 0 || (addr[i] != addr[i-1] + 1)) {
			jtag_info->tap_end_state = TAP_IRPAUSE;
			retval = arc_jtag_set_instruction(jtag_info, ARC_ADDRESS_REG);
			jtag_info->tap_end_state = TAP_IDLE;
			retval = arc_jtag_write_data(jtag_info, addr[i]);
		}

		jtag_info->tap_end_state = TAP_IRPAUSE;
		retval = arc_jtag_set_instruction(jtag_info, ARC_DATA_REG);
		jtag_info->tap_end_state = TAP_IDLE; /* OK, give us the read */
		retval = arc_jtag_read_data(jtag_info, values + i);
	}

	arc_jtag_transaction_reset(jtag_info);

	return retval;
}

/**
 * Write all AUX registers. This is much faster then calling
 * arc_jtag_write_aux_reg for each register, however unlike core registers AUX
 * registers are not necessarily sequential and we have to set addresses
 * explicitly.
 *
 * @param jtag_info
 * @param addr		Array of AUX register addresses.
 * @param values	Array of register values. 
 * @param count		Amount of registers in arrays.
 */
int arc_jtag_write_aux_reg_bulk(struct arc_jtag *jtag_info, uint32_t *addr,
	uint32_t *value, unsigned int count)
{
	int retval = ERROR_OK;
	unsigned int i;

	arc_jtag_transaction_reset(jtag_info);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_TRANSACTION_CMD_REG);
	jtag_info->tap_end_state = TAP_DRPAUSE;
	retval = arc_jtag_set_transaction(jtag_info, ARC_JTAG_WRITE_TO_AUX_REG);

	for (i = 0; i < count; i++) { 
		/* Some of AUX registers are sequential, so it is possible to reduce
		 * transaction duration by exploiting this. I've measure that
		 * transaction takes around 200ms less with this patch. */
		if ( i == 0 || (addr[i] != addr[i-1] + 1) ) {
			jtag_info->tap_end_state = TAP_IRPAUSE;
			retval = arc_jtag_set_instruction(jtag_info, ARC_ADDRESS_REG);
			jtag_info->tap_end_state = TAP_DRPAUSE;
			retval = arc_jtag_write_data(jtag_info, addr[i]);
		}

		jtag_info->tap_end_state = TAP_IRPAUSE;
		retval = arc_jtag_set_instruction(jtag_info, ARC_DATA_REG);
		jtag_info->tap_end_state = TAP_IDLE; /* OK, give us the write */
		retval = arc_jtag_write_data(jtag_info, *(value + i)  );
	}

	arc_jtag_transaction_reset(jtag_info);

	return retval;
}

int arc_jtag_status(struct arc_jtag *jtag_info, uint32_t *value)
{
	int retval = ERROR_OK;

	arc_jtag_transaction_reset(jtag_info);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_JTAG_STATUS_REG);
	if (retval != ERROR_OK)
		return retval;
	jtag_info->tap_end_state = TAP_IDLE; /* OK, give us the status */
	retval = arc_jtag_read_data(jtag_info, value);
	if (retval != ERROR_OK)
		return retval;

#ifdef DEBUG
	LOG_DEBUG(" JTAG status: 0x%08" PRIx32, *value);
#endif

	arc_jtag_transaction_reset(jtag_info);

	return retval;
}

int arc_jtag_idcode(struct arc_jtag *jtag_info, uint32_t *value)
{
	int retval = ERROR_OK;

	arc_jtag_transaction_reset(jtag_info);

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_IDCODE_REG);
	if (retval != ERROR_OK)
		return retval;
	jtag_info->tap_end_state = TAP_IDLE; /* OK, give us the idcode */
	retval = arc_jtag_read_data(jtag_info, value);
	if (retval != ERROR_OK)
		return retval;

	arc_jtag_transaction_reset(jtag_info);

	return retval;
}

