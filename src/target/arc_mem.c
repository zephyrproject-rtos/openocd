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

static int arc_mem_read_block(struct arc_jtag *jtag_info, uint32_t addr,
	int size, int count, void *buf)
{
	int retval = ERROR_OK;
	int i;

	assert(!(addr & 3));
	assert(size == 4);

	for (i = 0; i < count; i++) {
		retval = arc_jtag_read_memory(jtag_info, addr + (i * 4),
			buf + (i * 4));
	}

	return retval;
}

/* Write word at word-aligned address */
static int arc_mem_write_block32(struct target *target, uint32_t addr, int count,
	void *buf)
{
	struct arc32_common *arc32 = target_to_arc32(target);
	int retval = ERROR_OK;
	int i;

	/* Check arguments */
	if (addr & 0x3u)
		return ERROR_TARGET_UNALIGNED_ACCESS;

	for(i = 0; i < count; i++) {
		retval = arc_jtag_write_memory(&arc32->jtag_info,
			addr + i * sizeof(uint32_t) , buf + i * sizeof(uint32_t));
		if (retval != ERROR_OK)
			return retval;
	}

	return retval;
}

/* Write half-word at half-word-aligned address */
static int arc_mem_write_block16(struct target *target, uint32_t addr, int count,
	void *buf)
{
	struct arc32_common *arc32 = target_to_arc32(target);
	int retval = ERROR_OK;
	int i;

	/* Check arguments */
	if (addr & 1u)
		return ERROR_TARGET_UNALIGNED_ACCESS;

	uint32_t buffer_he;
	uint8_t buffer_te[sizeof(uint32_t)];
	uint8_t halfword_te[sizeof(uint16_t)];
	for(i = 0; i < count; i++) {
		/* We can read only word at word-aligned address. Also *jtag_read_memory
		 * functions return data in host endianness, so host endianness !=
		 * target endianness we have to convert data back to target endianness,
		 * or bytes will be at the wrong places.So:
		 *   1) read word
		 *   2) convert to target endianness
		 *   3) make changes
		 *   4) convert back to host endianness
		 *   5) write word back to target.
		 */
		retval = arc_jtag_read_memory(&arc32->jtag_info,
		            (addr + i * sizeof(uint16_t)) & ~3u, &buffer_he);
		target_buffer_set_u32(target, buffer_te, buffer_he);
		/* buf is in host endianness, convert to target */
		target_buffer_set_u16(target, halfword_te,
                        *((uint16_t*)(buf + i * sizeof(uint16_t))));
		memcpy(buffer_te  + ((addr + i * sizeof(uint16_t)) & 3u),
                        halfword_te + i * sizeof(uint16_t), sizeof(uint16_t));
		buffer_he = target_buffer_get_u32(target, buffer_te);
		retval = arc_jtag_write_memory(&arc32->jtag_info,
                        (addr + i * sizeof(uint16_t)) & ~3u, &buffer_he);
	}

	return retval;
}

/* Write byte at address */
static int arc_mem_write_block8(struct target *target, uint32_t addr, int count,
	void *buf)
{
	struct arc32_common *arc32 = target_to_arc32(target);
	int retval = ERROR_OK;
	int i;

	uint32_t buffer_he;
	uint8_t buffer_te[sizeof(uint32_t)];
	for(i = 0; i < count; i++) {
		/* See comment in arc_mem_write_block16 for details. Since it is a byte
		 * there is not need to convert write buffer to target endianness, but
		 * we still have to convert read buffer. */
		retval = arc_jtag_read_memory(&arc32->jtag_info, (addr + i) & ~3, &buffer_he);
		target_buffer_set_u32(target, buffer_te, buffer_he);
		memcpy(buffer_te  + ((addr + i) & 3), (uint8_t*)buf + i, 1);
		buffer_he = target_buffer_get_u32(target, buffer_te);
		retval = arc_jtag_write_memory(&arc32->jtag_info, (addr + i) & ~3, &buffer_he);
	}

	return retval;
}

/* ----- Exported functions ------------------------------------------------ */

int arc_mem_read(struct target *target, uint32_t address, uint32_t size,
	uint32_t count, uint8_t *buffer)
{
	int retval = ERROR_OK;
	struct arc32_common *arc32 = target_to_arc32(target);

	if (target->state != TARGET_HALTED) {
		LOG_WARNING("target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}

	/* Sanitize arguments */
	if (((size != 4) && (size != 2) && (size != 1)) || (count == 0) || !(buffer))
		return ERROR_COMMAND_SYNTAX_ERROR;

	if (((size == 4) && (address & 0x3u)) || ((size == 2) && (address & 0x1u)))
	    return ERROR_TARGET_UNALIGNED_ACCESS;

	void *tunnel_he;
	uint8_t *tunnel_te;
	uint32_t words_to_read, bytes_to_read;

	/* TODO: I think this function might be made much more clear if it
	 * would be splitted onto three based on size (4/2/1). That would
	 * duplicate some logic, but it would be much easier to understand it,
	 * those bit operations are just asking for a trouble. And they emulate
	 * size-specific logic, that is smart, but dangerous.  */
	/* Reads are word-aligned, so padding might be required if count > 1. */
	bytes_to_read = (count * size + 3) & ~3u;
	words_to_read = bytes_to_read >> 2;
	tunnel_he = malloc(bytes_to_read);
	tunnel_te = malloc(bytes_to_read);
	if (!tunnel_he || !tunnel_te) {
		LOG_ERROR("Out of memory");
		return ERROR_FAIL;
	}

	/* We can read only word-aligned words. */
	retval = arc_mem_read_block(&arc32->jtag_info, address & ~3u, sizeof(uint32_t),
		words_to_read, tunnel_he);

	/* arc32_..._read_mem with size 4/2 returns uint32_t/uint16_t in host */
	/* endianness, but byte array should represent target endianness      */

	if (ERROR_OK == retval) {
		switch (size) {
		case 4:
			target_buffer_set_u32_array(target, buffer, count,
				tunnel_he);
			break;
		case 2:
			target_buffer_set_u32_array(target, tunnel_te,
				words_to_read, tunnel_he);
			/* Will that work properly with count > 1 and big endian? */
			memcpy(buffer, tunnel_te + (address & 3u),
				count * sizeof(uint16_t));
			break;
		case 1:
			target_buffer_set_u32_array(target, tunnel_te,
				words_to_read, tunnel_he);
			/* Will that work properly with count > 1 and big endian? */
			memcpy(buffer, tunnel_te + (address & 3u), count);
			break;
		}
	}

	free(tunnel_he);
	free(tunnel_te);

	return retval;
}

int arc_mem_write(struct target *target, uint32_t address, uint32_t size,
	uint32_t count, const uint8_t *buffer)
{
	int retval = ERROR_OK;

	LOG_DEBUG("start writing @ address: 0x%8.8" PRIx32 " : %d times %d bytes",
			address, count, size);
	LOG_DEBUG("address: 0x%8.8" PRIx32 ", size: 0x%8.8" PRIx32 ", count: 0x%8.8"
		PRIx32 "", address, size, count);

	if (target->state != TARGET_HALTED) {
		LOG_WARNING("target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}

	/* sanitize arguments */

	if (((size != 4) && (size != 2) && (size != 1)) || (count == 0) || !(buffer))
		return ERROR_COMMAND_SYNTAX_ERROR;

	if (((size == 4) && (address & 0x3u)) || ((size == 2) && (address & 0x1u)))
		return ERROR_TARGET_UNALIGNED_ACCESS;

	/* correct endianess if we have word or hword access */
	void *tunnel = NULL;

	if (size > 1) {
		/*
		 * arc32_..._write_mem with size 4/2 requires uint32_t/uint16_t
		 * in host endianness, but byte array represents target endianness.
		 */
		tunnel = malloc(count * size * sizeof(uint8_t));
		if (tunnel == NULL) {
			LOG_ERROR("Out of memory");
			return ERROR_FAIL;
		}

		switch (size) {
		case 4:
			target_buffer_get_u32_array(target, buffer, count,
				(uint32_t *)tunnel);
			break;
		case 2:
			target_buffer_get_u16_array(target, buffer, count,
				(uint16_t *)tunnel);
			break;
		}
		buffer = tunnel;
	}

	if (size == 4) {
		retval = arc_mem_write_block32(target, address, count, (void *)buffer);
	} else if (size == 2) {
		/* We convert buffer from host endianness to target. But then in
		 * write_block16, we do the reverse. Is there a way to avoid this without
		 * breaking other cases? */
		retval = arc_mem_write_block16(target, address, count, (void *)buffer);
	} else {
		retval = arc_mem_write_block8(target, address, count, (void *)buffer);
	}

	if (tunnel != NULL)
		free(tunnel);

	return retval;
}

int arc_mem_bulk_write(struct target *target, uint32_t address, uint32_t count,
	const uint8_t *buffer)
{
	int retval = ERROR_OK;
	struct arc32_common *arc32 = target_to_arc32(target);
	struct arc_jtag *jtag_info = &arc32->jtag_info;

	if (target->state != TARGET_HALTED) {
		LOG_WARNING("target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}

	/* check alignment */
	if (address & 0x3u)
		return ERROR_TARGET_UNALIGNED_ACCESS;

	if (arc32->fast_data_area == NULL) {
		/*
		 * Get memory for block write handler
		 * we preserve this area between calls and gain a speed increase
		 * of about 3kb/sec when writing flash
		 * this will be released/nulled by the system when the target is
		 * resumed or reset.
		 */
		retval = target_alloc_working_area(target,
				ARC32_FASTDATA_HANDLER_SIZE,
				&arc32->fast_data_area);

		if (retval != ERROR_OK) {
			LOG_WARNING("No working area available, falling back to non-bulk write");
			return arc_mem_write(target, address, 4, count, buffer);
		} else {
			LOG_DEBUG("ARC32 fastdata handler uses 64Kb buffer.");
			retval = ERROR_OK;
		}

		/* reset fastadata state so the algo get reloaded */
		jtag_info->fast_access_save = -1;
	}

	/* arc32_pracc_fastdata_xfer requires uint32_t in host endianness, */
	/* but byte array represents target endianness                      */
	uint32_t *tunnel = NULL;
	tunnel = malloc(count * sizeof(uint32_t));
	if (tunnel == NULL) {
		LOG_ERROR("Out of memory");
		return ERROR_FAIL;
	}

	target_buffer_get_u32_array(target, buffer, count, tunnel);

	retval = arc_jtag_write_block(jtag_info, address, 4, count,
		(uint32_t *)tunnel);

	if (tunnel != NULL)
		free(tunnel);

	if (retval != ERROR_OK) {
		/* FASTDATA access failed, try normal memory write */
		LOG_DEBUG("Fastdata access Failed, falling back to non-bulk write");
		retval = arc_mem_write(target, address, 4, count, buffer);
	}

	return retval;
}

int arc_mem_checksum(struct target *target, uint32_t address, uint32_t count,
	uint32_t *checksum)
{
	int retval = ERROR_OK;

	LOG_USER(" > NOT SUPPORTED IN THIS RELEASE.");

	return retval;
}

int arc_mem_blank_check(struct target *target, uint32_t address,
	uint32_t count, uint32_t *blank)
{
	int retval = ERROR_OK;

	LOG_USER(" > NOT SUPPORTED IN THIS RELEASE.");

	return retval;
}

/* ......................................................................... */

int arc_mem_run_algorithm(struct target *target,
	int num_mem_params, struct mem_param *mem_params,
	int num_reg_params, struct reg_param *reg_params,
	uint32_t entry_point, uint32_t exit_point,
	int timeout_ms, void *arch_info)
{
	int retval = ERROR_OK;

	LOG_USER(" > NOT SUPPORTED IN THIS RELEASE.");

	return retval;
}

int arc_mem_start_algorithm(struct target *target,
	int num_mem_params, struct mem_param *mem_params,
	int num_reg_params, struct reg_param *reg_params,
	uint32_t entry_point, uint32_t exit_point,
	void *arch_info)
{
	int retval = ERROR_OK;

	LOG_USER(" > NOT SUPPORTED IN THIS RELEASE.");

	return retval;
}

int arc_mem_wait_algorithm(struct target *target,
	int num_mem_params, struct mem_param *mem_params,
	int num_reg_params, struct reg_param *reg_params,
	uint32_t exit_point, int timeout_ms,
	void *arch_info)
{
	int retval = ERROR_OK;

	LOG_USER(" > NOT SUPPORTED IN THIS RELEASE.");

	return retval;
}

/* ......................................................................... */

int arc_mem_virt2phys(struct target *target, uint32_t address,
	uint32_t *physical)
{
	int retval = ERROR_OK;

	LOG_USER(" > NOT SUPPORTED IN THIS RELEASE.");

	return retval;
}

int arc_mem_read_phys_memory(struct target *target, uint32_t phys_address,
	uint32_t size, uint32_t count, uint8_t *buffer)
{
	int retval = ERROR_OK;

	LOG_USER(" > NOT SUPPORTED IN THIS RELEASE.");

	return retval;
}

int arc_mem_write_phys_memory(struct target *target, uint32_t phys_address,
	uint32_t size, uint32_t count, const uint8_t *buffer)
{
	int retval = ERROR_OK;

	LOG_USER(" > NOT SUPPORTED IN THIS RELEASE.");

	return retval;
}

int arc_mem_mmu(struct target *target, int *enabled)
{
	int retval = ERROR_OK;

	/* (gdb) load command runs through here */

	LOG_DEBUG(" > NOT SUPPORTED IN THIS RELEASE.");
	LOG_DEBUG("    arc_mem_mmu() = entry point for performance upgrade");

	return retval;
}
