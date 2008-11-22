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
#ifndef _BIOSEMU_DEBUG_H_
#define _BIOSEMU_DEBUG_H_

#include <stdio.h>
#include <types.h>

//#define DEBUG_TRACE_X86EMU
//#undef DEBUG_TRACE_X86EMU

//#define DEBUG
#ifdef DEBUG

//#define DEBUG_IO
//#define DEBUG_MEM
//#define DEBUG_INTR
//#define DEBUG_VBE
// define to enable tracing of JMPs in x86emu
//#define DEBUG_JMP

#define DEBUG_PRINTF(_x...) printf(_x)
#else
#define DEBUG_PRINTF(_x...)

#endif				//DEBUG

#ifdef DEBUG_IO
#define DEBUG_PRINTF_IO(_x...) DEBUG_PRINTF("%x:%x ", M.x86.R_CS, M.x86.R_IP); DEBUG_PRINTF(_x)
#else
#define DEBUG_PRINTF_IO(_x...)
#endif

#ifdef DEBUG_MEM
#define DEBUG_PRINTF_MEM(_x...) DEBUG_PRINTF("%x:%x ", M.x86.R_CS, M.x86.R_IP); DEBUG_PRINTF(_x)
#else
#define DEBUG_PRINTF_MEM(_x...)
#endif

#ifdef DEBUG_INTR
#define DEBUG_PRINTF_INTR(_x...) DEBUG_PRINTF("%x:%x ", M.x86.R_CS, M.x86.R_IP); DEBUG_PRINTF(_x)
#else
#define DEBUG_PRINTF_INTR(_x...)
#endif

#ifdef DEBUG_VBE
#define DEBUG_PRINTF_VBE(_x...) DEBUG_PRINTF("%x:%x ", M.x86.R_CS, M.x86.R_IP); DEBUG_PRINTF(_x)
#else
#define DEBUG_PRINTF_VBE(_x...)
#endif

void dump(uint8_t * addr, uint32_t len);

#endif
