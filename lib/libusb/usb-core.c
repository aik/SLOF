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

#undef DEBUG
//#define DEBUG
#ifdef DEBUG
#define dprintf(_x ...) printf(_x)
#else
#define dprintf(_x ...)
#endif

struct usb_hcd_ops *head;

void usb_hcd_register(struct usb_hcd_ops *ops)
{
	struct usb_hcd_ops *list;

	if (!ops)
		printf("Error");
	dprintf("Registering %s %d\n", ops->name, ops->usb_type);

	if (head) {
		list = head;
		while (list->next)
			list = list->next;
		list->next = ops;
	} else
		head = ops;
}

void usb_hcd_init(void *hcidev)
{
	struct usb_hcd_dev *dev = hcidev;
	struct usb_hcd_ops *list = head;

	if (!dev) {
		printf("Device Error");
		return;
	}

	while (list) {
		if (list->usb_type == dev->type) {
			dprintf("usb_ops(%p) for the controller found\n", list);
			dev->ops = list;
			dev->ops->init(dev);
			return;
		}
		list = list->next;
	}

	dprintf("usb_ops for the controller not found\n");
}
