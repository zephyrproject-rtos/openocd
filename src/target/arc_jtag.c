/*
 * Copyright (C) 2012 Synopsys, Inc. (www.synopsys.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *   Maintainer: frank.dols@synopsys.com
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "target.h"

#include "arc_jtag.h"


/* ----- Supporting functions ---------------------------------------------- */

static int arc_jtag_set_instruction(struct arc_jtag *jtag_info, int new_instr)
{
	int retval = ERROR_OK;
	struct jtag_tap *tap;
	int busy = 0;

	LOG_DEBUG("  >>> Calling into <<<");

	tap = jtag_info->tap;
	if (tap == NULL)
		return ERROR_FAIL;

	LOG_INFO(" cur_instr: 0x%x new_instr: 0x%x",
		buf_get_u32(tap->cur_instr, 0, tap->ir_length), new_instr);

	if (buf_get_u32(tap->cur_instr, 0, tap->ir_length) != (uint32_t)new_instr) {
		do {
			struct scan_field field;
			uint8_t instr[4];
			uint8_t ret[4];

			field.num_bits = tap->ir_length;
			field.in_value = ret;
			buf_set_u32(instr, 0, field.num_bits, new_instr);
			field.out_value = instr;

			jtag_add_ir_scan(tap, &field, jtag_info->tap_end_state);
			if (jtag_execute_queue() != ERROR_OK) {
				LOG_ERROR("%s: setting new instruction failed", __func__);
				return ERROR_FAIL;
			} else {
				LOG_INFO(" set JTAG instruction: 0x%x", new_instr);
			}
			busy = buf_get_u32(ret, 2, 1);
		} while (busy); /* check for busy bit */
	}

	return retval;
}

static int arc_jtag_set_transaction(struct arc_jtag *jtag_info, int new_trans)
{
	int retval = ERROR_OK;
	struct jtag_tap *tap;
	int busy = 0;

	LOG_DEBUG("  >>> Calling into <<<");

	tap = jtag_info->tap;
	if (tap == NULL)
		return ERROR_FAIL;

	LOG_INFO(" in:  cur_trans: 0x%x new_trans: 0x%x",
		jtag_info->cur_trans,new_trans);

	if (jtag_info->cur_trans != (uint32_t)new_trans) {
		do {
			struct scan_field field[1];
			uint8_t trans[4];
			uint8_t ret[4];

			field[0].num_bits = 4;
			field[0].in_value = ret;
			buf_set_u32(trans, 0, field[0].num_bits, new_trans);
			field[0].out_value = trans;

			jtag_add_dr_scan(jtag_info->tap, 1, field, jtag_info->tap_end_state);
			if (jtag_execute_queue() != ERROR_OK) {
				LOG_ERROR("%s: setting new transaction failed", __func__);
				return ERROR_FAIL;
			} else {
				LOG_INFO(" set JTAG transaction: 0x%x\n", new_trans);
				jtag_info->cur_trans = new_trans;
			}
			busy = buf_get_u32(ret, 6, 1);
		} while (busy); /* check for busy bit */
	}

	return retval;
}

static int arc_jtag_read_data(struct arc_jtag *jtag_info, uint32_t *pdata)
{
	int retval = ERROR_OK;
	struct scan_field fields[1];
	uint8_t data_buf[4];
	uint8_t dummy_buf[4];
	int busy = 0;

	LOG_DEBUG("  >>> Calling into <<<");

	do {
		memset(data_buf, 0, sizeof(data_buf));
		memset(dummy_buf, 0, sizeof(dummy_buf));

		fields[0].num_bits = 32;
		fields[0].in_value = data_buf;
		fields[0].out_value = dummy_buf;

		jtag_add_dr_scan(jtag_info->tap, 1, fields, jtag_info->tap_end_state);
		if (jtag_execute_queue() != ERROR_OK) {
			LOG_ERROR("%s: add_dr_scan failed", __func__);
			return ERROR_FAIL;
		}

		busy = buf_get_u32(dummy_buf, 0, 1);
	} while (busy);

	*pdata = buf_get_u32(data_buf, 0, 32);

	return retval;
}

static int arc_jtag_write_data(struct arc_jtag *jtag_info,	uint32_t data)
{
	int retval = ERROR_OK;
	struct scan_field fields[1];
	uint8_t data_buf[4];
	uint8_t dummy_buf[4];
	uint32_t tmpdummy;
	int busy = 0;

	LOG_DEBUG("  >>> Calling into <<<");

	do {
		memset(data_buf, 0, sizeof(data_buf));
		memset(dummy_buf, 0, sizeof(dummy_buf));

		buf_set_u32(data_buf, 0, 32, data);
		buf_set_u32(dummy_buf, 0, 32, tmpdummy);

		fields[0].num_bits = 32;
		fields[0].in_value = dummy_buf;
		fields[0].out_value = data_buf;

		jtag_add_dr_scan(jtag_info->tap, 1, fields, jtag_info->tap_end_state);
		if (jtag_execute_queue() != ERROR_OK) {
			LOG_ERROR("%s: add_dr_scan failed", __func__);
			return ERROR_FAIL;
		}

		busy = buf_get_u32(dummy_buf, 0, 0);
	} while (busy);

	return ERROR_OK;
}


/* ----- Exported JTAG functions ------------------------------------------- */

int arc_jtag_startup(struct arc_jtag *jtag_info)
{
	int retval = ERROR_OK;

	LOG_DEBUG(">> Entering <<");

	jtag_info->tap_end_state = TAP_IDLE;

	retval = arc_jtag_set_instruction(jtag_info, ARC_TRANSACTION_CMD_REG);
	if (retval != ERROR_OK)
		return retval;
	retval = arc_jtag_set_transaction(jtag_info, 0x3); /* = NOP */
	if (retval != ERROR_OK)
		return retval;

#ifdef DEBUG
	uint32_t status;

	retval = arc_jtag_status(jtag_info, &status);
	if (retval != ERROR_OK)
		return retval;
	printf(" JTAG status(@:%d): 0x%x\n",__LINE__,status);
#endif

	return retval;
}

int arc_jtag_shutdown(struct arc_jtag *jtag_info)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	return retval;
}

int arc_jtag_read_memory(struct arc_jtag *jtag_info, uint32_t addr,
	uint32_t *value)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	return retval;
}

int arc_jtag_write_memory(struct arc_jtag *jtag_info, uint32_t addr,
	uint32_t *value)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	return retval;
}

int arc_jtag_read_core_reg(struct arc_jtag *jtag_info, uint32_t addr,
	uint32_t *value)
{
	int retval = ERROR_OK;

	LOG_DEBUG(">> Entering <<");

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_ADDRESS_REG);
	if (retval != ERROR_OK)
		return retval;
	jtag_info->tap_end_state = TAP_DRPAUSE;
	retval = arc_jtag_write_data(jtag_info, addr);
	if (retval != ERROR_OK)
		return retval;

	jtag_info->tap_end_state = TAP_IRPAUSE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_TRANSACTION_CMD_REG);
	if (retval != ERROR_OK)
		return retval;
	jtag_info->tap_end_state = TAP_DRPAUSE;
	retval = arc_jtag_set_transaction(jtag_info, ARC_JTAG_READ_FROM_CORE_REG);
	if (retval != ERROR_OK)
		return retval;

	jtag_info->tap_end_state = TAP_IDLE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_DATA_REG);
	if (retval != ERROR_OK)
		return retval;
	jtag_info->tap_end_state = TAP_IDLE;
	retval = arc_jtag_read_data(jtag_info, value);
	if (retval != ERROR_OK)
		return retval;

	return retval;
}

int arc_jtag_write_core_reg(struct arc_jtag *jtag_info, uint32_t addr,
	uint32_t *value)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	return retval;
}

int arc_jtag_read_aux_reg(struct arc_jtag *jtag_info, uint32_t addr,
	uint32_t *value)
{
	int retval = ERROR_OK;

	LOG_DEBUG(">> Entering <<");

	jtag_info->tap_end_state = TAP_IRPAUSE;

	retval = arc_jtag_set_instruction(jtag_info, ARC_ADDRESS_REG);
	if (retval != ERROR_OK)
		return retval;
	retval = arc_jtag_write_data(jtag_info, addr);
	if (retval != ERROR_OK)
		return retval;
	retval = arc_jtag_set_instruction(jtag_info, ARC_TRANSACTION_CMD_REG);
	if (retval != ERROR_OK)
		return retval;
	retval = arc_jtag_set_transaction(jtag_info, ARC_JTAG_READ_FROM_AUX_REG);
	if (retval != ERROR_OK)
		return retval;

	jtag_info->tap_end_state = TAP_IDLE;

	retval = arc_jtag_set_instruction(jtag_info, ARC_DATA_REG);
	if (retval != ERROR_OK)
		return retval;
	retval = arc_jtag_read_data(jtag_info, value);
	if (retval != ERROR_OK)
		return retval;

	return retval;
}

int arc_jtag_write_aux_reg(struct arc_jtag *jtag_info, uint32_t addr,
	uint32_t *value)
{
	int retval = ERROR_OK;

	LOG_DEBUG(">> Entering <<");

	jtag_info->tap_end_state = TAP_IRPAUSE;

	retval = arc_jtag_set_instruction(jtag_info, ARC_ADDRESS_REG);
	if (retval != ERROR_OK)
		return retval;
	retval = arc_jtag_write_data(jtag_info, addr);
	if (retval != ERROR_OK)
		return retval;
	retval = arc_jtag_set_instruction(jtag_info, ARC_TRANSACTION_CMD_REG);
	if (retval != ERROR_OK)
		return retval;
	retval = arc_jtag_set_transaction(jtag_info, ARC_JTAG_WRITE_TO_AUX_REG);
	if (retval != ERROR_OK)
		return retval;

	jtag_info->tap_end_state = TAP_IDLE;

	retval = arc_jtag_set_instruction(jtag_info, ARC_DATA_REG);
	if (retval != ERROR_OK)
		return retval;
	retval = arc_jtag_write_data(jtag_info, value);
	if (retval != ERROR_OK)
		return retval;

	return retval;
}

int arc_jtag_status(struct arc_jtag *jtag_info, uint32_t *value)
{
	int retval = ERROR_OK;

	LOG_DEBUG(">> Entering <<");

	/* be carefull, makes the whole TAP running */
	jtag_info->tap_end_state = TAP_IDLE;

	retval = arc_jtag_set_instruction(jtag_info, ARC_JTAG_STATUS_REG);
	if (retval != ERROR_OK)
		return retval;
	retval = arc_jtag_read_data(jtag_info, value);
	if (retval != ERROR_OK)
		return retval;

	LOG_INFO(" JTAG status: 0x%x",*value);

	return retval;
}

int arc_jtag_idcode(struct arc_jtag *jtag_info, uint32_t *value)
{
	int retval = ERROR_OK;

	LOG_DEBUG(">> Entering <<");

	jtag_info->tap_end_state = TAP_IDLE;
	retval = arc_jtag_set_instruction(jtag_info, ARC_IDCODE_REG);
	if (retval != ERROR_OK)
		return retval;
	jtag_info->tap_end_state = TAP_IDLE;
	retval = arc_jtag_read_data(jtag_info, value);
	if (retval != ERROR_OK)
		return retval;

	return retval;
}



/* ......................................................................... */

int arc_ocd_start_test(struct arc_jtag *jtag_info, int reg, uint32_t bits)
{
	int retval = ERROR_OK;
	int i;
	uint32_t value, status;

	LOG_DEBUG(">> Entering <<");
	printf("\n >> Start our tests: <<\n\n");

	printf("\t...\n");

	printf("\n << Done, hit ^C >>\n",value);
	sleep(10);
		
	return retval;
}


