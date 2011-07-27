\ *****************************************************************************
\ * Copyright (c) 2004, 2008 IBM Corporation
\ * All rights reserved.
\ * This program and the accompanying materials
\ * are made available under the terms of the BSD License
\ * which accompanies this distribution, and is available at
\ * http://www.opensource.org/licenses/bsd-license.php
\ *
\ * Contributors:
\ *     IBM Corporation - initial implementation
\ ****************************************************************************/

\ PAPR PCI host bridge.

." Populating " pwd cr

: decode-unit  2 hex-decode-unit  3 #join  8 lshift  0 0 rot F00000 + ;
: encode-unit  nip nip  ff00 and 8 rshift  3 #split
               over IF 2 ELSE nip 1 THEN hex-encode-unit ;

0 VALUE my-puid

: setup-puid
  s" reg" get-node get-property 0= IF
    decode-64 to my-puid 2drop
  THEN
;

setup-puid

: config-b@  puid >r my-puid TO puid rtas-config-b@ r> TO puid ;
: config-w@  puid >r my-puid TO puid rtas-config-w@ r> TO puid ;
: config-l@  puid >r my-puid TO puid rtas-config-l@ r> TO puid ;

\ define the config writes
: config-b!  puid >r my-puid TO puid rtas-config-b! r> TO puid ;
: config-w!  puid >r my-puid TO puid rtas-config-w! r> TO puid ;
: config-l!  puid >r my-puid TO puid rtas-config-l! r> TO puid ;

: open  true ;
: close ;
