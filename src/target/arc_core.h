/*
 * Copyright (C) 2012 Synopsys, Inc. (www.synopsys.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *   Maintainer: frank.dols@synopsys.com
 */

#ifndef ARC_CORE_H
#define ARC_CORE_H

/* ----- Exported functions ------------------------------------------------ */

int arc_core_soft_reset_halt_imp(struct target *target);
int arc_core_soft_reset_halt(struct target *target);

int arc_core_type_info(struct target *target);

#endif /* ARC_CORE_H */
