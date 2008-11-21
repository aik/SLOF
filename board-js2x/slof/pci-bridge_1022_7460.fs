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


\ AMD 8111 I/O hub.

\ See the documentation at http://www.amd.com ; the datasheet for this chip is
\ document #24674.


\ First, some master config.  Not all of this logically belongs to just
\ one function, and certainly not to the LPC bridge; also, we'll
\ initialize all functions in "downstream" order, and this code has to be
\ run first.  So do it now.

00 2042 config-b! 
03 2048 config-b! \ Enable LPC, IDE; disable I2C, SMM, AC'97 functions.
00 2049 config-b! \ Disable everything on second bus (USB, 100Mb enet).

01 204b config-b! \ Enable IO-APIC.
30 fec00000 rb! 0000a010 fec00010 rl!-le
31 fec00000 rb! ff000000 fec00010 rl!-le
32 fec00000 rb! 0000a011 fec00010 rl!-le
33 fec00000 rb! ff000000 fec00010 rl!-le
34 fec00000 rb! 0000a012 fec00010 rl!-le
35 fec00000 rb! ff000000 fec00010 rl!-le
36 fec00000 rb! 0000a013 fec00010 rl!-le
37 fec00000 rb! ff000000 fec00010 rl!-le \ Set PCI IRQs as 10..13.
                                         \ Leave ISA IRQs disabled.


\ Device A, function 0: PCI bridge.

\ We show this bridge in the device-tree, for completeness, as it is
\ impossible to completely disable this device; we don't assign any
\ address space to it though, or enable any transactions through it.
\ Maybe later we want to support the USB functions on its secondary
\ side; but not now.

new-device   s" /ht/pci@3" full-name

s" pci" 2dup device-name device-type

: open  true ;
: close ;

finish-device


\ Device B, function 0: LPC bridge.

new-device   s" /ht/isa@4" full-name

\ See the "ISA/EISA/ISA-PnP" OF binding document.

s" isa" 2dup device-name device-type
\ We have to say it's ISA i.s.o. LPC, as otherwise Linux can't find
\ the serial port for its console.  Linux uses the name instead of the
\ device type (and it completely ignores any "compatible" property).

\ 64kB of ISA I/O space, at PCI devfn 4:0.
1 encode-int 0 encode-int+
01002000 encode-int+ 0 encode-int+ 0 encode-int+
10000 encode-int+ s" ranges" property

: open  true ;
: close ;

\ There's a SIO chip on the LPC bus.
INCLUDE hw/sio.fs

finish-device


\ Device B, function 1: ATA controller.

new-device   s" /ht/ata@4,1" full-name

s" ata" 2dup device-name device-type
s" ide" compatible

2108 dup config-l@ 500 or swap config-l! \ Enable native PCI mode.
2104 dup config-l@ 5   or swap config-l! \ Enable I/O, bus master.
2140 dup config-l@ 03  or swap config-l! \ Enable both ports.
10 213c config-b!                        \ Set IRQ#.

: open  true ;
: close ;

\ Just assume there is always one disk.
INCLUDE hw/disk.fs

\ Enable HPET at address fe000000.
fe000001 20a0 config-l!

: >hpet  fe000000 + ;
: hpet@  >hpet rl@-le ;
: hpet!  >hpet rl!-le ;

INCLUDE hw/freq.fs

\ Disable HPET.
0 20a0 config-l!

finish-device
