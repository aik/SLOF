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

#ifndef DEVICE_LIB_H
#define DEVICE_LIB_H

#include "types.h"
#include <cpu.h>
#include "of.h"
#include <stdio.h>

typedef struct {
	uint8_t bus;
	uint8_t devfn;
	uint64_t puid;
	phandle_t phandle;
	ihandle_t ihandle;
	// store the address of the BAR that is used to simulate
	// legacy memory accesses
	uint64_t vmem_addr;
	uint64_t vmem_size;
	// used to buffer I/O Accesses, that do not access the I/O Range of the device...
	// 64k might be overkill, but we can buffer all I/O accesses...
	uint8_t io_buffer[64 * 1024];
	uint16_t pci_vendor_id;
	uint16_t pci_device_id;
} device_t;

typedef struct {
	uint8_t info;
	uint8_t bus;
	uint8_t devfn;
	uint8_t cfg_space_offset;
	uint64_t address;
	uint64_t address_offset;
	uint64_t size;
} __attribute__ ((__packed__)) translate_address_t;

// array to store address translations for this
// device. Needed for faster address translation, so
// not every I/O or Memory Access needs to call translate_address_dev
// and access the device tree
// 6 BARs, 1 Exp. ROM, 1 Cfg.Space, and 3 Legacy
// translations are supported... this should be enough for
// most devices... for VGA it is enough anyways...
translate_address_t translate_address_array[11];

// index of last translate_address_array entry
// set by get_dev_addr_info function
uint8_t taa_last_entry;

device_t bios_device;

uint8_t dev_init(char *device_name);

uint8_t dev_translate_address(uint64_t * addr);

/* endianness swap functions for 16 and 32 bit words
 * copied from axon_pciconfig.c
 */
static inline void
out32le(void *addr, uint32_t val)
{
	asm volatile ("stwbrx  %0, 0, %1"::"r" (val), "r"(addr));
}

static inline uint32_t
in32le(void *addr)
{
	uint32_t val;
	asm volatile ("lwbrx  %0, 0, %1":"=r" (val):"r"(addr));
	return val;
}

static inline void
out16le(void *addr, uint16_t val)
{
	asm volatile ("sthbrx  %0, 0, %1"::"r" (val), "r"(addr));
}

static inline uint16_t
in16le(void *addr)
{
	uint16_t val;
	asm volatile ("lhbrx %0, 0, %1":"=r" (val):"r"(addr));
	return val;
}

/* debug function, dumps HID1 and HID4 to detect wether caches are on/off */
static inline void
dumpHID()
{
	uint64_t hid;
	//HID1 = 1009
	__asm__ __volatile__("mfspr %0, 1009":"=r"(hid));
	printf("HID1: %016llx\n", hid);
	//HID4 = 1012
	__asm__ __volatile__("mfspr %0, 1012":"=r"(hid));
	printf("HID4: %016llx\n", hid);
}

#endif
