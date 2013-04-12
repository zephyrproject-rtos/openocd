/*
 * Copyright (C) 2012 Synopsys, Inc. (www.synopsys.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *   Maintainer: frank.dols@synopsys.com
 */

#ifndef ARC_TRGT_H
#define ARC_TRGT_H

/* ----- Exported functions ------------------------------------------------ */

int arc_trgt_request_data(struct target *target, uint32_t size,
	uint8_t *buffer);

#endif /* ARC_TRGT_H */
