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

	.section ".slof.loader","ax"

	# get our address

.base:	
	bcl	20,31,0f
	.align  3
.st:	.quad   _slof_text-.base
.stl:   .quad   _slof_text_size
.sd:	.quad	_slof_data-.base
.sdl:	.quad	_slof_data_size
.sbl:	.quad	_slof_bss_size
0:	
	mr	r16,r3			# ROM Base
 	mfspr	r17, HSPRG1		# Fixme, will be done in pcd 
	mflr	r31
	subi	r31,r31,4

	# copy paflof text

	ld	r3,.st-.base(r31)
	add	r3,r3,r31
	lis	r4,_slof_text@h		# Addr of engine code
	ori	r4,r4,_slof_text@l 
	ld	r5,.stl-.base(31)
	bl	.copy

	lis	r3,_slof_text@h		# Addr of engine code
	ori	r3,r3,_slof_text@l
	ld	r4,.stl-.base(r31)
	bl	.flush

	# copy paflof data

	ld	r3,.sd-.base(31)
	add	r3,r3,r31
	lis	r4,_slof_data@h		# Addr of engine data
	ori	r4,r4,_slof_data@l 
	ld	r5,.sdl-.base(r31)
	bl	.copy

	lis	r3,_slof_data@h		# Addr of engine data
	ori	r3,r3,_slof_data@l 
	ld	r4,.sdl-.base(r31)
	bl	.flush

	# zero paflof bss

	lis	r3,_slof_bss@h		# Addr of engine bss
	ori	r3,r3,_slof_bss@l 
	ld	r4,.sbl-.base(r31)
	bl	.zero

	lis	r3,_slof_bss@h		# Addr of engine bss
	ori	r3,r3,_slof_bss@l 
	ld	r4,.sbl-.base(r31)
	bl	.flush

	# fill in handler address

	lis	r3,_slof_text@h
	ori	r3,r3,_slof_text@l	
	ld	r3,0(r3)
	std	r3,0x2ff0(0)

	# GO!

	ba	0x100


.zero:	# zero from r3 size r4
	subi	r3,r3,8
	addi	r4,r4,7
	srwi	r4,r4,3
	mtctr	r4
	li	r5,0
0:
	stdu	r5,8(r3)
	bdnz	0b

	blr

.copy:	# copy from 3 to 4 size 5
	subi	r3,r3,8
	subi	r4,r4,8
	addi	r5,r5,7
	srwi	r5,r5,3
	mtctr	r5
0:
	ldu	r5,8(r3)
	stdu	r5,8(r4)
	bdnz	0b

	blr

.flush:	# flush at 3 size 4
	FLUSH_CACHE(r3, r4)
	blr
