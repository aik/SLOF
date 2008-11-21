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


\ Memory "heap" (de-)allocation.

\ For now, just allocate from the data space, and never take space back.

: alloc-mem ( len -- a-addr ) align here swap allot ;
: free-mem  ( a-addr len -- ) 2drop ;
