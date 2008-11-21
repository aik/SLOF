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


\ Client interface.

\ First, the machinery.

VOCABULARY client-voc \ We store all client-interface callable words here.

: client-data  ciregs >r3 @ ;
: nargs  client-data la1+ l@ ;
: nrets  client-data la1+ la1+ l@ ;
: client-data-to-stack
  client-data 3 la+ nargs 0 ?DO dup l@ swap la1+ LOOP drop ;
: stack-to-client-data
  client-data nargs nrets + 2 + la+ nrets 0 ?DO tuck l! /l - LOOP drop ;

: call-client ( p0 p1 client-entry -- )
  >r  ciregs >r7 !  ciregs >r6 !  client-entry-point @ ciregs >r5 !
  r> jump-client drop
  BEGIN
    client-data-to-stack
    client-data l@ zcount
    \ XXX: Should only look in client-voc...
    ALSO client-voc $find PREVIOUS dup 0= >r
    IF drop
    \ XXX: 6789 is magic...
    CATCH ?dup IF dup 6789 = IF drop r> drop EXIT THEN THROW THEN
    stack-to-client-data
    ELSE cr client-data l@ zcount type ."  NOT FOUND" THEN
    r> ciregs >r3 !  ciregs >r4 @ jump-client 
  UNTIL ;

: flip-stack ( a1 ... an n -- an ... a1 )  ?dup IF 1 ?DO i roll LOOP THEN ;



\ Now come the actual client interface words.

ALSO client-voc DEFINITIONS

: exit  6789 THROW ;

: finddevice ( zstr -- phandle )
  zcount find-package 0= IF -1 THEN ;

: getprop ( phandle zstr buf len -- len' )
  >r >r zcount rot get-property IF ( data dlen R: buf blen )
  r> swap dup r> min swap >r move r> ELSE r> r> 2drop -1 THEN ;

: getproplen ( phandle zstr -- len )
  zcount rot get-property IF nip ELSE -1 THEN ;

: setprop ( phandle zstr buf len -- size|-1 )
  dup >r here dup >r swap dup allot move r> r>
  dup >r 2swap swap current-package @ >r set-package
  zcount property r> set-package r> ;


: nextprop ( phandle zstr buf -- flag ) \ -1 invalid, 0 end, 1 ok
  >r zcount rot next-property IF r> zplace 1 ELSE r> drop 0 THEN ; 

: open ( zstr -- ihandle )  zcount open-dev ;
: close ( ihandle -- )  close-dev ;

\ XXX: should return -1 if no such method exists in that node
: write ( ihandle str len -- len' )       rot s" write" rot $call-method ;
: read  ( ihandle str len -- len' )       rot s" read"  rot $call-method ;
: seek  ( ihandle hi lo -- status  ) swap rot s" seek"  rot $call-method ;

: claim ( virt size align -- addr )
  \ We don't do any assigned-addresses bookkeeping; furthermore, we're
  \ running with translations off, so just tell the client it can have it.
  \ XXX: doesn't work if client doesn't ask for a specific address.
  2drop ;
: release ( virt size -- )
  2drop ;

: instance-to-package ( ihandle -- phandle )
  ihandle>phandle ;

: instance-to-path ( ihandle buf len -- len' )
   \ XXX: we do no buffer overflow checking!
   drop >r ihandle>phandle s" full_name" rot get-property drop
   r> swap dup >r move r> 1- ;

: package-to-path ( phandle buf len -- len' )
   \ XXX: we do no overflow checking!
   drop >r s" full_name" rot get-property IF r> swap dup >r move r> 1-
   ELSE true ABORT" No full_name property?!?" THEN ;

: call-method ( str ihandle arg ... arg -- result return ... return )
   nargs flip-stack zcount rot ['] $call-method CATCH
   dup IF nrets 1 ?DO -444 LOOP THEN
   nrets flip-stack ;

: interpret ( ... zstr -- result ... )
  \ XXX: we just throw away the arguments.
  nargs 0 ?DO drop LOOP  nrets 1 ?DO -555 LOOP  -667 ;

\ XXX: no real clock, but monotonically increasing, at least ;-)
VARIABLE milliseconds
: milliseconds  milliseconds @  1 milliseconds +! ;

: start-cpu ( phandle addr r3 -- )
  \ phandle isn't actually used, but that's no problem on a 2-CPU system.
  3fc0 l! 3f80 l! 3f40 l! ;

\ Just to shut up warnings resulting from Linux calling this whether it
\ exists or not.  It isn't even standard, but hey.
: quiesce ;

PREVIOUS DEFINITIONS
