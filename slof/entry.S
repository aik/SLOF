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
#include <macros.h>
	
	#
	# The generic exception code.
	#
	# Enter with GPR0 = vector, SPRG0 = saved GPR0
	#

	.section ".entry_text"

the_handler:
	.quad handler

eregs:
	.quad _slof_start  # XXX make configurable at startup time
			# should stay page aligned!


handler:
	mtsprg 1,1	# SPRG1 = saved GPR1
	bcl 20,31,$+4
	mflr 1
	ld 1,eregs-$+4(1)	# GPR1 = address of register save area

	.irp i, 2,3,4,5,6,7,8,9,10,11,12,13,14,15, \
		16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
	std \i,\i*8(1)
	.endr		# save GPR2..GPR31

	mr 3,0		// GPR3 = vector

	mfsprg 0,0
	std 0,0(1)	# save GPR0
	mfsprg 0,1
	std 0,8(1)	# save GPR1

	cmpwi	r3, 0x900	# Decrementer interrupt
	bne	0f
	mfdec	r4		# Save old value of decrementer as reason
	lis	r0,0x7fff	# Set decrementer to highest value 
	mtdec	r0
0:		
	cmpwi	r3, 0x500	# External interrupt
	bne	0f
	LOAD64(r4, 0x20000508408) 
	ld	r4, 0(r4)	# Read destructive interrupt reason
0:		
	mfcr 0
	std 0,0x100(1)
	mfxer 0
	std 0,0x108(1)
	mfsprg 0,3	# save lr	
	std 0,0x110(1)
	mfsprg 0,2	# save ctr	
	std 0,0x118(1)
	mfsrr0 0
	std 0,0x120(1)
	mfsrr1 0
	std 0,0x128(1)
	mfdar 0
	std 0,0x130(1)
	mfdsisr 0
	std 0,0x138(1)	# save special regs

	addi 1,1,0x7000
	li 0,0
	stdu 0,-0x10(1)
	stdu 1,-0x100(1) # set up stack

	lis 2,engine@ha
	ld 0,engine@l(2) # set up entry
	mtsrr0	0
		
	ld 2,8+engine@l(2) # set up TOC pointer

	rfid
#	b .engine	# ...and run!



	#
	# Swap non-volatile client interface regs, plus GPR3..GPR7.
	#

swap_ci_regs:
	lis 8,_slof_start@ha
	addi 8,8,0x0400

	.irp i, 1,2,3,4,5,6,7, \
		13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
	ld 0,\i*8(8)
	std \i,\i*8(8)
	mr \i,0
	.endr		# swap GPR1..7, GPR13..31

	ld 0,0x100(8)
	mfcr 9
	mtcrf 0xff,0
	std 9,0x100(8)	# swap CR

	ld 0,0x128(8)
	mfmsr 9
	mtmsrd 0
	sync
	isync
	std 9,0x128(8)	# swap MSR

	blr

	#
	# Entry point for the OF client interface.
	#

        .globl client_entry_point
        .section        ".opd","aw"
        .align 3
client_entry_point:
        .quad   .client_entry_point,.TOC.@tocbase,0
        .previous
        .type   .client_entry_point,@function
        .globl  .client_entry_point
.client_entry_point:
	mflr 4
	bl swap_ci_regs	# swap regs
	mtlr 4
	li 3, 0 # client call
	blr	

	#
	# Start the client.
	#

        .globl call_client
        .section        ".opd","aw"
        .align 3
call_client:
        .quad   .call_client,.TOC.@tocbase,0
        .previous
        .type   .call_client,@function
        .globl  .call_client

.call_client:	# called with r3 = address, returns r3
	mflr 4
	mtctr 3
	bl swap_ci_regs
	bctrl
	bl swap_ci_regs
	mtlr 4
	li 3, -1 # client app return
	blr
