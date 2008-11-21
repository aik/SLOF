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
# The loader.  This is position-independent code.
#

	.section ".slof.loader","ax"

. = 0

	# Get our address.

	bcl 20,31,$+4
	mflr 31

	# Copy exception vectors.

	addi 1,31,0x200-4-8
	li 0,0x3f00/8
	mtctr 0
	li 2,0x100-8
0:
	ldu 0,8(1)
	stdu 0,8(2)
	bdnz 0b

	# Copy Paflof text.

	addi 1,31,0x4100-4-8
	li 0,0x2000/8
	mtctr 0
	lis 2,0x111
	subi 2,2,8
0:
	ldu 0,8(1)
	stdu 0,8(2)
	bdnz 0b

	# Copy paflof data.

	addi 1,31,0x6100-4-8
	lis 0,1
	mtctr 0
	lis 2,0x112
	subi 2,2,8
0:
	ldu 0,8(1)
	stdu 0,8(2)
	bdnz 0b

	# Flush L1-D cache.

	sync

	# Flush L1-I cache for Paflof and exception vector code.

	li 0,0x4000/128
	mtctr 0
	li 1,0
0:
	icbi 0,1
	addi 1,1,128
	bdnz 0b

	li 0,0x2000/128
	mtctr 0
	lis 1,0x111
0:
	icbi 0,1
	addi 1,1,128
	bdnz 0b

	isync

	# Go!

	ba 0x100
