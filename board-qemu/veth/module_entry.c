/******************************************************************************
 * Copyright (c) 2004, 2011 IBM Corporation
 * All rights reserved.
 * This program and the accompanying materials
 * are made available under the terms of the BSD License
 * which accompanies this distribution, and is available at
 * http://www.opensource.org/licenses/bsd-license.php
 *
 * Contributors:
 *     IBM Corporation - initial implementation
 *****************************************************************************/

#include "netdriver_int.h"

static void* memset( void *dest, int c, size_t n )
{
	while( n-- ) {
		*( char * ) dest++ = ( char ) c;
	}
	return dest;
}

extern char __bss_start;
extern char __bss_size;

extern snk_module_t* veth_module_init(snk_kernel_t *snk_kernel_int,
				      vio_config_t *conf);

snk_module_t* module_init(snk_kernel_t *snk_kernel_int, pci_config_t *pciconf)
{
	char              *bss      = &__bss_start;
	unsigned long long bss_size = (unsigned long long) &__bss_size;
	vio_config_t	  *vioconf  = (vio_config_t *)pciconf;

	if (((unsigned long long) bss) + bss_size >= 0xFF00000 
	 || bss_size >= 0x2000000) {
		snk_kernel_int->print("BSS size (%llu bytes) is too big!\n",
				      bss_size);
		return 0;
	}
	memset(bss, 0, bss_size);

	return veth_module_init(snk_kernel_int, vioconf);
}
