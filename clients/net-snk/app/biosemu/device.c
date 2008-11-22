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


#include "device.h"
#include "rtas.h"
#include <stdio.h>
#include <string.h>
#include "debug.h"

typedef struct {
	uint8_t info;
	uint8_t bus;
	uint8_t devfn;
	uint8_t cfg_space_offset;
	uint64_t address;
	uint64_t size;
} __attribute__ ((__packed__)) assigned_address_t;

// use translate_address_dev and get_puid from net-snk's net_support.c
void translate_address_dev(uint64_t *, phandle_t);
uint64_t get_puid(phandle_t node);


// scan all adresses assigned to the device ("assigned-addresses" and "reg")
// store in translate_address_array for faster translation using dev_translate_address
void
dev_get_addr_info()
{
	// get bus/dev/fn from assigned-addresses
	int32_t len;
	//max. 6 BARs and 1 Exp.ROM plus CfgSpace and 3 legacy ranges
	assigned_address_t buf[11];
	len =
	    of_getprop(bios_device.phandle, "assigned-addresses", buf,
		       sizeof(buf));
	bios_device.bus = buf[0].bus;
	bios_device.devfn = buf[0].devfn;
	DEBUG_PRINTF("bus: %x, devfn: %x\n", bios_device.bus,
		     bios_device.devfn);
	//store address translations for all assigned-addresses and regs in
	//translate_address_array for faster translation later on...
	int i = 0;
	// index to insert data into translate_address_array
	int taa_index = 0;
	uint64_t address_offset;
	for (i = 0; i < (len / sizeof(assigned_address_t)); i++, taa_index++) {
		//copy all info stored in assigned-addresses
		translate_address_array[taa_index].info = buf[i].info;
		translate_address_array[taa_index].bus = buf[i].bus;
		translate_address_array[taa_index].devfn = buf[i].devfn;
		translate_address_array[taa_index].cfg_space_offset =
		    buf[i].cfg_space_offset;
		translate_address_array[taa_index].address = buf[i].address;
		translate_address_array[taa_index].size = buf[i].size;
		// translate first address and store it as address_offset
		address_offset = buf[i].address;
		translate_address_dev(&address_offset, bios_device.phandle);
		translate_address_array[taa_index].address_offset =
		    address_offset - buf[i].address;
	}
	//get "reg" property
	len = of_getprop(bios_device.phandle, "reg", buf, sizeof(buf));
	for (i = 0; i < (len / sizeof(assigned_address_t)); i++) {
		if ((buf[i].size == 0) || (buf[i].cfg_space_offset != 0)) {
			// we dont care for ranges with size 0 and
			// BARs and Expansion ROM must be in assigned-addresses... so in reg
			// we only look for those without config space offset set...
			// i.e. the legacy ranges
			continue;
		}
		//copy all info stored in assigned-addresses
		translate_address_array[taa_index].info = buf[i].info;
		translate_address_array[taa_index].bus = buf[i].bus;
		translate_address_array[taa_index].devfn = buf[i].devfn;
		translate_address_array[taa_index].cfg_space_offset =
		    buf[i].cfg_space_offset;
		translate_address_array[taa_index].address = buf[i].address;
		translate_address_array[taa_index].size = buf[i].size;
		// translate first address and store it as address_offset
		address_offset = buf[i].address;
		translate_address_dev(&address_offset, bios_device.phandle);
		translate_address_array[taa_index].address_offset =
		    address_offset - buf[i].address;
		taa_index++;
	}
	// store last entry index of translate_address_array
	taa_last_entry = taa_index - 1;
#ifdef DEBUG
	//dump translate_address_array
	printf("translate_address_array: \n");
	translate_address_t ta;
	for (i = 0; i <= taa_last_entry; i++) {
		ta = translate_address_array[i];
		printf
		    ("%d: %02x%02x%02x%02x\n\taddr: %016llx\n\toffs: %016llx\n\tsize: %016llx\n",
		     i, ta.info, ta.bus, ta.devfn, ta.cfg_space_offset,
		     ta.address, ta.address_offset, ta.size);
	}
#endif
}

// to simulate accesses to legacy VGA Memory (0xA0000-0xBFFFF)
// we look for the first prefetchable memory BAR, if no prefetchable BAR found,
// we use the first memory BAR
// dev_translate_addr will translate accesses to the legacy VGA Memory into the found vmem BAR
void
dev_find_vmem_addr()
{
	int i = 0;
	translate_address_t ta;
	int8_t tai_np = -1, tai_p = -1;	// translate_address_array index for non-prefetchable and prefetchable memory
	//search backwards to find first entry
	for (i = taa_last_entry; i >= 0; i--) {
		ta = translate_address_array[i];
		if ((ta.cfg_space_offset >= 0x10)
		    && (ta.cfg_space_offset <= 0x24)) {
			//only BARs
			if ((ta.info & 0x03) >= 0x02) {
				//32/64bit memory
				tai_np = i;
				if ((ta.info & 0x40) != 0) {
					// prefetchable
					tai_p = i;
				}
			}
		}
	}
	if (tai_p != -1) {
		ta = translate_address_array[tai_p];
		bios_device.vmem_addr = ta.address;
		bios_device.vmem_size = ta.size;
		DEBUG_PRINTF
		    ("%s: Found prefetchable Virtual Legacy Memory BAR: %llx, size: %llx\n",
		     __FUNCTION__, bios_device.vmem_addr,
		     bios_device.vmem_size);
	} else if (tai_np != -1) {
		ta = translate_address_array[tai_np];
		bios_device.vmem_addr = ta.address;
		bios_device.vmem_size = ta.size;
		DEBUG_PRINTF
		    ("%s: Found non-prefetchable Virtual Legacy Memory BAR: %llx, size: %llx",
		     __FUNCTION__, bios_device.vmem_addr,
		     bios_device.vmem_size);
	}
	// disable vmem
	//bios_device.vmem_size = 0;
}

void
dev_get_puid()
{
	// get puid
	bios_device.puid = get_puid(bios_device.phandle);
	DEBUG_PRINTF("puid: 0x%llx\n", bios_device.puid);
}

void
dev_get_device_vendor_id()
{
	uint32_t pci_config_0 =
	    rtas_pci_config_read(bios_device.puid, 4, bios_device.bus,
				 bios_device.devfn, 0x0);
	bios_device.pci_device_id =
	    (uint16_t) ((pci_config_0 & 0xFFFF0000) >> 16);
	bios_device.pci_vendor_id = (uint16_t) (pci_config_0 & 0x0000FFFF);
	DEBUG_PRINTF("PCI Device ID: %04x, PCI Vendor ID: %x\n",
		     bios_device.pci_device_id, bios_device.pci_vendor_id);
}

uint8_t
dev_init(char *device_name)
{
	//init bios_device struct
	memset(&bios_device, 0, sizeof(bios_device));
	bios_device.ihandle = of_open(device_name);
	if (bios_device.ihandle == 0) {
		DEBUG_PRINTF("%s is no valid device!\n", device_name);
		return -1;
	}
	bios_device.phandle = of_finddevice(device_name);
	dev_get_addr_info();
	dev_find_vmem_addr();
	dev_get_puid();
	dev_get_device_vendor_id();
	return 0;
}

// translate address function using translate_address_array assembled
// by dev_get_addr_info... MUCH faster than calling translate_address_dev
// and accessing client interface for every translation...
// returns: 0 if addr not found in translate_address_array, 1 if found.
uint8_t
dev_translate_address(uint64_t * addr)
{
	int i = 0;
	translate_address_t ta;
	//check if it is an access to legacy VGA Mem... if it is, map the address
	//to the vmem BAR and then translate it...
	// (translation info provided by Ben Herrenschmidt)
	// NOTE: the translation seems to only work for NVIDIA cards... but it is needed
	// to make some NVIDIA cards work at all...
	if ((bios_device.vmem_size > 0)
	    && ((*addr >= 0xA0000) && (*addr < 0xB8000))) {
		*addr = (*addr - 0xA0000) * 4 + 2 + bios_device.vmem_addr;
	}
	if ((bios_device.vmem_size > 0)
	    && ((*addr >= 0xB8000) && (*addr < 0xC0000))) {
		uint8_t shift = *addr & 1;
		*addr &= 0xfffffffe;
		*addr = (*addr - 0xB8000) * 4 + shift + bios_device.vmem_addr;
	}
	for (i = 0; i <= taa_last_entry; i++) {
		ta = translate_address_array[i];
		if ((*addr >= ta.address) && (*addr <= (ta.address + ta.size))) {
			*addr += ta.address_offset;
			return 1;
		}
	}
	return 0;
}
