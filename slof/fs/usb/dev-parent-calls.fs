\ ****************************************************************************/
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

get-node CONSTANT my-phandle
: dma-alloc   s" dma-alloc"     my-phandle parent $call-static ;
: dma-map-in  s" dma-map-in"    my-phandle parent $call-static ;
: dma-map-out s" dma-map-out"   my-phandle parent $call-static ;
: dma-free    s" dma-free"      my-phandle parent $call-static ;
