\ *****************************************************************************
\ * Copyright (c) 2004, 2011 IBM Corporation
\ * All rights reserved.
\ * This program and the accompanying materials
\ * are made available under the terms of the BSD License
\ * which accompanies this distribution, and is available at
\ * http://www.opensource.org/licenses/bsd-license.php
\ *
\ * Contributors:
\ *     IBM Corporation - initial implementation
\ ****************************************************************************/
\ * PCI setup functions
\ *****************************************************************************

3a0 cp

#include "pci-helper.fs"

3b0 cp

\ provide the device-alias definition words
#include "pci-aliases.fs"

3c0 cp

#include "pci-class-code-names.fs"

3d0 cp

\ Provide a generic setup function ... note that we
\ do not assign BARs here, it's done by QEMU already.
: pci-device-generic-setup  ( config-addr -- )
   pci-class-name 2dup device-name device-type
;
