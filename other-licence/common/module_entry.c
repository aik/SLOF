/******************************************************************************
 * Copyright (c) 2004, 2008 IBM Corporation
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

extern snk_module_t snk_module_interface;

snk_kernel_t *snk_kernel_interface = 0;

extern int
check_driver( pci_config_t *pci_conf );


static void*
memset( void *dest, int c, size_t n )
{
	while( n-- ) {
		*( char * ) dest++ = ( char ) c;
	}
	return dest;
}


extern char __bss_start;
extern char __bss_size;

snk_module_t*
module_init(snk_kernel_t *snk_kernel_int, pci_config_t *pciconf)
{
	/* Need to clear bss, heavy linker script dependency, expert change only */
	char              *bss      = &__bss_start;
	unsigned long long bss_size = (unsigned long long) &__bss_size;

	if (((unsigned long long) bss) + bss_size >= 0xFF00000 
	 || bss_size >= 0x2000000) {
		snk_kernel_int->print("BSS size (%llu bytes) is too big!\n", bss_size);
		return 0;
	}

	memset(bss, 0, bss_size);

	if (snk_kernel_int->version != snk_module_interface.version) {
		return 0;
	}

	snk_kernel_interface = snk_kernel_int;

	/* Check if this is the right driver */
	if (check_driver(pciconf) < 0) {
		return 0;
	}

	snk_module_interface.link_addr = module_init;
	return &snk_module_interface;
}
