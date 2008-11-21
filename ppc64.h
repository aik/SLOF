// ============================================================================
//  * Copyright (c) 2004, 2005 IBM Corporation
//  * All rights reserved. 
//  * This program and the accompanying materials 
//  * are made available under the terms of the BSD License 
//  * which accompanies this distribution, and is available at
//  * http://www.opensource.org/licenses/bsd-license.php
//  * 
//  * Contributors:
//  *     IBM Corporation - initial implementation
// ============================================================================


// The big Forth source file that contains everything but the core engine.
// We include it as a hunk of data into the C part of SLOF; at startup
// time, this will be EVALUATE'd.

extern char _binary_ppc64_fs_start[], _binary_ppc64_fs_end[];
