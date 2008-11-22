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

\ #include "scsi-support.fs"

\ Set usb-debug flag to TRUE for debugging output:
0 VALUE usb-debug-flag
0 VALUE usb-test-flag

VARIABLE ihandle-bulk-tran
VARIABLE ihandle-scsi-tran

\ Print a debug message when usb-debug-flag is set
: usb-debug-print  ( str len -- )
   usb-debug-flag  IF type cr ELSE 2drop THEN
;

\ Print a debug message with corresponding value when usb-debug-flag is set
: usb-debug-print-val  ( str len val -- )
   usb-debug-flag  IF -ROT type . cr ELSE drop 2drop THEN
;


0 VALUE ohci-alias-num

\ create a new ohci device alias for the current node:
: set-ohci-alias  ( -- )
   ohci-alias-num dup 1+ TO ohci-alias-num    ( num )
   s" ohci" rot $cathex strdup   \ create alias name
   get-node node>path            \ get path string
   set-alias                     \ and set the alias
;

0 VALUE cdrom-alias-num

\ create a new ohci device alias for the current node:
: set-cdrom-alias  ( --  )
   cdrom-alias-num dup 1+ TO cdrom-alias-num    ( num )
   s" cdrom" rot $cathex strdup   \ create alias name
   get-node node>path            \ get path string
   set-alias                     \ and set the alias
;

: usb-create-alias-name ( num -- str len )
    >r s" ohciX" 2dup + 1-           ( str len last-char-ptr  R: num )
    r> [char] 0 + swap c!            ( str len  R: )
;
    
\ Scan all USB host controllers for attached devices:
: usb-scan
   \ Scan all OHCI chips:
   ." Scan USB... " cr 
   0 >r                             \ Counter for alias
   BEGIN
      r@ usb-create-alias-name
      find-alias ?dup               ( false | str len len  R: num )
   WHILE
      usb-debug-flag IF
         ." * Scanning hub " 2dup type ." ..." cr
      THEN
      open-dev ?dup IF              ( ihandle  R: num )
         dup to my-self
         dup ihandle>phandle dup set-node
          child ?dup IF
              delete-node s" Deleting node" usb-debug-print
          THEN
         >r s" enumerate" r@ $call-method   \ Scan host controller
         r> close-dev  0 set-node 0 to my-self
      THEN                          ( R: num )
      r> 1+ >r                      ( R: num+1 )
   REPEAT   r> drop
   0 TO ohci-alias-num
   0 TO cdrom-alias-num
   s" cdrom0" find-alias            ( false | dev-path len )
   dup IF
       s" cdrom" 2swap              ( alias-name len' dev-path len )
       set-alias                    ( -- )
       \ cdrom-alias-num 1 + TO cdrom-alias-num
   ELSE 
       drop                         ( -- )
   THEN
;

: usb-probe

  usb-scan

  cdrom-alias-num 0= IF
     ." Not found CDROM! " cr
  THEN
     ." CDROM found " cdrom-alias-num . cr 
;

 
: usb-dev-test ( -- TRUE )
   s" USB Device Test " usb-debug-print
   1 usb-create-alias-name
   find-alias ?dup IF
      ." * open " 2dup type . cr
   ELSE
      s" can't found alias " usb-debug-print
   THEN
   open-dev ?dup IF
      dup to my-self
      dup ihandle>phandle dup set-node
\     ihandle-bulk-tran s" bulk" open-package
\     ihandle-scsi-tran s" scsi" open-package
      s" bulk" $open-package ihandle-bulk-tran !
      s" scsi" $open-package ihandle-scsi-tran !

\      make-media-ready

      s" close all " usb-debug-print
      close-dev 0 set-node 0 to my-self

      ihandle-bulk-tran close-package
      ihandle-scsi-tran close-package
   ELSE
      s" can't open usb hub" usb-debug-print
   THEN

   TRUE
;

