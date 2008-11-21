\ =============================================================================
\  * Copyright (c) 2004, 2005 IBM Corporation
\  * All rights reserved. 
\  * This program and the accompanying materials 
\  * are made available under the terms of the BSD License 
\  * which accompanies this distribution, and is available at
\  * http://www.opensource.org/licenses/bsd-license.php
\  * 
\  * Contributors:
\  *     IBM Corporation - initial implementation
\ =============================================================================


\ ELF loader.

\ Author: Hartmut Penner <hpenner@de.ibm.com>

\ ELF 32 bit header

STRUCT
        /l field ehdr>e_ident
        /c field ehdr>e_class
        /c field ehdr>e_data
        /c field ehdr>e_version
        /c field ehdr>e_pad     
	/l field ehdr>e_ident_2
	/l field ehdr>e_ident_3
	/w field ehdr>e_type
	/w field ehdr>e_machine
	/l field ehdr>e_version
	/l field ehdr>e_entry
	/l field ehdr>e_phoff
	/l field ehdr>e_shoff
	/l field ehdr>e_flags
	/w field ehdr>e_ehsize
	/w field ehdr>e_phentsize
	/w field ehdr>e_phnum
	/w field ehdr>e_shentsize
	/w field ehdr>e_shnum
	/w field ehdr>e_shstrndx
END-STRUCT


\ ELF 32 bit program header

STRUCT
	/l field phdr>p_type
	/l field phdr>p_offset
	/l field phdr>p_vaddr
	/l field phdr>p_paddr
	/l field phdr>p_filesz
	/l field phdr>p_memsz
	/l field phdr>p_flags
	/l field phdr>p_align
END-STRUCT

\ ELF 64 bit header

STRUCT
        /l field ehdr64>e_ident
        /c field ehdr64>e_class
        /c field ehdr64>e_data
        /c field ehdr64>e_version
        /c field ehdr64>e_pad     
	/l field ehdr64>e_ident_2
	/l field ehdr64>e_ident_3
	/w field ehdr64>e_type
	/w field ehdr64>e_machine
	/l field ehdr64>e_version
	cell field ehdr64>e_entry
	cell field ehdr64>e_phoff
	cell field ehdr64>e_shoff
	/l field ehdr64>e_flags
	/w field ehdr64>e_ehsize
	/w field ehdr64>e_phentsize
	/w field ehdr64>e_phnum
	/w field ehdr64>e_shentsize
	/w field ehdr64>e_shnum
	/w field ehdr64>e_shstrndx
END-STRUCT


\ ELF 64 bit program header

STRUCT
        /l field phdr64>p_type
        /l field phdr64>p_flags    
	cell field phdr64>p_offset
	cell field phdr64>p_vaddr
	cell field phdr64>p_paddr
	cell field phdr64>p_filesz
	cell field phdr64>p_memsz
	cell field phdr64>p_align
END-STRUCT

: load-segment ( file-addr program-header-addr -- ) 

  ( file-addr  program-header-addr ) 
    dup >r phdr>p_vaddr l@ r@ phdr>p_memsz l@ erase 

  ( file-addr R: programm-header-addr )
    r@ phdr>p_vaddr l@ r@ phdr>p_memsz l@ dup 0= IF 2drop ELSE flushcache THEN

  ( file-addr R: programm-header-addr ) 
    r@ phdr>p_offset l@ +  r@ phdr>p_vaddr l@  r> phdr>p_filesz l@  move 
;


: load-segments ( file-addr -- )
  ( file-addr ) 
    dup dup ehdr>e_phoff l@ +	  \ Calculate program header address

  ( file-addr program-header-addr )
    over ehdr>e_phnum w@ 0 ?DO	  \ loop e_phnum times

  ( file-addr program-header-addr )  
      dup phdr>p_type l@ 1 = IF	  \ PT_LOAD ?
    
  ( file-addr program-header-addr )
        2dup load-segment THEN	  \ copy segment

  ( file-addr program-header-addr )
      over ehdr>e_phentsize w@ + LOOP  \ step to next header  

  ( file-addr program-header-addr )
      over ehdr>e_entry l@

  ( file-addr program-header-addr )
      nip nip 			  \ cleanup
;

: load-segment64 ( file-addr program-header-addr -- ) 

  ( file-addr  program-header-addr ) 
    dup >r phdr64>p_vaddr @ r@ phdr64>p_memsz @ erase 

  ( file-addr R: programm-header-addr )
    r@ phdr64>p_vaddr @ r@ phdr64>p_memsz @ dup 0= IF 2drop ELSE flushcache THEN
    
  ( file-addr R: programm-header-addr ) 
    r@ phdr64>p_offset @ +  r@ phdr64>p_vaddr @  r> phdr64>p_filesz @  move 
;


: load-segments64 ( file-addr -- entry )
  ( file-addr ) 
    dup dup ehdr64>e_phoff @ +	  \ Calculate program header address

  ( file-addr program-header-addr )
    over ehdr64>e_phnum w@ 0 ?DO	  \ loop e_phnum times

  ( file-addr program-header-addr )  
      dup phdr64>p_type l@ 1 = IF	  \ PT_LOAD ?
    
  ( file-addr program-header-addr )
        2dup load-segment64 THEN	  \ copy segment

  ( file-addr program-header-addr )
      over ehdr64>e_phentsize w@ + LOOP  \ step to next header  

  ( file-addr program-header-addr )
      over ehdr64>e_entry @
  
  ( file-addr program-header-addr entry )
      nip nip 			  \ cleanup
;

: elf-check-file ( file-addr --  1 : 32, 2 : 64, else bad  )
  ( file-addr )
  dup ehdr>e_ident l@ 7f454c46 <> ABORT" Not an ELF file" 
    
  ( file-addr )
  dup ehdr>e_data c@ 2 <> ABORT" Not a Big Endian ELF file" 

  ( file-addr )
  dup ehdr>e_type w@ 2 <> ABORT" Not an ELF executable" 

  ( file-addr )
  dup ehdr>e_machine w@ dup 14 <> swap 15 <> and ABORT" Not a PPC ELF executable" 

  ( file-addr)
  ehdr>e_class c@
;    

: load-elf32 ( file-addr -- )

  ( file-addr)  
  load-segments
;

: load-elf64 ( file-addr -- )

  ( file-addr)  
  load-segments64
;

: load-elf-file ( file-addr -- entry )

  ( file-addr )
  dup elf-check-file

  ( file-addr 1|2|x )

    CASE
	1 OF load-elf32 ENDOF
	2 OF load-elf64 ENDOF
	dup OF true ABORT" Neither 32- nor 64-bit ELF file" ENDOF
    ENDCASE
;
