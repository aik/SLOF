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

#include <string.h>
#include "usb.h"
#include "usb-core.h"

#undef DEBUG
//#define DEBUG
#ifdef DEBUG
#define dprintf(_x ...) printf(_x)
#else
#define dprintf(_x ...)
#endif

static void ohci_init(struct usb_hcd_dev *hcidev)
{
	dprintf("In ohci_init: start resetting and configuring the device %p\n", hcidev->base);
}

static void ohci_detect(void)
{

}

static void ohci_disconnect(void)
{

}

struct usb_hcd_ops ohci_ops = {
	.name        = "ohci-hcd",
	.init        = ohci_init,
	.detect      = ohci_detect,
	.disconnect  = ohci_disconnect,
	.usb_type    = USB_OHCI,
	.next        = NULL,
};

void usb_ohci_register(void)
{
	usb_hcd_register(&ohci_ops);
}
