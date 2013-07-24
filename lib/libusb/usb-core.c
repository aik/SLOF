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

#ifndef DEBUG
#define validate_hcd_ops(dev) (dev && dev->hcidev && dev->hcidev->ops)
#else
int validate_hcd_ops(struct usb_dev *dev)
{
	int ret = true;

	if (!dev) {
		printf("dev is NULL\n");
		ret = false;
	} else if (!dev->hcidev) {
		printf("hcidev is NULL\n");
		ret = false;
	} else if (!dev->hcidev->ops)  {
		printf("ops is NULL\n");
		ret = false;
	}
	return ret;
}
#endif

struct usb_pipe *usb_get_pipe(struct usb_dev *dev, struct usb_ep_descr *ep,
			char *buf, size_t len)
{
	if (validate_hcd_ops(dev) && dev->hcidev->ops->get_pipe)
		return dev->hcidev->ops->get_pipe(dev, ep, buf, len);
	else
		return NULL;
}

void usb_put_pipe(struct usb_pipe *pipe)
{
	struct usb_dev *dev = NULL;
	if (pipe && pipe->dev) {
		dev = pipe->dev;
		if (validate_hcd_ops(dev) && dev->hcidev->ops->put_pipe)
			dev->hcidev->ops->put_pipe(pipe);
	}
}

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
