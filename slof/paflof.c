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
//
// Copyright 2002,2003,2004  Segher Boessenkool  <segher@kernel.crashing.org>
//


#define XSTR(x) #x
#define ISTR(x,y) XSTR(x.y)
#undef unix

#include "paflof.h"
#include <string.h>
#include ISTR(TARG,h)

#define LAST_ELEMENT(x) x[sizeof x / sizeof x[0] - 1]

#include ISTR(TARG,c)


void engine(long error, long reason)
{
	cell *restrict dp;
	cell *restrict rp;
	cell *restrict ip;
	cell *restrict cfa;
	cell handler_stack[160];

	#include "prep.h"
	#include "dict.xt"

	static int init_lw = 0;
	if (init_lw == 0) {
		init_lw = 1;
		LAST_ELEMENT(xt_FORTH_X2d_WORDLIST).a = xt_LASTWORD;
	}

	dp = the_data_stack;
	rp = handler_stack - 1;
	if (error != 0x100) {
		dp->n = reason;
		dp++;
	}
	dp->n = error;
	ip = xt_SYSTHROW;

	#include "prim.code"
	#include "board.code"
	#include ISTR(TARG,code)
}
