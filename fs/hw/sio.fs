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


\ National Semiconductor SIO.
\ See http://www.national.com/pf/PC/PC87417.html for the datasheet.

\ We only use the first serial port, and the RTC.

\ See 3.7.5.
new-device   s" /ht/isa@4/serial" full-name

s" serial" 2dup device-name device-type

\ 8 bytes of ISA I/O space
1 encode-int 0 >serial encode-int+ 8 encode-int+ s" reg" property
d# 19200 encode-int s" current-speed" property

: open  true ;
: close ;
: write ( adr len -- actual )  tuck type ;
: read  ( adr len -- actual )  IF key swap c! 1 ELSE 0 THEN ;

finish-device



\ See the "Device Support Extensions" OF Recommended Practice document.
new-device   s" /ht/isa@4/rtc" full-name

s" rtc" 2dup device-name device-type
\ Following is for Linux, to recognize this RTC:
s" pnpPNP,b00" compatible

\ Enable the RTC, set its address at 1070.
10 7 siocfg!  1 30 siocfg!  10 60 siocfg!  10 62 siocfg!

: rtc!  1070 io-c!  1071 io-c! ;
: rtc@  1070 io-c!  1071 io-c@ ;

\ 10 bytes of ISA I/O space, at 1070.
1 encode-int 1070 encode-int+ 10 encode-int+ s" reg" property

\ Set sane configuration; BCD mode is required by Linux.
20 0a rtc!  02 0b rtc!  00 0c rtc!

: open   true ;
: close ;

\ XXX: dummy methods.
: get-time ( -- sec min hr day mth yr )  0 0 0 1 1 d# 1973 ;
: set-time ( sec min hr day mth yr -- )  3drop 3drop ;

finish-device
