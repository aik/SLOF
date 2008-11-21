# =============================================================================
#  * Copyright (c) 2004, 2005 IBM Corporation
#  * All rights reserved. 
#  * This program and the accompanying materials 
#  * are made available under the terms of the BSD License 
#  * which accompanies this distribution, and is available at
#  * http://www.opensource.org/licenses/bsd-license.php
#  * 
#  * Contributors:
#  *     IBM Corporation - initial implementation
# =============================================================================


#
# The entry points into the engine, as well as everything else in low memory.
#

	.section ".slof.vectors","ax"

	. = 0

	.asciz "SLOF, the SlimLine Open Firmware"


	#
	# The reset exception.
	#

	. = 0x0100
	mtsprg 0,0
	li 0,0x0100
	b handler



	#
	# All other exceptions.
	#

	.irp i, 0x0200,0x0300,0x0380,0x0400,0x0480,0x0500,0x0600,0x0700, \
		0x0800,0x0900,0x0a00,0x0b00,0x0c00,0x0d00,0x0e00,0x0f00, \
		0x1000,0x1100,0x1200,0x1300,0x1400,0x1500,0x1600,0x1700, \
		0x1800,0x1900,0x1a00,0x1b00,0x1c00,0x1d00,0x1e00,0x1f00, \
		0x2000,0x2100,0x2200,0x2300,0x2400,0x2500,0x2600,0x2700, \
		0x2800,0x2900,0x2a00,0x2b00,0x2c00,0x2d00,0x2e00,0x2f00
	. = \i
	mtsprg 0,0 ; li 0,\i ; b handler
	.endr



	#
	# The generic exception code.
	#
	# Enter with GPR0 = vector, SPRG0 = saved GPR0.
	#

	. = 0x3000

handler:
	mtsprg 1,1	# SPRG1 = saved GPR1
	lis 1,0x0110	# GPR1 = address of register save area

	.irp i, 2,3,4,5,6,7,8,9,10,11,12,13,14,15, \
		16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
	std \i,\i*8(1)
	.endr		# save GPR2..GPR31

	mr 3,0		# GPR3 = vector

	mfsprg 0,0 ; std 0,0(1) # save GPR0
	mfsprg 0,1 ; std 0,8(1)	# save GPR1

	mfcr 0 ;    std 0,0x100(1)
	mfxer 0 ;   std 0,0x108(1)
	mflr 0 ;    std 0,0x110(1)
	mfctr 0 ;   std 0,0x118(1)
	mfsrr0 0 ;  std 0,0x120(1)
	mfsrr1 0 ;  std 0,0x128(1)
	mfdar 0 ;   std 0,0x130(1)
	mfdsisr 0 ; std 0,0x138(1)	# save special regs

	addi 1,1,0x7000 ; li 0,0 ; stdu 0,-16(1)	# set up stack
	lis 2,engine@ha ; ld 2,8+engine@l(2)		# set up TOC pointer
	b .engine					# ...and go!




	#
	# Swap non-volatile client interface regs, plus GPR3..GPR7.
	#

swap_ci_regs:
	lis 8,0x0110
	addi 8,8,0x0400

	.irp i, 1,2,3,4,5,6,7, \
		13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
	ld 0,\i*8(8)
	std \i,\i*8(8)
	mr \i,0
	.endr		# swap GPR1..7, GPR13..31

	ld 0,0x100(8) ; mfcr 9 ; mtcrf 0xff,0 ; std 9,0x100(8)	# swap CR
	ld 0,0x128(8) ; mfmsr 9 ; mtmsrd 0 ; sync ; isync ; std 9,0x128(8)
								# swap MSR
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
	mflr 4 ; bl swap_ci_regs ; mtlr 4 ; li 3,0 ; blr	


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

.call_client:	# called with GPR3 = address, returns GPR3
	mflr 4 ; mtctr 3 ; bl swap_ci_regs ; bctrl
	bl swap_ci_regs ; mtlr 4 ; li 3,-1 ; blr


        .globl flush_cache
        .section        ".opd","aw"
        .align 3
flush_cache:
        .quad   .flush_cache,.TOC.@tocbase,0
        .previous
        .type   .flush_cache,@function
	.globl	.flush_cache
.flush_cache:	# flush at GPR3 size GPR4
	add 4,4,3
	addi 4,4,127
	rlwinm 3,3,0,0,24
	rlwinm 4,4,0,0,24
	sub 4,4,3
	srwi 4,4,7
	mtctr 4
0:
	dcbst 0,3
	sync
	icbi 0,3
	sync
	isync
	addi 3,3,128
	bdnz 0b

	blr


	#
	# This is where the secondary CPUs sit and wait.
	#

	. = 0x3f00
slaveloop:
	lis 0,10
	mtctr 0
	bdnz $		# do some waiting, to prevent flooding the buses
	lwz 0,0x3f40(0)
	and. 0,0,0
	beq $-20	# wait for our flag

	lwz 0,0x3f80(0)
	lwz 3,0x3fc0(0)
	mtctr 0
	bctr		# jump to specified address, with specified GPR3




. = 0x3fff
.byte 0x36		# to fill out to exactly 16kB
