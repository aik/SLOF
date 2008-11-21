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


\ Device nodes.

VARIABLE device-tree
VARIABLE current-node
: get-node  current-node @ dup 0= ABORT" No active device tree node" ;

STRUCT
  cell FIELD node>peer
  cell FIELD node>parent
  cell FIELD node>child
  cell FIELD node>properties
  cell FIELD node>words
  cell FIELD node>instance
  cell FIELD node>instance-size
  cell FIELD node>space?
  cell FIELD node>space
  cell FIELD node>addr1
  cell FIELD node>addr2
  cell FIELD node>addr3
END-STRUCT

: find-method ( str len phandle -- false | xt true )
  node>words @ voc-find dup IF link> true THEN ;

\ Instances.
#include "instance.fs"

1000 CONSTANT max-instance-size
3000000 CONSTANT space-code-mask

: create-node ( parent -- new )
  max-instance-size alloc-mem dup max-instance-size erase >r
  align wordlist >r wordlist >r
  here 0 , swap , 0 , r> , r> , r> , /instance-header , 0 , 0 , 0 , 0 , ;

: peer    node>peer   @ ;
: parent  node>parent @ ;
: child   node>child  @ ;
: peer  dup IF peer ELSE drop device-tree @ THEN ;


: link ( new head -- ) \ link a new node at the end of a linked list
  BEGIN dup @ WHILE @ REPEAT ! ;
: link-node ( parent child -- )
  swap dup IF node>child link ELSE drop device-tree ! THEN ;

\ Set a node as active node.
: set-node ( phandle -- )
  current-node @ IF previous THEN
  dup current-node !
  ?dup IF node>words @ also context ! THEN
  definitions ;
: get-parent  get-node parent ;


: new-node ( -- phandle ) \ active node becomes new node's parent;
                          \ new node becomes active node
\ XXX: change to get-node, handle root node creation specially
  current-node @ dup create-node
  tuck link-node dup set-node ;

: finish-node ( -- )
\ we should resize the instance template buffer, but that doesn't help with our
\ current implementation of alloc-mem anyway, so never mind. XXX
  get-node parent set-node ;

: device-end ( -- )  0 set-node ;

\ Properties.
CREATE $indent 100 allot  VARIABLE indent 0 indent !
#include "property.fs"

\ Unit address.
: #address-cells  s" #address-cells" rot parent get-property
  ABORT" parent doesn't have a #address-cells property!"
  decode-int nip nip ;
: my-#address-cells  get-node #address-cells ; \ bit of a misnomer...  "my-"

: encode-phys  ( phys.hi ... phys.low -- str len )
   encode-first?  IF  encode-start  ELSE  here 0  THEN
   my-#address-cells 0 ?DO rot encode-int+ LOOP ;

: decode-phys
  my-#address-cells BEGIN dup WHILE 1- >r decode-int r> swap >r REPEAT drop
  my-#address-cells BEGIN dup WHILE 1- r> swap REPEAT drop ;
: decode-phys-and-drop
  my-#address-cells BEGIN dup WHILE 1- >r decode-int r> swap >r REPEAT 3drop
  my-#address-cells BEGIN dup WHILE 1- r> swap REPEAT drop ;
: reg  >r encode-phys r> encode-int+ s" reg" property ;


: >space    node>space @ ;
: >space?   node>space? @ ;
: >address  dup >r #address-cells dup 3 > IF r@ node>addr3 @ swap THEN
                                  dup 2 > IF r@ node>addr2 @ swap THEN
                                      1 > IF r@ node>addr1 @ THEN r> drop ;
: >unit     dup >r >address r> >space ;

: my-space ( -- phys.hi )
   my-self ihandle>phandle >space ;
: my-address  my-self ihandle>phandle >address ;
: my-unit     my-self ihandle>phandle >unit ;

\ Return lower 64 bit of address
: my-unit-64 ( -- phys.lo+1|phys.lo )
   my-unit                                ( phys.lo ... phys.hi )
   my-self ihandle>phandle #address-cells ( phys.lo ... phys.hi #ad-cells )
   CASE
      1   OF EXIT ENDOF
      2   OF lxjoin EXIT ENDOF
      3   OF drop lxjoin EXIT ENDOF
      dup OF 2drop lxjoin EXIT ENDOF
   ENDCASE
;

: set-space    get-node dup >r node>space ! true r> node>space? ! ;
: set-address  my-#address-cells 1 ?DO
               get-node node>space i cells + ! LOOP ;
: set-unit     set-space set-address ;
: set-unit-64 ( phys.lo|phys.hi -- )
   my-#address-cells 2 <> IF
      ." set-unit-64: #address-cells <> 2 " abort
   THEN
   xlsplit set-unit
;

\ Never ever use this in actual code, only when debugging interactively.
\ Thank you.
: set-args ( arg-str len unit-str len -- )
           s" decode-unit" get-parent $call-static set-unit set-my-args ;

: $cat-unit  dup parent 0= IF drop EXIT THEN
             dup >space? not IF drop EXIT THEN
             dup >r >unit s" encode-unit" r> parent $call-static dup IF
             dup >r here swap move s" @" $cat here r> $cat
             ELSE 2drop THEN ;

\ Getting basic info about a node.
: node>name  dup >r s" name" rot get-property IF r> (u.) ELSE 1- r> drop THEN ;
: node>qname dup node>name rot ['] $cat-unit CATCH IF drop THEN ;
: node>path  here 0 rot  BEGIN dup WHILE dup parent REPEAT 2drop
             dup 0= IF [char] / c, THEN
             BEGIN dup WHILE [char] / c, node>qname here over allot swap move
	     REPEAT drop here 2dup - allot over - ;

: interposed? ( ihandle -- flag )
  \ We cannot actually detect if an instance is interposed; instead, we look
  \ if an instance is part of the "normal" chain that would be opened by
  \ open-dev and friends, if there were no interposition.
  dup instance>parent @ dup 0= IF 2drop false EXIT THEN
  ihandle>phandle swap ihandle>phandle parent <> ;
: instance>qname  dup >r interposed? IF s" %" ELSE 0 0 THEN
                  r@ ihandle>phandle node>qname $cat  r> instance>args 2@
                  dup IF 2>r s" :" $cat 2r> $cat ELSE 2drop THEN ;
: instance>qpath \ With interposed nodes.
  here 0 rot BEGIN dup WHILE dup instance>parent @ REPEAT 2drop
  dup 0= IF [char] / c, THEN
  BEGIN dup WHILE [char] / c, instance>qname here over allot swap move
  REPEAT drop here 2dup - allot over - ;
: instance>path \ Without interposed nodes.
  here 0 rot BEGIN dup WHILE
  dup interposed? 0= IF dup THEN instance>parent @ REPEAT 2drop
  dup 0= IF [char] / c, THEN
  BEGIN dup WHILE [char] / c, instance>qname here over allot swap move
  REPEAT drop here 2dup - allot over - ;

: .node  node>path type ;
: pwd  get-node .node ;

: .instance instance>qpath type ;
: .chain    dup instance>parent @ ?dup IF recurse THEN
            cr dup . instance>qname type ;


\ Alias helper
defer find-node
: set-alias ( alias-name len device-name len -- )
    encode-string
    2swap s" /aliases" find-node dup IF set-property ELSE drop THEN ;

: find-alias ( alias-name len -- false | dev-path len )
    s" /aliases" find-node dup IF
	get-property 0= IF 1- dup 0= IF nip THEN ELSE false THEN
    THEN ;

: .alias ( alias-name len -- )
    find-alias dup IF type ELSE ." no alias available" THEN ;

: (.print-alias) ( lfa -- )
    link> dup >name name>string
    \ Don't print name property
    2dup s" name" string=ci IF 2drop drop
    ELSE cr type space ." : " execute type
    THEN ;

: (.list-alias) ( phandle -- )
    node>properties @ cell+ @ BEGIN dup WHILE dup (.print-alias) @ REPEAT drop ;

: list-alias ( -- )
    s" /aliases" find-node dup IF (.list-alias) THEN ;

: devalias ( "{alias-name}<>{device-specifier}<cr>" -- )
    parse-word parse-word dup IF set-alias
    ELSE 2drop dup IF .alias
    ELSE 2drop list-alias THEN THEN ;

\ sub-alias does a single iteration of an alias at the begining od dev path
\ expression. de-alias will repeat this until all indirect alising is resolved
: sub-alias ( arg-str arg-len -- arg' len' | false )
	2dup
	2dup [char] / findchar ?dup IF ELSE 2dup [char] : findchar THEN
	( a l a l [p] -1|0 ) IF nip dup ELSE 2drop 0 THEN >r
	( a l l p -- R:p | a l -- R:0 )
	find-alias ?dup IF ( a l a' p' -- R:p | a' l' -- R:0 )
		r@ IF 2swap r@ - swap r> + swap $cat strdup ( a" l-p+p' -- )
		ELSE ( a' l' -- R:0 ) r> drop ( a' l' -- ) THEN
	ELSE ( a l -- R:p | -- R:0 ) r> IF 2drop THEN false ( 0 -- ) THEN
;

: de-alias ( arg-str arg-len -- arg' len' )
	BEGIN over c@ [char] / <> dup IF drop 2dup sub-alias ?dup THEN
	WHILE 2swap 2drop REPEAT
;


\ Display the device tree.
: +indent ( not-last? -- )
  IF s" |   " ELSE s"     " THEN $indent indent @ + swap move 4 indent +! ;
: -indent ( -- )  -4 indent +! ;
: ls-node ( node -- )
  cr $indent indent @ type
  dup peer IF ." |-- " ELSE ." +-- " THEN node>qname type ;
: (ls) ( node -- )
  child BEGIN dup WHILE dup ls-node dup child IF
  dup peer +indent dup recurse -indent THEN peer REPEAT drop ;
: ls ( -- )  get-node dup cr node>path type (ls) 0 indent ! ;

: show-devs ( {device-specifier}<eol> -- )
   skipws 0 parse dup IF de-alias ELSE 2drop s" /" THEN   ( str len )
   find-node dup 0= ABORT" No such device path" (ls)
;


VARIABLE interpose-node
2VARIABLE interpose-args
: interpose ( arg len phandle -- )  interpose-node ! interpose-args 2! ;
: open-node ( arg len phandle -- ihandle | 0 )
  current-node @ >r set-node create-instance set-my-args
  ( and set unit-addr )
\ XXX: assume default of success for nodes without open method
  s" open" ['] $call-my-method CATCH IF 2drop true THEN
  0= IF my-self destroy-instance 0 to my-self THEN
  my-self my-parent to my-self r> set-node
  \ Handle interposition.
  interpose-node @ IF my-self >r to my-self
  interpose-args 2@ interpose-node @
  interpose-node off recurse  r> to my-self THEN ;
: close-node ( ihandle -- )
  my-self >r to my-self
  s" close" ['] $call-my-method CATCH IF 2drop THEN
  my-self destroy-instance r> to my-self ;

: close-dev ( ihandle -- )
  my-self >r to my-self
  BEGIN my-self WHILE my-parent my-self close-node to my-self REPEAT
  r> to my-self ;

: new-device ( -- )
  my-self new-node node>instance @ dup to my-self instance>parent !
  get-node my-self instance>node ! ;
: finish-device ( -- )
  ( check for "name" property here, delete this node if not there )
  finish-node my-parent my-self max-instance-size free-mem to my-self ;

: split ( str len char -- left len right len )
  >r 2dup r> findchar IF >r over r@ 2swap r> 1+ /string ELSE 0 0 THEN ;
: generic-decode-unit ( str len ncells -- addr.lo ... addr.hi )
  dup >r -rot BEGIN r@ WHILE r> 1- >r [char] , split 2swap
  $number IF 0 THEN r> swap >r >r REPEAT r> 3drop
  BEGIN dup WHILE 1- r> swap REPEAT drop ;
: generic-encode-unit ( addr.lo ... addr.hi ncells -- str len )
  0 0 rot ?dup IF 0 ?DO rot (u.) $cat s" ," $cat LOOP 1- THEN ;
: hex-decode-unit ( str len ncells -- addr.lo ... addr.hi )
  base @ >r hex generic-decode-unit r> base ! ;
: hex-encode-unit ( addr.lo ... addr.hi ncells -- str len )
  base @ >r hex generic-encode-unit r> base ! ;

: handle-leading-/ ( path len -- path' len' )
  dup IF over c@ [char] / = IF 1 /string device-tree @ set-node THEN THEN ;
: match-name ( name len node -- match? )
  over 0= IF 3drop true EXIT THEN
  s" name" rot get-property IF 2drop false EXIT THEN
  1- string=ci ; \ XXX should use decode-string
0 VALUE #search-unit   CREATE search-unit 4 cells allot
: match-unit ( node -- match? )
  node>space search-unit #search-unit 0 ?DO 2dup @ swap @ <> IF
  2drop false UNLOOP EXIT THEN cell+ swap cell+ swap LOOP 2drop true ;
: match-node ( name len node -- match? )
  dup >r match-name r> match-unit and ; \ XXX e3d
: find-kid ( name len -- node|0 )
  dup -1 = IF \ are we supposed to stay in the same node? -> resolve-relatives
    2drop get-node
  ELSE
    get-node child >r BEGIN r@ WHILE 2dup r@ match-node
    IF 2drop r> EXIT THEN r> peer >r REPEAT
    r> 3drop false
  THEN ;
: set-search-unit ( unit len -- )
  dup 0= IF to #search-unit drop EXIT THEN
  s" #address-cells" get-node get-property THROW
  decode-int to #search-unit 2drop
  s" decode-unit" get-node $call-static
  #search-unit 0 ?DO search-unit i cells + ! LOOP ;
: resolve-relatives ( path len -- path' len' )
  \ handle ..
  2dup 2 = swap s" .." comp 0= and IF
    get-node parent ?dup IF
      set-node drop -1
    ELSE
      s" Already in root node." type
    THEN
  THEN
  \ handle .
  2dup 1 = swap c@ [CHAR] . = and IF
    drop -1
  THEN
  ;
: find-component ( path len -- path' len' args len node|0 )
  [char] / split 2swap ( path'. component. )
  [char] : split 2swap ( path'. args. node-addr. )
  [char] @ split ['] set-search-unit CATCH IF 2drop 2drop 0 EXIT THEN
  resolve-relatives find-kid ;

: .find-node ( path len -- phandle|0 )
  current-node @ >r
  handle-leading-/ current-node @ 0= IF 2drop r> set-node 0 EXIT THEN
  BEGIN dup WHILE \ handle one component:
  find-component ( path len args len node ) dup 0= IF
  3drop 2drop r> set-node 0 EXIT THEN
  set-node 2drop REPEAT 2drop
  get-node r> set-node ;
' .find-node to find-node
: find-node ( path len -- phandle|0 ) de-alias find-node ;

: delete-node ( phandle -- )
   dup node>parent @ node>child @ ( phandle 1st peer )
   2dup = IF
     node>peer @ swap node>parent @ node>child !
     EXIT
   THEN
       dup node>peer @
       BEGIN 2 pick 2dup <> WHILE
	     drop
        	nip dup node>peer @
	   dup 0= IF 2drop drop unloop EXIT THEN
      REPEAT
         drop
    node>peer @ 	swap node>peer !
      drop
;


: open-dev ( path len -- ihandle|0 )
  de-alias current-node @ >r
  handle-leading-/ current-node @ 0= IF 2drop r> set-node 0 EXIT THEN
  my-self >r 0 to my-self
  0 0 >r >r BEGIN dup WHILE \ handle one component:
  ( arg len ) r> r> get-node open-node to my-self
  find-component ( path len args len node ) dup 0= IF
  3drop 2drop my-self close-dev r> to my-self r> set-node 0 EXIT THEN
  set-node >r >r REPEAT 2drop
  \ open final node
  r> r> get-node open-node to my-self
  my-self r> to my-self r> set-node ;
: select-dev  open-dev dup to my-self ihandle>phandle set-node ;

: find-device ( str len -- ) \ set as active node
  find-node dup 0= ABORT" No such device path" set-node ;
: dev  skipws 0 parse find-device ;

: (lsprop) ( node --)
	dup cr $indent indent @ type ."     node: " node>qname type
	false +indent (.properties) cr -indent ;
: (show-children) ( node -- )
    child BEGIN dup WHILE
	dup (lsprop) dup child IF false +indent dup recurse -indent THEN peer
    REPEAT drop
;
: lsprop ( {device-specifier}<eol> -- )
   skipws 0 parse dup IF de-alias ELSE 2drop s" /" THEN
   find-device get-node dup dup
   cr ." node: " node>path type (.properties) cr (show-children) 0 indent ! ;


\ node>path does not allot the memory, since it is internally only used
\ for typing.
\ The external variant needs to allot memory !

: (node>path) node>path ;

: node>path ( phandle -- str len )
   node>path dup allot
;

\ Support for support packages.

\ The /packages node.
0 VALUE packages

\ We can't use the standard find-node stuff, as we are required to find the
\ newest (i.e., last in our tree) matching package, not just any.
: find-package  ( name len -- false | phandle true )
  0 >r packages child BEGIN dup WHILE dup >r node>name 2over string=ci r> swap
  IF r> drop dup >r THEN peer REPEAT 3drop r> dup IF true THEN ;

: open-package ( arg len phandle -- ihandle | 0 )  open-node ;
: close-package ( ihandle -- )  close-node ;
: $open-package ( arg len name len -- ihandle | 0 )
  find-package IF open-package ELSE 2drop false THEN ;


\ Pseudocode in C Syntax
\ if((addr>=child)&&(addr<=child+size)
\    return (addr - child) + parent
\ else return false
\
: translate-range ( child parent size addr -- taddr true | addr false )
    swap 3 pick + over                    \ calculate child+size address
    ( child parent size addr child+size )
    > IF                                  \ verify if addr is below child+size address
        ( child parent addr )
        2 pick over                       \ fetch child and addr for compare
        ( child parend addr child addr )
        <= IF                             \ verify if addr is above child address
            ( child parent addr )
            2 pick - + nip true           \ pick child, calculate addr-child + parent, drop child and return true
            ( taddr true )
        ELSE
            2drop false                   \ drop child parent size and return false
            ( addr false )
        THEN
    ELSE
        ( child parent addr )
        nip nip false                     \ drop child parent size and return false
        ( addr false )
    THEN
;

\ helper function based on decode-int to decode an integer property
\ from a prop-encoded-array
\ my-property cannot be used since this depends on a current instance
: get-property-decoded ( addr len -- n )
  get-node get-property
  IF cr cr cr ." get-property-decoded: no such property" EXIT THEN decode-int nip nip
;

0 VALUE pci-phys-hi
1C000000 CONSTANT pci-stop-mapping-code
\ Explanation to pci-stop-mapping-code:
\   Bits 26..28 are unsused in phys.hi in the IEEE 1275 PCI binding
\   and set to 0. Use value where these bits are set in pci-phys-hi to communicate that
\   translation sould stop.

\ Helper function to extract one element of the child parent size tuple coded
\ into the ranges properties array, element being exactly one of child, parent
\ and size
: extract-range-element ( ranges-addr ranges-len #cells -- element ranges-addr' ranges-len' )
    \  -rot decode-int 3 roll 1 > IF 20 lshift -rot decode-int 3 roll + THEN -rot
    CASE
        1 OF decode-int -rot ENDOF
        2 OF decode-int 20 lshift -rot decode-int 3 roll + -rot ENDOF
        3 OF
            BEGIN
                dup 0= IF
                    false                                        ( ranges-addr ranges-len false )
                ELSE
                    decode-int
                    pci-phys-hi   \ for PCI phys.hi lies on the stack below addr
                    space-code-mask and
                    <> \ compare phys.hi
                THEN
            WHILE
                \ discard phys.mid, phys.lo, parent, and size values. Then go to next PCI ranges tuple
                    18 dup -rot - -rot + swap
            REPEAT

            dup 0= IF                                            ( ranges-addr ranges-len )
                pci-stop-mapping-code to pci-phys-hi             ( ranges-addr ranges-len )
            ELSE
                \ ranges size >= 8, since phys.hi
                \ was read in ELSE of WHILE condition
                decode-int 20 lshift -rot decode-int 3 roll + -rot
            THEN
        ENDOF
    ENDCASE
;

\ Function to convert a whole child parent size sequence into decoded-int format
: extract-range ( ranges-addr ranges-len -- child parent size ranges-addr' ranges-len' )
    \ child
    s" #address-cells" get-property-decoded
    extract-range-element
    \ exit criterium for PCI: ranges-len is 0 and false on top of stack
    pci-phys-hi pci-stop-mapping-code = IF EXIT THEN    ( ranges-addr ranges-len )

    \ parent                                            ( child ranges-addr' ranges-len' )
    decode-phys                                         ( child ranges-addr" ranges-len" phys.lo .. phys.hi )
    my-#address-cells 1 > IF 20 lshift + THEN           ( child ranges-addr''' ranges-len''' parent )
    -rot                                                ( child parent ranges-addr''' ranges-len''' )

    \ size
    s" #size-cells" get-property-decoded                ( child parent ranges-addr''' ranges-len''' #size-cells )
    extract-range-element                               ( child parent size ranges-addr"" ranges-len"" )
;

\ Function to process a whole array one or more of child parent size sequences
\ Prerequisite: Empty ranges handing is assumed to already exist.
: translate-ranges-node ( addr ranges-addr ranges-len -- taddr true|false )
    BEGIN
        dup 0 >                         \ ranges-len > 0
    WHILE
        extract-range
        pci-phys-hi pci-stop-mapping-code =
        IF                                              ( addr ranges-addr ranges-len )
            nip nip EXIT                                ( false )
        THEN
        ( addr child parent size ranges-addr' ranges-len' )
        2rot                                            ( parent size ranges-addr' ranges-len' addr child )
        5 roll                                          ( size ranges-addr' ranges-len' addr child parent )
        5 roll                                          ( ranges-addr' ranges-len' addr child parent size )
        3 roll                                          ( ranges-addr' ranges-len' child parent size addr )
        translate-range                 ( ranges-addr' ranges-len' taddr true | ranges-addr' ranges-len' addr false )
        IF nip nip true EXIT
        ELSE ( ranges-addr' ranges-len' addr )
            -rot
            ( addr ranges-addr' ranges-len' )
        THEN
    REPEAT
    ( ranges-addr' ranges-len' taddr true | ranges-addr' ranges-len' false )
    \ remove  addr ranges-addr' ranges-len' from stack
    nip nip \ leaving the 0 ranges-len' as false
    ( false )
;

\ Helper function to search the first ranges in current node or one of its parents
\ and make that node the 'current node'
\ Prerequisite: root node must have a ranges property
\ Returns address, length, true if ranges property was found, otherwise false.
: translate-set-to-next-ranges-node ( -- addr-ranges len-ranges true|false )
    s" ranges" 2dup get-node get-property
    IF
        ( addr len true )
        get-parent dup set-node get-property
        IF
            cr cr cr
            s" no translatable address space due to missing ranges property" type
            cr cr cr
            false
        ELSE
            true
        THEN
    ELSE
        ( addr len addr-ranges len-ranges )
        rot drop rot drop true
    THEN
    ( ranges-addr ranges-len true|false )
;

: translate-address-end ( phandle-start taddr true|phandle-start false )
    \ get back to the node where translation was started
    dup IF
        rot                                               ( taddr true phandle-start )
        set-node                                          ( taddr true )
    ELSE
        swap                                              ( phandle-start false )
        set-node                                          ( false )
    THEN
;

\ Function to step up the device tree up to the root node.
\ Contains empty ranges handling.
\ Returns the translated address and true, when the address is translatable, otherwise false.
: translate-ranges ( addr -- taddr true|false )
    BEGIN
        \ set-node semantic required here to continue from nodes found below.
        translate-set-to-next-ranges-node
        not IF false EXIT THEN ( false )   \ address is not translatable
                                                 \ due to missing ranges property in the hierarchy.
        ( phandle-start addr ranges-addr ranges-len )
        dup 0=
        IF
            \ empty ranges property detected, assume 1 : 1 translation
            2drop true
            ( phandle-start addr true )
        ELSE
            ( phandle-start addr ranges-addr ranges-len )
            translate-ranges-node
            ( phandle-start taddr true|phandle-start false )
        THEN
        dup IF
            ( phandle-start taddr true )      \ found a translation
            drop
            get-parent
            dup 0=
            IF                                \ arrived at root node, stop translation
                drop true dup
                ( phandle-start taddr true true )
            ELSE
                \ go to parent and continue
                set-node false
                ( phandle-start taddr true phandle-parent false )
            THEN
        ELSE
            true                              \ address translation failed, exit loop
            ( phandle-start false true )
        THEN
    UNTIL
    ( phandle-start taddr true|phandle-start false )
;


: translate-address-back-to-start-node ( phandle-start taddr true|phandle-start false )
    \ get back to the node where translation was started
    dup IF
        rot                                               ( taddr true phandle-start )
        set-node                                          ( taddr true )
    ELSE
        swap                                              ( phandle-start false )
        set-node                                          ( false )
    THEN
;

: translate-address ( addr -- taddr true|false )
    get-node swap         \ save current node             ( phandle-start addr  )
    translate-ranges                                      ( phandle-start taddr true|phandle-start false )
    translate-address-back-to-start-node                  ( taddr true|false )
;


: translate-address-pci ( phys.lo phys.mid phys.hi -- taddr true|false )
    to pci-phys-hi                                        ( phys.lo phys.mid )
    lxjoin                                                ( phys.addr )
    get-node              \ save current node             ( phys.addr phandle-start )
    swap                                                  ( phandle-start phys.addr )
    translate-ranges      \ fetches phys.hi for PCI       ( phandle-start taddr true|phandle-start false )
    translate-address-back-to-start-node                  ( taddr true|false )
;

\ device tree translate-address
#include <translate.fs>
