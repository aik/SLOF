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

\ needed to find the right path in the device tree
: decode-unit ( addr len -- phys.lo ... phys.hi )
   2 hex-decode-unit       \ decode string
   b lshift swap           \ shift the devicenumber to the right spot
   8 lshift or             \ add the functionnumber
   \ my-bus 10 lshift or   \ add the busnumber (assume always bus 0)
   0 0 rot                 \ make phys.lo = 0 = phys.mid
;

\ needed to have the right unit address in the device tree listing
\ phys.lo=phys.mid=0 , phys.hi=config-address
: encode-unit ( phys.lo phys-mid phys.hi -- unit-str unit-len )
   nip nip                     \ forget the phys.lo and phys.mid
   dup 8 rshift 7 and swap     \ calculate function number
   B rshift 1F and             \ calculate device number
   over IF 2 ELSE nip 1 THEN   \ create string with dev#,fn# or dev# only?
   hex-encode-unit
;


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


\ Scan the child nodes of the pci root node to assign bars, fixup
\ properties etc.
: setup-children
   my-self >r                       \ Save old value of my-self
   puid >r                          \ Save old value of puid
   my-puid TO puid                  \ Set current puid
   get-node child
   BEGIN
      dup                           \ Continue as long as there are children
   WHILE
      \ ." Working on " dup node>path type cr
      \ Open child node:
      dup set-node
      dup 0 0 rot open-node ?dup 0<> IF
         ( child-phandle child-ihandle )
         dup to my-self
         dup ihandle>phandle node>instance-size @   \ Remember instance size
         \ Include the PCI device functions:
         s" pci-device.fs" included
         \ Clean up the temporary instance. Note that we can not use close-node
         \ or destroy-instance here since node>instance-size might have changed.
         ( child-phandle child-ihandle instance-size )
         free-mem
      THEN                          ( child-phandle )
      peer
   REPEAT
   drop
   r> TO puid                       \ Restore previous puid
   r> to my-self                    \ Restore previous my-self
;

setup-children
