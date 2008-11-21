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

s" scsi" device-name


\ Standard Open Firmware method

: open true  ;


\ Standard Open Firmware method

: close ;


\ Temporary pointer to SCSI command area

0 VALUE command


\ Temporary pointer to SCSI response Buffer

0 VALUE response


\ Builds SCSI READ command in the buffer
\ This method will take starting Address as an input

: build-read ( address lba #blocks -- )
   2 pick to command
   command 0c erase
   dup 7fff < IF
      \ Use READ (10) command - understood by all devices
      28 command c!     ( address lba #blocks )
      command 7 + w!    ( address lba ) \ Set transfer length
      command 2 + l!    ( address )     \ Set logical block address
   ELSE
      \ Use READ (12) command - needed for large #blocks
      A8 command c!     ( address lba #blocks )
      command 2 + l!    ( address lba ) \ Set transfer length
      command 6 + l!    ( address )     \ Set logical block address
  THEN
  drop
;


\ Builds SCSI MODE-SENSE command in the Buffer
\ This method will take the starting address as an input

: build-mode-sense ( address alloc-len page-code page-control -- )
   3 pick to command            ( address alloc-len page-code page-control )
   command 0c erase             ( address alloc-len page-code page-control )
   6 lshift or command 2 + c!   ( address alloc-len )
   swap 7 + w!                  \ Configure allocation length
;


\ Builds READ CAPACITY command in the buffer

: build-read-capacity  ( address -- )
   TO command
   command 0c erase     \ Clear buffer
   25 command c!        \ set Opcode
;


\ Builds SCSI TEST-UNIT-READY command in the Buffer
\ This method will take the starting address as an input

: build-test-unit-ready ( address -- )  TO command command 0c erase  ;


\ Builds SCSI INQUIRY command in the Buffer
\ This method will take the starting address as an input

: build-inquiry ( address alloc-len -- )
   swap TO command      ( alloc-len )
   command 0c erase     ( alloc-len )
   command 4 + c!       \ Set allocation length
   12 command c!        \ set Opcode
;


\ Analyse response of build-inquiry command

: return-inquiry ( address -- verson peripheral-type )
   TO response
   response 3 + c@ 4 rshift      ( version# ) \ SCSI version num
   response c@                   ( version#  peripheral-device-type )
;


\ Builds SCSI REQUEST-SENSE command in the Buffer
\ This method will take the starting address as an input

: build-request-sense ( address alloc-len -- )
   swap TO command    ( alloc-len )
   command 0c erase   ( alloc-len )
   03 command c!      ( alloc-len)
   command 4 + c!     \ Configure the allocation length
;


\ Analyse reply of REQUEST-SENSE command in the Buffer
\ This method will take Starting address as an input

: return-request-sense ( address -- false|ascq asc sense-key true )
   TO response
   response c@ 71
   = response c@ 70 = or IF ( TRUE | FALSE )
      response 0D + c@      ( ASCQ ) \  additional sense code qualifier
      response 0c + c@      ( ASCQ ASC) \ additional sense code
      response 2 + c@       ( ASCQ ASC sense-key ) \ sense key error descriptor
      TRUE                  ( ASCQ ASC sense-key TRUE )
   ELSE
      FALSE                 ( FALSE )
   THEN
;


\ Builds SCSI SEEK command in the Buffer
\ This method will take the starting address as an input

: build-seek ( address lba -- )
   swap TO command      ( lba )
   command 0c erase     ( lba )
   2b command c!        ( lba )  \ Configure the Opcode
   command 2 + l!       \ Configure the logical block address
;


\ Builds SCSI LOAD command in the Buffer
\ This method will take the starting address as an input

\ : build-load ( address -- )
\    TO command
\    command 0c erase
\    1b command c!     \ Cofigure opcode
\    03 command 4 + c! \ configure load bit and start bit
\ ;


\ Builds SCSI UNLOAD command in the Buffer
\ This method will take the starting address as an input

\ : build-unload ( address -- )
\    to command
\    command 0c erase
\    1b command c!             \ Configure Opcode
\    02 command 4 + c!         \ Configure unload bit and start bit
\ ;


\ Builds SCSI START command in the Buffer
\ This method will take the starting address as an input

: build-start ( address -- )
   TO command
   command 0c erase
   1b command c!            \ Configure Opcode
   01 command 4 + c!
;


\ Builds SCSI STOP command in the Buffer
\ This method will take the starting address as an input

: build-stop ( address -- )
   TO command
   command 0c erase
   1b command c!           \ Configure Opcode
;

