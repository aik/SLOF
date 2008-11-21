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


\ Configuration variables.  Not actually used yet, nor shown in /options.

wordlist CONSTANT envvars

: listenv  get-current envvars set-current words set-current ;

: create-env ( "name" -- )
  get-current >r envvars set-current CREATE r> set-current ;

: env-int ( n -- )  1 c, align , DOES> char+ aligned @ ;
: env-bytes ( a len -- )  2 c, align dup , here swap dup allot move
  DOES> char+ aligned dup @ >r cell+ r> ;
: env-string ( a len -- )  3 c, string, DOES> char+ count ;
: env-flag ( f -- )  4 c, c, DOES> char+ c@ 0<> ;
: env-secmode ( sm -- )  5 c, c, DOES> char+ c@ ;

: default-int ( n "name" -- )  create-env env-int ;
: default-bytes ( a len "name" -- )  create-env env-bytes ;
: default-string ( a len "name" -- )  create-env env-string ;
: default-flag ( f "name" -- )  create-env env-flag ;
: default-secmode ( sm "name" -- )  create-env env-secmode ;

: findenv ( name len -- adr def-adr type )
  2dup envvars voc-find dup 0= ABORT" not a configuration variable"
  link> >body char+ >r (find-order) link> >body dup char+ swap c@ r> swap ;
: $setenv ( param len name len -- )
  2dup findenv nip nip -rot $CREATE CASE
  1 OF evaluate env-int ENDOF \ XXX: wants decimal and 0x...
  2 OF env-bytes ENDOF
  3 OF env-string ENDOF
  4 OF evaluate env-flag ENDOF
  5 OF evaluate env-secmode ENDOF \ XXX: recognize none, command, full
  ENDOF ENDCASE ;
: setenv  parse-word skipws 0 parse 2swap $setenv ;

: (printenv) ( adr type -- )
  CASE
  1 OF aligned @ . ENDOF
  2 OF aligned dup cell+ swap @ dump ENDOF
  3 OF count type ENDOF
  4 OF c@ IF ." true" ELSE ." false" THEN ENDOF
  5 OF c@ . ENDOF \ XXX: print symbolically
  ENDCASE ;
: printenv  parse-word findenv rot over cr ." Current: " (printenv)
                                        cr ." Default: " (printenv) ;
: (set-default)  ( def-xt -- )
  dup >name name>string $CREATE dup >body c@ >r execute r> CASE
  1 OF env-int ENDOF
  2 OF env-bytes ENDOF
  3 OF env-string ENDOF
  4 OF env-flag ENDOF
  5 OF env-secmode ENDOF ENDCASE ;
: set-default  parse-word envvars voc-find
  dup 0= ABORT" not a configuration variable" link> (set-default) ;
: set-defaults  envvars cell+ BEGIN @ dup WHILE dup link> (set-default) REPEAT
                drop ;

true default-flag auto-boot?
s" " default-string boot-device
s" " default-string boot-file
s" " default-string diag-device
s" " default-string diag-file
false default-flag diag-switch?
true default-flag fcode-debug?
s" " default-string input-device
s" 1 2 3 * + ." default-string nvramrc
s" " default-string oem-banner
false default-flag oem-banner?
0 0 default-bytes oem-logo
false default-flag oem-logo?
s" " default-string output-device
50 default-int screen-#columns
18 default-int screen-#rows	
0 default-int security-#badlogins
0 default-secmode security-mode	
s" " default-string security-password
0 default-int selftest-#megs
false default-flag use-nvramrc?

set-defaults
