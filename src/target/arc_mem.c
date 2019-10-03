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

#include "arc.h"

/* ----- Supporting functions ---------------------------------------------- */

static int arc_mem_read_block(struct target *target, uint32_t addr,
	int size, int count, void *buf)
{
	struct arc32_common *arc32 = target_to_arc32(target);
	int retval = ERROR_OK;
	int i;

	assert(!(addr & 3));
	assert(size == 4);

	/* Always call D$ flush, it will decide whether to perform actual
	 * flush. */
	retval = arc32_dcache_flush(target);
	if (ERROR_OK != retval)
		return retval;

	for (i = 0; i < count; i++) {
		retval = arc_jtag_read_memory(&arc32->jtag_info, addr + (i * 4),
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

	/* No need to flush cache, because we don't read values from memory. */

	for(i = 0; i < count; i++) {
		retval = arc_jtag_write_memory(&arc32->jtag_info,
			addr + i * sizeof(uint32_t) , buf + i * sizeof(uint32_t));
		if (retval != ERROR_OK)
			return retval;
	}

	/* Invalidate caches. */
	retval = arc32_cache_invalidate(target);

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

	/* We will read data from memory, so we need to flush D$. */
	retval = arc32_dcache_flush(target);
	if (ERROR_OK != retval)
		return retval;

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

	/* Invalidate caches. */
	retval = arc32_cache_invalidate(target);

	return retval;
}

/* Write byte at address */
static int arc_mem_write_block8(struct target *target, uint32_t addr, int count,
	void *buf)
{
	struct arc32_common *arc32 = target_to_arc32(target);
	int retval = ERROR_OK;
	int i;

	/* We will read data from memory, so we need to flush D$. */
	retval = arc32_dcache_flush(target);
	if (ERROR_OK != retval)
		return retval;

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

	/* Invalidate caches. */
	retval = arc32_cache_invalidate(target);

	return retval;
}

/* ----- Exported functions ------------------------------------------------ */

int arc_mem_read(struct target *target, uint32_t address, uint32_t size,
	uint32_t count, uint8_t *buffer)
{
	int retval = ERROR_OK;

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
	retval = arc_mem_read_block(target, address & ~3u, sizeof(uint32_t),
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

	LOG_DEBUG("address: 0x%08" PRIx32 ", size: %" PRIu32 ", count: %" PRIu32,
		address, size, count);

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

int arc_mem_checksum(struct target *target, uint32_t address, uint32_t count,
	uint32_t *checksum)
{
	int retval = ERROR_OK;

	LOG_ERROR("arc_mem_checksum NOT SUPPORTED IN THIS RELEASE.");

	return retval;
}

int arc_mem_blank_check(struct target *target, uint32_t address,
	uint32_t count, uint32_t *blank)
{
	int retval = ERROR_OK;

	LOG_ERROR("arc_mem_blank_check NOT SUPPORTED IN THIS RELEASE.");

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

	LOG_ERROR("arc_mem_run_algorithm NOT SUPPORTED IN THIS RELEASE.");

	return retval;
}

int arc_mem_start_algorithm(struct target *target,
	int num_mem_params, struct mem_param *mem_params,
	int num_reg_params, struct reg_param *reg_params,
	uint32_t entry_point, uint32_t exit_point,
	void *arch_info)
{
	int retval = ERROR_OK;

	LOG_ERROR("arc_mem_start_algorithm NOT SUPPORTED IN THIS RELEASE.");

	return retval;
}

int arc_mem_wait_algorithm(struct target *target,
	int num_mem_params, struct mem_param *mem_params,
	int num_reg_params, struct reg_param *reg_params,
	uint32_t exit_point, int timeout_ms,
	void *arch_info)
{
	int retval = ERROR_OK;

	LOG_ERROR("arc_mem_wait_algorithm NOT SUPPORTED IN THIS RELEASE.");

	return retval;
}

/* ......................................................................... */

int arc_mem_virt2phys(struct target *target, uint32_t address,
	uint32_t *physical)
{
	int retval = ERROR_OK;

	LOG_ERROR("arc_mem_virt2phys NOT SUPPORTED IN THIS RELEASE.");

	return retval;
}

int arc_mem_read_phys_memory(struct target *target, uint32_t phys_address,
	uint32_t size, uint32_t count, uint8_t *buffer)
{
	int retval = ERROR_OK;

	LOG_ERROR("arc_mem_read_phys_memory NOT SUPPORTED IN THIS RELEASE.");

	return retval;
}

int arc_mem_write_phys_memory(struct target *target, uint32_t phys_address,
	uint32_t size, uint32_t count, const uint8_t *buffer)
{
	int retval = ERROR_OK;

	LOG_ERROR("arc_mem_write_phys_memory NOT SUPPORTED IN THIS RELEASE.");

	return retval;
}

int arc_mem_mmu(struct target *target, int *enabled)
{
	int retval = ERROR_OK;

	/* (gdb) load command runs through here */

	LOG_DEBUG("arc_mem_mmu NOT SUPPORTED IN THIS RELEASE.");
	LOG_DEBUG("    arc_mem_mmu() = entry point for performance upgrade");

	return retval;
}
