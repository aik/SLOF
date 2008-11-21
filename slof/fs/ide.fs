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


\ ATA disk.

\ We run it at PIO mode 0; this is a) not too slow for the sizes we have to
\ handle, and b) works with all disks.
\ We assume there is a disk drive connected; however, if not, nothing will
\ crash.

new-device   s" /ht/ata@4,1/disk" full-name

s" disk" device-name  s" block" device-type

: open  true ;
: close ;

\ Some register access helpers.
: ata-ctrl! 3f6 io-c! ; \ device control reg
: ata-data@ 1f0 io-w@ ; \ data reg
: ata-cnt!  1f2 io-c! ; \ sector count reg
: ata-lbal! 1f3 io-c! ; \ lba low reg
: ata-lbam! 1f4 io-c! ; \ lba mid reg
: ata-lbah! 1f5 io-c! ; \ lba high reg
: ata-dev!  1f6 io-c! ; \ device reg
: ata-cmd!  1f7 io-c! ; \ command reg
: ata-stat@ 1f7 io-c@ ; \ status reg

\ Init controller; we use the master device only.
02 ata-ctrl!
00 ata-dev!

CREATE sector 200 allot

: wait-for-ready  BEGIN ata-stat@ 80 and WHILE REPEAT ;
: pio-sector ( addr -- )  100 0 DO ata-data@ over w! wa1+ LOOP drop ;
: pio-sector ( addr -- )  wait-for-ready pio-sector ;
: pio-sectors ( n addr -- )  swap 0 ?DO dup pio-sector 200 + LOOP drop ;
: read-ident  ec ata-cmd!  1 sector pio-sectors ;

read-ident                        sector d# 54 + d# 40 wbflips
cr .( Disk drive identifies as: ) sector d# 54 + d# 40 type

: lba!  lbsplit f and 40 or ata-dev! ata-lbah! ata-lbam! ata-lbal! ;
: read-sectors ( lba count addr -- )
  >r dup >r ata-cnt! lba! 20 ata-cmd! r> r> pio-sectors ;
: read-sectors ( lba count addr -- )
   BEGIN >r dup 100 > WHILE
   over 100 R@ read-sectors
   >r 100 + r> 100 - r> 20000 + REPEAT
   r> read-sectors ;

' read-sectors to disk-read
200 CONSTANT block-size
0 VALUE disk-offset
CREATE deblock 20000 allot

: seek ( lo hi -- status )  20 lshift or to disk-offset 1 ;

: read ( str len -- len' ) \ max 20000 bytes
  disk-offset 200 / over disk-offset + 1ff + 200 / over - deblock disk-read
  >r deblock disk-offset 1ff and + swap r@ move r>
  disk-offset over + to disk-offset ;

: read ( str len -- len' )
  dup >r BEGIN dup WHILE 2dup 20000 min read tuck - >r + r> REPEAT 2drop r> ;

finish-device
