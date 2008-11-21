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


\ A simple little hash table, to speed up compiling.  Quite important if
\ running on a CPU emulator; on real hardware, not so.

CREATE name-hash  20 cells allot
: clean-hash  name-hash 20 cells erase ;
clean-hash

: hash ( str len -- hash ) swap c@ xor 1f and cells name-hash + ;
: hash-find ( str len head -- 0 | link )
  >r 2dup 2dup hash
  dup >r @ dup IF link>name name>string string=ci ELSE nip nip THEN
  IF 2drop r> @ r> drop exit THEN
  r> r> swap >r ((find))
  dup IF dup r> ! ELSE r> drop THEN ;
: hash-reveal  hash off ;
' hash-reveal to (reveal)
' hash-find to (find)
