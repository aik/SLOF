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

static unsigned long __attribute__((noinline))
call_c(cell arg0, cell arg1, cell arg2, cell entry)
{
    register unsigned long r3 asm("r3") = arg0.u;
    register unsigned long r4 asm("r4") = arg1.u;
    register unsigned long r5 asm("r5") = arg2.u;
    register unsigned long r6 = entry.u         ;

    asm volatile("mflr 31 ; mtctr %4 ; bctrl ; mtlr 31"
            : "=r"(r3)
            : "r"(r3), "r"(r4), "r"(r5), "r"(r6)
            : "ctr", "r31");

    return r3;
}

long
writeLogByte_wrapper(long x, long y)
{
	unsigned long result;
	set_ci();
	result = writeLogByte(x, y);
	clr_ci();
	return result;
}
