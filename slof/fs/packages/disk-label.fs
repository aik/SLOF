\ *****************************************************************************
\ * Copyright (c) 2004, 2007 IBM Corporation
\ * All rights reserved.
\ * This program and the accompanying materials
\ * are made available under the terms of the BSD License
\ * which accompanies this distribution, and is available at
\ * http://www.opensource.org/licenses/bsd-license.php
\ *
\ * Contributors:
\ *     IBM Corporation - initial implementation
\ ****************************************************************************/


s" disk-label" device-name

INSTANCE VARIABLE partition
INSTANCE VARIABLE part-offset
INSTANCE VARIABLE block-size
INSTANCE VARIABLE block
INSTANCE VARIABLE args
INSTANCE VARIABLE args-len

INSTANCE VARIABLE block#  \ variable to store logical sector#
INSTANCE VARIABLE hit#    \ partition counter
INSTANCE VARIABLE success-flag
0ff constant END-OF-DESC
3 constant  PARTITION-ID
48 constant VOL-PART-LOC

: seek  lxjoin part-offset @ + xlsplit s" seek" $call-parent ;
: read  s" read" $call-parent ;

: init-block ( -- )
  s" block-size" ['] $call-parent CATCH IF ABORT" no block-size" THEN
  block-size !
  block-size @ alloc-mem  dup block-size @ erase  block ! ;

: parse-partition ( -- okay? )
  0 part-offset !  0 partition !  my-args args-len ! args !

  \ Fix up the "0" thing yaboot does.
  args-len @ 1 = IF args @ c@ [char] 0 = IF 0 args-len ! THEN THEN

  \ Check for "full disk" arguments.
  my-args [char] , findchar 0= IF true EXIT THEN drop \ no comma
  my-args [char] , split args-len ! args !
  dup 0= IF 2drop true EXIT THEN \ no first argument

  \ Check partition #.
  base @ >r decimal $number r> base !
  IF cr ." Not a partition #" false EXIT THEN

  \ Store part #, done.
  partition !  true ;

: try-dos-partition ( -- okay? )
  partition @ 1 5 within 0= IF cr ." Partition # not 1-4" false EXIT THEN

  \ Read partition table.
  0 0 seek drop  block @ block-size @ read drop
  block @ 1fe + 2c@ bwjoin aa55 <> IF cr ." No partitions" false EXIT THEN

  \ Could/should check for valid partition here...  aa55 is not enough really.

  \ Get the partition offset.
  partition @ 10 * 1b6 + block @ + 4c@ bljoin  block-size @ * part-offset !
  true ;

\ Check for an ISO-9660 filesystem on the disk
\ : try-iso9660-partition ( -- true|false )
\ implement me if you can ;-)
\ ;


\ Check for an ISO-9660 filesystem on the disk
\ (cf. CHRP IEEE 1275 spec., chapter 11.1.2.3)
: has-iso9660-filesystem  ( -- TRUE|FALSE )
   \ Seek and read starting from 16th sector:
   10 800 * 0 seek drop
   block @ block-size @ read drop
   \ Check for CD-ROM volume magic:
   block @ c@ 1 =
   block @ 1+ 5 s" CD001"  str=
   and
;


: try-dos-files ( -- found? )
   block @ 1fe + 2c@ bwjoin aa55 <> IF false EXIT THEN
   block @ c@ e9 <> IF
   block @ c@ eb <> block @ 2+ c@ 90 <> or IF false EXIT THEN THEN
   s" fat-files" find-package IF args @ args-len @ rot interpose THEN true
;

CREATE ext2-magic 2 allot
: try-ext2-files ( -- found? )
   438 0 seek drop  ext2-magic 2 read drop
   ext2-magic w@-le ef53 <> IF false EXIT THEN
   s" ext2-files" find-package IF args @ args-len @ rot interpose THEN true
;

: try-iso9660-files
   \ seek and read starting from 16th sector for volume descriptors
   block @ 1+ 5 s" CD001" str=
   IF   \ found ISO9660 signature
      s" iso-9660" find-package IF args @ args-len @ rot interpose THEN
      TRUE
   ELSE
      FALSE
   THEN
;


: try-files ( -- found? )
   \ If no path, then full disk.
   args-len @ 0= IF true EXIT THEN

   0 0 seek drop
   block @ block-size @ read drop
   try-dos-files IF true EXIT THEN
   try-ext2-files IF true EXIT THEN

   \ Seek to the begining of logical 2048-byte sector 16
   \ refer to Chapter C.11.1 in PAPR 2.0 Spec
   10 800 * 0 seek drop
   block @ block-size @ read drop
   try-iso9660-files IF true EXIT THEN

   \ ... more filesystem types here ...

   false
;

: try-partitions ( -- found? )
  try-dos-partition IF try-files EXIT THEN
  \ try-iso9660-partition IF try-files EXIT THEN
  \ ... more partition types here...
  false ;

: open
   init-block
   parse-partition 0= IF
      false EXIT
   THEN
   partition @ 0= IF
       try-files EXIT
   THEN
   try-partitions
;

: close
  block @ block-size @ free-mem ;

\ Workaround for not having "value" variables yet.
: block-size  block-size @ ;

STRUCT 
	/c field part-entry>active
	/c field part-entry>start-head
	/c field part-entry>start-sect
	/c field part-entry>start-cyl
	/c field part-entry>id
	/c field part-entry>end-head
	/c field part-entry>end-sect
	/c field part-entry>end-cyl
	/l field part-entry>sector-offset
	/l field part-entry>sector-count

CONSTANT /partition-entry  


\ Load from first active DOS boot partition.
\ Note: sector block size is always 512 bytes for DOS partition tables.

: load-from-dos-boot-partition ( addr -- size )
   0 0 seek drop
   block @ 200 read drop
   \ Check for DOS partition table magic:
   block @ 1fe + 2c@ bwjoin aa55 <> IF FALSE EXIT THEN
   \ Now step through the partition table:
   block @ 1be +                              ( addr part-off )
   4 0 DO 
      dup part-entry>active c@ 80 =           ( addr part-off active? )
      over part-entry>id c@ 41 = and IF       ( addr part-off )
         dup part-entry>sector-offset 4c@ bljoin  ( addr part-off sect-off )
         \ seek to the boot partition
         200 * 0 seek drop                    ( addr part-off )
         part-entry>sector-count 4c@ bljoin   ( addr sect-count )
         200 * read                           ( size )
         UNLOOP EXIT
      THEN
      /partition-entry +                      ( addr part-off )
   LOOP
   2drop 0
;

: load-from-boot-partition ( addr -- size )
   load-from-dos-boot-partition
   \ More boot partition formats ...
;


\ Extract the boot loader path from a bootinfo.txt file
\ In: address and length of buffer where the bootinfo.txt has been loaded to.
\ Out: string address and length of the boot loader (within the input buffer)
\      or a string with length = 0 when parsing failed.

: parse-bootinfo-txt  ( addr len -- str len )
   2dup s" <boot-script>" find-substr       ( addr len pos1 )
   2dup = IF
      \ String not found
      3drop 0 0 EXIT
   THEN
   dup >r - swap r> + swap                  ( addr1 len1 )
   2dup [char] \ findchar drop              ( addr1 len1 pos2 )
   dup >r - swap r> + swap                  ( addr2 len2 )
   2dup s" </boot-script>" find-substr nip  ( addr2 len3 )
;

\ Try to load \ppc\bootinfo.txt from the disk (used mainly on CD-ROMs), and if
\ available, get the boot loader path from this file and load it.
\ See the "CHRP system binding to IEEE 1275" specification for more information
\ about bootinfo.txt.

: load-chrp-boot-file ( addr -- size )
   \ Create bootinfo.txt path name and load that file:
   my-self parent ihandle>phandle node>path
   s" :\ppc\bootinfo.txt" $cat strdup       ( addr str len )
   open-dev dup 0= IF 2drop 0 EXIT THEN
   >r dup                                   ( addr addr R:ihandle )
   dup s" load" r@ $call-method             ( addr addr size R:ihandle )
   r> close-dev                             ( addr addr size )
   \ Now parse the information from bootinfo.txt:
   parse-bootinfo-txt                       ( addr fnstr fnlen )
   dup 0= IF 3drop 0 EXIT THEN
   \ Create the full path to the boot loader:
   my-self parent ihandle>phandle node>path ( addr fnstr fnlen nstr nlen )
   s" :" $cat 2swap $cat strdup             ( addr str len )
   \ Update the bootpath:
   2dup encode-string s" bootpath" set-chosen
   \ And finally load the boot loader itself:
   open-dev dup 0= IF ." failed to load CHRP boot loader." 2drop 0 EXIT THEN
   >r s" load" r@ $call-method              ( size R:ihandle )
   r> close-dev                             ( size )
;


\ Boot & Load w/o arguments is assumed to be boot from boot partition

: load ( addr -- size )
   args-len @ IF
      TRUE ABORT" Load done w/o filesystem" 
   ELSE
      partition @ IF
         0 0 seek drop
         200000 read
      ELSE
         has-iso9660-filesystem IF
             dup load-chrp-boot-file ?dup 0 > IF nip EXIT THEN
         THEN
         load-from-boot-partition
         dup 0= ABORT" No boot partition found"
      THEN
   THEN
;
