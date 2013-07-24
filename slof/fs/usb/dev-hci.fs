\ *****************************************************************************
\ * Copyright (c) 2006, 2012, 2013 IBM Corporation
\ * All rights reserved.
\ * This program and the accompanying materials
\ * are made available under the terms of the BSD License
\ * which accompanies this distribution, and is available at
\ * http://www.opensource.org/licenses/bsd-license.php
\ *
\ * Contributors:
\ *     IBM Corporation - initial implementation
\ ****************************************************************************/
\ *
\ * [OEX]HCI functions
\ *
\ ****************************************************************************

\ ( num $name type )

VALUE usb_type \ USB type

\ Open Firmware Properties
2dup device-name
s" usb" device-type

rot
VALUE usb_num                           \ controller number
usb_num $cathex strdup			\ create alias name
2dup find-alias 0= IF
   get-node node>path set-alias
ELSE 3drop THEN

/hci-dev BUFFER: hcidev
usb_num hcidev usb-setup-hcidev
TRUE VALUE first-time-init?
0 VALUE open-count

false VALUE dev-hci-debug?

: get-base-address ( -- baseaddr )
    hcidev hcd>base @
;

get-base-address CONSTANT baseaddrs

1 encode-int s" #address-cells" property
0 encode-int s" #size-cells" property

\ converts physical address to text unit string
: encode-unit ( port -- unit-str unit-len ) 1 hex-encode-unit ;

\ Converts text unit string to phyical address
: decode-unit ( addr len -- port ) 1 hex-decode-unit ;

: get-hci-dev ( -- hcidev )
    hcidev
;

\ set HCI into suspend mode
\ this disables all activities to shared RAM
\ called when linux starts (quiesce)
: hc-suspend  ( -- )
   hcidev hcd>type @ dup 1 = IF
      00C3 baseaddrs 4  + rl!-le             \ Suspend OHCI controller
   THEN
   2 = IF
      baseaddrs dup c@ + rl@-le FFFFFFFE and
      baseaddrs dup c@ + rl!-le              \ Stop EHCI controller
   ELSE
      drop
   THEN
;

: hc-cleanup ( -- )
    my-phandle set-node
    dev-hci-debug? IF ." USB-HCI: Cleaning up " pwd cr THEN
    hcidev USB-HCD-EXIT
    hc-suspend
    0 set-node
;

: open   ( -- true | false )
    true
;

: close
;

\ create a new entry to cleanup and suspend HCI
\ after first init
first-time-init? IF
   ['] hc-cleanup add-quiesce-xt
   false to first-time-init?
THEN
