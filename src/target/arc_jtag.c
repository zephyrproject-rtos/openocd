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

#include "arc.h"

/* ----- Supporting functions ---------------------------------------------- */

static int arc_jtag_set_instruction(struct arc_jtag *jtag_info, int new_instr)
{
	int retval = ERROR_OK;
	struct jtag_tap *tap;

#ifdef DEBUG
	LOG_DEBUG("  >>> Calling into <<<");
#endif

	tap = jtag_info->tap;
	if (tap == NULL)
		return ERROR_FAIL;

#ifdef DEBUG
	LOG_INFO(" cur_instr: 0x%x new_instr: 0x%x",
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
			LOG_ERROR("%s: setting new instruction failed", __func__);
			return ERROR_FAIL;
#ifdef DEBUG
		} else {
			LOG_INFO(" set JTAG instruction: 0x%x", new_instr);
#endif
		}
	}

	return retval;
}

static int arc_jtag_set_transaction(struct arc_jtag *jtag_info, int new_trans)
{
	int retval = ERROR_OK;
	struct jtag_tap *tap;

#ifdef DEBUG
	LOG_DEBUG("  >>> Calling into <<<");
#endif

	tap = jtag_info->tap;
	if (tap == NULL)
		return ERROR_FAIL;

#ifdef DEBUG
	LOG_INFO(" in:  cur_trans: 0x%x new_trans: 0x%x",
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
			LOG_ERROR("%s: setting new transaction failed", __func__);
			return ERROR_FAIL;
		} else {
#ifdef DEBUG
			LOG_INFO(" set JTAG transaction: 0x%x\n", new_trans);
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
	LOG_DEBUG("  >>> Calling into <<<");
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
	LOG_DEBUG("  >>> Calling into <<<");
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
	LOG_USER(" JTAG status(@:%d): 0x%x",__LINE__,status);
#endif

	return retval;
}

int arc_jtag_shutdown(struct arc_jtag *jtag_info)
{
	int retval = ERROR_OK;

	LOG_WARNING(" !! @ software to do so :-) !!");

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

	LOG_WARNING(" !! @ software to do so :-) !!");

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
		arc_jtag_set_instruction(jtag_info, ARC_TRANSACTION_CMD_REG);
		jtag_info->tap_end_state = TAP_DRPAUSE;
		arc_jtag_set_transaction(jtag_info, ARC_JTAG_WRITE_TO_MEMORY);
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
	LOG_INFO(" JTAG status: 0x%x",*value);
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

/* ......................................................................... */

int arc_ocd_start_test(struct arc_jtag *jtag_info, int reg, uint32_t bits)
{
	int retval = ERROR_OK;
	uint32_t value;

	printf("\n >> Start our tests: <<\n\n");

	printf("\t...\n");
	printf(" # SET_CORE_SINGLE_STEP       : %u\n",SET_CORE_SINGLE_STEP);
	printf(" # SET_CORE_FORCE_HALT        : %u\n",SET_CORE_FORCE_HALT);
	printf(" # SET_CORE_SINGLE_INSTR_STEP : %u\n",SET_CORE_SINGLE_INSTR_STEP);
	printf(" # SET_CORE_RESET_APPLIED     : %u\n",SET_CORE_RESET_APPLIED);
	printf(" # SET_CORE_SLEEP_MODE        : %u\n",SET_CORE_SLEEP_MODE);
	printf(" # SET_CORE_USER_BREAKPOINT   : %u\n",SET_CORE_USER_BREAKPOINT);
	printf(" # SET_CORE_BREAKPOINT_HALT   : %u\n",SET_CORE_BREAKPOINT_HALT);
	printf(" # SET_CORE_SELF_HALT         : %u\n",SET_CORE_SELF_HALT);
	printf(" # SET_CORE_LOAD_PENDING      : %u\n",SET_CORE_LOAD_PENDING);
	printf("\t...\n");

	printf(" !! @ software to do so :-) !!\n\n");

	printf("\n\t...\n");

	arc_jtag_read_core_reg(jtag_info, 22, &value);
	printf(" R22 = 0x%x (?)\n",value);
	arc_jtag_read_core_reg(jtag_info, 9, &value);
	printf(" R9 = 0x%x (?)\n",value);
	arc_jtag_read_core_reg(jtag_info, 63, &value);
	printf(" R63(PC) = 0x%x (?)\n",value);

	value = 8;
	arc_jtag_write_core_reg(jtag_info, 9, &value);
	value = 11;
	arc_jtag_write_core_reg(jtag_info, 22, &value);
	value = 0x8fd00000;
	arc_jtag_write_core_reg(jtag_info, 63, &value);
	printf("\n\t...\n");

	arc_jtag_read_aux_reg(jtag_info, AUX_STATUS_REG, &value);
	printf(" STATUS:           0x%08X\t(@:0x%03X)\n", value, AUX_STATUS_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_SEMAPHORE_REG, &value);
	printf(" SEMAPHORE:        0x%08X\t(@:0x%03X)\n", value, AUX_SEMAPHORE_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_LP_START_REG, &value);
	printf(" LP START:         0x%08X\t(@:0x%03X)\n", value, AUX_LP_START_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_LP_END_REG, &value);
	printf(" LP END:           0x%08X\t(@:0x%03X)\n", value, AUX_LP_END_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_IDENTITY_REG, &value);
	printf(" IDENTITY:         0x%08X\t(@:0x%03X)\n", value, AUX_IDENTITY_REG);

	arc_jtag_read_aux_reg(jtag_info, AUX_DEBUG_REG, &value);
	printf("\n DEBUG:            0x%08X\t(@:0x%03X)\n", value, AUX_DEBUG_REG);
	value = SET_CORE_RESET_APPLIED;
	arc_jtag_write_aux_reg(jtag_info, AUX_DEBUG_REG, &value);
	value = 0;
	arc_jtag_read_aux_reg(jtag_info, AUX_DEBUG_REG, &value);
	printf(" DEBUG:            0x%08X\t(@:0x%03X)\n\n", value, AUX_DEBUG_REG);

	arc_jtag_read_aux_reg(jtag_info, AUX_PC_REG, &value);
	printf(" PC:               0x%08X\t(@:0x%03X)\n", value, AUX_PC_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_STATUS32_REG, &value);
	printf(" STATUS32:         0x%08X\t(@:0x%03X)\n", value, AUX_STATUS32_REG);

	printf("\n\t...\n");
	value = 0x5463728;
	arc_jtag_write_aux_reg(jtag_info, AUX_LP_START_REG, &value);
	value = 0x8765432;
	arc_jtag_write_aux_reg(jtag_info, AUX_LP_END_REG, &value);
	value = 0x8fd00000;
	arc_jtag_write_aux_reg(jtag_info, AUX_PC_REG, &value);
	printf("\n\t...\n");

	arc_jtag_read_aux_reg(jtag_info, AUX_STATUS_REG, &value);
	printf(" STATUS:           0x%08X\t(@:0x%03X)\n", value, AUX_STATUS_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_SEMAPHORE_REG, &value);
	printf(" SEMAPHORE:        0x%08X\t(@:0x%03X)\n", value, AUX_SEMAPHORE_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_LP_START_REG, &value);
	printf(" LP START:         0x%08X\t(@:0x%03X)\n", value, AUX_LP_START_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_LP_END_REG, &value);
	printf(" LP END:           0x%08X\t(@:0x%03X)\n", value, AUX_LP_END_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_IDENTITY_REG, &value);
	printf(" IDENTITY:         0x%08X\t(@:0x%03X)\n", value, AUX_IDENTITY_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_DEBUG_REG, &value);
	printf(" DEBUG:            0x%08X\t(@:0x%03X)\n", value, AUX_DEBUG_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_PC_REG, &value);
	printf(" PC:               0x%08X\t(@:0x%03X)\n", value, AUX_PC_REG);
	arc_jtag_read_aux_reg(jtag_info, AUX_STATUS32_REG, &value);
	printf(" STATUS32:         0x%08X\t(@:0x%03X)\n", value, AUX_STATUS32_REG);

	arc_jtag_read_core_reg(jtag_info, 22, &value);
	printf(" R22 = 0x%x (?=11)\n",value);
	arc_jtag_read_core_reg(jtag_info, 9, &value);
	printf(" R9 = 0x%x (?=8)\n",value);
	arc_jtag_read_core_reg(jtag_info, 63, &value);
	printf(" R63(PC) = 0x%x (?=0x8fd00000)\n",value);

	printf("\n << Done, hit ^C >>\n");
	sleep(10);
		
	arc_jtag_transaction_reset(jtag_info);

	return retval;
}
