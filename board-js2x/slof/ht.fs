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


\ Hypertransport.

\ See the PCI OF binding document.

new-device


s" ht" 2dup device-name device-type
s" u3-ht" compatible
s" U3" encode-string s" model" property

3 encode-int s" #address-cells" property
2 encode-int s" #size-cells" property

s" /mpic" find-node encode-int s" interrupt-parent" property
\ XXX should have interrupt map, etc.  this works for now though.
\ linux gets the PCI irqs right; only need to tell it the lpc irqs are edge.

: decode-unit  2 hex-decode-unit 3 lshift or
               8 lshift 0 0 rot ;
: encode-unit  nip nip ff00 and 8 rshift dup 7 and swap 3 rshift
               over IF 2 ELSE nip 1 THEN hex-encode-unit ;

f2000000 CONSTANT my-puid
\ Configuration space accesses.
: >config  dup ffff > IF 1000000 + THEN f2000000 + ;

\ : config-l!  >config cr ." config-l! " 2dup . space . rl!-le ;
\ : config-l@  >config cr ." config-l@ " dup . rl@-le space dup . ;
\ : config-w!  >config cr ." config-w! " 2dup . space . rw!-le ;
\ : config-w@  >config cr ." config-w@ " dup . rw@-le space dup . ;
\ : config-b!  >config cr ." config-b! " 2dup . space . rb! ;
\ : config-b@  >config cr ." config-b@ " dup . rb@ space dup . ;

: config-l!  >config rl!-le ;
: config-l@  >config rl@-le ;
: config-w!  >config rw!-le ;
: config-w@  >config rw@-le ;
: config-b!  >config rb! ;
: config-b@  >config rb@ ;

: config-dump ( addr size -- )  ['] config-l@ 4 (dump) ;


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
00000000 encode-int+ 4000000 encode-int+ s" ranges" property

\ Host bridge, so full bus range.
0 encode-int ff encode-int+ s" bus-range" property

: enable-ht-apic-space 3c0300fe f8070200 rl! ;
enable-ht-apic-space

\ spare out 0xc0000000-0xefffffff for pcie
f8070200 rl@ fffffff0 and f8070200 rl!
\ enable io memory for pcie @ c0000000-efffffff
70000003 f80903f0 rl!-le


\ Workaround for "takeover" boot on JS20: the top 8131 is programmed to be
\ device #1f, while it should be #01.
u3? IF f800 config-l@ 74501022 = IF 41 f8c2 config-w! THEN THEN

\ Assign BUIDs.

: find-ht-primary
  34 BEGIN config-b@ dup 0= ABORT" No HT capability block found!"
  dup config-l@ e00000ff and 8 = IF 2 + EXIT THEN 1 + AGAIN ;

: assign-buid ( this -- next )
  find-ht-primary dup >r config-w@ 5 rshift 1f and over r> config-b! + ;

: assign-buids ( -- )
  1 BEGIN 0 config-l@ ffffffff <> WHILE assign-buid REPEAT drop ;

assign-buids 

: ldtstop  f8000840 rl@ 40000 or f8000840 rl! ;
: delay 100000 0 DO LOOP ;
: wait-for-done  BEGIN f8070110  rl@ 30 and UNTIL
                 BEGIN 8b4 config-l@ 30 and UNTIL ;
: ldtstop1  f8000840 rl@ dup 20000 or f8000840 rl! delay
            f8000840 rl! wait-for-done ;
: warm  400000 f8070300 rl! 0 f8070300 rl! ;

: dumpht  cr f8070110 rl@ 8 0.r space 8b4 config-l@ 8 0.r
       space f8070122 rb@ 2 0.r space 8bd config-b@ 2 0.r ; 

: clearht  f8070110 dup rl@ swap rl!
           f8070120 dup rl@ swap rl!
           08b4 dup config-l@ swap config-l!
           08bc dup config-l@ swap config-l! ;

: setwidth  dup f8070110 rb! 8b7 config-b! ;
: set8   00 setwidth ;
: set16  11 setwidth ;

: setfreq  dup f8070122 rb! 8bd config-b! ;
: set200   0 setfreq ;
: set300   1 setfreq ;
: set400   2 setfreq ;
: set500   3 setfreq ;
: set600   4 setfreq ;
: set800   5 setfreq ;
: set1000  6 setfreq ;
: set1200  7 setfreq ;
: set1400  8 setfreq ;
: set1600  9 setfreq ;

: ht>freq  2 + dup 6 > IF 2* 6 - THEN d# 100 * ;
\ XXX: looks only at the U3/U4 side for max. link speed and width.
clearht f8070111 rb@ setwidth
f8070120 rw@ 2log dup .(  Switching top HT bus to ) ht>freq 0 d# .r .( MHz...) cr
setfreq u3? IF ldtstop THEN u4? IF ldtstop1 THEN


\ #include <pci-scan.fs>


: open  true ;
: close ;


\ 80000000 next-pci-mem !
\ b8000000  max-pci-mem !
\ b8000000 next-pci-mmio !
\ c0000000  max-pci-mmio !
\    10000 next-pci-io !
\ 100000000  max-pci-io !
\        0 next-pci-bus !

\ 0 probe-pci
\ : probe-pci-host-bridge ( bus-max bus-min mmio-max mmio-base mem-max mem-base io-max io-base my-puid -- )
s" /mpic" find-node my-puid pci-irq-init drop
1f 0 c0000000 b8000000 b8000000 80000000 100000000 10000
my-puid probe-pci-host-bridge




: msi
\  \ on citrine (pass2)
\  f8005000 101054 config-l!   0 101058 config-l!
\         f 10105c config-w!  81 101052 config-b!

\  \ on citrine (pass4)
\  f8005000 010854 config-l!   0 010858 config-l!
\         f 01085c config-w!  81 010852 config-b!

  \ on citrine (pass4), using old vector
  f80040f0 010854 config-l!   0 010858 config-l!
      ffff 01085c config-w!  81 010852 config-b!

  \ on pxb1
\  f8000000 08a4 config-l!   0 08a8 config-l!  1 08a2 config-b! ;
\  f8000000 08a4 config-l!  fd 08a8 config-l!  1 08a2 config-b! ;

;



\ This works.  Needs cleaning up though; and we need to communicate the
\ MSI address range to the client program.  (We keep the default range
\ at fee00000 for now).
: msi-on  7 1 DO 10000 i 800 * a0 + config-l! LOOP ;
msi-on



\ \ \
\ \ \ IRQ DEBUG CODE
\ \ \

: >mpic0  f8041000 + ;
: mpic0@  >mpic0 rl@ ;
: mpic0!  >mpic0 rl! ;

: >mpic  f8050000 + ;
: mpic@  >mpic rl@ ;
: mpic!  >mpic rl! ;

: mpic-on  dup 20 * 00880000 rot + over mpic! 1 swap 10 + mpic! ;
: mpic-all-on  7c 0 DO i mpic-on LOOP ;

: mpic-cpu-on  0 f8060080 rl! ;
: mpic-cur  cr ." pending: " 4 0 DO f80600a0 i c lshift + rl@ . LOOP ;
: mpic-eoi  0 f80600b0 rl! ;

: one-by-one  7c 0 DO cr i . i mpic-on mpic-cur mpic-eoi LOOP ;

CREATE pcie-cnfg-space 24 allot

: .pci-express-capabilites-reg ( val -- )
  cr ." Cap ID    :" dup ff and u.
  cr ." Next Cap  :" dup 8 rshift ff and u.
  cr ." Type      :" dup 14 rshift f and
  CASE
  0 OF ." PCI Express Endpoint" ENDOF
  1 OF ." Legacy PCI Express Endpoint" ENDOF
  4 OF ." Root Port" ENDOF
  5 OF ." Switch upstream port" ENDOF
  6 OF ." Switch downstream port" ENDOF
  7 OF ." Express-to-PCI/PCI-X bridge" ENDOF
  8 OF ." PCI/PCI-X to Express bridge" ENDOF
  dup OF 0 ENDOF ." Reserved" ENDCASE  
  drop
;

: .pci-device-capabilites-reg ( val -- )
  cr ." MaxPS    :" dup 7 and u.
  drop
;

: .pci-device-control-reg ( val -- )
  cr ." MaxPS act:" dup 5 rshift 7 and u.
  cr ." MaxRS act:" dup c rshift 7 and u.
  cr ." ERROR    :" dup 10 rshift 3f and u.
  drop
;

: .pci-link-capabilites-reg ( val -- )
  cr ." Max linkW:" dup 4 rshift 3f and u.
  drop
;

: .pci-link-control-reg ( val -- )
  cr ." Neg linkW:" dup 4 10 + rshift 3f and u.
  cr ." Train ERR:" dup a 10 + rshift 1 and u.
  cr ." Train act:" dup b 10 + rshift 1 and u.
  drop
;

: .pcie-ext
  8 0 DO dup i 4 * dup >r + config-l@  r> pcie-cnfg-space + l! LOOP
\  pcie-cnfg-space 24 dump
  pcie-cnfg-space      l@ .pci-express-capabilites-reg
  pcie-cnfg-space 4  + l@ .pci-device-capabilites-reg
  pcie-cnfg-space 8  + l@ .pci-device-control-reg
  pcie-cnfg-space c  + l@ .pci-link-capabilites-reg
  pcie-cnfg-space 10 + l@ .pci-link-control-reg
;


\ PCIe debug / fixup
: find-pcie-cap ( devfn -- offset | 0 )
  >r 34 BEGIN r@ + config-b@ dup ff <> over and WHILE
  dup r@ + config-b@ 10 = IF r> drop EXIT THEN 1+ REPEAT r> 2drop 0 ;
: .pcie ( devfn -- )
  dup find-pcie-cap ?dup IF cr over . ." cap @ " dup . +
\ .pcie-ext
  dup 8 + config-w@ 5 rshift 7 and 80 swap lshift cr ."  max payload size: " .d
  dup 8 + config-w@ c rshift 7 and 80 swap lshift cr ."  max read req: " .d
  dup 12 + config-w@ 4 rshift 3f and              cr ."  link width: " .d
  THEN drop ;
: .pcies ( -- )
  cr cr ." PCIe:"
  10000 0 DO i 8 lshift .pcie LOOP ;

: (set-ps) ( ps addr -- )
  8 + >r 5 lshift r@ config-w@ ff1f and or r> config-w! ;
: set-ps ( ps -- )
  log2 7 -
  10000 0 DO i 8 lshift dup find-pcie-cap ?dup IF
  + 2dup (set-ps) THEN drop LOOP drop ;

: (set-rr) ( rr addr -- )
  8 + >r c lshift r@ config-w@ 8fff and or r> config-w! ;
: set-rr ( rr -- )
  log2 7 -
  10000 0 DO i 8 lshift dup find-pcie-cap ?dup IF
  + 2dup (set-rr) THEN drop LOOP drop ;

bimini? IF 
	100 set-ps  200 set-rr  
\	.pcies
ELSE
	100 set-ps  200 set-rr  
\	.pcies
THEN

: set-ps  set-ps .pcies ;
: set-rr  set-rr .pcies ;


finish-device
