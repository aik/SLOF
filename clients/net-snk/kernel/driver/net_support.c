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


#include <pci.h>
#include <net.h>
#include <netdriver_int.h>
#include <kernel.h>
#include <rtas.h>
#include <stdlib.h>
#include <of.h>
#include <types.h>

extern int net_init(pci_config_t * pciconf, char *mac_addr);
extern int net_term(pci_config_t * pciconf);
extern int net_xmit(pci_config_t * pciconf, char *buffer, int len);
extern int net_receive(pci_config_t * pciconf, char *buffer, int len);
extern int net_ioctl(pci_config_t * conf, int request, void *data);
extern int load_module(const char *, pci_config_t *);


static char *mod_names[] = {
	"net_e1000",
	"net_bcm",
	"net_nx203x",
	"net_mcmal",
	"net_spider",
	0
};

net_driver_t *
find_net_driver(pci_config_t pci_conf)
{
	int rc;
	int i = 0;
	static net_driver_t module_net_driver;

	/* Loop over module name */
	while (mod_names[i]) {
		rc = load_module(mod_names[i], &pci_conf);
		if (rc == 0) {
			module_net_driver.net_init = net_init;
			module_net_driver.net_term = net_term;
			module_net_driver.net_transmit = net_xmit;
			module_net_driver.net_receive = net_receive;
			module_net_driver.net_ioctl = net_ioctl;
			return &module_net_driver;
		}
		i++;
	}
	return 0;
}

static phandle_t
get_boot_device(void)
{
	static int nameprinted = 1;
	char buf[1024];
	phandle_t dev = of_finddevice("/chosen");

	if (dev == -1) {
		printk("/chosen not found\n");
		dev = of_finddevice("/aliases");
		if (dev == -1)
			return dev;
		printk("/aliases found, net=%s\n", buf);
		of_getprop(dev, "net", buf, 1024);
	} else
		of_getprop(dev, "bootpath", buf, 1024);

// FIXME: This printk is nice add to "Reading MAC from device" from netboot
// However it is too hackish having "kernel" code finish the application
// output line
	if (!nameprinted++)
		printk("%s: ", buf);

	return of_finddevice(buf);
}

static void
get_mac(char *mac)
{
	phandle_t net = get_boot_device();

	if (net == -1)
		return;

	of_getprop(net, "local-mac-address", mac, 6);
}

void translate_address_dev(uint64_t *, phandle_t);

/**
 * get_puid walks up in the device tree until it finds a parent
 * node without a reg property. get_puid is assuming that if the
 * parent node has no reg property it has found the pci host bridge
 *
 * this is not the correct way to find PHBs but it seems to work
 * for all our systems
 *
 * @param node   the device for which to find the puid
 *
 * @return       the puid or 0
 */
uint64_t
get_puid(phandle_t node)
{
	uint64_t puid = 0;
	uint64_t tmp = 0;
	phandle_t curr_node = of_parent(node);
	if (!curr_node)
		/* no parent found */
		return 0;
	for (;;) {
		puid = tmp;
		if (of_getprop(curr_node, "reg", &tmp, 8) < 8) {
			/* if the found PHB is not directly under
			 * root we need to translate the found address */
			translate_address_dev(&puid, node);
			return puid;
		}
		curr_node = of_parent(curr_node);
		if (!curr_node)
			return 0;
	}
	return 0;
}

/* Fill in the pci config structure from the device tree */
int
set_pci_config(pci_config_t * pci_config)
{
	unsigned char buf[1024];
	int len, bar_nr;
	unsigned int *assigned_ptr;
	phandle_t net = get_boot_device();

	if (net == -1)
		return -1;
	of_getprop(net, "vendor-id", &pci_config->vendor_id, 4);
	of_getprop(net, "device-id", &pci_config->device_id, 4);
	of_getprop(net, "revision-id", &pci_config->revision_id, 4);
	of_getprop(net, "class-code", &pci_config->class_code, 4);
	of_getprop(net, "interrupts", &pci_config->interrupt_line, 4);

	len = of_getprop(net, "assigned-addresses", buf, 400);
	if (len <= 0)
		return -1;
	assigned_ptr = (unsigned int *) &buf[0];
	pci_config->bus = (assigned_ptr[0] & 0x00ff0000) >> 16;
	pci_config->devfn = (assigned_ptr[0] & 0x0000ff00) >> 8;

	while (len > 0) {
		/* Fixme 64 bit bars */
		bar_nr = ((assigned_ptr[0] & 0xff) - 0x10) / 4;
		pci_config->bars[bar_nr].type =
		    (assigned_ptr[0] & 0x0f000000) >> 24;
		pci_config->bars[bar_nr].addr = assigned_ptr[2];
		pci_config->bars[bar_nr].size = assigned_ptr[4];
		assigned_ptr += 5;
		len -= 5 * sizeof(int);
	}

	pci_config->puid = get_puid(net);

	return 0;
}


typedef struct {
	pci_config_t pci_conf;
	net_driver_t net_driv;

	char mac_addr[6];
	int running;
} net_status_t;

static net_status_t net_status;

int
init_net_driver()
{
	int result = 0;
	net_driver_t *net_driver;

	/* Find net device and initialize pci config */
	if (set_pci_config(&net_status.pci_conf) == -1) {
		printk(" No net device found \n");
		return -1;
	}

	/* Get mac address from device tree */
	get_mac(net_status.mac_addr);

	/* Find net device driver */

	net_driver = find_net_driver(net_status.pci_conf);
	if (net_driver == 0) {
		printk(" No net device driver found \n");
		return -1;
	}
	net_status.net_driv = *net_driver;
	/* Init net device driver */
	result = net_status.net_driv.net_init(&net_status.pci_conf,
					      &net_status.mac_addr[0]);
	if (result == -1) {
		net_status.running = 0;
		return -2;
	} else {
		net_status.running = 1;
	}
	return 0;
}


void
term_net_driver()
{
	if (net_status.running == 1)
		net_status.net_driv.net_term(&net_status.pci_conf);
}


int
_socket(int domain, int type, int proto, char *mac_addr)
{
	static int first_time = 1;
	if (first_time) {
		switch (init_net_driver()) {
		case -1:
			return -1;
		case -2:
			return -2;
		default:
			break;
		}
		first_time = 0;
	}

	memcpy(mac_addr, &net_status.mac_addr[0], 6);
	return 0;
}


long
_recv(int fd, void *packet, int packet_len, int flags)
{
	return net_status.net_driv.net_receive(&net_status.pci_conf,
					       packet, packet_len);
}


long
_sendto(int fd, void *packet, int packet_len, int flags,
	void *sock_addr, int sock_addr_len)
{
	return net_status.net_driv.net_transmit(&net_status.pci_conf,
						packet, packet_len);
}

long
_send(int fd, void *packet, int packet_len, int flags)
{
	return net_status.net_driv.net_transmit(&net_status.pci_conf,
						packet, packet_len);
}

long
_ioctl(int fd, int request, void *data)
{
	net_driver_t *net_driver;

	/* Find net device and initialize pci config */
	if (set_pci_config(&net_status.pci_conf) == -1) {
		printk(" No net device found \n");
		return -1;
	}

	/* Find net device driver */

	net_driver = find_net_driver(net_status.pci_conf);
	if (net_driver == 0) {
		printk(" No net device driver found \n");
		return -1;
	}
	net_status.net_driv = *net_driver;

	return net_status.net_driv.net_ioctl(&net_status.pci_conf,
					     request, data);
}


void *
malloc_aligned(size_t size, int align)
{
	unsigned long p = (unsigned long) malloc(size + align - 1);
	p = p + align - 1;
	p = p & ~(align - 1);

	return (void *) p;
}

#define CONFIG_SPACE 0
#define IO_SPACE 1
#define MEM_SPACE 2

#define ASSIGNED_ADDRESS_PROPERTY 0
#define REG_PROPERTY 1

#define DEBUG_TRANSLATE_ADDRESS 0
#if DEBUG_TRANSLATE_ADDRESS != 0
#define DEBUG_TR(str...) printk(str)
#else
#define DEBUG_TR(str...)
#endif

/**
 * pci_address_type tries to find the type for which a
 * mapping should be done. This is PCI specific and is done by
 * looking at the first 32bit of the phys-addr in
 * assigned-addresses
 *
 * @param node     the node of the device which requests
 *                 translatation
 * @param address  the address which needs to be translated
 * @param prop_type the type of the property to search in (either REG_PROPERTY or ASSIGNED_ADDRESS_PROPERTY)
 * @return         the corresponding type (config, i/o, mem)
 */
static int
pci_address_type(phandle_t node, uint64_t address, uint8_t prop_type)
{
	char *prop_name = "assigned-addresses";
	if (prop_type == REG_PROPERTY)
		prop_name = "reg";
	/* #address-cells */
	const unsigned int nac = 3;	//PCI
	/* #size-cells */
	const unsigned int nsc = 2;	//PCI
	/* up to 11 pairs of (phys-addr(3) size(2)) */
	unsigned char buf[11 * (nac + nsc) * sizeof(int)];
	unsigned int *assigned_ptr;
	int result = -1;
	int len;
	len = of_getprop(node, prop_name, buf, 11 * (nac + nsc) * sizeof(int));
	assigned_ptr = (unsigned int *) &buf[0];
	while (len > 0) {
		if ((prop_type == REG_PROPERTY)
		    && ((assigned_ptr[0] & 0xFF) != 0)) {
			//BARs and Expansion ROM must be in assigned-addresses... so in reg
			// we only look for those without config space offset set...
			assigned_ptr += (nac + nsc);
			len -= (nac + nsc) * sizeof(int);
			continue;
		}
		DEBUG_TR("%s %x size %x\n", prop_name, assigned_ptr[2],
			 assigned_ptr[4]);
		if (address >= assigned_ptr[2]
		    && address <= assigned_ptr[2] + assigned_ptr[4]) {
			DEBUG_TR("found a match\n");
			result = (assigned_ptr[0] & 0x03000000) >> 24;
			break;
		}
		assigned_ptr += (nac + nsc);
		len -= (nac + nsc) * sizeof(int);
	}
	/* this can only handle 32bit memory space and should be
	 * removed as soon as translations for 64bit are available */
	return (result == 3) ? MEM_SPACE : result;
}

/**
 * this is a hack which returns the lower 64 bit of any number of cells
 * all the higher bits will silently discarded
 * right now this works pretty good as long 64 bit addresses is all we want
 *
 * @param addr  a pointer to the first address cell
 * @param nc    number of cells addr points to
 * @return      the lower 64 bit to which addr points
 */
static uint64_t
get_dt_address(uint32_t *addr, uint32_t nc)
{
	uint64_t result = 0;
	while (nc--)
		result = (result << 32) | *(addr++);
	return result;
}

/**
 * this functions tries to find a mapping for the given address
 * it assumes that if we have #address-cells == 3 that we are trying
 * to do a PCI translation
 *
 * @param  addr    a pointer to the address that should be translated
 *                 if a translation has been found the address will
 *                 be modified
 * @param  type    this is required for PCI devices to find the
 *                 correct translation
 * @param ranges   this is one "range" containing the translation
 *                 information (one range = nac + pnac + nsc)
 * @param nac      the OF property #address-cells
 * @param nsc      the OF property #size-cells
 * @param pnac     the OF property #address-cells from the parent node
 * @return         -1 if no translation was possible; else 0
 */
static int
map_one_range(uint64_t *addr, int type, uint32_t *ranges, uint32_t nac,
	      uint32_t nsc, uint32_t pnac)
{
	long offset;
	/* cm - child mapping */
	/* pm - parent mapping */
	uint64_t cm, size, pm;
	/* only check for the type if nac == 3 (PCI) */
	DEBUG_TR("type %x, nac %x\n", ranges[0], nac);
	if (((ranges[0] & 0x03000000) >> 24) != type && nac == 3)
		return -1;
	/* okay, it is the same type let's see if we find a mapping */
	size = get_dt_address(ranges + nac + pnac, nsc);
	if (nac == 3)		/* skip type if PCI */
		cm = get_dt_address(ranges + 1, nac - 1);
	else
		cm = get_dt_address(ranges, nac);

	DEBUG_TR("\t\tchild_mapping %lx\n", cm);
	DEBUG_TR("\t\tsize %lx\n", size);
	DEBUG_TR("\t\t*address %lx\n", (uint64_t) * addr);
	if (cm + size <= (uint64_t) * addr || cm > (uint64_t) * addr)
		/* it is not inside the mapping range */
		return -1;
	/* get the offset */
	offset = *addr - cm;
	/* and add the offset on the parent mapping */
	if (pnac == 3)		/* skip type if PCI */
		pm = get_dt_address(ranges + nac + 1, pnac - 1);
	else
		pm = get_dt_address(ranges + nac, pnac);
	DEBUG_TR("\t\tparent_mapping %lx\n", pm);
	*addr = pm + offset;
	DEBUG_TR("\t\t*address %lx\n", *addr);
	return 0;
}

/**
 * translate_address_dev tries to translate the device specific address
 * to a host specific address by walking up in the device tree
 *
 * @param address  a pointer to a 64 bit value which will be
 *                 translated
 * @param current_node phandle of the device from which the
 *                     translation will be started
 */
void
translate_address_dev(uint64_t *addr, phandle_t current_node)
{
	unsigned char buf[1024];
	phandle_t parent;
	unsigned int pnac;
	unsigned int nac;
	unsigned int nsc;
	int addr_type;
	int len;
	unsigned int *ranges;
	unsigned int one_range;
	DEBUG_TR("translate address %lx, node: %lx\n", *addr, current_node);
	of_getprop(current_node, "name", buf, 400);
	DEBUG_TR("current node: %s\n", buf);
	addr_type =
	    pci_address_type(current_node, *addr, ASSIGNED_ADDRESS_PROPERTY);
	if (addr_type == -1) {
		// check in "reg" property if not found in "assigned-addresses"
		addr_type = pci_address_type(current_node, *addr, REG_PROPERTY);
	}
	DEBUG_TR("address_type %x\n", addr_type);
	current_node = of_parent(current_node);
	while (1) {
		parent = of_parent(current_node);
		if (!parent) {
			DEBUG_TR("reached root node...\n");
			break;
		}
		of_getprop(current_node, "#address-cells", &nac, 4);
		of_getprop(current_node, "#size-cells", &nsc, 4);
		of_getprop(parent, "#address-cells", &pnac, 4);
		one_range = nac + pnac + nsc;
		len = of_getprop(current_node, "ranges", buf, 400);
		if (len < 0) {
			DEBUG_TR("no 'ranges' property; not translatable\n");
			return;
		}
		ranges = (unsigned int *) &buf[0];
		while (len > 0) {
			if (!map_one_range
			    ((uint64_t *) addr, addr_type, ranges, nac, nsc,
			     pnac))
				/* after a successful mapping we stop
				 * going through the ranges */
				break;
			ranges += one_range;
			len -= one_range * sizeof(int);
		}
		DEBUG_TR("address %lx\n", *addr);
		of_getprop(current_node, "name", buf, 400);
		DEBUG_TR("current node: %s\n", buf);
		DEBUG_TR("\t#address-cells: %x\n", nac);
		DEBUG_TR("\t#size-cells: %x\n", nsc);
		of_getprop(parent, "name", buf, 400);
		DEBUG_TR("parent node: %s\n", buf);
		DEBUG_TR("\t#address-cells: %x\n", pnac);
		current_node = parent;
	}
}

/**
 * translate_address tries to translate the device specific address
 * of the boot device to a host specific address
 *
 * @param address  a pointer to a 64 bit value which will be
 *                 translated
 */
void
translate_address(uint64_t *addr)
{
	translate_address_dev(addr, get_boot_device());
}
