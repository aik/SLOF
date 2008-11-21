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


\ Support for device node instances.

0 VALUE my-self

\ Instance-init is a linked list, before finish-package.
\ entry format: offset in instance, link, initial value.
\ after finish-package it's a pointer to a memory block
\ that is copied verbatim for every instance.
\ This will have to be revisited, as it is not quite right:
\ an instance variable can be used before the package is
\ completed.

: (create-instance-var) ( "name" initial-value link-addr size-addr -- )
  CREATE  dup @ ,  1 cells swap +!  linked  , ;

: create-instance-var ( "name" initial-value -- )
  current-package @ dup pkg>instance-init swap pkg>instance-size
  (create-instance-var)  PREVIOUS DEFINITIONS ;

: >instance  my-self + ;

VOCABULARY instance-words  ALSO instance-words DEFINITIONS

: VARIABLE  0 create-instance-var DOES> @ >instance ;
: VALUE       create-instance-var DOES> @ >instance @ ;
: DEFER     0 create-instance-var DOES> @ >instance @ execute ;
\ No support for BUFFER: yet.

PREVIOUS DEFINITIONS

: INSTANCE  current-package @ 0= ABORT" No current package"
            ALSO instance-words ;

VARIABLE shared-instance-link
VARIABLE shared-instance-size

: SIVARIABLE  0 shared-instance-link shared-instance-size (create-instance-var)
              DOES> @ >instance ;

VOCABULARY shared-instance-vars  ALSO shared-instance-vars DEFINITIONS

SIVARIABLE the-package \ needs to be first!
SIVARIABLE the-parent
SIVARIABLE the-addr
SIVARIABLE the-addr1
SIVARIABLE the-addr2
SIVARIABLE the-args
SIVARIABLE the-args-len

PREVIOUS DEFINITIONS
: shared-instance-words  ['] shared-instance-vars >body cell+ @ ;


ALSO shared-instance-vars

: my-parent  the-parent @ ;
: my-args  the-args 2@ ;
: set-my-args  dup alloc-mem swap 2dup the-args 2! move ;

\ Current package has already been set, when this is called.
: create-instance-data ( -- instance )
   current-package @ dup pkg>instance-init @ swap pkg>instance-size @
   dup alloc-mem dup >r swap move r> ;
: create-instance  my-self create-instance-data to my-self the-parent !
                   current-package @ the-package ! ;
: destroy-instance ( instance -- )
  dup @ pkg>instance-size @ free-mem ;

PREVIOUS


: ihandle>phandle  @ ;

: push-my-self ( ihandle -- )  r> my-self >r >r to my-self ;
: pop-my-self ( -- )  r> r> to my-self >r ;
: call-package  push-my-self execute pop-my-self ;
: $call-my-method  ( str len -- ) my-self ihandle>phandle find-method
                                  0= ABORT" no such method"  execute ;
: $call-method  push-my-self $call-my-method pop-my-self ;
: $call-parent  my-parent $call-method ;

