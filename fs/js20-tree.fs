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


\ The root of the device tree and some of its kids.

new-device   s" /" full-name

s" /" device-name
2 encode-int s" #address-cells" property
2 encode-int s" #size-cells" property

\ XXX: what requires this?  Linux?
0 encode-int  f8040000 encode-int+
0 encode-int+ f8050000 encode-int+ s" platform-open-pic" property

\ Yaboot is stupid.  Without this, it can't/won't find /etc/yaboot.conf.
s" chrp we are not, but smart yaboot is not" device-type

\ To get linux-2.6.10 and later to work out-of-the-box.
s" Momentum,Maple" compatible

: open  true ;
: close ;

\ See 3.5.
new-device   s" /openprom" full-name
s" openprom" device-name
s" SlimLine Open Firmware (SLOF) version 0.0" encode-string s" model" property
0 0 s" relative-addressing" property
finish-device

\ See 3.5.
new-device   s" /chosen" full-name
s" chosen" device-name
finish-device

\ See 3.6.5, and the PowerPC OF binding document.
new-device   s" /mmu" full-name
s" mmu" 2dup device-name device-type
0 0 s" translations" property
finish-device

INCLUDE hw/memory.fs
INCLUDE hw/mpic.fs
INCLUDE hw/dart.fs
INCLUDE hw/ht.fs

\ See the PowerPC OF binding document.
new-device   s" /cpus" full-name
s" cpus" device-name
1 encode-int s" #address-cells" property
0 encode-int s" #size-cells" property

INCLUDE hw/cpu.fs
: create-cpu-nodes
  master-cpu create-cpu-node
  slave? IF master-cpu 1 xor create-cpu-node THEN ;
create-cpu-nodes

finish-device

finish-device



: set-chosen ( prop len name len -- )
  s" /chosen" find-package drop set-property ;

master-cpu s" /cpus/cpu@" rot (u.) $cat open-dev encode-int s" cpu" set-chosen
s" /memory" open-dev encode-int s" memory" set-chosen
s" /mmu" open-dev encode-int s" mmu" set-chosen

: io  open-dev dup encode-int s" stdout" set-chosen
                   encode-int s" stdin"  set-chosen ;

s" /ht/isa@4/serial" io
