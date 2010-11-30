/******************************************************************************
 * Copyright (c) 2004, 2008 IBM Corporation
 * All rights reserved.
 * This program and the accompanying materials
 * are made available under the terms of the BSD License
 * which accompanies this distribution, and is available at
 * http://www.opensource.org/licenses/bsd-license.php
 *
 * Contributors:
 *     IBM Corporation - initial implementation
 *****************************************************************************/


	.globl call_client_interface
call_client_interface:	
	.globl .call_client_interface
.call_client_interface:
	ld	r4, _prom_entry@got(r2)	# Load prom entry point
	mflr	r0
	ld	r4, 0(r4)
	stdu	r1, -16(r1)
	mtctr	r4
	std	r0, 8(r1)
	bctrl
	ld	r0, 8(r1)
	mtlr	r0
	addi	r1, r1, 16
	blr

	.globl rtas_call_entry
rtas_call_entry:	
	.globl .rtas_call_entry
.rtas_call_entry:
	mtctr	r5
	bctr	


