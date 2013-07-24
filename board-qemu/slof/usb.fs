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

STRUCT
    /n FIELD hcd>base
    /n FIELD hcd>type
    /n FIELD hcd>num
CONSTANT /hci-dev

: usb-setup-hcidev ( num hci-dev -- )
    >r
    s" 10 config-l@ translate-my-address 3 not AND" evaluate
    ( io-base ) r@ hcd>base !
    s" 08 config-l@ 8 rshift  0000000F0 AND 4 rshift " evaluate
    ( usb-type ) r@ hcd>type !
    ( usb-num )  r@ hcd>num !
    r> drop
;