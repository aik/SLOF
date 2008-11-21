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


\ The /memory node.

\ See 3.7.6.
new-device   s" /memory" full-name

s" memory" 2dup device-name device-type

: mem-size  0 4 oco ;

: encode-our-reg
  0 encode-int 0 encode-int+
  mem-size dup >r 80000000 > IF
  0 encode-int+ 80000000 encode-int+
  1 encode-int+ 0 encode-int+ r> 80000000 - >r THEN
  r@ 20 rshift encode-int+ r> ffffffff and encode-int+ ;
encode-our-reg s" reg" property

: mem-report
  base @ decimal cr mem-size 1e rshift 0 .r
  mem-size 3fffffff and IF ." .5" THEN ."  GB of RAM detected." base ! ;
mem-report

finish-device
