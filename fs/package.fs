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


\ Packages.

VARIABLE device-tree
VARIABLE current-package

STRUCT
  cell FIELD pkg>peer
  cell FIELD pkg>parent
  cell FIELD pkg>child
  cell FIELD pkg>properties
  cell FIELD pkg>words
  cell FIELD pkg>instance-init
  cell FIELD pkg>instance-size
END-STRUCT

: find-method ( str len phandle -- false | xt true )
  pkg>words @ voc-find dup IF link> true THEN ;

\ Instances.
INCLUDE instance.fs

: create-package ( parent -- new )
  align wordlist >r wordlist >r
  here 0 , swap , 0 , r> , r@ ,
  shared-instance-link @ , shared-instance-size @ ,
  shared-instance-words r> cell+ ! ;

: peer    pkg>peer   @ ;
: parent  pkg>parent @ ;
: child   pkg>child  @ ;
: peer  dup IF peer ELSE drop device-tree @ THEN ;


: link ( new head -- ) \ link a new node at the end of a linked list
  BEGIN dup @ WHILE @ REPEAT ! ;
: link-package ( parent child -- )
  swap dup IF pkg>child link ELSE drop device-tree ! THEN ;

\ Set a package as active package.
: set-package ( phandle -- )
  current-package @ IF previous THEN
  dup current-package !
  ?dup IF pkg>words @ also context ! THEN
  definitions ;

: new-package ( -- phandle ) \ active package becomes new package's parent;
                             \ new package becomes active package
  current-package @ dup create-package
  tuck link-package dup set-package ;

\ This will have to be rewritten, as mentioned in instance.fs.
: init-instance-template ( link mem -- )
  BEGIN over WHILE 2dup >r dup cell+ @ swap cell- @ r> + ! >r @ r> REPEAT
  2drop ;
: finish-package ( -- )
  current-package @ dup dup
  pkg>instance-init swap pkg>instance-size @ alloc-mem
  over @ over init-instance-template swap !
  parent set-package ;

: device-end ( -- )  0 set-package ;


\ Properties.
INCLUDE property.fs


\ Display the device tree.
defer (ls)
: (ls-node) ( pkg -- )
  cr dup pkg>path type space child (ls) ;
: ((ls)) ( pkg -- )
  BEGIN dup WHILE dup (ls-node) peer REPEAT drop ;
' ((ls)) to (ls)
: ls ( -- ) \ XXX FIXME: should list nodes under current package only
  device-tree @ (ls) ;


\ Iterate over the tree, depth-first order.
: next-in-dfw-up ( phandle -- phandle | 0 )
  dup peer ?dup IF nip EXIT THEN
  parent dup IF recurse THEN ; \ XXX FIXME: tail recurse this by hand
: next-in-dfw ( phandle -- phandle | 0 )
  dup child ?dup IF nip exit THEN
  dup peer ?dup IF nip exit THEN
  parent dup IF next-in-dfw-up THEN ;


\ This could use some refactoring.  Or a complete rewrite.
: comp_pname ( pstr plen str len  -- true | false )
  2 pick 1- <> IF 2drop drop false ELSE
    swap 1- comp 0= THEN ;	
: find-package ( str len -- false | phandle true )
  \ CHEAT: if len == 1, use the root node
  dup 1 = IF 2drop device-tree @ true exit THEN
  \ CHEAT: ignore everything after a colon
  2dup [char] : findchar IF nip THEN

  device-tree @ BEGIN dup WHILE
  s" full_name" 2 pick get-property IF
  4 pick 4 pick comp_pname IF nip nip true EXIT THEN THEN
  s" name" 2 pick get-property IF
  4 pick 4 pick comp_pname IF nip nip true EXIT THEN THEN
  next-in-dfw
  REPEAT nip nip ;

: open-package ( arg len phandle -- ihandle | 0 )
  current-package @ >r set-package create-instance set-my-args
  ( now call open, returning error code, etc )
  my-self my-parent to my-self r> set-package ;
: close-package ( ihandle -- )
  ( call the close method here )
  destroy-instance ;

: open-dev ( name len -- ihandle | 0 )
  find-package 0= IF 0 EXIT THEN   my-self >r 0 to my-self
  0 swap BEGIN dup parent dup WHILE REPEAT drop
  BEGIN dup WHILE >r 0 0 r> open-package to my-self REPEAT drop
  my-self   r> to my-self ;
: close-dev ( ihandle -- )
  my-self >r to my-self
  BEGIN my-self WHILE my-parent my-self close-package to my-self REPEAT
  r> to my-self ;

: new-device ( -- )  new-package >r 0 0 r> open-package to my-self ;
: finish-device ( -- )
  ( check for "name" property here, delete this node if not there )
  finish-package my-parent my-self close-package to my-self ;
