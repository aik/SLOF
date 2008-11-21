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


\ Properties.

\ Words on the property list for a package are actually executable words,
\ that return the address and length of the property's data.  Special
\ nodes like /options can have their properties use specialized code to
\ dynamically generate their data; most nodes just use a 2CONSTANT.

: encode-int     here swap lbsplit c, c, c, c, /l ;
: encode-bytes   dup >r here >r bounds ?DO i c@ c, LOOP r> r> ;
: encode-string  encode-bytes 0 c, char+ ;

: encode+  nip + ;
: encode-int+  encode-int encode+ ;

\ Remove a word from a wordlist.
: (prune) ( name len head -- )
  dup >r (find) ?dup IF r> BEGIN dup @ WHILE 2dup @ = IF
  >r @ r> ! EXIT THEN @ REPEAT 2drop ELSE r> drop THEN ;
: prune ( name len -- )  last (prune) ;

: set-property ( data dlen name nlen phandle -- )
  get-current >r  pkg>properties @ set-current
  2dup prune  $2CONSTANT  r> set-current ;
: property ( data dlen name nlen -- )  current-package @ set-property ;
: get-property ( str len phandle -- false | data dlen true )
  pkg>properties @ voc-find dup IF link> execute true THEN ;

\ Print out properties.  Just a hexdump, nothing fancy for strings etc.
: .propbytes ( xt -- )
  execute bounds ?DO space i c@ 2 0.r LOOP ;
: .property ( lfa -- )
  cr link> dup >name name>string type space .propbytes ;
: (.properties) ( phandle -- )
  pkg>properties @ cell+ @ BEGIN dup WHILE dup .property @ REPEAT drop ;
: .properties ( -- )
  current-package @ (.properties) ;

: next-property ( str len phandle -- false | str' len' true )
  ?dup 0= IF device-tree @ THEN  \ XXX: is this line required?
  pkg>properties @
  >r 2dup 0= swap 0= or IF 2drop r> cell+ ELSE r> voc-find THEN
  @ dup IF link>name name>string true THEN ;


\ Helpers for common nodes.  Should perhaps remove "compatible", as it's
\ not typically a single string.
: device-name  encode-string s" name"        property ;
: device-type  encode-string s" device_type" property ;
: compatible   encode-string s" compatible"  property ;
: full-name    encode-string s" full_name"   property ;

\ Getting basic info about a package.
: pkg>name  dup >r s" name" rot get-property IF 1- r> drop ELSE r> (u.) THEN ;
: pkg>path  dup >r s" full_name" rot get-property drop 1- r> drop ;
