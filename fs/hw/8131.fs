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


\ AMD 8131 dual PCI-X bridge with tunnel.

\ See the documentation at http://www.amd.com ; the datasheet for this chip is
\ document #24637.


\ Device A, function 0: PCI-X bridge.

new-device   s" /ht/pci-x@1" full-name

s" pci-x" 2dup device-name device-type
s" pci" compatible

07 0804 config-b! \ Enable master, memory, I/O.
00 0818 config-b! \ Set primary bus#.
10 0819 config-b! \ Set secondary bus#.
1f 081a config-b! \ Set subordinate bus#.
f000 081c config-l!  00010001 0830 config-l! \ Set I/O space: 64kB @ 10000.
9ff09000 0820 config-l! \ Set memory space: 256MB @ 90000000.

: open  true ;
: close ;

\ This is the JS20 extension card slot; we assume there is nothing there.

finish-device


\ Device A, function 1: IO-APIC.

new-device   s" /ht/io-apic@1,1" full-name

s" io-apic" 2dup device-name device-type

03 0944 config-b! \ Enable IO-APIC.
06 0904 config-b! \ Enable IRQ generation and memory space.
fec01000 0948 config-l!  0 094c config-l! \ Set APIC address.
10 fec01000 rb! 0000a018 fec01010 rl!-le
11 fec01000 rb! ff000000 fec01010 rl!-le
12 fec01000 rb! 0000a019 fec01010 rl!-le
13 fec01000 rb! ff000000 fec01010 rl!-le
14 fec01000 rb! 0000a01a fec01010 rl!-le
15 fec01000 rb! ff000000 fec01010 rl!-le
16 fec01000 rb! 0000a01b fec01010 rl!-le
17 fec01000 rb! ff000000 fec01010 rl!-le \ Set IRQs as 18..1b.

: open  true ;
: close ;

finish-device


\ Device B, function 0: PCI-X bridge.

new-device   s" /ht/pci-x@2" full-name

s" pci-x" 2dup device-name device-type
s" pci" compatible

07 1004 config-b! \ Enable master, memory, I/O.
00 1018 config-b! \ Set primary bus#.
20 1019 config-b! \ Set secondary bus#.
2f 101a config-b! \ Set subordinate bus#.
f000 101c config-l!  00020002 1030 config-l! \ Set I/O space: 64kB @ 20000.
aff0a000 1020 config-l! \ Set memory space: 256MB @ a0000000.

: open  true ;
: close ;

\ This PCI-X bus is where the dual Gb ethernet is connected.

INCLUDE hw/enet.fs

finish-device


\ Device B, function 1: IO-APIC.

new-device   s" /ht/io-apic@2,1" full-name

s" io-apic" 2dup device-name device-type

03 1144 config-b! \ Enable IO-APIC.
06 1104 config-b! \ Enable IRQ generation and memory space.
fec02000 1148 config-l!  0 114c config-l! \ Set APIC address.
10 fec02000 rb! 0000a01c fec02010 rl!-le
11 fec02000 rb! ff000000 fec02010 rl!-le
12 fec02000 rb! 0000a01d fec02010 rl!-le
13 fec02000 rb! ff000000 fec02010 rl!-le
14 fec02000 rb! 0000a01e fec02010 rl!-le
15 fec02000 rb! ff000000 fec02010 rl!-le
16 fec02000 rb! 0000a01f fec02010 rl!-le
17 fec02000 rb! ff000000 fec02010 rl!-le \ Set IRQs as 1c..1f.

: open  true ;
: close ;

finish-device
