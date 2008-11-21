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


\ -----------------------------------------------------------
\ OF properties
\ -----------------------------------------------------------

s" storage" device-name
s" block" device-type

2 encode-int s" #address-cells" property
0 encode-int s" #size-cells" property


\ -----------------------------------------------------------
\ Specific properties
\ -----------------------------------------------------------

8 VALUE mps-bulk-out
8 VALUE mps-bulk-in
8 VALUE mps-dcp
0 VALUE bulk-in-ep
0 VALUE bulk-out-ep
0 VALUE bulk-in-toggle
0 VALUE bulk-out-toggle
0 VALUE lun
0 VALUE my-usb-address


\ ----------------------------------------------------------
\ Instance specific values
\ ----------------------------------------------------------

0  VALUE csw-buffer
0e VALUE cfg-buffer
0  VALUE response-buffer
0  VALUE command-buffer
0  VALUE resp-size
0  VALUE resp-buffer
INSTANCE VARIABLE ihandle-bulk
INSTANCE VARIABLE ihandle-scsi
INSTANCE VARIABLE ihandle-deblocker
INSTANCE VARIABLE flag
INSTANCE VARIABLE count
0  VALUE max-transfer
0  VALUE block-size


\ -------------------------------------------------------
\ General Constants
\ -------------------------------------------------------

0f CONSTANT SCSI-COMMAND-OFFSET


\ -------------------------------------------------------
\ All support methods inherited from parent or imported
\ from support packages are included here. Also included
\ are the internal methods
\ -------------------------------------------------------

s" usb-storage-support.fs" INCLUDED

\ ---------------------------------------------------------------
\ COLON Definitions: Implementation of Standard SCSI commands
\ over USB OHCI
\ ---------------------------------------------------------------


\ to use the general bulk command a lot of global variables
\ must be set. See for example the inquiry command.
0 VALUE bulk-cnt
: do-bulk-command ( resp-buffer resp-size -- TRUE | FALSE )
   TO resp-size
   TO resp-buffer
   2 TO bulk-cnt
   FALSE dup
   BEGIN 0= WHILE
   drop
   \ prepare and send bulk CBW
   1 1 bulk-out-toggle command-buffer 1f mps-bulk-out
   ( pt ed-type toggle buffer length mps-bulk-out )
   my-usb-address bulk-out-ep 7 lshift or
   ( pt ed-type toggle buffer length mps address )
   rw-endpoint swap		             ( TRUE toggle | FALSE toggle )
   to bulk-out-toggle                        ( TRUE | FALSE )
   IF
      resp-size 0<> IF  \ do we need a response ?!
        \ read the bulk response
        0 1 bulk-in-toggle resp-buffer resp-size mps-bulk-in
        ( pt ed-type toggle buffer length mps-bulk-in )
        my-usb-address bulk-in-ep 7 lshift or
        ( pt ed-type toggle buffer length mps address )
        rw-endpoint swap                       ( TRUE toggle | FALSE toggle )
        to bulk-in-toggle                      ( TRUE | FALSE )
      ELSE
        TRUE
      THEN
      IF
         \ read the bulk CSW
         0 1 bulk-in-toggle csw-buffer D mps-bulk-in
         ( pt ed-type toggle buffer length mps-bulk-in )
         my-usb-address bulk-in-ep 7 lshift or
         ( pt ed-type toggle buffer length mps address )
         rw-endpoint swap                    ( TRUE toggle | FALSE toggle )
         to bulk-in-toggle                   ( TRUE | FALSE )
         IF
	    s" Command successful." usb-debug-print
	    TRUE dup
         ELSE
            s" Command failed in CSW stage" usb-debug-print
	    FALSE dup
         THEN
      ELSE
         s" Command failed while receiving DATA... read CSW..." usb-debug-print
         \ STALLED: Get CSW to send the CBW again
         0 1 bulk-in-toggle csw-buffer D mps-bulk-in
         ( pt ed-type toggle buffer length mps-bulk-in )
         my-usb-address bulk-in-ep 7 lshift or
         ( pt ed-type toggle buffer length mps address )
         rw-endpoint swap                    ( TRUE toggle | FALSE toggle )
         to bulk-in-toggle                   ( TRUE | FALSE )
	 IF
	   s" OK evaluate the CSW ..." usb-debug-print
           csw-buffer c + l@-le
           2 = IF \ Phase Error
	     s" do a bulk reset-recovery ..." usb-debug-print
             bulk-out-ep bulk-in-ep my-usb-address
             bulk-reset-recovery-procedure
           THEN
	 \ ELSE 
	 \ don't abort if the read fails.
	 THEN
         FALSE dup
      THEN
   ELSE
       s" Command failed while Sending CBW ..." usb-debug-print
       FALSE dup
   THEN
   bulk-cnt 1 - TO bulk-cnt
   bulk-cnt 0= IF
      2drop FALSE dup
   THEN
   REPEAT
;

\ ---------------------------------------------------------------
\ Method to 1. Send the INQUIRY command 2. Recieve and analyse
\ (pending) INQUIRY data
\ ---------------------------------------------------------------

: inquiry ( -- )
   s" usb-storage: inquiry" usb-debug-print
   command-buffer 1 20 80 lun 0c
   ( address tag transfer-len direction lun command-len )
   build-cbw
   command-buffer SCSI-COMMAND-OFFSET + 20   ( address alloc-len )
   build-inquiry
   response-buffer 20 
   do-bulk-command
   IF
     s" Successfully read INQUIRY data" usb-debug-print
     s" Inquiry data for 0x20 bytes availabe in Response buffer"
     usb-debug-print
   ELSE
     \ TRUE ABORT" USB device transaction error. (inquiry)"
     5040 error" (USB) Device transaction error. (inquiry)"
     ABORT
   THEN
;

\ ---------------------------------------------------------------
\ Method to 1. Send the READ CAPACITY command
\           2. Recieve and analyse the response data
\ ---------------------------------------------------------------

: read-capacity ( -- )
   s" usb-storage: read-capacity" usb-debug-print
   command-buffer 1 8 80 lun 0c
   ( address tag transfer-len direction lun command-len )
   build-cbw
   command-buffer SCSI-COMMAND-OFFSET +      ( address )
   build-read-capacity
   response-buffer 8 do-bulk-command
   IF
     s" Successfully read READ CAPACITY data" usb-debug-print
   ELSE
     \ TRUE ABORT" USB device transaction error. (capacity)"
     5040 error" (USB) Device transaction error. (capacity)"
     ABORT
   THEN
;


\ --------------------------------------------------------------------
\ Method to 1. Send TEST UNIT READY command 2. Analyse the status
\ of the response
\ -------------------------------------------------------------------

: test-unit-ready ( -- TRUE | FALSE )
   command-buffer 1 0 80 lun 0c
   ( address tag transfer-len direction lun command-len )
   build-cbw
   command-buffer SCSI-COMMAND-OFFSET +      ( address )
   build-test-unit-ready
   response-buffer 0 do-bulk-command
   IF
     s" Successfully read test unit ready data" usb-debug-print
     s" Test Unit STATUS availabe in csw-buffer" usb-debug-print
     csw-buffer 0c + c@ 0=  IF
       s" Test Unit Command Successfully Executed" usb-debug-print
       TRUE                             ( TRUE )
     ELSE
       s" Test Unit Command Failed to execute" usb-debug-print
       FALSE                            ( FALSE )
     THEN
   ELSE
     \ TRUE ABORT" USB device transaction error. (test-unit-ready)"
      5040 error" (USB) Device transaction error. (test-unit-ready)"
      ABORT
   THEN
;

\ -------------------------------------------------
\ Method to 1. read sense data 2. analyse sesnse
\ data(pending)
\ ------------------------------------------------

: request-sense ( -- )
   s" request-sense: Command ready." usb-debug-print
   command-buffer 1 12 80 lun 0c
   ( address tag transfer-len direction lun command-len )
   build-cbw
   command-buffer SCSI-COMMAND-OFFSET + 12   ( address alloc-len )
   build-request-sense
   response-buffer 12 do-bulk-command
   IF
     s" Read Sense data successfully" usb-debug-print
   ELSE
     \ TRUE ABORT" USB device transaction error. (request-sense)"
     5040 error" (USB) Device transaction error. (request-sense)"
     ABORT
   THEN
;

: start ( -- )
   command-buffer 1 0 80 lun 0c
   ( address tag transfer-len direction lun command-len )
   build-cbw
   command-buffer SCSI-COMMAND-OFFSET +  ( address )
   build-start
   response-buffer 0 do-bulk-command
   IF
     s" Start successfully" usb-debug-print
   ELSE
     \ TRUE ABORT" USB device transaction error. (start)"
     5040 error" (USB) Device transaction error. (start)"
     ABORT
   THEN
;


\ To transmit SCSI Stop command

: stop ( -- )
   command-buffer 1 0 80 lun 0c
   ( address tag transfer-len direction lun command-len )
   build-cbw
   command-buffer SCSI-COMMAND-OFFSET +      ( address )
   build-stop
   response-buffer 0 do-bulk-command
   IF
     s" Stop successfully" usb-debug-print
   ELSE
     \ TRUE ABORT" USB device transaction error. (stop)"
     5040 error" (USB) Device transaction error. (stop)"
     ABORT
   THEN
;


0 VALUE temp1
0 VALUE temp2
0 VALUE temp3


\ -------------------------------------------------------------
\          block device's seek
\ -------------------------------------------------------------

: seek ( pos-hi pos-lo -- status )
   s" seek" ihandle-deblocker @ $call-method
;


\ -------------------------------------------------------------
\          block device's read
\ -------------------------------------------------------------

: read ( address length -- actual )
   s" read" ihandle-deblocker @  $call-method
;


\ -------------------------------------------------------------
\         read-blocks to be used by deblocker
\ -------------------------------------------------------------
: read-blocks ( address block# #blocks -- #read-blocks )
   block-size * command-buffer  ( address block# transfer-len command-buffer )
   1 2 pick 80 lun 0c build-cbw ( address block# transfer-len )
   dup to temp1                 ( address block# transfer-len)
   block-size /                 ( address block# #blocks )
   command-buffer               ( address block# #block command-addr )
   SCSI-COMMAND-OFFSET + -rot   ( address command-addr block# #blocks )
   build-read                   ( address )
   temp1 do-bulk-command
   IF
     s" Read Sense data successfully" usb-debug-print
   ELSE
     \ TRUE ABORT" USB device transaction error. (read-blocks)"
     5040 error" (USB) Device transaction error. (read-blocks)"
     ABORT
   THEN
   temp1 block-size /  ( #read-blocks )
;

\ ------------------------------------------------
\ To bring the the media to seekable and readable
\ condition.
\ ------------------------------------------------

0 VALUE temp1
0 VALUE temp2
0 VALUE temp3
d# 800 CONSTANT media-ready-retry

: make-media-ready ( -- )
   s" usb-storage: make-media-ready" usb-debug-print
   0  flag !
   0  count !
   BEGIN
      flag @  0=
   WHILE
      test-unit-ready IF
         s" Media ready for access." usb-debug-print
         1  flag !
      ELSE
         count @  1 +  count !
         count @ media-ready-retry = IF
            1 flag !
            5000 error" (USB) Media or drive not ready for this blade."
            ABORT
         THEN
         request-sense
         response-buffer return-request-sense
         ( FALSE | ascq asc sense-key TRUE )
         IF
            to temp1                          ( ascq asc )
            to temp2                          ( ascq )
            to temp3
            temp1 2 = temp2 3a = and          ( TRUE | FALSE )
            IF
               5010 error" (USB) No Media found! Check for the drawer/inserted media."
               ABORT
            THEN
            temp1 2 = temp2 06 = and         ( TRUE | FALSE )
            IF
               5020 error" (USB) Unknown media format."
               ABORT
            THEN
            temp1 0<> temp2 4 = temp3 2 = and and ( TRUE | FALSE )
            IF
               start stop
            THEN
         THEN
      THEN
      d# 10 ms
   REPEAT
   usb-debug-flag IF
      ." make-media-ready finished after "
      count @ decimal . hex ." tries." cr
   THEN
;


\ Set up the block-size of the device, using the READ CAPACITY command.
\ Note: Media must be ready (=> make-media-ready) or READ CAPACITY
\ might fail!

: (init-block-size)
   read-capacity
   response-buffer 4 + 
   l@ to block-size
   s" usb-storage: block-size=" block-size usb-debug-print-val
;


\ Standard OF methods

: open ( -- TRUE )
   s" usb-storage: open" usb-debug-print
   ihandle-bulk s" bulk" (open-package)
   ihandle-scsi s" scsi" (open-package)

   make-media-ready
   (init-block-size)           \ Init block-size before opening the deblocker

   ihandle-deblocker s" deblocker" (open-package)

   s" disk-label" find-package IF  ( phandle )
      usb-debug-flag IF ." my-args for disk-label = " my-args swap . . cr THEN
      my-args rot interpose
   THEN
   TRUE                        ( TRUE )
;


: close  ( -- )
   ihandle-deblocker (close-package)
   ihandle-scsi (close-package)
   ihandle-bulk (close-package)
;


\ Set device name according to type

: (init-device-name)  ( -- )
   inquiry
   response-buffer c@
   CASE
      1 OF s" tape" device-name ENDOF
      5 OF s" cdrom" device-name ENDOF
      \ dup OF s" storage" device-name ENDOF
   ENDCASE
;


\ Initial device node setup

: (initial-setup)
   ihandle-bulk s" bulk" (open-package)
   ihandle-scsi s" scsi" (open-package)

   device-init
   (init-device-name)
   set-cdrom-alias
   200 to block-size       \ Default block-size, will be overwritten in "open"
   10000 to max-transfer

   ihandle-bulk (close-package)
   ihandle-scsi (close-package)
;

(initial-setup)
