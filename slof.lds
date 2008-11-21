/* ============================================================================= */
/*  * Copyright (c) 2004, 2005 IBM Corporation
/*  * All rights reserved. 
/*  * This program and the accompanying materials 
/*  * are made available under the terms of the BSD License 
/*  * which accompanies this distribution, and is available at
/*  * http://www.opensource.org/licenses/bsd-license.php
/*  * 
/*  * Contributors:
/*  *     IBM Corporation - initial implementation
/* ============================================================================= */


OUTPUT_FORMAT("elf64-powerpc", "elf64-powerpc", "elf64-powerpc")
OUTPUT_ARCH(powerpc:common64)

SECTIONS {
	.startup 0x2000000 : { startup.o }
	image = ALIGN(128);
	.slof.loader image : { loader.o }
	.slof.vectors image + 0x100 : { *(.slof.vectors) }
	.slof.text image + 0x4100 : { *(.slof.text) }
	.slof.data image + 0x6100 : { *(.slof.data) }
}
