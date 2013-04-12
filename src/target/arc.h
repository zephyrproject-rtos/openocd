/*
 * Copyright (C) 2012 Synopsys, Inc. (www.synopsys.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *   Maintainer: frank.dols@synopsys.com
 */

#ifndef ARC_H
#define ARC_H

#include <helper/time_support.h>
#include <jtag/jtag.h>

#include "algorithm.h"
#include "breakpoints.h"
#include "jtag/interface.h"
#include "register.h"
#include "target.h"
#include "target_request.h"
#include "target_type.h"

#include "arc32.h"
#include "arc700.h"
#include "arc_core.h"
#include "arc_dbg.h"
#include "arc_jtag.h"
#include "arc_mem.h"
#include "arc_mntr.h"
#include "arc_ocd.h"
#include "arc_regs.h"
#include "arc_trgt.h"

#define ARC_COMMON_MAGIC 0x1A471AC5  /* just a unique number */

struct arc_common {
	int common_magic;
	bool is_4wire;
	struct arc32_common arc32;
};

#endif /* ARC_H */
