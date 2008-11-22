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

#include <rtas.h>

#include "mem.h"
#include "device.h"
#include "debug.h"

#include <x86emu/x86emu.h>
#include <x86emu/prim_ops.h>



//setup to run the code at the address, that the Interrupt Vector points to...
void
setupInt(int intNum)
{
	DEBUG_PRINTF_INTR("%s(%x): executing interrupt handler @%08x\n",
			  __FUNCTION__, intNum, my_rdl(intNum * 4));
	// push current R_FLG... will be popped by IRET
	push_word((u16) M.x86.R_FLG);
	CLEAR_FLAG(F_IF);
	CLEAR_FLAG(F_TF);
	// push current CS:IP to the stack, will be popped by IRET
	push_word(M.x86.R_CS);
	push_word(M.x86.R_IP);
	// set CS:IP to the interrupt handler address... so the next executed instruction will
	// be the interrupt handler
	M.x86.R_CS = my_rdw(intNum * 4 + 2);
	M.x86.R_IP = my_rdw(intNum * 4);
}

// handle int1a (PCI BIOS Interrupt)
void
handleInt1a()
{
	// function number in AX
	uint8_t bus, devfn, offs;
	switch (M.x86.R_AX) {
	case 0xb101:
		// Installation check
		CLEAR_FLAG(F_CF);	// clear CF
		M.x86.R_EDX = 0x20494350;	// " ICP" endian swapped "PCI "
		M.x86.R_AL = 0x1;	// Config Space Mechanism 1 supported
		M.x86.R_BX = 0x0210;	// PCI Interface Level Version 2.10
		M.x86.R_CL = 0xff;	// number of last PCI Bus in system TODO: check!
		break;
	case 0xb102:
		// Find PCI Device
		// NOTE: we currently only allow the device to find itself...
		// it SHOULD be all we ever need...
		// device_id in CX, vendor_id in DX
		DEBUG_PRINTF_INTR("%s(): function: %x: PCI Find Device\n",
				  __FUNCTION__, M.x86.R_AX);
		if ((M.x86.R_CX == bios_device.pci_device_id)
		    && (M.x86.R_DX == bios_device.pci_vendor_id)) {
			CLEAR_FLAG(F_CF);
			M.x86.R_AH = 0x00;	// return code: success
			M.x86.R_BH = bios_device.bus;
			M.x86.R_BL = bios_device.devfn;
			DEBUG_PRINTF_INTR
			    ("%s(): function %x: PCI Find Device --> 0x%04x\n",
			     __FUNCTION__, M.x86.R_AX, M.x86.R_BX);
		} else {
			DEBUG_PRINTF_INTR
			    ("%s(): function %x: invalid device/vendor! (%04x/%04x expected: %04x/%04x) \n",
			     __FUNCTION__, M.x86.R_AX, M.x86.R_CX, M.x86.R_DX,
			     bios_device.pci_device_id,
			     bios_device.pci_vendor_id);
			SET_FLAG(F_CF);
			M.x86.R_AH = 0x86;	// return code: device not found
		}
		break;
	case 0xb108:		//read configuration byte
	case 0xb109:		//read configuration word
	case 0xb10a:		//read configuration dword
		bus = M.x86.R_BH;
		devfn = M.x86.R_BL;
		offs = M.x86.R_DI;
		if ((bus != bios_device.bus)
		    || (devfn != bios_device.devfn)) {
			// fail accesses to any device but ours...
			printf
			    ("%s(): Config read access invalid! bus: %x (%x), devfn: %x (%x), offs: %x\n",
			     __FUNCTION__, bus, bios_device.bus, devfn,
			     bios_device.devfn, offs);
			SET_FLAG(F_CF);
			M.x86.R_AH = 0x87;	//return code: bad pci register
			HALT_SYS();
			return;
		} else {
			switch (M.x86.R_AX) {
			case 0xb108:
				M.x86.R_CL =
				    (uint8_t) rtas_pci_config_read(bios_device.
								   puid, 1,
								   bus, devfn,
								   offs);
				DEBUG_PRINTF_INTR
				    ("%s(): function %x: PCI Config Read @%02x --> 0x%02x\n",
				     __FUNCTION__, M.x86.R_AX, offs,
				     M.x86.R_CL);
				break;
			case 0xb109:
				M.x86.R_CX =
				    (uint16_t) rtas_pci_config_read(bios_device.
								    puid, 2,
								    bus, devfn,
								    offs);
				DEBUG_PRINTF_INTR
				    ("%s(): function %x: PCI Config Read @%02x --> 0x%04x\n",
				     __FUNCTION__, M.x86.R_AX, offs,
				     M.x86.R_CX);
				break;
			case 0xb10a:
				M.x86.R_ECX =
				    (uint32_t) rtas_pci_config_read(bios_device.
								    puid, 4,
								    bus, devfn,
								    offs);
				DEBUG_PRINTF_INTR
				    ("%s(): function %x: PCI Config Read @%02x --> 0x%08x\n",
				     __FUNCTION__, M.x86.R_AX, offs,
				     M.x86.R_ECX);
				break;
			}
			CLEAR_FLAG(F_CF);
			M.x86.R_AH = 0x0;	// return code: success
		}
		break;
	case 0xb10b:		//write configuration byte
	case 0xb10c:		//write configuration word
	case 0xb10d:		//write configuration dword
		bus = M.x86.R_BH;
		devfn = M.x86.R_BL;
		offs = M.x86.R_DI;
		if ((bus != bios_device.bus)
		    || (devfn != bios_device.devfn)) {
			// fail accesses to any device but ours...
			printf
			    ("%s(): Config read access invalid! bus: %x (%x), devfn: %x (%x), offs: %x\n",
			     __FUNCTION__, bus, bios_device.bus, devfn,
			     bios_device.devfn, offs);
			SET_FLAG(F_CF);
			M.x86.R_AH = 0x87;	//return code: bad pci register
			HALT_SYS();
			return;
		} else {
			switch (M.x86.R_AX) {
			case 0xb10b:
				rtas_pci_config_write(bios_device.puid, 1, bus,
						      devfn, offs, M.x86.R_CL);
				DEBUG_PRINTF_INTR
				    ("%s(): function %x: PCI Config Write @%02x <-- 0x%02x\n",
				     __FUNCTION__, M.x86.R_AX, offs,
				     M.x86.R_CL);
				break;
			case 0xb10c:
				rtas_pci_config_write(bios_device.puid, 2, bus,
						      devfn, offs, M.x86.R_CX);
				DEBUG_PRINTF_INTR
				    ("%s(): function %x: PCI Config Write @%02x <-- 0x%04x\n",
				     __FUNCTION__, M.x86.R_AX, offs,
				     M.x86.R_CX);
				break;
			case 0xb10d:
				rtas_pci_config_write(bios_device.puid, 4, bus,
						      devfn, offs, M.x86.R_ECX);
				DEBUG_PRINTF_INTR
				    ("%s(): function %x: PCI Config Write @%02x <-- 0x%08x\n",
				     __FUNCTION__, M.x86.R_AX, offs,
				     M.x86.R_ECX);
				break;
			}
			CLEAR_FLAG(F_CF);
			M.x86.R_AH = 0x0;	// return code: success
		}
		break;
	default:
		DEBUG_PRINTF_INTR
		    ("%s(): unknown function (%x) for int1a handler.\n",
		     __FUNCTION__, M.x86.R_AX);
		DEBUG_PRINTF_INTR("AX=%04x BX=%04x CX=%04x DX=%04x\n",
				  M.x86.R_AX, M.x86.R_BX, M.x86.R_CX,
				  M.x86.R_DX);
		HALT_SYS();
		break;
	}

}

// main Interrupt Handler routine, should be registered as x86emu interrupt handler
void
handleInterrupt(int intNum)
{
	uint8_t int_handled = 0;
	DEBUG_PRINTF_INTR("%s(%x)\n", __FUNCTION__, intNum);
	switch (intNum) {
	case 0x10:		//BIOS video interrupt
	case 0x42:		// INT 10h relocated by EGA/VGA BIOS
	case 0x6d:		// INT 10h relocated by VGA BIOS
		// get interrupt vector from IDT (4 bytes per Interrupt starting at address 0
		if (my_rdl(intNum * 4) == 0xF000F065)	//F000:F065 is default BIOS interrupt handler address
		{
			// default handler called, ignore interrupt...
			DEBUG_PRINTF_INTR
			    ("%s(%x): default interrupt Vector (%08x) found, interrupt ignored...\n",
			     __FUNCTION__, intNum, my_rdl(intNum * 4));
			DEBUG_PRINTF_INTR("AX=%04x BX=%04x CX=%04x DX=%04x\n",
					  M.x86.R_AX, M.x86.R_BX, M.x86.R_CX,
					  M.x86.R_DX);
			//HALT_SYS();
			int_handled = 1;
		}
		break;
	case 0x1a:
		// PCI BIOS Interrupt
		handleInt1a();
		int_handled = 1;
		break;
	default:
		DEBUG_PRINTF_INTR("Interrupt %#x (Vector: %x) not implemented\n", intNum, my_rdl(intNum * 4));	// 4bytes per interrupt vector...
		int_handled = 1;
		HALT_SYS();
		break;
	}
	// if we did not handle the interrupt, jump to the interrupt vector...
	if (!int_handled) {
		setupInt(intNum);
	}
}

// prepare and execute Interrupt 10 (VGA Interrupt)
void
runInt10()
{
	// Initialize stack and data segment
	M.x86.R_SS = 0x0030;
	M.x86.R_DS = 0x0040;
	M.x86.R_SP = 0xfffe;

	// push a HLT instruction and a pointer to it onto the stack
	// any return will pop the pointer and jump to the HLT, thus
	// exiting (more or less) cleanly
	push_word(0xf4f4);	//F4=HLT
	//push_word(M.x86.R_SS);
	//push_word(M.x86.R_SP + 2);

	// setupInt will push the current CS and IP to the stack to return to it,
	// but we want to halt, so set CS:IP to the HLT instruction we just pushed
	// to the stack
	M.x86.R_CS = M.x86.R_SS;
	M.x86.R_IP = M.x86.R_SP;	// + 4;

#ifdef DEBUG_TRACE_X86EMU
	X86EMU_trace_on();
#endif
#ifdef DEBUG_JMP
	M.x86.debug |= DEBUG_TRACEJMP_REGS_F;
	M.x86.debug |= DEBUG_TRACEJMP_REGS_F;
	M.x86.debug |= DEBUG_TRACECALL_F;
	M.x86.debug |= DEBUG_TRACECALL_REGS_F;
#endif

	setupInt(0x10);
	DEBUG_PRINTF_INTR("%s(): starting execution of INT10...\n",
			  __FUNCTION__);
	X86EMU_exec();
	DEBUG_PRINTF_INTR("%s(): execution finished\n", __FUNCTION__);
}
