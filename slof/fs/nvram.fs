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

: rztype ( str len -- ) \ stop at zero byte, read with rb@
  BEGIN dup WHILE swap dup rb@ dup WHILE
  emit char+ swap 1- REPEAT drop THEN 2drop ;

: rtype  ( str len -- )
   0  DO  dup rb@ emit char+  LOOP  drop ;

: mrzplace ( str len buf -- )  2dup + 0 swap rb! swap mrmove ;

create tmpStr 500 allot
: rzcount ( zstr -- str len )
	dup tmpStr >r BEGIN dup rb@ dup r> dup 1+ >r  c! WHILE char+ REPEAT
	r> drop over - swap drop tmpStr swap ;

: >nvram nvram-base + ;

: calc-header-cksum ( offset -- cksum )
   >nvram dup rb@
   10 2 DO
      over i + rb@ +
   LOOP
   wbsplit + nip
;

: bad-header? ( offset -- flag )
   dup >nvram 2+ rw@      ( offset length )
   0= IF                  ( offset )
      drop true EXIT      ( )
   THEN
   dup calc-header-cksum  ( offset checksum' )
   swap >nvram 1+ rb@     ( checksum ' checksum )
   <>                     ( flag )
;

: .header ( offset -- )
   cr                    ( offset )
   dup bad-header? IF    ( offset )
      ."   BAD HEADER -- trying to print it anyway" cr
   THEN
   space >nvram          ( adr )
   \ print type
   dup rb@ 2 0.r         ( adr )
   space space           ( adr )
   \ print length
   dup 2+ rw@ 10 * 5 .r  ( adr )
   space space           ( adr )
   \ print name
   4 + 0c rztype         ( )
;

: .headers ( -- )
   cr cr ." Type  Size  Name"
   cr ." ========================"
   0 BEGIN                      ( offset )
      dup >nvram                ( offset adr )
      rb@                       ( offset type )
   WHILE
      dup .header               ( offset )
      dup >nvram 2+ rw@ 10 * +  ( offset offset' )
      dup nvram-size < IF       ( offset )
      ELSE
	 drop EXIT              ( )
      THEN
   REPEAT
   drop                         ( )
   cr cr
;

: find-header ( type -- offset false | true )
   0 >r                       ( type R: offset )
   BEGIN
      r@ >nvram               ( type adr )          ( R: offset )
      rb@ 2dup                ( type sig type sig ) ( R: offset )
      = IF                    ( type sig )          ( R: offset )
	 2drop r> false EXIT  ( offset false )
      THEN
   WHILE
      r> dup                  ( type offset offset )
      bad-header? IF          ( type offset )
	 2drop true EXIT      ( true )
      THEN
      dup >nvram 2+ rw@ 10 *  ( tyoe offset length )
      + >r                    ( type ) ( R: offset' )
   REPEAT
   r> 2drop true              ( true )
;

: get-header ( type -- data len false | true )
   find-header ?dup IF  ( offset false | true )
      EXIT              ( true )
   THEN
   dup                  ( offset offset )
   bad-header? ?dup IF  ( offset true | offset )
      nip EXIT          ( true )
   THEN
   >nvram               ( adr )
   dup 10 + swap        ( adr' adr )
   2+ rw@ 1- 10 *       ( adr length )
   false                ( adr length true )
;

\ FIXME: This function should return if it succeeded!
: add-header ( type size name len -- )
   rot dup >r 10 /                              ( type name len size/10 R:size )
   7f get-header IF                             ( type name len size/10 R:size )
      r> drop 4drop
      ." couldn't find free partition"          \ FIXME this should be a warning!!!
      EXIT
   THEN
   r> 2dup <= IF
      2drop 2drop 3drop
      ." couldn't find space in free partition"
      EXIT
   THEN
   - 10 + 10 / >r dup e - r> swap rw!   \ write new free size
   10 - dup dup nvram-base - calc-header-cksum swap 1+ rb!
   dup 2+ rw@ 10 * +     \ now we are on next header offset
   dup >r 2+ rw!         \ write new size
   rot r@ rb!            \ write type
   r@ 4 + mrzplace       \ write name
   r@ nvram-base - calc-header-cksum r> 1+ rb!
;

: create-header ( type size name len -- )
   0 find-header ABORT" couldn't find space for new NVRAM partition"
   \ write name
   dup >r >nvram 4 + mrzplace  ( type size )      ( R: offset )
   \ adr of first byte behind partition
   r@ >nvram over +            ( type size adr' ) ( R: offset )
   \ clear first byte behind new partition
   dup nvram-size >nvram < IF  ( type size adr' ) ( R: offset )
      0 swap rb!               ( type size )      ( R: offset )
   ELSE
      drop                     ( type size )      ( R: offset )
   THEN
   \ write size
   10 / r@ >nvram 2+ rw!       ( type ) ( R: offset )
   \ write type
   dup r@ >nvram rb!           ( type ) ( R: offset )
   \ write checksum
   r@ calc-header-cksum        ( type cksum ) ( R: offset)
   r> >nvram 1+ rb!            ( type )
   \ zero out partition
   get-header drop 0 rfill     ( )
;

: calc-used-nvram-space ( -- used )
   0 dup >r BEGIN              ( offset )        ( R: used )
      dup >nvram rb@           ( offset sig )    ( R: used )
   WHILE
      dup >nvram 2+ rw@ 10 *   ( offset length ) ( R: used )
      r> + >r                  ( offset )        ( R: used )
      dup >nvram 2+ rw@ 10 *   ( offset length ) ( R: used )
      +                        ( offset' )       ( R: used )
   REPEAT
   drop r>                     ( used )
;

: create-default-headers
   s" Creating common NVRAM partition" nvramlog-write-string-cr
   70 1000 s" common" create-header     ( )
   \ calculate free partition
   nvram-size calc-used-nvram-space -   ( free )
   dup 1 < IF                           ( free )
      drop                              ( )
   ELSE
      s" Creating free space NVRAM partition with 0x" nvramlog-write-string
      dup 6 nvramlog-write-number       ( free )
      s"  bytes" nvramlog-write-string-cr
      7f swap                           ( 7f type )
      here 10 allot                     ( 7f type adr )
      10 0 DO
	 dup i + FF swap c!             ( 7f type adr )
      LOOP
      e create-header                   ( )
   THEN
;

: reset-nvram ( -- )
   nvram-base nvram-size 0 rfill          ( )
   51 20000 s" ibm,BE0log" create-header  ( )
   51  5000 s" ibm,BE1log" create-header  ( )
   nvram-base 10 + dup                    ( adr adr )
   1 swap x!                              ( adr )
   40 swap w!                             ( )
   20000 nvram-base + 10 + dup            ( adr adr )
   1 swap x!                              ( adr )
   40 swap w!                             ( )
   create-default-headers                 ( )
;

: type-no-zero ( addr len -- )
  0 do dup i + dup rb@ 0= IF drop ELSE 1 rtype THEN loop drop ;

: .dmesg ( base-addr -- ) dup 14 + rl@ dup >r
  ( base-addr act-off ) ( R: act-off )
  over over over + swap 10 + rw@ + >r
  ( base-addr act-off ) ( R:  act-off nvram-act-addr )
  over 2 + rw@ 10 * swap - over swap
  ( base-addr base-addr start-size ) ( R:  act-off nvram-act-addr )
  r> swap rot 10 + rw@ - cr type-no-zero
  ( base-addr ) ( R: act-off )
  dup 10 + rw@ + r> type-no-zero ;


: type-no-zero-part ( from-str cnt-str addr len )
  0 do
    dup i + dup c@ 0= IF
      drop
    ELSE
      ( from-str cnt-str addr addr+i )
      ( from-str==0 AND cnt-str > 0 )
      3 pick 0= 3 pick 0 > AND IF
        dup 1 type
      THEN

      c@ a = IF
        2 pick 0= IF
          over 1- 0 max
          rot drop swap
        THEN
        2 pick 1- 0 max
        3 roll drop rot rot
        ( from-str-- cnt-str-- addr addr+i )
      THEN
    THEN
  loop drop ;

: .dmesg-part ( from-str cnt-str base-addr -- ) dup 14 + l@ dup >r
  ( base-addr act-off ) ( R: act-off )
  over over over + swap 10 + w@ + >r
  ( base-addr act-off ) ( R:  act-off nvram-act-addr )
  over 2 + w@ 10 * swap - over swap
  ( base-addr base-addr start-size ) ( R:  act-off nvram-act-addr )
  r> swap rot 10 + w@ - cr
  rot 4 roll 4 roll 4 roll 4 roll
  ( base-addr from-str cnt-str addr len )
  type-no-zero-part rot
  ( base-addr ) ( R: act-off )
  dup 10 + w@ + r> type-no-zero-part ;

: dmesg-part ( from-str cnt-str -- from-str cnt-str )
   2dup nvram-base .dmesg-part nip nip ;

: dmesg ( -- ) nvram-base .dmesg ;

: dmesg2 ( -- ) nvram-log-be1-base .dmesg ;
