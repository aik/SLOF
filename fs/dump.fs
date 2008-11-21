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


\ Hexdump thingy.  A bit simplistic, always prints full lines.

hex

DEFER dump-c@
: .2 ( u -- )  0 <# # # #> type space ;
: .char ( c -- )  dup bl 7e between 0= IF drop [char] . THEN emit ;
: dumpline ( addr -- )
  cr dup 8 u.r ." : " dup 10 bounds DO i dump-c@ .2    LOOP
  space space             10 bounds DO i dump-c@ .char LOOP ;
: (dump) ( addr size -- )  bounds DO i dumpline 10 +LOOP ;
: dump  ['] c@ to dump-c@  (dump) ;
: rdump  ['] rb@ to dump-c@  (dump) ;
