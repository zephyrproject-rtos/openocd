/*
 * Copyright (C) 2012 Synopsys, Inc. (www.synopsys.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *   Maintainer: frank.dols@synopsys.com
 */

#ifndef ARC_MEM_H
#define ARC_MEM_H


/* ----- Inlined functions ------------------------------------------------- */



/* ----- Exported functions ------------------------------------------------ */

int arc_mem_read(struct target *target, uint32_t address, uint32_t size,
	uint32_t count, uint8_t *buffer);

int arc_mem_write(struct target *target, uint32_t address, uint32_t size,
	uint32_t count, const uint8_t *buffer);

int arc_mem_bulk_write(struct target *target, uint32_t address, uint32_t count,
	const uint8_t *buffer);

int arc_mem_checksum(struct target *target, uint32_t address, uint32_t count,
	uint32_t *checksum);

int arc_mem_blank_check(struct target *target, uint32_t address,
	uint32_t count, uint32_t *blank);

/* ......................................................................... */

int arc_mem_run_algorithm(struct target *target,
	int num_mem_params, struct mem_param *mem_params,
	int num_reg_params, struct reg_param *reg_params,
	uint32_t entry_point, uint32_t exit_point,
	int timeout_ms, void *arch_info);

int arc_mem_start_algorithm(struct target *target,
	int num_mem_params, struct mem_param *mem_params,
	int num_reg_params, struct reg_param *reg_params,
	uint32_t entry_point, uint32_t exit_point,
	void *arch_info);

int arc_mem_wait_algorithm(struct target *target,
	int num_mem_params, struct mem_param *mem_params,
	int num_reg_params, struct reg_param *reg_params,
	uint32_t exit_point, int timeout_ms,
	void *arch_info);

/* ......................................................................... */

int arc_mem_virt2phys(struct target *target, uint32_t address,
	uint32_t *physical);

int arc_mem_read_phys_memory(struct target *target, uint32_t phys_address,
	uint32_t size, uint32_t count, uint8_t *buffer);

int arc_mem_write_phys_memory(struct target *target, uint32_t phys_address,
	uint32_t size, uint32_t count, const uint8_t *buffer);

int arc_mem_mmu(struct target *target, int *enabled);

#endif /* ARC_MEM_H */
