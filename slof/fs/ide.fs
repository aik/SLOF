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
\ 26.06.2007  added: two devices (Master/Slave) per channel
 
1 encode-int s" #address-cells" property
0 encode-int s" #size-cells" property

: decode-unit  1 hex-decode-unit ;
: encode-unit  1 hex-encode-unit ;

0 VALUE >ata                                       \ base address for command-block
0 VALUE >ata1                                      \ base address for control block

true VALUE no-timeout                              \ flag that no timeout occured

0c CONSTANT #cdb-bytes                             \ command descriptor block (12 bytes)

\ *****************************
\ Some register access helpers.
\ *****************************
: ata-ctrl! 2 >ata1 + io-c! ;                      \ device control reg
: ata-astat@ 2 >ata1 + io-c@ ;                     \ read alternate status
                                                   
: ata-data@ 0 >ata + io-w@ ;                       \ data reg
: ata-data! 0 >ata + io-w! ;                       \ data reg
: ata-err@  1 >ata + io-c@ ;                       \ error reg
: ata-feat! 1 >ata + io-c! ;                       \ feature reg
: ata-cnt@  2 >ata + io-c@ ;                       \ sector count reg
: ata-cnt!  2 >ata + io-c! ;                       \ sector count reg
: ata-lbal! 3 >ata + io-c! ;                       \ lba low reg
: ata-lbal@ 3 >ata + io-c@ ;                       \ lba low reg
: ata-lbam! 4 >ata + io-c! ;                       \ lba mid reg
: ata-lbam@ 4 >ata + io-c@ ;                       \ lba mid reg
: ata-lbah! 5 >ata + io-c! ;                       \ lba high reg
: ata-lbah@ 5 >ata + io-c@ ;                       \ lba high reg
: ata-dev!  6 >ata + io-c! ;                       \ device reg
: ata-dev@  6 >ata + io-c@ ;                       \ device reg
: ata-cmd!  7 >ata + io-c! ;                       \ command reg
: ata-stat@ 7 >ata + io-c@ ;                       \ status reg

\ **********************************************************************
\ ATA / ATAPI Commands specifications:
\ - AT Attachment 8 - ATA/ATAPI Command Set (ATA8-ACS)
\ - ATA Packet Interface for CD-ROMs SFF-8020i
\ - ATA/ATAPI Host Adapters Standard (T13/1510D)
\ **********************************************************************
00 CONSTANT cmd#nop                                \ ATA and ATAPI
08 CONSTANT cmd#device-reset                       \ ATAPI only (mandatory)
20 CONSTANT cmd#read-sector                        \ ATA and ATAPI
90 CONSTANT cmd#execute-device-diagnostic          \ ATA and ATAPI
a0 CONSTANT cmd#packet                             \ ATAPI only (mandatory)
a1 CONSTANT cmd#identify-packet-device             \ ATAPI only (mandatory)
ec CONSTANT cmd#identify-device                    \ ATA and ATAPI

\ *****************************
\ Setup Regs for ATA:
\ BAR 0 & 1 : Device 0
\ BAR 2 & 3 : Device 1
\ *****************************
: set-regs ( n -- )
\    dup ."  < Set #" .         \ *** DEBUG LINE **** 
   dup
   01 and                                          \ only Chan 0 or Chan 1 allowed
   3 lshift dup 10 + config-l@ -4 and to >ata
   14 + config-l@ -4 and to >ata1
   02 and
   IF
      10
   ELSE
      00
   THEN
   ata-dev!
\    >ata ." /" . ata-astat@ ." /" . ." > "       \ *** DEBUG LINE ***   
;

  200 VALUE block-size
80000 VALUE max-transfer                     \ Arbitrary, really

CREATE sector d# 512 allot
CREATE packet-cb #cdb-bytes allot
CREATE packet-buffer 800 allot

\ ********************************
\ show all ATAPI-registers
\ data-register not read in order
\ to not influence PIO mode
\ ********************************
: show-regs
   cr
   cr ." alt. Status: " ata-astat@ .
   cr ." Status     : " ata-stat@ .
   cr ." Device     : " ata-dev@ .
   cr ." Error-Reg  : " ata-err@ .
   cr ." Sect-Count : " ata-cnt@ .
   cr ." LBA-Low    : " ata-lbal@ .
   cr ." LBA-Med    : " ata-lbam@ .
   cr ." LBA-High   : " ata-lbah@ .
;

\ ***************************************************
\ reads ATAPI-Status and displays it if check-bit set
\ ***************************************************
: status-check               ( -- )
   ata-stat@
   dup   
   01 and                                    \ is 'check' flag set ?
   IF
      cr
      ."    - ATAPI-Status: " .
      ata-err@                               \ retrieve sense code
      dup
      60 =                                   \ sense code = 6 ?
      IF
         ." ( media changed or reset )"      \ 'unit attention'
         drop                                \ drop err-reg content
      ELSE
         ." (Err : " .   ." )"               \ show err-reg content
      THEN
      cr
   ELSE
      drop                                   \ remove unused status      
   THEN      
;

\ *************************************
\ Wait for interface ready condition
\ Bit 7 of Status-Register is busy flag
\ new version with abort after 5 sec.
\ *************************************
: wait-for-ready
   get-msecs                                 \ start timer
   BEGIN
      ata-stat@ 80 and 0<>                   \ busy flag still set ?
      no-timeout and
      WHILE                                  \ yes
         dup get-msecs swap
         -                                   \ calculate timer difference
         FFFF AND                            \ reduce to 65.5 seconds
         d# 5000 >                           \ difference > 5 seconds ?
         IF
            false to no-timeout
         THEN
      REPEAT
   drop
;

\ *************************************
\ wait for specific status bits
\ new version with abort after 5 sec.
\ *************************************
: wait-for-status          ( val mask -- )
   get-msecs                                 \ initial timer value (start)
   >r
   BEGIN
      2dup                                   \ val mask
      ata-stat@ and <>                       \ expected status ?
      no-timeout and                         \ and no timeout ?
      WHILE      
      get-msecs r@ -                         \ calculate timer difference
      FFFF AND                               \ mask-off overflow bits
      d# 5000 >                              \ 5 seconds exceeded ?
      IF
         false to no-timeout                 \ set global flag
      THEN      
   REPEAT                  
   r>                                        \ clean return stack
   3drop
;

\ *********************************    
\ remove extra spaces from string end
\ *********************************    
: cut-string      ( saddr nul -- )
   swap
   over +
   swap   
   1 rshift                                  \ bytecount -> wordcount
   0 do
      /w -
      dup               ( addr -- addr addr )
      w@                ( addr addr -- addr nuw )
      dup               ( addr nuw -- addr nuw nuw )
      2020 =
      IF
         drop
         0 
      ELSE
         LEAVE         
      THEN
      over         
      w!
   LOOP
   drop
   drop
; 

\ ****************************************************
\ prints model-string received by identify device
\ ****************************************************
: show-model          ( dev# chan# -- )
   2dup
   ."    CH " .                  \ channel 0 / 1
   0= IF ." / MA"                \ Master / Slave
   ELSE  ." / SL"
   THEN
   swap
   2 * + ."  (@" . ." ) : "      \ device number
   sector 1 +
   c@
   80 AND 0=
   IF
      ." ATA-Drive    "
   ELSE
      ." ATAPI-Drive  "
   THEN

   22 emit                       \ start string display with "
   sector d# 54 +                \ string starts 54 bytes from buffer start
   dup
   d# 40                         \ and is 40 chars long
   cut-string                    \ remove all trailing spaces
   
   BEGIN
      dup
      w@
      wbflip
      wbsplit
      dup 0<>                    \ first char
      IF                   
         emit
         dup 0<>                 \ second char
         IF
            emit
            wa1+                 \ increment address for next
            false
         ELSE                    \ second char = EndOfString
            drop
            true
         THEN   
      ELSE                       \ first char = EndOfString
         drop
         drop
         true
      THEN
   UNTIL                         \ end of string detected
   drop
   22 emit                       \ end string display
                                                  
   sector c@                     \ get lower byte of first doublet
   80 AND                        \ check bit 7
   IF
      ."  (removable media)"
   THEN
   
   sector 1 +
   c@
   80 AND 0= IF                  \ is this an ATA drive ?
      sector d# 120 +            \ get word 60 + 61
      rl@-le                     \ read 32-bit as little endian value 
      d# 1000 /                  \ bytes -> kbytes (avoid 32-bit overflow)
      d# 512 *                   \ LBA = 512 Bytes
      d# 500 +                   \ round +- 0.5
      d# 1000 /                  \ kB -> MB
      dup
      d# 1000 >
      IF   
         d# 500 +
         d# 1000 /
         ."  (" .d ." GB)"      
      ELSE
         ."  (" .d ." MB)"            
      THEN
   THEN         
   
    sector d# 98 +               \ goto word 49
    w@
    wbflip
    200 and 0= IF cr ."    ** LBA is not supported " THEN   

   sector c@                     \ get lower byte of first doublet
   03 AND 01 =                   \ we use 12-byte packet commands (=00b)
   IF
      cr ."    packet size = 16 ** not supported ! **"
   THEN
   no-timeout not                \ any timeout occured so far ?
   IF
      cr   ."    ** timeout **"
   THEN
;

\ ****************************
\ ATA functions
\ ****************************
: pio-sector ( addr -- )  100 0 DO ata-data@
   over w! wa1+ LOOP drop ;
: pio-sector ( addr -- ) 
  wait-for-ready pio-sector ;
: pio-sectors ( n addr -- )  swap 0 ?DO dup pio-sector 200 + LOOP drop ;

: lba!  lbsplit   
   0f and 40 or                  \ always set LBA-mode + LBA (27..24)
   ata-dev@ 10 and or            \ add current device-bit (DEV)
   ata-dev!                      \ set LBA (27..24)
   ata-lbah!                     \ set LBA (23..16)
   ata-lbam!                     \ set LBA (15..8)
   ata-lbal!                     \ set LBA (7..0)
;

: read-sectors ( lba count addr -- ) 
  >r dup >r ata-cnt! lba! 20 ata-cmd! r> r> pio-sectors ;

: read-sectors ( lba count addr dev-nr -- )
    set-regs             ( lba count addr ) \ Set ata regs 
    BEGIN >r dup 100 > WHILE
       over 100 r@ read-sectors
       >r 100 + r> 100 - r> 20000 + REPEAT
    r> read-sectors
;

: ata-read-blocks                ( addr block# #blocks dev# -- #read )
   swap dup >r swap >r rot r>    ( addr block# #blocks dev # R: #blocks )
   read-sectors r>               ( R: #read )
;    

\ *******************************
\ ATAPI functions
\ preset LBA register with maximum
\ allowed block-size (16-bits)
\ *******************************
: set-lba            ( block-length -- )
   lbsplit           ( quad -- b1.lo b2 b3 b4.hi )
   drop                                \ skip upper two bytes
   drop
   ata-lbah!
   ata-lbam!
;
   
\ *******************************************
\ gets byte-count and reads a block of words
\ from data-register to a buffer
\ *******************************************
: read-pio-block                        ( buff-addr -- buff-addr-new)
   ata-lbah@ 8 lshift                  \ get block length High
   ata-lbam@ or                        \ get block length Low
   1 rshift                            \ bcount -> wcount
   dup
   0> IF                               \ any data to transfer?
      0 DO                             \ words to read
         dup                           \ buffer-address
         ata-data@ swap w!             \ write 16-bits
         wa1+                          \ address of next entry
         LOOP
      ELSE
         drop                          ( buff-addr wcount -- buff-addr )
      THEN
   wait-for-ready
;

\ ********************************************
\ ATAPI support
\ Send a command block (12 bytes) in PIO mode
\ read data if requested
\ ********************************************
: send-atapi-packet                    ( req-buffer req-len -- )
   >r                                  ( req-len  R: req-buffer )
   800 set-lba                         \ set regs to length limit
   00 ata-feat!
   cmd#packet ata-cmd!                 \ A0 = ATAPI packet command
   48 C8  wait-for-status     ( val mask -- )  \ BSY:0 DRDY:1 DRQ:1
   6 0  do
      packet-cb i 2 * +                \ transfer command block (12 bytes)
      w@
      ata-data!                        \ 6 doublets PIO transfer to device
      loop                             \ copy packet to data-reg
   status-check                        \ status err bit set ? -> display
   wait-for-ready                      \ busy released ?
   BEGIN
   ata-stat@ 08 and 08 = WHILE         \ Data-Request-Bit set ?
      r>                               \ get target buffer address
      read-pio-block                   \ only if from device requested
      >r                               \ start of next block
      REPEAT
   r>
   drop
;   

\ ********************************
\ ATAPI packet commands
\ ********************************
03 CONSTANT scsi-cmd#request-sense
12 CONSTANT scsi-cmd#inquiry
28 CONSTANT scsi-cmd#read10
A8 CONSTANT scsi-cmd#read12
25 CONSTANT scsi-cmd#read-capacity
2B CONSTANT scsi-cmd#seek

\ Methods to access atapi disk

: atapi-test ( -- true|false )
   packet-cb #cdb-bytes erase                      \ command-code 0
   packet-buffer send-atapi-packet
   ata-stat@ 1 and IF false ELSE true THEN
;

: atapi-sense ( -- ASC sense-key )
   packet-cb #cdb-bytes erase
   scsi-cmd#request-sense packet-cb c!             \ set command-code 03h
   12 packet-cb 4 + c!                             \ allocation length = 18
   packet-buffer send-atapi-packet
   packet-buffer d# 12 + c@                        \ additional sense code (ASC)
   packet-buffer 2 + c@ f and                      \ sense key
;

: atapi-inquiry ( -- )
   packet-cb #cdb-bytes erase                      \ set command-code 12h
   scsi-cmd#inquiry packet-cb c!
   24 packet-cb 4 + c!
   packet-buffer send-atapi-packet
;

: atapi-capacity ( -- )
   packet-cb #cdb-bytes erase
   scsi-cmd#read-capacity packet-cb c!             \ set command-code 25h
   packet-buffer send-atapi-packet
;

: atapi-seek ( offset -- )
   packet-cb #cdb-bytes erase
   scsi-cmd#seek packet-cb c!                      \ set command code 2bh
   packet-cb 4 + l!
   packet-buffer send-atapi-packet
;

: atapi-start ( cmd -- )
   packet-cb #cdb-bytes erase
   1b packet-cb c!
   packet-cb 4 + c!
   packet-buffer send-atapi-packet
;

: atapi-toc (  -- )
   packet-cb #cdb-bytes erase
   43 packet-cb c!
   200 packet-cb 7 + w!
   packet-buffer send-atapi-packet
;

: atapi-read ( offset cnt -- )
   packet-cb #cdb-bytes erase
   scsi-cmd#read10 packet-cb c!                    \ set command code 28h
   packet-cb 7 + w!                                \ 2 bytes: Transfer Length
   packet-cb 2 + l!                                \ 4 bytes: Block-Address
   packet-buffer send-atapi-packet
;

: atapi-read-blocks        ( address block# #blocks dev# -- #read-blocks )
   set-regs                ( dev# -- )      
   dup >r
   packet-cb #cdb-bytes erase
   scsi-cmd#read10 packet-cb c!                    \ set command code 28h
   packet-cb 7 + w!                                \ 2 bytes: Transfer Length                           
   packet-cb 2 + l!                                \ 4 bytes: Block-Address
   send-atapi-packet
   r>
;

\ ***********************************************
\ wait until media in drive is ready ( max 5 sec)
\ ***********************************************
: wait-for-media-ready      ( -- true|false )
   get-msecs                                       \ initial timer value (start)
   >r
   BEGIN
      atapi-test                                   \ unit ready? false if not      
      not
      no-timeout and
      WHILE
         atapi-sense      ( -- asc sensekey )
         02 =                                      \ sense key 2 = media error 
         IF                                        \ check add. sense code
            3A = IF false to no-timeout ."  empty" THEN      \ medium not present, abort waiting
         ELSE
            drop                                   \ discard add. sense code
         THEN   
         get-msecs r@ -                            \ calculate timer difference
         FFFF AND                                  \ mask-off overflow bits
         d# 5000 >                                 \ 5 seconds exceeded ?
         IF
            false to no-timeout                    \ set global flag
         THEN      
   REPEAT
   r>
   drop
   no-timeout
;

\ ******************************************************
\ Method pointer for read-blocks methods
\ controller implements 2 channels (primary / secondary)
\ for 2 devices each (master / slasve)
\ ******************************************************
\ 2 channels (primary/secondary) per controller
2 CONSTANT #chan 

\ 2 devices (master/slacve) per channel
2 CONSTANT #dev

\ results in a total of devices
\ connected to a controller with
\ two separate channels (4)
: #totaldev #dev #chan * ;
 
CREATE read-blocks-xt #totaldev cells allot read-blocks-xt #totaldev cells erase

\ Execute read-blocks of device
: dev-read-blocks ( address block# #blocks dev# -- #read-blocks )
   dup cells read-blocks-xt + @ execute    
;

\ **********************************************************
\ Read device type
\ Signature      ATAPI             ATA
\ ---------------------------------------------
\ Sector Count    01h              01h
\ Sector Number   01h              01h
\ Cylinder Low    14h              00h
\ Cylinder High   EBh              00h
\ Device/Head     00h or 10h       00h or 01h
\ see also ATA/ATAPI errata at:
\ http://suif.stanford.edu/~csapuntz/blackmagic.html
\ **********************************************************
: read-ident       ( -- true|false ) 
   false
   00 ata-lbal!                                          \ clear previous signature
   00 ata-lbam!
   00 ata-lbah!
   cmd#identify-device ata-cmd! wait-for-ready           \ first try ATA, ATAPI aborts command
   ata-stat@ CF and 48 =
   IF
      drop true                                          \ cmd accepted, this is a ATA
      d# 512 set-lba                                     \ set LBA to sector-length
   ELSE                                                  \ ATAPI sends signature instead
      ata-lbam@ 14 = IF                                  \ cylinder low  = 14 ?
         ata-lbah@ EB = IF                               \ cylinder high = EB ?
            cmd#device-reset ata-cmd! wait-for-ready     \ only supported by ATAPI
            cmd#identify-packet-device ata-cmd! wait-for-ready                     \ first try ata
            ata-stat@ CF and 48 = IF               
               drop true                                 \ replace flag
               THEN
            THEN
         THEN
      THEN
   dup IF
      ata-stat@ 8 AND IF                        \ data requested (as expected) ?      
         sector read-pio-block 
         drop                                   \ discard address end 
         ELSE
         drop false
         THEN
      THEN
   
   no-timeout not IF                            \ check without any timeout ?
      drop
      false                                     \ no, detection discarded
      THEN
;

\ *************************************************
\ Init controller ( chan 0 and 1 )
\ device 0 (= master) and device 1 ( = slave)
\  #dev  #chan   Dev-ID
\ ----------------------
\   0      0        0          Master of Channel 0
\   0      1        1          Master of Channel 1
\   1      0        2          Slave  of Channel 0
\   1      1        3          Slave  of Channel 1
\ *************************************************

: find-disks      ( -- )   
   #chan 0 DO                                      \ check 2 channels (primary & secondary)
      #dev 0 DO                                    \ check 2 devices per channel (master / slave)
         i 2 * j + set-regs                        \ set base address and dev-register for register access
         02 ata-ctrl!                              \ disable interrupts
         ata-stat@ 7f and 7f <>                    \ Check, if device is connected
         IF
            true to no-timeout                     \ preset timeout-flag
            read-ident        ( -- true|false )
            IF
               i j show-model                      \ print manufacturer + device string
               sector 1+ c@ C0 and 80 =            \ Check for ata or atapi
               IF
                  wait-for-media-ready             \ wait up to 5 sec if not ready
                  no-timeout and
                  IF
                     800 to block-size             \ ATAPI: 2048 bytes
                     80000 to max-transfer
                     ['] atapi-read-blocks i 2 * j + cells read-blocks-xt + !    
                     s" cdrom" strdup i 2 * j + s" generic-disk.fs" included
                  ELSE
                     ."  -"                        \ show hint for not registered
                  THEN    
               ELSE
                  200 to block-size                \ ATA: 512 bytes
                  80000 to max-transfer
                  ['] ata-read-blocks i 2 * j + cells read-blocks-xt + !
                  s" disk" strdup i 2 * j + s" generic-disk.fs" included
               THEN
            cr
            THEN    
         THEN
      LOOP
   LOOP
;  

find-disks

