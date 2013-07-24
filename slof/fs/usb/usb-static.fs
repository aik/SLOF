\ *****************************************************************************
\ * Copyright (c) 2004, 2011, 2013 IBM Corporation
\ * All rights reserved.
\ * This program and the accompanying materials
\ * are made available under the terms of the BSD License
\ * which accompanies this distribution, and is available at
\ * http://www.opensource.org/licenses/bsd-license.php
\ *
\ * Contributors:
\ *     IBM Corporation - initial implementation
\ ****************************************************************************/

\ register device alias
: do-alias-setting ( num name-str name-len )
   rot $cathex strdup            \ create alias name
   get-node node>path            \ get path string
   set-alias                     \ and set the alias
;

0 VALUE ohci-alias-num
0 VALUE ehci-alias-num
0 VALUE xhci-alias-num

\ create a new ohci device alias for the current node
: set-ohci-alias  (  -- )
    ohci-alias-num dup 1+ TO ohci-alias-num    ( num )
    s" ohci" do-alias-setting
;

\ create a new ehci device alias for the current node
: set-ehci-alias  (  -- )
    ehci-alias-num dup 1+ TO ehci-alias-num    ( num )
    s" ehci" do-alias-setting
;

\ create a new xhci device alias for the current node
: set-xhci-alias  (  -- )
    xhci-alias-num dup 1+ TO xhci-alias-num    ( num )
    s" xhci" do-alias-setting
;

: usb-scan ( -- )
    ." Scanning USB " cr
;
