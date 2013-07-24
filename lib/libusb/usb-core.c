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
#include "usb-core.h"

#undef DEBUG
//#define DEBUG
#ifdef DEBUG
#define dprintf(_x ...) printf(_x)
#else
#define dprintf(_x ...)
#endif

struct usb_hcd_ops *head;
struct usb_dev *devpool;
#define USB_DEVPOOL_SIZE 4096

static struct usb_dev *usb_alloc_devpool(void)
{
	struct usb_dev *head, *curr, *prev;
	unsigned int dev_count = 0, i;

	head = SLOF_dma_alloc(USB_DEVPOOL_SIZE);
	if (!head)
		return NULL;

	dev_count = USB_DEVPOOL_SIZE/sizeof(struct usb_dev);
	dprintf("%s: %d number of devices\n", __func__, dev_count);
	/* Although an array, link them*/
	for (i = 0, curr = head, prev = NULL; i < dev_count; i++, curr++) {
		if (prev)
			prev->next = curr;
		curr->next = NULL;
		prev = curr;
	}

#ifdef DEBUG
	for (i = 0, curr = head; curr; curr = curr->next)
		printf("%s: %d dev %p\n", __func__, i++, curr);
#endif

	return head;
}

struct usb_dev *usb_devpool_get(void)
{
	struct usb_dev *new;

	if (!devpool) {
		devpool = usb_alloc_devpool();
		if (!devpool)
			return NULL;
	}

	new = devpool;
	devpool = devpool->next;
	memset(new, 0, sizeof(*new));
	new->next = NULL;
	return new;
}

void usb_devpool_put(struct usb_dev *dev)
{
	struct usb_dev *curr;
	if (!dev && !devpool)
		return;

	curr = devpool;
	while (curr->next)
		curr = curr->next;
	curr->next = dev;
	dev->next = NULL;
}

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

int usb_send_ctrl(struct usb_pipe *pipe, struct usb_dev_req *req, void *data)
{
	struct usb_dev *dev = NULL;
	if (!pipe)
		return false;
	dev = pipe->dev;
	if (validate_hcd_ops(dev) && dev->hcidev->ops->send_ctrl)
		return dev->hcidev->ops->send_ctrl(pipe, req, data);
	else {
		printf("%s: Failed\n", __func__);
		return false;
	}
}
