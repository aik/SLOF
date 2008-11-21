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


: input  ( dev-str dev-len -- )
   open-dev ?dup IF
      \ Close old stdin:
      s" stdin" get-chosen IF
         decode-int nip nip ?dup IF close-dev THEN
      THEN
      \ Now set the new stdin:
      encode-int s" stdin"  set-chosen
   THEN
;

: output  ( dev-str dev-len -- )
   open-dev ?dup IF
      \ Close old stdout:
      s" stdout" get-chosen IF
         decode-int nip nip ?dup IF close-dev THEN
      THEN
      \ Now set the new stdout:
      encode-int s" stdout" set-chosen
   THEN
;

: io  ( dev-str dev-len -- )
   2dup input output
;


1 BUFFER: (term-io-char-buf)

: term-io-key  ( -- char )
   s" stdin" get-chosen IF
      decode-int nip nip dup 0= IF 0 EXIT THEN
      >r BEGIN
         (term-io-char-buf) 1 s" read" r@ $call-method
         0 >
      UNTIL
      (term-io-char-buf) c@
      r> drop
   THEN
;

' term-io-key to key

\ TODO: Implement: ' term-io-key? to key?
