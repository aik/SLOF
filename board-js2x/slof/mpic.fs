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


new-device   s" /mpic" full-name

s" mpic" device-name  s" interrupt-controller" device-type
s" open-pic" compatible

0 encode-int  f8040000 encode-int+
0 encode-int+    30000 encode-int+ s" reg" property

: enable-mpic  0 6 oco drop ;
enable-mpic

: open  true ;
: close ;

finish-device
