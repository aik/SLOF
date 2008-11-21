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

SECTIONS
{
	.slof.vectors 0	: { *(.slof.vectors) }

	.slof.text 0x1110000 : { *(.text) } = 0x60000000

	.slof.data 0x1120000 : {
		KEEP (*(.opd))
		. = ALIGN(8);
		*(.got .toc)
		*(.data .data.*)
	}
}
