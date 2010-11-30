\ *****************************************************************************
\ * Copyright (c) 2004, 2008 IBM Corporation
\ * All rights reserved.
\ * This program and the accompanying materials
\ * are made available under the terms of the BSD License
\ * which accompanies this distribution, and is available at
\ * http://www.opensource.org/licenses/bsd-license.php
\ *
\ * Contributors:
\ *     IBM Corporation - initial implementation
\ ****************************************************************************/

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

\ Provide word to load image to an offset of vaddr
0 value elf-segment-offset

: xlate-vaddr32 ( programm-header-addr -- addr )
    phdr>p_vaddr l@ elf-segment-offset + 
;


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


\ Claim memory for segment
\ Abort, if no memory available

false value elf-claim?
0     value last-claim

: claim-segment ( file-addr program-header-addr -- )
    elf-claim? IF
       >r
       here last-claim , to last-claim                \ Setup ptr to last claim
       \ Put addr and size ain the data space
       r@ phdr>p_vaddr l@ dup , r> phdr>p_memsz l@ dup , ( file-addr addr size )
       0 ['] claim CATCH IF ABORT" Memory for ELF file already in use " THEN
    THEN
    2drop
;

: claim-segment64 ( file-addr program-header-addr -- )
    elf-claim? IF
       >r
       here last-claim , to last-claim                \ Setup ptr to last claim
       \ Put addr and size ain the data space
       r@ phdr64>p_vaddr @ dup , r> phdr64>p_memsz @ dup , ( file-addr addr size )
       0 ['] claim CATCH IF ABORT" Memory for ELF file already in use " THEN
    THEN
    2drop
;

: load-segment ( file-addr program-header-addr -- )
  >r
  ( file-addr  R: program-header-addr )
  \ Copy into storage
    r@ phdr>p_offset l@ +  r@ xlate-vaddr32 r@ phdr>p_filesz l@  move

  ( R: programm-header-addr )
  \ Clear BSS
    r@ xlate-vaddr32 r@ phdr>p_filesz l@ +
    r@ phdr>p_memsz l@ r@ phdr>p_filesz l@ - erase

  ( R: programm-header-addr )
  \ Flush cache
    r@ xlate-vaddr32 r> phdr>p_memsz l@ dup 0= IF 2drop ELSE flushcache THEN
;

: load-segments ( file-addr -- )
  ( file-addr )
    dup dup ehdr>e_phoff l@ +	  \ Calculate program header address

  ( file-addr program-header-addr )
    over ehdr>e_phnum w@ 0 ?DO	  \ loop e_phnum times

  ( file-addr program-header-addr )
      dup phdr>p_type l@ 1 = IF	  \ PT_LOAD ?

  ( file-addr program-header-addr )
        2dup claim-segment	  \ claim segment

  ( file-addr program-header-addr )
        2dup load-segment THEN	  \ copy segment

  ( file-addr program-header-addr )
      over ehdr>e_phentsize w@ + LOOP  \ step to next header

  ( file-addr program-header-addr )
      over ehdr>e_entry l@

  ( file-addr program-header-addr )
      nip nip			  \ cleanup
;

: load-segment64 ( file-addr program-header-addr -- )
  >r
  ( file-addr  R: program-header-addr )
  \ Copy into storage
    r@ phdr64>p_offset @ +  r@ phdr64>p_vaddr @  r@ phdr64>p_filesz @  move

  ( R: programm-header-addr )
  \ Clear BSS
    r@ phdr64>p_vaddr @ r@ phdr64>p_filesz @ +
    r@ phdr64>p_memsz @ r@ phdr64>p_filesz @ - erase

  ( R: programm-header-addr )
  \ Flush cache
    r@ phdr64>p_vaddr @ r> phdr64>p_memsz @ dup 0= IF 2drop ELSE flushcache THEN
;

: load-segments64 ( file-addr -- entry )
  ( file-addr )
    dup dup ehdr64>e_phoff @ +	  \ Calculate program header address

  ( file-addr program-header-addr )
    over ehdr64>e_phnum w@ 0 ?DO	  \ loop e_phnum times

  ( file-addr program-header-addr )
      dup phdr64>p_type l@ 1 = IF	  \ PT_LOAD ?

  ( file-addr program-header-addr )
        2dup claim-segment64	          \ claim segment

  ( file-addr program-header-addr )
        2dup load-segment64 THEN	  \ copy segment

  ( file-addr program-header-addr )
      over ehdr64>e_phentsize w@ + LOOP  \ step to next header

  ( file-addr program-header-addr )
      over ehdr64>e_entry @

  ( file-addr program-header-addr entry )
      nip nip			  \ cleanup
;

\ Return type of ELF image, abort if not valid
\ 1: 32 Bit PPC image
\ 2: 64 Bit PPC image
\ 5: 32 Bit SPU image

: elf-check-file ( file-addr --  image-type  )
  ( file-addr )
  dup ehdr>e_ident l@-be 7f454c46 <> IF
     ABORT" Not an ELF executable"
  THEN

  ( file-addr )
  dup ehdr>e_data c@
  ?bigendian IF
    2 <> ABORT" Not a Big Endian ELF file"
  ELSE
    2 = ABORT" Not a Little Endian ELF file"
  THEN

  ( file-addr )
  dup ehdr>e_type w@ 2 <> ABORT" Not an ELF executable"

  ( file-addr )
  dup ehdr>e_machine w@
  CASE
      14 OF ehdr>e_class c@ ENDOF       \ PPC 32 bit executable        
      15 OF ehdr>e_class c@ ENDOF       \ PPC 64 bit executable        
      17 OF ehdr>e_class c@ 4 or ENDOF  \ SPU 32 bit executable
      dup OF drop ABORT" Not a PPC / SPU ELF executable" ENDOF 
  ENDCASE
;

: load-elf32 ( file-addr -- entry )

  ( file-addr)
  load-segments
;

: load-elf32-claim ( file-addr -- claim-list entry )
    true to elf-claim?
    0 to last-claim
    ['] load-elf32 CATCH IF false to elf-claim? ABORT THEN
    last-claim swap
    false to elf-claim?
;


: load-elf64 ( file-addr -- entry )

  ( file-addr)
  load-segments64
;

: load-elf64-claim ( file-addr -- claim-list entry )
    true to elf-claim?
    0 to last-claim
    ['] load-elf64 CATCH IF false to elf-claim? ABORT THEN
    last-claim swap
    false to elf-claim?
;

: load-elf-file ( file-addr -- entry 32-bit )

   ( file-addr )
   dup elf-check-file

  ( file-addr 1|2|x )

    CASE
	1 OF 0 to elf-segment-offset load-elf32 true ENDOF
	2 OF 0 to elf-segment-offset load-elf64 false ENDOF
	5 OF load-elf32 true ENDOF
	dup OF true ABORT" load-elf-file: Not valid image" ENDOF
    ENDCASE
;

\ Method to load SPU image 

: elf-spu-load ( ls-start-addr file-addr -- entry )
    swap to elf-segment-offset
    load-elf-file drop
;

\ Release memory claimed before

: elf-release ( claim-list -- )
   BEGIN
      dup cell+                   ( claim-list claim-list-addr )
      dup @ swap cell+ @          ( claim-list claim-list-addr claim-list-sz )
      release                     ( claim-list )
      @ dup 0=                    ( Next-element )
   UNTIL
   drop
;
