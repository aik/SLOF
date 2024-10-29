\ *****************************************************************************
\ * Copyright (c) 2015-2020 IBM Corporation
\ * All rights reserved.
\ * This program and the accompanying materials
\ * are made available under the terms of the BSD License
\ * which accompanies this distribution, and is available at
\ * http://www.opensource.org/licenses/bsd-license.php
\ *
\ * Contributors:
\ *     IBM Corporation - initial implementation
\ ****************************************************************************/

." Populating " pwd cr

false VALUE vtpm-debug?
0     VALUE vtpm-unit

0     VALUE    log-base
40000 CONSTANT LOG-SIZE   \ 256k per VTPM FW spec.

e     CONSTANT VTPM_DRV_ERROR_SML_HANDED_OVER

LOG-SIZE BUFFER: log-base

\ firmware API call
: sml-get-allocated-size ( -- buffer-size)
   LOG-SIZE
;

\ firmware API call
: sml-get-handover-size ( -- size)
   tpm-get-logsize
;

\ firmware API call
: sml-handover ( dest size -- )
   log-base    ( dest size src )
   -rot        ( src dest size )
   move

   VTPM_DRV_ERROR_SML_HANDED_OVER tpm-driver-set-failure-reason
;

\ firmware API call
: get-failure-reason ( -- reason )
   tpm-driver-get-failure-reason           ( reason )
;

\ firmware API call
: 2hash-ext-log ( pcr eventtype info info-len data data-len -- success?)
    vtpm-debug? IF
        ." Call to 2hash-ext-log" cr
    THEN
    tpm-2hash-ext-log                      ( success? )
    dup 0= IF
        ." VTPM: tpm-2hash-ext-log failed: " dup . cr
    THEN
;

\ firmware API call
: pass-through-to-tpm ( buf-addr cmd-size -- rsp-size )
    vtpm-debug? IF
        ." Call to pass-through-to-tpm" cr
    THEN
    tpm-pass-through-to-tpm                ( rsp-size )
    vtpm-debug? IF
        ." VTPM: tpm-pass-through-to-tpm returned size: " dup . cr
    THEN
;

\ firmware API call
: get-maximum-cmd-size ( -- max-size )
    vtpm-debug? IF
        ." Call to get-maximum-cmd-size" cr
    THEN
    tpm-get-maximum-cmd-size               ( max-size )
;

0 0 s" ibm,sml-efi-reformat-supported" property

\ firmware API call
: reformat-sml-to-efi-alignment ( -- success )
   true
;

: open true ;
: close ;

: vtpm-cleanup ( -- )
   vtpm-debug? IF ." VTPM: Disabling RTAS bypass" cr THEN
   tpm-finalize
   \ Disable TCE bypass
   vtpm-unit 0 rtas-set-tce-bypass
;

: vtpm-init ( -- success )
   0 0 get-node open-node ?dup 0= IF false EXIT THEN
   my-self >r
   dup to my-self

   vtpm-debug? IF ." VTPM: Initializing for c-driver" cr THEN

   my-unit to vtpm-unit

   \ Enable TCE bypass special qemu feature
   vtpm-unit 1 rtas-set-tce-bypass

   \ Have TCE bypass cleaned up
   ['] vtpm-cleanup add-quiesce-xt

   \ close temporary node
   close-node
   r> to my-self

   tpm-start ?dup 0= IF
      vtpm-debug? IF ." VTPM: Success from tpm-start" cr THEN
      true
   ELSE
      ." VTPM: Error code from tpm-start: " . cr
      false
   THEN
;

\ inititialize unit and set RTAS bypass
vtpm-init IF
   \ pass logbase and size to the C driver; we may only do this after
   \ init of the lower levels since this calls needs to know the PCR banks
   \ when setting up the log
   log-base LOG-SIZE tpm-set-log-parameters
   s" vtpm-sml.fs" included
ELSE
   ." VTPM: vtpm-init failed" cr
THEN
