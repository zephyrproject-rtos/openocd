/*
 * Copyright (C) 2012 Synopsys, Inc. (www.synopsys.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *   Maintainer: frank.dols@synopsys.com
 */

#ifndef ARC700_H
#define ARC700_H

//#include "target.h"
//#include "arc32.h"
//#include "arc_jtag.h"


#define ARC700_COMMON_MAGIC 0x1A471AC5  /* just a unique number */
#define ARC700_NUM_REGS		32

#define SYSTEM_CONTROL_BASE 0x400FE000  // needs check
#define CPUID				0xE000ED00  // needs check

struct arc700_common {
	int common_magic;
	bool is_4wire; // was:  is_pic32mx; in mips_4k context
	struct arc32_common arc32;
};


#endif /* ARC700_H */
