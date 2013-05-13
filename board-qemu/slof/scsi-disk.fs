\ *****************************************************************************
\ * Copyright (c) 2011 IBM Corporation
\ * All rights reserved.
\ * This program and the accompanying materials
\ * are made available under the terms of the BSD License
\ * which accompanies this distribution, and is available at
\ * http://www.opensource.org/licenses/bsd-license.php
\ *
\ * Contributors:
\ *     IBM Corporation - initial implementation
\ ****************************************************************************/

\ Create new VSCSI child device

\ Create device
new-device

\ Set name
s" disk" device-name

s" block" device-type

false VALUE scsi-disk-debug?

\ Get SCSI bits
scsi-open

\ Required interface for deblocker

0 INSTANCE VALUE block-size
0 INSTANCE VALUE max-block-num
0 INSTANCE VALUE max-transfer
0 INSTANCE VALUE is_cdrom
INSTANCE VARIABLE deblocker

: read-blocks ( addr block# #blocks -- #read )
    block-size " dev-read-blocks" $call-parent
    not IF
        ." SCSI-DISK: Read blocks failed !" cr -1 throw
    THEN
;

: open ( -- true | false )
    scsi-disk-debug? IF
        ." SCSI-DISK: open [" .s ." ] unit is " my-unit . . ."  [" .s ." ]" cr
    THEN
    my-unit " set-target" $call-parent

    " inquiry" $call-parent dup 0= IF drop false EXIT THEN
    scsi-disk-debug? IF
        ." ---- inquiry: ----" cr
        dup 200 dump cr
        ." ------------------" cr
    THEN

    \ Skip devices with PQ != 0
    dup inquiry-data>peripheral c@ e0 and 0 <> IF
        ." SCSI-DISK: Unsupported PQ != 0" cr
	false EXIT
    THEN

    inquiry-data>peripheral c@ CASE
        5   OF true to is_cdrom ENDOF
        7   OF true to is_cdrom ENDOF
    ENDCASE

    scsi-disk-debug? IF
        is_cdrom IF
            ." SCSI-DISK: device treated as CD-ROM" cr
        ELSE
            ." SCSI-DISK: device treated as disk" cr
        THEN
    THEN

    is_cdrom IF " dev-prep-cdrom" ELSE " dev-prep-disk" THEN $call-parent
    " dev-max-transfer" $call-parent to max-transfer

    " dev-get-capacity" $call-parent to max-block-num to block-size
    max-block-num 0=  block-size 0= OR IF
       ." SCSI-DISK: Failed to get disk capacity!" cr
       FALSE EXIT
    THEN

    scsi-disk-debug? IF
        ." Capacity: " max-block-num . ." blocks of " block-size . cr
    THEN

    0 0 " deblocker" $open-package dup deblocker ! dup IF 
        " disk-label" find-package IF
            my-args rot interpose
        THEN
   THEN 0<>
;

: close ( -- )
    deblocker @ close-package ;

: seek ( pos.lo pos.hi -- status )
    s" seek" deblocker @ $call-method ;

: read ( addr len -- actual )
    s" read" deblocker @ $call-method ;

\ Get rid of SCSI bits
scsi-close

finish-device
