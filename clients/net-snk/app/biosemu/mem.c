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
#include <types.h>
#include <cpu.h>
#include "debug.h"
#include "device.h"
#include "x86emu/x86emu.h"

// read byte from memory
uint8_t
my_rdb(uint32_t addr)
{
	uint64_t translated_addr = addr;
	uint8_t translated = dev_translate_address(&translated_addr);
	if (translated != 0) {
		//translation successfull, access VGA Memory (BAR or Legacy...)
		DEBUG_PRINTF_MEM("%s(%08x): access to VGA Memory\n",
				 __FUNCTION__, addr);
		//DEBUG_PRINTF_MEM("%s(%08x): translated_addr: %llx\n", __FUNCTION__, addr, translated_addr);
		set_ci();
		uint8_t rval = *((uint8_t *) translated_addr);
		clr_ci();
		DEBUG_PRINTF_MEM("%s(%08x) VGA --> %02x\n", __FUNCTION__, addr,
				 rval);
		return rval;
	} else if (addr > M.mem_size) {
		DEBUG_PRINTF("%s(%08x): Memory Access out of range!\n",
			     __FUNCTION__, addr);
		//disassemble_forward(M.x86.saved_cs, M.x86.saved_ip, 1);
		HALT_SYS();
	} else {
		/* read from virtual memory */
		return *((uint8_t *) (M.mem_base + addr));
	}
	// never reached
	return -1;
}

//read word from memory
uint16_t
my_rdw(uint32_t addr)
{
	uint64_t translated_addr = addr;
	uint8_t translated = dev_translate_address(&translated_addr);
	if (translated != 0) {
		//translation successfull, access VGA Memory (BAR or Legacy...)
		DEBUG_PRINTF_MEM("%s(%08x): access to VGA Memory\n",
				 __FUNCTION__, addr);
		//DEBUG_PRINTF_MEM("%s(%08x): translated_addr: %llx\n", __FUNCTION__, addr, translated_addr);
		uint16_t rval;
		// check for legacy memory, because of the remapping to BARs, the reads must
		// be byte reads...
		if ((addr >= 0xa0000) && (addr < 0xc0000)) {
			//read bytes a using my_rdb, because of the remapping to BARs
			//words may not be contiguous in memory, so we need to translate
			//every address...
			rval = ((uint8_t) my_rdb(addr)) |
			    (((uint8_t) my_rdb(addr + 1)) << 8);
		} else {
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
		}
		DEBUG_PRINTF_MEM("%s(%08x) VGA --> %04x\n", __FUNCTION__, addr,
				 rval);
		return rval;
	} else if (addr > M.mem_size) {
		DEBUG_PRINTF("%s(%08x): Memory Access out of range!\n",
			     __FUNCTION__, addr);
		//disassemble_forward(M.x86.saved_cs, M.x86.saved_ip, 1);
		HALT_SYS();
	} else {
		/* read from virtual memory */
		return in16le((void *) (M.mem_base + addr));
	}
	// never reached
	return -1;
}

//read long from memory
uint32_t
my_rdl(uint32_t addr)
{
	uint64_t translated_addr = addr;
	uint8_t translated = dev_translate_address(&translated_addr);
	if (translated != 0) {
		//translation successfull, access VGA Memory (BAR or Legacy...)
		DEBUG_PRINTF_MEM("%s(%x): access to VGA Memory\n",
				 __FUNCTION__, addr);
		//DEBUG_PRINTF_MEM("%s(%08x): translated_addr: %llx\n", __FUNCTION__, addr, translated_addr);
		uint32_t rval;
		// check for legacy memory, because of the remapping to BARs, the reads must
		// be byte reads...
		if ((addr >= 0xa0000) && (addr < 0xc0000)) {
			//read bytes a using my_rdb, because of the remapping to BARs
			//dwords may not be contiguous in memory, so we need to translate
			//every address...
			rval = ((uint8_t) my_rdb(addr)) |
			    (((uint8_t) my_rdb(addr + 1)) << 8) |
			    (((uint8_t) my_rdb(addr + 2)) << 16) |
			    (((uint8_t) my_rdb(addr + 3)) << 24);
		} else {
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
		}
		DEBUG_PRINTF_MEM("%s(%08x) VGA --> %08x\n", __FUNCTION__, addr,
				 rval);
		//HALT_SYS();
		return rval;
	} else if (addr > M.mem_size) {
		DEBUG_PRINTF("%s(%08x): Memory Access out of range!\n",
			     __FUNCTION__, addr);
		//disassemble_forward(M.x86.saved_cs, M.x86.saved_ip, 1);
		HALT_SYS();
	} else {
		/* read from virtual memory */
		return in32le((void *) (M.mem_base + addr));
	}
	// never reached
	return -1;
}

//write byte to memory
void
my_wrb(uint32_t addr, uint8_t val)
{
	uint64_t translated_addr = addr;
	uint8_t translated = dev_translate_address(&translated_addr);
	if (translated != 0) {
		//translation successfull, access VGA Memory (BAR or Legacy...)
		DEBUG_PRINTF_MEM("%s(%x, %x): access to VGA Memory\n",
				 __FUNCTION__, addr, val);
		//DEBUG_PRINTF_MEM("%s(%08x): translated_addr: %llx\n", __FUNCTION__, addr, translated_addr);
		set_ci();
		*((uint8_t *) translated_addr) = val;
		clr_ci();
	} else if (addr > M.mem_size) {
		DEBUG_PRINTF("%s(%08x): Memory Access out of range!\n",
			     __FUNCTION__, addr);
		//disassemble_forward(M.x86.saved_cs, M.x86.saved_ip, 1);
		HALT_SYS();
	} else {
		/* write to virtual memory */
		*((uint8_t *) (M.mem_base + addr)) = val;
	}
}

void
my_wrw(uint32_t addr, uint16_t val)
{
	uint64_t translated_addr = addr;
	uint8_t translated = dev_translate_address(&translated_addr);
	if (translated != 0) {
		//translation successfull, access VGA Memory (BAR or Legacy...)
		DEBUG_PRINTF_MEM("%s(%x, %x): access to VGA Memory\n",
				 __FUNCTION__, addr, val);
		//DEBUG_PRINTF_MEM("%s(%08x): translated_addr: %llx\n", __FUNCTION__, addr, translated_addr);
		// check for legacy memory, because of the remapping to BARs, the reads must
		// be byte reads...
		if ((addr >= 0xa0000) && (addr < 0xc0000)) {
			//read bytes a using my_rdb, because of the remapping to BARs
			//words may not be contiguous in memory, so we need to translate
			//every address...
			my_wrb(addr, (uint8_t) (val & 0x00FF));
			my_wrb(addr + 1, (uint8_t) ((val & 0xFF00) >> 8));
		} else {
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
		}
	} else if (addr > M.mem_size) {
		DEBUG_PRINTF("%s(%08x): Memory Access out of range!\n",
			     __FUNCTION__, addr);
		//disassemble_forward(M.x86.saved_cs, M.x86.saved_ip, 1);
		HALT_SYS();
	} else {
		/* write to virtual memory */
		out16le((void *) (M.mem_base + addr), val);
	}
}
void
my_wrl(uint32_t addr, uint32_t val)
{
	uint64_t translated_addr = addr;
	uint8_t translated = dev_translate_address(&translated_addr);
	if (translated != 0) {
		//translation successfull, access VGA Memory (BAR or Legacy...)
		DEBUG_PRINTF_MEM("%s(%x, %x): access to VGA Memory\n",
				 __FUNCTION__, addr, val);
		//DEBUG_PRINTF_MEM("%s(%08x): translated_addr: %llx\n",  __FUNCTION__, addr, translated_addr);
		// check for legacy memory, because of the remapping to BARs, the reads must
		// be byte reads...
		if ((addr >= 0xa0000) && (addr < 0xc0000)) {
			//read bytes a using my_rdb, because of the remapping to BARs
			//words may not be contiguous in memory, so we need to translate
			//every address...
			my_wrb(addr, (uint8_t) (val & 0x000000FF));
			my_wrb(addr + 1, (uint8_t) ((val & 0x0000FF00) >> 8));
			my_wrb(addr + 2, (uint8_t) ((val & 0x00FF0000) >> 16));
			my_wrb(addr + 3, (uint8_t) ((val & 0xFF000000) >> 24));
		} else {
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
		}
	} else if (addr > M.mem_size) {
		DEBUG_PRINTF("%s(%08x): Memory Access out of range!\n",
			     __FUNCTION__, addr);
		//disassemble_forward(M.x86.saved_cs, M.x86.saved_ip, 1);
		HALT_SYS();
	} else {
		/* write to virtual memory */
		out32le((void *) (M.mem_base + addr), val);
	}
}
