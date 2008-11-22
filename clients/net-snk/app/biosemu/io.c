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

#include <stdio.h>
#include <cpu.h>
#include "device.h"
#include "rtas.h"
#include "debug.h"
#include "device.h"
#include <types.h>
#include <x86emu/x86emu.h>

// those are defined in net-snk/oflib/pci.c
// currently not used...
//extern unsigned int read_io(void *, size_t);
//extern int write_io(void *, unsigned int, size_t);

// these are not used, only needed for linking,  must be overridden using X86emu_setupPioFuncs
// with the functions and struct below
void
outb(uint8_t val, uint16_t port)
{
	printf("WARNING: outb not implemented!\n");
	HALT_SYS();
}

void
outw(uint16_t val, uint16_t port)
{
	printf("WARNING: outw not implemented!\n");
	HALT_SYS();
}

void
outl(uint32_t val, uint16_t port)
{
	printf("WARNING: outl not implemented!\n");
	HALT_SYS();
}

uint8_t
inb(uint16_t port)
{
	printf("WARNING: inb not implemented!\n");
	HALT_SYS();
	return 0;
}

uint16_t
inw(uint16_t port)
{
	printf("WARNING: inw not implemented!\n");
	HALT_SYS();
	return 0;
}

uint32_t
inl(uint16_t port)
{
	printf("WARNING: inl not implemented!\n");
	HALT_SYS();
	return 0;
}

uint8_t
my_inb(X86EMU_pioAddr addr)
{
	uint64_t translated_addr = addr;
	uint8_t translated = dev_translate_address(&translated_addr);
	if (translated != 0) {
		//translation successfull, access VGA I/O (BAR or Legacy...)
		DEBUG_PRINTF_IO("%s(%x): access to VGA I/O\n", __FUNCTION__,
				addr);
		//DEBUG_PRINTF_IO("%s(%04x): translated_addr: %llx\n", __FUNCTION__, addr, translated_addr);
		set_ci();
		uint8_t rval = *((uint8_t *) translated_addr);
		DEBUG_PRINTF_IO("%s(%04x) VGA I/O --> %02x\n", __FUNCTION__,
				addr, rval);
		clr_ci();
		return rval;
	} else {
		DEBUG_PRINTF_IO("%s(%04x) reading from bios_device.io_buffer\n",
				__FUNCTION__, addr);
		uint8_t rval = *((uint8_t *) (bios_device.io_buffer + addr));
		DEBUG_PRINTF_IO("%s(%04x) I/O Buffer --> %02x\n", __FUNCTION__,
				addr, rval);
		return rval;
	}
}

uint16_t
my_inw(X86EMU_pioAddr addr)
{
	uint64_t translated_addr = addr;
	uint8_t translated = dev_translate_address(&translated_addr);
	if (translated != 0) {
		//translation successfull, access VGA I/O (BAR or Legacy...)
		DEBUG_PRINTF_IO("%s(%x): access to VGA I/O\n", __FUNCTION__,
				addr);
		//DEBUG_PRINTF_IO("%s(%04x): translated_addr: %llx\n", __FUNCTION__, addr, translated_addr);
		uint16_t rval;
		if ((translated_addr & (uint64_t) 0x1) == 0) {
			// 16 bit aligned access...
			set_ci();
			rval = in16le((void *) translated_addr);
			clr_ci();
		} else {
			// unaligned access, read single bytes
			set_ci();
			rval = (*((uint8_t *) translated_addr)) |
			    (*((uint8_t *) translated_addr + 1) << 8);
			clr_ci();
		}
		DEBUG_PRINTF_IO("%s(%04x) VGA I/O --> %04x\n", __FUNCTION__,
				addr, rval);
		return rval;
	} else {
		DEBUG_PRINTF_IO("%s(%04x) reading from bios_device.io_buffer\n",
				__FUNCTION__, addr);
		uint16_t rval = in16le((void *) bios_device.io_buffer + addr);
		DEBUG_PRINTF_IO("%s(%04x) I/O Buffer --> %04x\n", __FUNCTION__,
				addr, rval);
		return rval;
	}
}

uint32_t
my_inl(X86EMU_pioAddr addr)
{
	uint64_t translated_addr = addr;
	uint8_t translated = dev_translate_address(&translated_addr);
	if (translated != 0) {
		//translation successfull, access VGA I/O (BAR or Legacy...)
		DEBUG_PRINTF_IO("%s(%x): access to VGA I/O\n", __FUNCTION__,
				addr);
		//DEBUG_PRINTF_IO("%s(%04x): translated_addr: %llx\n", __FUNCTION__, addr, translated_addr);
		uint32_t rval;
		if ((translated_addr & (uint64_t) 0x2) == 0) {
			// 32 bit aligned access...
			set_ci();
			rval = in32le((void *) translated_addr);
			clr_ci();
		} else {
			// unaligned access, read single bytes
			set_ci();
			rval = (*((uint8_t *) translated_addr)) |
			    (*((uint8_t *) translated_addr + 1) << 8) |
			    (*((uint8_t *) translated_addr + 2) << 16) |
			    (*((uint8_t *) translated_addr + 3) << 24);
			clr_ci();
		}
		DEBUG_PRINTF_IO("%s(%04x) VGA I/O --> %08x\n", __FUNCTION__,
				addr, rval);
		return rval;
	} else if (addr == 0xcfc) {
		// PCI Configuration Mechanism 1 step 1
		// write to 0xCF8, sets bus, device, function and Config Space offset
		// later read from 0xCFC returns the value...
		uint8_t bus, devfn, offs;
		uint32_t port_cf8_val = my_inl(0xcf8);
		if ((port_cf8_val & 0x80000000) != 0) {
			//highest bit enables config space mapping
			bus = (port_cf8_val & 0x00FF0000) >> 16;
			devfn = (port_cf8_val & 0x0000FF00) >> 8;
			offs = (port_cf8_val & 0x000000FF);
			if ((bus != bios_device.bus)
			    || (devfn != bios_device.devfn)) {
				// fail accesses to any device but ours...
				printf
				    ("Config access invalid! bus: %x, devfn: %x, offs: %x\n",
				     bus, devfn, offs);
				HALT_SYS();
				return 0xFFFFFFFF;
			} else {
				DEBUG_PRINTF_IO("%s(%04x) PCI Config Access\n",
						__FUNCTION__, addr);
				uint32_t rval =
				    (uint32_t) rtas_pci_config_read(bios_device.
								    puid, 4,
								    bus, devfn,
								    offs);
				DEBUG_PRINTF_IO
				    ("%s(%04x) PCI Config Access --> 0x%08x\n",
				     __FUNCTION__, addr, rval);
				return rval;
			}
		} else {
			return 0xFFFFFFFF;
		}
	} else {
		DEBUG_PRINTF_IO("%s(%04x) reading from bios_device.io_buffer\n",
				__FUNCTION__, addr);
		uint32_t rval = in32le((void *) bios_device.io_buffer + addr);
		DEBUG_PRINTF_IO("%s(%04x) I/O Buffer --> %08x\n", __FUNCTION__,
				addr, rval);
		return rval;
	}
}

void
my_outb(X86EMU_pioAddr addr, uint8_t val)
{
	uint64_t translated_addr = addr;
	uint8_t translated = dev_translate_address(&translated_addr);
	if (translated != 0) {
		//translation successfull, access VGA I/O (BAR or Legacy...)
		DEBUG_PRINTF_IO("%s(%x, %x): access to VGA I/O\n",
				__FUNCTION__, addr, val);
		//DEBUG_PRINTF_IO("%s(%04x): translated_addr: %llx\n", __FUNCTION__, addr, translated_addr);
		set_ci();
		*((uint8_t *) translated_addr) = val;
		clr_ci();
	} else {
		DEBUG_PRINTF_IO
		    ("%s(%04x,%02x) writing to bios_device.io_buffer\n",
		     __FUNCTION__, addr, val);
		*((uint8_t *) (bios_device.io_buffer + addr)) = val;
	}
}

void
my_outw(X86EMU_pioAddr addr, uint16_t val)
{
	uint64_t translated_addr = addr;
	uint8_t translated = dev_translate_address(&translated_addr);
	if (translated != 0) {
		//translation successfull, access VGA I/O (BAR or Legacy...)
		//DEBUG_PRINTF_IO("%s(%04x): translated_addr: %llx\n", __FUNCTION__, addr, translated_addr);
		if ((translated_addr & (uint64_t) 0x1) == 0) {
			// 16 bit aligned access...
			set_ci();
			out16le((void *) translated_addr, val);
			clr_ci();
		} else {
			// unaligned access, write single bytes
			set_ci();
			*((uint8_t *) translated_addr) =
			    (uint8_t) (val & 0x00FF);
			*((uint8_t *) translated_addr + 1) =
			    (uint8_t) ((val & 0xFF00) >> 8);
			clr_ci();
		}
	} else {
		DEBUG_PRINTF_IO
		    ("%s(%04x,%04x) writing to bios_device.io_buffer\n",
		     __FUNCTION__, addr, val);
		out16le((void *) bios_device.io_buffer + addr, val);
	}
}

void
my_outl(X86EMU_pioAddr addr, uint32_t val)
{
	uint64_t translated_addr = addr;
	uint8_t translated = dev_translate_address(&translated_addr);
	if (translated != 0) {
		//translation successfull, access VGA I/O (BAR or Legacy...)
		//DEBUG_PRINTF_IO("%s(%04x): translated_addr: %llx\n", __FUNCTION__, addr, translated_addr);
		if ((translated_addr & (uint64_t) 0x3) == 0) {
			// 32 bit aligned access...
			set_ci();
			out32le((void *) translated_addr, val);
			clr_ci();
		} else {
			// unaligned access, write single bytes
			set_ci();
			*((uint8_t *) translated_addr) =
			    (uint8_t) (val & 0x000000FF);
			*((uint8_t *) translated_addr + 1) =
			    (uint8_t) ((val & 0x0000FF00) >> 8);
			*((uint8_t *) translated_addr + 2) =
			    (uint8_t) ((val & 0x00FF0000) >> 16);
			*((uint8_t *) translated_addr + 3) =
			    (uint8_t) ((val & 0xFF000000) >> 24);
			clr_ci();
		}
	} else {
		DEBUG_PRINTF_IO
		    ("%s(%04x,%08x) writing to bios_device.io_buffer\n",
		     __FUNCTION__, addr, val);
		out32le((void *) bios_device.io_buffer + addr, val);
	}
}
