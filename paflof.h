// ============================================================================
//  * Copyright (c) 2004, 2005 IBM Corporation
//  * All rights reserved. 
//  * This program and the accompanying materials 
//  * are made available under the terms of the BSD License 
//  * which accompanies this distribution, and is available at
//  * http://www.opensource.org/licenses/bsd-license.php
//  * 
//  * Contributors:
//  *     IBM Corporation - initial implementation
// ============================================================================


//
// Copyright 2002,2003,2004  Segher Boessenkool  <segher@kernel.crashing.org>
//

#include "types.h"

#define TIBSIZE 256
#define POCKETSIZE 256

// Where we put the exception save areas, and the stacks.
// Stacks grow upwards, just like in real life.  You should see my desk.
#define the_exception_frame ((cell *)0x1100000)
#define the_client_frame    ((cell *)0x1100400)
#define the_data_stack      ((cell *)0x1102000)
#define the_return_stack    ((cell *)0x1104000)
#define the_system_stack    ((cell *)0x1106000)

// These buffers are allocated in C code to ease implementation.
#define the_tib     ((cell *)0x1108000)
#define the_pockets ((cell *)0x1109000)

// This is where the run-time data space starts.
#define the_mem ((cell *)0x1200000)


// Some binary blob that is linked in to the image.  Use an ELF file
// for example; we can execute that as a client program, then.
// You could use yaboot or a (small enough) Linux kernel, for example.
extern char _binary_payload_start[];

// Assembler glue routine for switching context between the client
// program and SLOF itself.
extern void client_entry_point();
extern unsigned long call_client(cell);

// Magic function to perform stuff that we don't give source for.
extern type_u oco(cell, cell);

// Synchronize instruction cache with data cache.
extern void flush_cache (void*, long);
