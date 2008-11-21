\ =============================================================================
\  * Copyright (c) 2004, 2005 IBM Corporation
\  * All rights reserved. 
\  * This program and the accompanying materials 
\  * are made available under the terms of the BSD License 
\  * which accompanies this distribution, and is available at
\  * http://www.opensource.org/licenses/bsd-license.php
\  * 
\  * Contributors:
\  *     IBM Corporation - initial implementation
\ =============================================================================


\ Dual Gb Ethernet controller.

\ Function 0.

new-device   s" /ht/pci-x@2/enet@1" full-name

s" enet" 2dup device-name device-type

07 200804 config-b! \ Enable master, memory, I/O.
a0000000 200810 config-l!  0 200814 config-l!
a0010000 200818 config-l!  0 20081c config-l! \ Set memory ranges: twice 64kB
                                              \ @ a0000000 and @ a0010000.
1c 20083c config-b! \ Set IRQ#.

: open  true ;
: close ;

finish-device


\ Function 1.

new-device   s" /ht/pci-x@2/enet@1,1" full-name

s" enet" 2dup device-name device-type

07 200904 config-b! \ Enable master, memory, I/O.
a0020000 200910 config-l!  0 200914 config-l!
a0030000 200918 config-l!  0 20091c config-l! \ Set memory ranges: twice 64kB
                                              \ @ a0020000 and @ a0030000.
1d 20093c config-b! \ Set IRQ#.

finish-device
