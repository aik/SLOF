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


\ CPC925 DART.

new-device   s" /dart" full-name

s" dart" 2dup device-name device-type
s" u3-dart" compatible

0 encode-int  f8033000 encode-int+
0 encode-int+     7000 encode-int+ s" reg" property

: open  true ;
: close ;

finish-device
