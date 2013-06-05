\ *****************************************************************************
\ * Copyright (c) 2012 IBM Corporation
\ * All rights reserved.
\ * This program and the accompanying materials
\ * are made available under the terms of the BSD License
\ * which accompanies this distribution, and is available at
\ * http://www.opensource.org/licenses/bsd-license.php
\ *
\ * Contributors:
\ *     IBM Corporation - initial implementation
\ ****************************************************************************/

." Populating " pwd cr

0 CONSTANT virtio-scsi-debug
1 CONSTANT virtio-scsi \ scsi-type for device address handling

1 encode-int s" #address-cells" property
1 encode-int s" #size-cells" property

: decode-unit  1 hex-decode-unit ;
: encode-unit  1 hex-encode-unit ;

\ s" block" device-type

FALSE VALUE initialized?

/vd-len BUFFER: virtiodev
virtiodev virtio-setup-vd

STRUCT
    100 FIELD >cdb-req
constant /cdb-buf

scsi-open

CREATE sector d# 512 allot
CREATE sectorlun d# 512 allot
0 INSTANCE VALUE current-target
CREATE cdb 100 allot

: inquiry ( -- )
    ff cdb >cdb-req scsi-build-inquiry
    virtiodev current-target cdb sector ff virtio-scsi-send
    0<> IF  ." Inquiry failed " cr FALSE EXIT THEN
    TRUE
;

: request-sense ( -- )
    ff cdb >cdb-req scsi-build-request-sense
    virtiodev current-target cdb sector ff virtio-scsi-send
    0<> IF ." Error " cr FALSE EXIT THEN
    sector scsi-get-sense-data
;

: report-luns ( -- )
    200 cdb >cdb-req scsi-build-report-luns
    virtiodev current-target cdb sectorlun 200 virtio-scsi-send
    0<> IF  ." Report-luns failed " cr request-sense FALSE EXIT THEN
    virtio-scsi-debug IF
	sectorlun 20 dump cr
    THEN
    TRUE
;

: test-unit-ready ( -- true | [ ascq asc sense-key false ] )
    cdb >cdb-req scsi-build-test-unit-ready
    virtiodev current-target cdb sector ff virtio-scsi-send
    0<> IF
	request-sense FALSE
	EXIT THEN
    TRUE
;

: read-capacity ( -- TRUE | FALSE )
    cdb >cdb-req scsi-build-read-cap-10
    virtiodev current-target cdb sector ff virtio-scsi-send
    0<> IF ." Error reading capacity virtio-scsi block " cr FALSE EXIT THEN
    TRUE
;

: get-media-event ( -- true | false )
    cdb >cdb-req scsi-build-get-media-event
    virtiodev current-target cdb sector ff virtio-scsi-send
    0<> IF ." Error get-media-event " cr FALSE EXIT THEN
    TRUE
;

: start-stop-unit ( state# -- true | false )
    cdb >cdb-req scsi-build-start-stop-unit
    virtiodev current-target cdb sector ff virtio-scsi-send
    0<> IF ." Error start-stop-unit " cr FALSE EXIT THEN
    TRUE
;

: read-blocks ( address block# #blocks block-size -- [#read-blocks true] | false )
    over *                          ( address block# #block len )
    -rot tuck                       ( address len #block block# #block )
    cdb >cdb-req scsi-build-read-10 ( address len #blocks )
    -rot >r >r                      ( #block ) ( R: len address )
    virtiodev current-target cdb r> r> virtio-scsi-send
    0<> IF drop false ." Error reading virtio-scsi block " cr ABORT THEN
    true
;

: set-target ( srplun -- )
    virtio-scsi-debug IF
	dup ." Setting target " . cr
    THEN
    to current-target
;

: wrapped-inquiry ( -- true | false )
    inquiry not IF false EXIT THEN
    \ Skip devices with PQ != 0
    sector inquiry-data>peripheral c@ e0 and 0 =
;

: initial-test-unit-ready ( -- true | [ ascq asc sense-key false ] )
    0 0 0 false
    3 0 DO
        2drop 2drop
	test-unit-ready dup IF UNLOOP EXIT THEN
    LOOP
;

: compare-sense ( ascq asc key ascq2 asc2 key2 -- true | false )
    3 pick =	    ( ascq asc key ascq2 asc2 keycmp )
    swap 4 pick =   ( ascq asc key ascq2 keycmp asccmp )
    rot 5 pick =    ( ascq asc key keycmp asccmp ascqcmp )
    and and nip nip nip
;

#include "generic-cdrom.fs"

: dev-prep-disk ( -- )
    initial-test-unit-ready 0= IF
	." Disk not ready!" cr
        3drop
    THEN
;

: dev-max-transfer ( -- n )
    10000 \ Larger value seem to have problems with some CDROMs
;

: dev-get-capacity ( -- blocksize #blocks )
    \ Make sure that there are zeros in the buffer in case something
    \ goes wrong:
    sector 10 erase
    \ Now issue the read-capacity command
    read-capacity not IF
        0 0 EXIT
    THEN
    sector scsi-get-capacity-10
;

: dev-read-blocks  ( addr block# #blocks block-size -- [#blocks-read true] | false)
    read-blocks
;

: virtio-scsi-create-disk	( srplun -- )
    " disk" 0 virtio-scsi " generic-scsi-device.fs" included
;

: virtio-scsi-create-cdrom	( srplun -- )
    " cdrom" 1 virtio-scsi " generic-scsi-device.fs" included
;

: virtio-scsi-find-disks      ( -- )
    ." VIRTIO-SCSI: Looking for devices" cr
    0 set-target
    report-luns IF
	sectorlun 8 +                     ( lunarray )
	dup sectorlun l@ 3 >> 0 DO       ( lunarray lunarraycur )
	    \ fixme: Read word, as virtio-scsi writes u16 luns
	    dup w@ set-target wrapped-inquiry IF
		."   " current-target (u.) type ."  "
		\ XXX FIXME: Check top bits to ignore unsupported units
		\            and maybe provide better printout & more cases
		\ XXX FIXME: Actually check for LUNs
		sector inquiry-data>peripheral c@ CASE
		    0   OF ." DISK     : " current-target virtio-scsi-create-disk ENDOF
		    5   OF ." CD-ROM   : " current-target virtio-scsi-create-cdrom ENDOF
		    7   OF ." OPTICAL  : " current-target virtio-scsi-create-cdrom ENDOF
		    e   OF ." RED-BLOCK: " current-target virtio-scsi-create-disk ENDOF
		    dup dup OF ." ? (" . 8 emit 29 emit 5 spaces ENDOF
		ENDCASE
		sector .inquiry-text cr
	    THEN
	    8 +
	LOOP drop
    THEN
    drop
;

scsi-close        \ no further scsi words required

0 VALUE queue-control-addr
0 VALUE queue-event-addr
0 VALUE queue-cmd-addr

: setup-virt-queues
    \ add 3 queues 0-controlq, 1-eventq, 2-cmdq
    \ fixme: do we need to find more than the above 3 queues if exists
    virtiodev 0 virtio-get-qsize virtio-vring-size
    alloc-mem to queue-control-addr
    virtiodev 0 queue-control-addr virtio-set-qaddr

    virtiodev 1 virtio-get-qsize virtio-vring-size
    alloc-mem to queue-event-addr
    virtiodev 1 queue-event-addr virtio-set-qaddr

    virtiodev 2 virtio-get-qsize virtio-vring-size
    alloc-mem to queue-cmd-addr
    virtiodev 2 queue-cmd-addr virtio-set-qaddr
;

\ Set disk alias if none is set yet
: setup-alias
    s" disk" find-alias 0= IF
	s" disk" get-node node>path set-alias
    ELSE
	drop
    THEN
;

: virito-scsi-shutdown ( -- )
    virtiodev virtio-scsi-shutdown
    FALSE to initialized?
;

: virtio-scsi-init-and-scan  ( -- )
    \ Create instance for scanning:
    0 0 get-node open-node ?dup 0= IF ." exiting " cr EXIT THEN
    my-self >r
    dup to my-self
    \ Scan the VSCSI bus:
    virtiodev virtio-scsi-init
    dup 0= IF
	setup-virt-queues
	virtio-scsi-find-disks
	setup-alias
	TRUE to initialized?
	['] virtio-scsi-shutdown add-quiesce-xt
    THEN
    \ Close the temporary instance:
    close-node
    r> to my-self
;

virtio-scsi-init-and-scan
