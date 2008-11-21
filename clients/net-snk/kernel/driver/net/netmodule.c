/******************************************************************************
 * Copyright (c) 2004, 2007 IBM Corporation
 * All rights reserved.
 * This program and the accompanying materials
 * are made available under the terms of the BSD License
 * which accompanies this distribution, and is available at
 * http://www.opensource.org/licenses/bsd-license.php
 *
 * Contributors:
 *     IBM Corporation - initial implementation
 *****************************************************************************/

#include <netdriver_int.h>
#include <kernel.h>
#include <of.h>
#include <rtas.h>
#include <cpu.h>

# define MODULE_ADDR 0xF800000

unsigned int read_io(void *, size_t);
int write_io(void *, unsigned int, size_t);
void translate_address(unsigned long *);

static snk_kernel_t snk_kernel_interface = {
	.version          = 1,
	.print            = printk,
	.us_delay         = udelay,
	.ms_delay         = mdelay,
	.pci_config_read  = rtas_pci_config_read,
	.pci_config_write = rtas_pci_config_write,
	.k_malloc_aligned = malloc_aligned,
	.k_romfs_lookup   = romfs_lookup,
	.translate_addr   = translate_address
};

static snk_module_t *snk_module_interface;

typedef snk_module_t *(*module_init_t) (snk_kernel_t *, pci_config_t *);

/* Load module and call init code.
   Init code will check, if module is responsible for device.
   Returns -1, if not responsible for device, 0 otherwise.
*/

int
load_module(const char *name, pci_config_t * pciconf)
{
	int len;
	void *addr;
	module_init_t module_init;

	/* Read module from FLASH */
	len = romfs_lookup(name, &addr);

	if (len <= 0) {
		return -1;
	}
	/* Copy image from flash to RAM
	 * FIXME fix address 8MB
	 */

	memcpy((void *) MODULE_ADDR, addr, len);

	flush_cache((void *) MODULE_ADDR, len);

	/* Module starts with opd structure of the module_init
	 * function.
	 */
	module_init = (module_init_t) MODULE_ADDR;
	snk_module_interface = module_init(&snk_kernel_interface, pciconf);
	if (snk_module_interface == 0)
		return -1;
	return 0;
}

int
net_init(pci_config_t * pciconf, char *mac_addr)
{
	snk_kernel_interface.io_read  = read_io;
	snk_kernel_interface.io_write = write_io;

	if (snk_module_interface->net_init){
		return snk_module_interface->net_init(pciconf, mac_addr);
	}
	else {
		printk("No net_init function available");
		return -1;
	}
}

int
net_term(pci_config_t * pciconf)
{
	if (snk_module_interface->net_term)
		return snk_module_interface->net_term(pciconf);
	else {
		printk("No net_term function available");
		return -1;
	}
}


int
net_xmit(pci_config_t * pciconf, char *buffer, int len)
{
	if (snk_module_interface->net_xmit)
		return snk_module_interface->net_xmit(pciconf, buffer, len);
	else {
		printk("No net_xmit function available");
		return -1;
	}
}

int
net_receive(pci_config_t * pciconf, char *buffer, int len)
{
	if (snk_module_interface->net_receive)
		return snk_module_interface->net_receive(pciconf, buffer, len);
	else {
		printk("No net_receive function available");
		return -1;
	}
}

int
net_ioctl(pci_config_t * pciconf, int request, void* data)
{
	snk_kernel_interface.io_read  = read_io;
	snk_kernel_interface.io_write = write_io;

	if (snk_module_interface->net_ioctl)
		return snk_module_interface->net_ioctl(pciconf, request, data);
	else {
		printk("No net_ioctl function available");
		return -1;
	}
}
