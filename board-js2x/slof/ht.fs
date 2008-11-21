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


\ Hypertransport.

\ See the PCI OF binding document.

new-device   s" /ht" full-name

s" ht" 2dup device-name device-type
s" u3-ht" compatible
s" U3" encode-string s" model" property

3 encode-int s" #address-cells" property
2 encode-int s" #size-cells" property

\ 16MB of configuration space, seperate for type 0 and type 1.
00000000 encode-int  f2000000 encode-int+
00000000 encode-int+ 02000000 encode-int+ s" reg" property

\ 4MB of I/O space.
01000000 encode-int  00000000 encode-int+ 00000000 encode-int+ 
00000000 encode-int+ f4000000 encode-int+ 
00000000 encode-int+ 00400000 encode-int+

\ 1.75GB of memory space @ 80000000.
02000000 encode-int+ 00000000 encode-int+ 80000000 encode-int+ 
00000000 encode-int+ 80000000 encode-int+ 
00000000 encode-int+ 70000000 encode-int+ s" ranges" property

\ Host bridge, so full bus range.
0 encode-int ff encode-int+ s" bus-range" property

: enable-ht-apic-space  0 5 oco drop ;
enable-ht-apic-space

\ This works for the AMD devices; should walk PCI capabilities list
\ to be more portable.
: assign-buid ( this -- next )
  00c2 config-w@ 5 rshift over 00c2 config-w! + ;
: assign-buids
  1 BEGIN 0 config-l@ ffffffff <> WHILE assign-buid REPEAT drop ;
assign-buids

: open  true ;
: close ;

\ The devices on our HT chain
INCLUDE hw/8131.fs
INCLUDE hw/8111.fs

finish-device
