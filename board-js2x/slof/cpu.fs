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


\ CPU node.  Pretty minimal...

: create-cpu-node ( cpu# -- )
  new-device
  dup s" /cpus/cpu@" rot (u.) $cat full-name

  s" cpu" 2dup device-name device-type
  encode-int s" reg" property

  tb-frequency  encode-int s" timebase-frequency" property
  cpu-frequency encode-int s" clock-frequency" property

   8000 encode-int s" d-cache-size"      property
     80 encode-int s" d-cache-line-size" property
  10000 encode-int s" i-cache-size"      property
     80 encode-int s" i-cache-line-size" property

  finish-device ;
