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

: check-bootlist ( -- true | false )
  vpd-bootlist l@
  dup 0= IF 
    ( bootlist == 0 means that probably nothing from vpd has been received )
    s" Boot list could not be read from VPD" log-string cr
    s" Boot watchdog has been rearmed" log-string cr
    2 set-watchdog
  exit THEN
  FFFFFFFF = IF
    ( bootlist all FFs means that the vpd has no useful information )
    .banner
    -6b boot-exception-handler
    \ The next message is duplicate, but sent w. log-string
    s" Boot list successfully read from VPD but no useful information received" log-string cr
    s" Please specify the boot device in the management module" log-string cr
    s" Specified Boot Sequence not valid" mm-log-warning
  false ELSE true THEN ;

\ the following words are necessary for vpd-boot-import
defer set-boot-device
defer add-boot-device
defer bootdevice

\ Import boot device list from VPD
\ If none, keep the existing list in NVRAM
\ This word can be used to overwrite read-bootlist if wanted

: vpd-boot-import  ( -- )
   0 0 set-boot-device
   vpd-read-bootlist
   check-bootlist  IF
      4 0  DO  vpd-bootlist i + c@
         CASE
            6  OF  \ cr s" 2B Booting from Network" log-string cr
               s" net" furnish-boot-file $cat strdup add-boot-device  
	    ENDOF

            \ 7  OF  cr s" Booting from no device not supported" 2dup mm-log-warning log-string cr
            \ 7  OF  cr s" 2B Booting from NVRAM boot-device list: " boot-device $cat
            \ log-string cr
            \ boot-device add-boot-device  ENDOF

            8  OF  \ cr s" 2B Booting from disk0" log-string cr
               s" disk disk0" add-boot-device ENDOF

            9  OF  \ cr s" 2B Booting from disk1" log-string cr
               s" disk1" add-boot-device ENDOF

            A  OF  \ cr s" 2B Booting from disk2" log-string cr
               s" disk2" add-boot-device ENDOF

            B  OF  \ cr s" 2B Booting from disk3" log-string cr
               s" disk3" add-boot-device ENDOF

            C  OF  \ cr s" 2B Booting from CDROM" log-string cr
               s" cdrom" add-boot-device ENDOF

            E  OF  \ cr s" 2B Booting from disk4" log-string cr
               s" disk4" add-boot-device ENDOF

            F  OF  \ cr s" 2B Booting from SAS - w. Timeout" log-string cr
   		s" sas" add-boot-device ENDOF
           10  OF  \ cr s" 2B Booting from SAS - Continuous Retry" log-string cr
   		s" sas" add-boot-device ENDOF
         ENDCASE
      LOOP
      bootdevice 2@ dup >r s" boot-device" $setenv
      r> IF 0 ELSE -6b THEN
   ELSE -6a THEN
   boot-exception-handler
;
