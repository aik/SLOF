\ *****************************************************************************
\ * Copyright (c) 2004, 2011, 2013 IBM Corporation
\ * All rights reserved.
\ * This program and the accompanying materials
\ * are made available under the terms of the BSD License
\ * which accompanies this distribution, and is available at
\ * http://www.opensource.org/licenses/bsd-license.php
\ *
\ * Contributors:
\ *     IBM Corporation - initial implementation
\ ****************************************************************************/

\ Load dev hci
: load-dev-hci ( num name-str name-len )
   s" dev-hci.fs" INCLUDED
;

0 VALUE ohci-alias-num
0 VALUE ehci-alias-num
0 VALUE xhci-alias-num

\ create a new ohci device alias for the current node
: set-ohci-alias  (  -- )
    ohci-alias-num dup 1+ TO ohci-alias-num    ( num )
    s" ohci" 1 load-dev-hci
;

\ create a new ehci device alias for the current node
: set-ehci-alias  (  -- )
    ehci-alias-num dup 1+ TO ehci-alias-num    ( num )
    s" ehci" 2 load-dev-hci
;

\ create a new xhci device alias for the current node
: set-xhci-alias  (  -- )
    xhci-alias-num dup 1+ TO xhci-alias-num    ( num )
    s" xhci" 3 load-dev-hci
;

: usb-enumerate ( hcidev -- )
    USB-HCD-INIT
;

: usb-scan ( -- )
    ." Scanning USB " cr
    ohci-alias-num 1 >= IF
	USB-OHCI-REGISTER
    THEN

    ohci-alias-num 0 ?DO
	" ohci" i $cathex find-device
	" get-hci-dev" get-node find-method
	IF
	    execute usb-enumerate
	ELSE
	    ." get-base-address method not found for ohci" i . cr
	THEN
    LOOP

    ehci-alias-num 1 >= IF
	USB-EHCI-REGISTER
    THEN

    ehci-alias-num 0 ?DO
	" ehci" i $cathex find-device
	" get-hci-dev" get-node find-method
	IF
	    execute usb-enumerate
	ELSE
	    ." get-base-address method not found for ehci" i . cr
	THEN
    LOOP

    0 set-node     \ FIXME Setting it back
;
