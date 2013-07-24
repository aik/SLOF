/*****************************************************************************
 * Copyright (c) 2013 IBM Corporation
 * All rights reserved.
 * This program and the accompanying materials
 * are made available under the terms of the BSD License
 * which accompanies this distribution, and is available at
 * http://www.opensource.org/licenses/bsd-license.php
 *
 * Contributors:
 *     IBM Corporation - initial implementation
 *****************************************************************************/

#include "usb-core.h"

void usb_hcd_register(struct usb_hcd_ops *ops)
{
	if (!ops)
		printf("Error");
	printf("Registering %s\n", ops->name);
}
