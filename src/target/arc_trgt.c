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



/* ----- Exported functions ------------------------------------------------ */

int arc_trgt_request_data(struct target *target, uint32_t size,
	uint8_t *buffer)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	printf(" > NOT SUPPORTED IN THIS RELEASE.\n");

	return retval;
}
