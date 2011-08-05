\ *****************************************************************************
\ * Copyright (c) 2011 IBM Corporation
\ * All rights reserved.
\ * This program and the accompanying materials
\ * are made available under the terms of the BSD License
\ * which accompanies this distribution, and is available at
\ * http://www.opensource.org/licenses/bsd-license.php
\ *
\ * Contributors:
\ *     IBM Corporation - initial implementation
\ ****************************************************************************/
\ * Open Firmware 1275 DMA transfer mapping functions
\ *****************************************************************************

0 VALUE dma-debug?

0 VALUE dma-window-liobn        \ Logical I/O bus number
0 VALUE dma-window-base         \ Start address of window
0 VALUE dma-window-size         \ Size of the window


\ Read DMA window base and size from ibm,dma-window property
: update-dma-window-vars  ( -- )
   s" ibm,dma-window" get-node get-property 0= IF
      decode-int TO dma-window-liobn
      decode-64 TO dma-window-base
      decode-64 TO dma-window-size
      2drop
   THEN
;
update-dma-window-vars


: dma-alloc ( size -- virt )
   dma-debug? IF cr ." dma-alloc called: " .s cr THEN
   fff + fff not and                  \ Align size to next 4k boundary
   alloc-mem
   \ alloc-mem always returns aligned memory - double check just to be sure
   dup fff and IF
      ." Warning: dma-alloc got unaligned memory!" cr
   THEN
;

: dma-free ( virt size -- )
   dma-debug? IF cr ." dma-free called: " .s cr THEN
   fff + fff not and                  \ Align size to next 4k boundary
   free-mem
;


\ We assume that firmware never maps more than the whole dma-window-size
\ so we cheat by calculating the remainder of addr/windowsize instead
\ of taking care to maintain a list of assigned device addresses
: dma-virt2dev  ( virt -- devaddr )
   dma-window-size mod dma-window-base +
;

: dma-map-in  ( virt size cachable? -- devaddr )
   dma-debug? IF cr ." dma-map-in called: " .s cr THEN
   drop                               ( virt size )
   bounds dup >r                      ( v+s virt  R: virt )
   swap fff + fff not and             \ Align end to next 4k boundary
   swap fff not and                   ( v+s' virt'  R: virt )
   ?DO
      \ ." mapping " i . cr
      dma-window-liobn                \ liobn
      i dma-virt2dev                  \ ioba
      i 3 OR                          \ Make a read- & writeable TCE
      ( liobn ioba tce  R: virt )
      hv-put-tce ABORT" H_PUT_TCE failed"
   1000 +LOOP
   r> dma-virt2dev
;

: dma-map-out  ( virt devaddr size -- )
   dma-debug? IF cr ." dma-map-out called: " .s cr THEN
   nip                                ( virt size )
   bounds                             ( v+s virt )
   swap fff + fff not and             \ Align end to next 4k boundary
   swap fff not and                   ( v+s' virt' )
   ?DO
      \ ." unmapping " i . cr
      dma-window-liobn                \ liobn
      i dma-virt2dev                  \ ioba
      i                               \ Lowest bits not set => invalid TCE
      ( liobn ioba tce )
      hv-put-tce ABORT" H_PUT_TCE failed"
   1000 +LOOP
;

: dma-sync  ( virt devaddr size -- )
   dma-debug? IF cr ." dma-sync called: " .s cr THEN
   \ TODO: Call flush-cache or sync here?
   3drop
;
