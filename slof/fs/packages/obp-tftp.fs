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

s" obp-tftp" device-name

INSTANCE VARIABLE ciregs-buffer

: open ( -- okay? ) 
    ciregs-size alloc-mem ciregs-buffer ! 
    true
;

: load ( addr -- size )

    \ Save old client interface register 
    ciregs ciregs-buffer @ ciregs-size move

    s" bootargs" get-chosen 0= IF 0 0 THEN >r >r
    s" bootpath" get-chosen 0= IF 0 0 THEN >r >r

    \ Set bootpath to current device
    my-parent ihandle>phandle node>path encode-string
    s" bootpath" set-chosen

    \ Generate arg string for snk like
    \ "netboot load-addr length filename"
    (u.) s" netboot " 2swap $cat s"  60000000 " $cat
    my-args $cat

    \ Call SNK netboot loadr
    (client-exec) dup 0< IF drop 0 THEN

    \ Restore to old client interface register 
    ciregs-buffer @ ciregs ciregs-size move

    r> r> over IF s" bootpath" set-chosen ELSE 2drop THEN
    r> r> over IF s" bootargs" set-chosen ELSE 2drop THEN
;

: close ( -- )
   ciregs-buffer @ ciregs-size free-mem 
;

: ping  ( -- )
   s" ping " my-args $cat (client-exec)
;
