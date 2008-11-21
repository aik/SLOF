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

\ AMD 8111 I/O hub.

\ See the documentation at http://www.amd.com ; the datasheet for this chip is
\ document #24674.

\ First, some master config.  Not all of this logically belongs to just
\ one function, and certainly not to the LPC bridge; also, we'll
\ initialize all functions in "downstream" order, and this code has to be
\ run first.  So do it now.

00 842 config-b! \ Disable 8237 & 8254 & 8259's.  We're not a PC.
80 847 config-b! \ Disable EHCI, as it is terminally broken.
03 848 config-b! \ Enable LPC, IDE; disable I2C, SMM, AC'97 functions.
01 849 config-b! \ Enable USB, disable 100Mb enet.
01 84b config-b! \ Enable IO-APIC.

fec00000 s" ioapic.fs" included
00 init-ioapic

\ Program PNPIRQ[0,1,2] as IRQ #D,E,F; switch those GPIOs to low-active.
  0b 848 config-b! \ enable devB:3
7000 b58 config-l! \ map PMxx at pci i/o 7000
  d1 b41 config-b! \ enable access to PMxx space

\ on JS20 the planar id is encoded in GPIO 29, 30 and 31
\ >=5 is GA2 else it is GA1
: (planar-id) ( -- planar-id)
   [ 70dd io-c@ 5 rshift 1 and ]  LITERAL
   [ 70de io-c@ 5 rshift 2 and ]  LITERAL
   [ 70df io-c@ 5 rshift 4 and ]  LITERAL
   + + 7 xor
;

u3?  IF  [']  (planar-id) to planar-id  THEN

8 70d3 io-c!  8 70d4 io-c!  8 70d5 io-c! \ set the pins to low-active
 bimini? IF 5 70c4 io-c! THEN \ on bimini set gpio4 as output and high to power up USB
 fec b44 config-w! \ set PNPIRQ pnpirq2 -> f , pnpirq1 -> e pnpirq0 -> c
  51 b41 config-b! \ disable access to PMxx space
  03 848 config-b! \ disable devB:3

\ The function of the PCI controller BARs change depending on the mode the
\ controller is in.
\ And the default is legacy mode.  Gross.
05 909 config-b! \ Enable native PCI mode.
03 940 config-b! \ Enable both ports.

\ Enable HPET on 8111, at address fe000000.
fe000001 8a0 config-l!

: >hpet  fe000000 + ;
: hpet@  >hpet rl@-le ;
: hpet!  >hpet rl!-le ;

INCLUDE freq.fs

\ Disable HPET.
0 8a0 config-l!

\ 8111 has only 16 bits of PCI I/O space.  Get the address in range.
8000 next-pci-io !

my-space pci-class-name type cr
my-space pci-bridge-generic-setup
s" pci" device-name
