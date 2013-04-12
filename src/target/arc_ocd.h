/*
 * Copyright (C) 2012 Synopsys, Inc. (www.synopsys.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *   Maintainer: frank.dols@synopsys.com
 */

#ifndef ARC_OCD_H
#define ARC_OCD_H

/* ----- Exported functions ------------------------------------------------ */

int arc_ocd_poll(struct target *target);

/* ......................................................................... */

int arc_ocd_assert_reset(struct target *target);
int arc_ocd_deassert_reset(struct target *target);

/* ......................................................................... */

int arc_ocd_target_create(struct target *target, Jim_Interp *interp);
int arc_ocd_init_target(struct command_context *cmd_ctx, struct target *target);
int arc_ocd_examine(struct target *target);

#endif /* ARC_OCD_H */
