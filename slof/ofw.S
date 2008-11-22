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
#include <xvect.h>

	.section ".slof.loader","ax"

	mr	r16, r4			# ROM Base
	mfspr	r17, HSPRG1		# Fixme, will be done in pcd

	# fill in handler address

	lis	r3, _slof_text@h
	ori	r3, r3, _slof_text@l
	ld	r3, 0(r3)
	std	r3, XVECT_M_HANDLER(0)

	# GO!
	ba	0x100
