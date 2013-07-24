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


0 VALUE open-count

: USB-HCI-INIT drop 1 ;
: USB-HCI-EXIT drop 1 ;

: open   ( -- true | false )
    dev-hci-debug? IF ." DEV-HCI: Opening (count is " open-count . ." )" cr THEN
    open-count 0= IF
	hcidev USB-HCI-INIT 0= IF
	    ." HCI Device setup failed " pwd cr
	    false EXIT
	THEN
    THEN
    open-count 1 + to open-count
    true
;

: close
    dev-hci-debug? IF ." DEV-HCI: Closing (count is " open-count . ." )" cr THEN
    open-count 0> IF
	open-count 1 - dup to open-count
	0= IF
	    hcidev USB-HCI-EXIT drop
	THEN
    THEN
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

\ create a new entry to suspend HCI
' hc-suspend add-quiesce-xt
