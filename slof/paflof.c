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

#define XSTR(x) #x
#define ISTR(x,y) XSTR(x.y)
#undef unix

#include "paflof.h"
#include ISTR(TARG,h)

void engine(long error)
{
	cell *restrict dp;
	cell *restrict rp;
	cell *restrict ip;
	cell *restrict cfa;
	cell handler_stack[16];

	#include "prim.h"
	#include "dict.xt"

	dp = the_data_stack;
	rp = handler_stack - 1;
	dp->n = error;
	ip = xt_SYSTHROW;

	#include "prim.code"
	#include ISTR(TARG,code)
}
