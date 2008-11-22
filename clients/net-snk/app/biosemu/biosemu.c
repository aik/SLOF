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
#include <stdlib.h>
#include <string.h>

#include <types.h>
#include <cpu.h>

#include "debug.h"

#include <x86emu/x86emu.h>
#include <x86emu/regs.h>
#include <x86emu/prim_ops.h>	// for push_word

#include "io.h"
#include "mem.h"
#include "interrupt.h"

#include <rtas.h>

#include "device.h"

static X86EMU_memFuncs my_mem_funcs = {
	my_rdb, my_rdw, my_rdl,
	my_wrb, my_wrw, my_wrl
};

static X86EMU_pioFuncs my_pio_funcs = {
	my_inb, my_inw, my_inl,
	my_outb, my_outw, my_outl
};

void dump(uint8_t * addr, uint32_t len);

uint32_t
biosemu(char argc, char **argv)
{
	uint8_t *rom_image;
	int i = 0;
	int32_t len;
	uint8_t *biosmem;
	uint32_t biosmem_size;
	if (argc < 3) {
		printf("Usage %s <vmem_base> <device_path>\n", argv[0]);
		for (i = 0; i < argc; i++) {
			printf("argv[%d]: %s\n", i, argv[i]);
		}
		return -1;
	}
	// argv[1] is address of virtual BIOS mem... it should be 1MB large...
	biosmem = (uint8_t *) strtoul(argv[1], 0, 16);
	biosmem_size = 0x100000;
	// argv[2] is the device to open and use...
	if (dev_init(argv[2]) != 0) {
		printf("Error initializing device!\n");
		return -1;
	}
	// get expROM address using rtas_pci_config_read
	uint64_t rom_base_addr =
	    rtas_pci_config_read(bios_device.puid, 4, bios_device.bus,
				 bios_device.devfn, 0x30);
	if ((rom_base_addr & 0x1) != 1) {
		printf("Error: invalid Expansion ROM address: 0x%llx!\n",
		       rom_base_addr);
		return -1;
	}
	// unset lowest bit...
	rom_base_addr = rom_base_addr & 0xFFFFFFFE;
	DEBUG_PRINTF("rom_base: %llx\n", rom_base_addr);

	dev_translate_address(&rom_base_addr);
	DEBUG_PRINTF("translated rom_base: %llx\n", rom_base_addr);

	rom_image = (uint8_t *) rom_base_addr;
	DEBUG_PRINTF("executing rom_image from %p\n", rom_image);
	DEBUG_PRINTF("biosmem at %p\n", biosmem);

	// first of all, we need the size (3rd byte)
	set_ci();
	len = *(rom_image + 2);
	clr_ci();
	// size is in 512 byte blocks
	len = len * 512;
	DEBUG_PRINTF("Length: %d\n", len);

	// in case we jump somewhere unexpected, or execution is finished,
	// fill the biosmem with hlt instructions (0xf4)
	memset(biosmem, 0xf4, sizeof(biosmem));

	M.mem_base = (long) biosmem;
	M.mem_size = biosmem_size;
	DEBUG_PRINTF("membase set: %08x, size: %08x\n", (int) M.mem_base,
		     (int) M.mem_size);

	// copy expansion ROM image to segment C000
	// NOTE: this sometimes fails, some bytes are 0x00... so we compare
	// after copying and do some retries...
	uint8_t *vga_img = biosmem + 0xc0000;
	uint8_t copy_count = 0;
	uint8_t cmp_result = 0;
	do {
#if 0
		set_ci();
		memcpy(vga_img, rom_image, len);
		clr_ci();
#else
		// memcpy fails... try copy byte-by-byte with set/clr_ci
		uint8_t c;
		for (i = 0; i < len; i++) {
			set_ci();
			c = *(rom_image + i);
			if (c != *(rom_image + i)) {
				clr_ci();
				printf("Copy failed at: %x/%x\n", i, len);
				printf("rom_image(%x): %x, vga_img(%x): %x\n",
				       i, *(rom_image + i), i, *(vga_img + i));
				break;
			}
			clr_ci();
			*(vga_img + i) = c;
		}
#endif
		copy_count++;
		set_ci();
		cmp_result = memcmp(vga_img, rom_image, len);
		clr_ci();
	}
	while ((copy_count < 5) && (cmp_result != 0));
	if (cmp_result != 0) {
		printf
		    ("\nCopying Expansion ROM Image to Memory failed after %d retries! (%x)\n",
		     copy_count, cmp_result);
		dump(rom_image, 0x20);
		dump(vga_img, 0x20);
		return 0;
	}
	// setup BIOS area
	char *date = "06/11/99";
	for (i = 0; date[i]; i++)
		my_wrb(0xffff5 + i, date[i]);
	/* set up eisa ident string */
	strcpy((char *) (biosmem + 0x0FFD9), "PCI_ISA");

	/* write system model id for IBM-AT */
	*((unsigned char *) (biosmem + 0x0FFFE)) = 0xfc;

	//setup interrupt handler
	X86EMU_intrFuncs intrFuncs[256];
	for (i = 0; i < 256; i++)
		intrFuncs[i] = handleInterrupt;
	X86EMU_setupIntrFuncs(intrFuncs);
	X86EMU_setupPioFuncs(&my_pio_funcs);
	X86EMU_setupMemFuncs(&my_mem_funcs);

	// setup the CPU
	M.x86.R_AH = bios_device.bus;
	M.x86.R_AL = bios_device.devfn;
	M.x86.R_DX = 0x80;
	M.x86.R_EIP = 3;
	M.x86.R_CS = 0xc000;

	// Initialize stack and data segment
	M.x86.R_SS = 0x0030;
	M.x86.R_DS = 0x0040;
	M.x86.R_SP = 0xfffe;

	// push a HLT instruction and a pointer to it onto the stack
	// any return will pop the pointer and jump to the HLT, thus
	// exiting (more or less) cleanly
	push_word(0xf4f4);	//F4=HLT
	push_word(M.x86.R_SS);
	push_word(M.x86.R_SP + 2);

	M.x86.R_ES = 0x0000;
#ifdef DEBUG_TRACE_X86EMU
	X86EMU_trace_on();
	//since we have our own mem and io functions and dont use the x86emu functions,
	// we dont need to enabled the debug...
	//M.x86.debug |= DEBUG_MEM_TRACE_F;
	//M.x86.debug |= DEBUG_IO_TRACE_F;
#endif
#ifdef DEBUG_JMP
	M.x86.debug |= DEBUG_TRACEJMP_F;
	M.x86.debug |= DEBUG_TRACEJMP_REGS_F;
	M.x86.debug |= DEBUG_TRACECALL_F;
	M.x86.debug |= DEBUG_TRACECALL_REGS_F;
#endif
#ifdef DEBUG
	M.x86.debug |= DEBUG_SAVE_IP_CS_F;
	M.x86.debug |= DEBUG_DECODE_F;
	M.x86.debug |= DEBUG_DECODE_NOPRINT_F;
#endif

	DEBUG_PRINTF("Los gehts...\n");
	X86EMU_exec();
	DEBUG_PRINTF("Fertig\n");

	return 0;
}
