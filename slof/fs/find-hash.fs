\ *****************************************************************************
\ * Copyright (c) 2004, 2007 IBM Corporation
\ * All rights reserved.
\ * This program and the accompanying materials
\ * are made available under the terms of the BSD License
\ * which accompanies this distribution, and is available at
\ * http://www.opensource.org/licenses/bsd-license.php
\ *
\ * Contributors:
\ *     IBM Corporation - initial implementation
\ ****************************************************************************/


CREATE name-hash  200 cells allot
: clean-hash  name-hash 200 cells erase ;
clean-hash

\ The hash algorithm (AND with 1f is necessary because of case insensitivity)
: hash ( str len -- hash )
   f and 5 lshift swap c@ 1f and xor cells name-hash +
;

: hash-find ( str len head -- 0 | link )
  >r 2dup 2dup hash
  dup >r @ dup IF link>name name>string string=ci ELSE nip nip THEN
  IF 2drop r> @ r> drop exit THEN
  r> r> swap >r ((find))
  dup IF dup r> ! ELSE r> drop THEN ;

: hash-reveal  hash off ;

' hash-reveal to (reveal)
' hash-find to (find)
