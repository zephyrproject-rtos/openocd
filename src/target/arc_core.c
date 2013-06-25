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

/* ----- Exported functions ------------------------------------------------ */

int arc_core_soft_reset_halt(struct target *target)
{
	int retval = ERROR_OK;

	LOG_USER(" > NOT SUPPORTED IN THIS RELEASE.");

	return retval;
}

int arc_core_type_info(struct target *target)
{
	int retval = ERROR_OK;

	struct arc32_common *arc32 = target_to_arc32(target);

	if (strncmp(target_name(target), ARCEM_STR, 6) == 0) {
		arc32->processor_type = ARCEM_NUM;
		LOG_USER("Processor type: %s", ARCEM_STR);

	} else if (strncmp(target_name(target), ARC600_STR, 6) == 0) {
		arc32->processor_type = ARC600_NUM;
		LOG_USER("Processor type: %s", ARC600_STR);

	} else if (strncmp(target_name(target), ARC700_STR, 6) == 0) {
		arc32->processor_type = ARC700_NUM;
		LOG_USER("Processor type: %s", ARC700_STR);

	} else
	LOG_WARNING(" THIS IS A UNSUPPORTED TARGET: %s", target_name(target));

	return retval;
}
