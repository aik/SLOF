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


\ I/O accesses.

\ Little-endian accesses.  Also known as `wrong-endian'.
: l!-le  >r lbflip r> l! ;
: l@-le  l@ lbflip ;

: w!-le  >r wbflip r> w! ;
: w@-le  w@ wbflip ;

: rl!-le  >r lbflip r> rl! ;
: rl@-le  rl@ lbflip ;

: rw!-le  >r wbflip r> rw! ;
: rw@-le  rw@ wbflip ;

\ Legacy I/O accesses.
: >io  f4000000 + ;

: io-c!  >io rb! ;
: io-c@  >io rb@ ;

: io-w!  >io rw! ;
: io-w@  >io rw@ ;

\ Accessing the SIO config registers.
: siocfg!  2e io-c! 2f io-c! ;
: siocfg@  2e io-c! 2f io-c@ ;

\ Configuration space accesses.
: >config  dup ffff > IF 1000000 + THEN f2000000 + ;
: config-l!  >config rl!-le ;
: config-l@  >config rl@-le ;
: config-w!  >config rw!-le ;
: config-w@  >config rw@-le ;
: config-b!  >config rb! ;
: config-b@  >config rb@ ;
