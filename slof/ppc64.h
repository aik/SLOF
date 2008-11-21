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

#include <cpu.h>
#include "types.h"

#define SET_CI set_ci()
#define CLR_CI clr_ci()

// The big Forth source file that contains everything but the core engine.
// We include it as a hunk of data into the C part of SLOF; at startup
// time, this will be EVALUATE'd.

extern char _slof_start[];
extern char _slof_here_start[];

#define the_exception_frame ((cell *) (_slof_start))
#define the_client_frame ((cell *)   (_slof_start+0x400))
#define the_data_stack ((cell *)     (_slof_start+0x2000))
#define the_return_stack ((cell *)   (_slof_start+0x4000))
#define the_system_stack ((cell *)   (_slof_start+0x6000))

// these two really need to be implemented as a plain
// normal BUFFER: in the data space
#define the_tib ((cell *)            (_slof_start+0x8000))
#define the_pockets ((cell *)        (_slof_start+0x9000))
#define the_comp_buffer ((cell *)    (_slof_start+0xA000))
#define the_client_stack ((cell *)   (_slof_start+0xBf00))

// wasteful, but who cares.  14MB should be enough.
#define the_mem ((cell *)            (_slof_here_start))

#define the_heap_start ((cell *)     (_slof_start+0x700000))
#define the_heap_end   ((cell *)     (_slof_start+0x700000+0x800000))


extern char _binary_OF_fsi_start[], _binary_OF_fsi_end[];
//extern char _binary_vmlinux_start[], _binary_vmlinux_end[];
void client_entry_point();

extern unsigned long call_client(cell);
extern long c_romfs_lookup(long, long, void *);
extern long writeLogByte(long, long);
