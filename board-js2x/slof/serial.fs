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


\ Serial console.  Enabled very early.

: >serial  3f8 + ;
: serial!  >serial io-c! ;
: serial@  >serial io-c@ ;

: serial-init  0 1 serial!  0 2 serial!
               80 3 serial! d# 115200 swap / 0 serial!  0 1 serial!
               3 3 serial!  3 4 serial! ;
: serial-emit  BEGIN 5 serial@ 20 and UNTIL  0 serial! ;
: serial-key   BEGIN 5 serial@  1 and UNTIL  0 serial@ ;
: serial-key?  5 serial@  1 and 0<> ;

d# 19200 serial-init
' serial-emit to emit
' serial-key  to key
' serial-key? to key?

( .( SLOF)
.(  has started execution, serial console enabled.)
