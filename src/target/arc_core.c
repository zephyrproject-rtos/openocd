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

int arc_core_soft_reset_halt_imp(struct target *target)
{
	int retval = ERROR_OK;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	printf(" > NOT SUPPORTED IN THIS RELEASE.\n");

	return retval;
}

int arc_core_soft_reset_halt(struct target *target)
{
	int retval = ERROR_OK;
	struct arc32_common *arc32 = target_to_arc32(target);
	struct arc_jtag *jtag_info = &arc32->jtag_info;

	printf(" >> Entering: %s(%s @ln:%d)\n",__func__,__FILE__,__LINE__);
	LOG_DEBUG(">> Entering <<");

	/*
	 * A jump to the Reset vector, a soft reset, will not pre-set any
	 * of the internal states of the ARCompact based processor.
	 */

	printf(" !! @ software to do so :-) !!\n");

	printf("\n CALL INTO TEST PROCEDURES FOR NOW !! \n\n");
	arc_ocd_start_test(jtag_info,0,0);

	return retval;
}
