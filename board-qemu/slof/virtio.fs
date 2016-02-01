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

\ This struct must match "struct virtio_device" in virtio.h!
STRUCT
   /n FIELD vd>base
   /l FIELD vd>is-modern
   10 FIELD vd>legacy
   10 FIELD vd>common
   10 FIELD vd>notify
   10 FIELD vd>isr
   10 FIELD vd>device
   10 FIELD vd>pci
   /l FIELD vd>notify_off_mul
CONSTANT /vd-len


\ Initialize virtiodev structure for the current node
\ This routine gets called from pci device files
: virtio-setup-vd  ( vdstruct -- )
   >r
   \ Set up for PCI device interface
   s" 10 config-l@ translate-my-address 3 not AND" evaluate
   ( io-base ) r@ vd>base !
   r> drop
;
