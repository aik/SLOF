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

#ifndef __USB_CORE_H
#define __USB_CORE_H

#include <stdio.h>
#include <stdbool.h>
#include "helpers.h"
#include "usb.h"
#include "tools.h"

enum usb_hcd_type {
	USB_OHCI = 1,
	USB_EHCI = 2,
	USB_XHCI = 3,
};

struct usb_hcd_dev;

struct usb_hcd_dev {
	void *base;
	long type;
	long num;
	struct usb_hcd_ops *ops;
	void *priv; /* hcd owned structure */
	long nextaddr; /* address for devices */
};

struct usb_pipe;

/*******************************************/
/* Standard Endpoint Descriptor            */
/*******************************************/
/* bmAttributes */
#define USB_EP_TYPE_MASK          0x03
#define USB_EP_TYPE_CONTROL       0
#define USB_EP_TYPE_ISOC          1
#define USB_EP_TYPE_BULK          2
#define USB_EP_TYPE_INTR          3

struct usb_ep_descr {
	uint8_t		bLength;		/* size of descriptor */
	uint8_t		bDescriptorType;	/* Type = 5 */
	uint8_t		bEndpointAddress;
	uint8_t		bmAttributes;
	uint16_t	wMaxPacketSize;
	uint8_t		bInterval;
} __attribute__((packed));

/* Max number of endpoints supported in a device */
#define USB_DEV_EP_MAX 4

struct usb_dev {
	struct usb_dev     *next;
	struct usb_hcd_dev *hcidev;
	struct usb_pipe    *intr;
	struct usb_pipe    *control;
	struct usb_pipe    *bulk_in;
	struct usb_pipe    *bulk_out;
	struct usb_ep_descr ep[USB_DEV_EP_MAX];
	uint32_t ep_cnt;
	uint32_t type;
	uint32_t speed;
	uint32_t addr;
};

struct usb_pipe {
	struct usb_dev *dev;
	struct usb_pipe *next;
	uint32_t type;
	uint32_t speed;
	uint32_t dir;
	uint16_t epno;
	uint16_t mps;
} __attribute__((packed));

#define	REQ_GET_STATUS		     0	/* see Table 9-4 */
#define	REQ_CLEAR_FEATURE	     1
#define	REQ_GET_STATE		     2	/* HUB specific */
#define	REQ_SET_FEATURE		     3
#define	REQ_SET_ADDRESS		     5
#define	REQ_GET_DESCRIPTOR	     6
#define	REQ_SET_DESCRIPTOR	     7
#define	REQ_GET_CONFIGURATION	     8
#define	REQ_SET_CONFIGURATION	     9
#define	REQ_GET_INTERFACE	     10
#define	REQ_SET_INTERFACE	     11
#define	REQ_SYNCH_FRAME              12

#define REQT_REC_DEVICE              0
#define REQT_REC_INTERFACE           1
#define REQT_REC_EP                  2
#define REQT_REC_OTHER               3
#define REQT_TYPE_STANDARD           (0 << 5)
#define REQT_TYPE_CLASS              (1 << 5)
#define REQT_TYPE_VENDOR             (2 << 5)
#define REQT_TYPE_RSRVD              (3 << 5)
#define REQT_DIR_OUT                 (0 << 7) /* host -> device */
#define REQT_DIR_IN                  (1 << 7) /* device -> host */

struct usb_dev_req {
	uint8_t		bmRequestType;		/* direction, recipient */
	uint8_t		bRequest;		/* see spec: Table 9-3 */
	uint16_t	wValue;
	uint16_t	wIndex;
	uint16_t	wLength;		/* number of bytes to transfer */
} __attribute__((packed));

struct usb_hcd_ops {
	const char *name;
	void (*init)(struct usb_hcd_dev *);
	void (*detect)(void);
	void (*disconnect)(void);
	int  (*send_ctrl)(struct usb_pipe *pipe, struct usb_dev_req *req, void *data);
	struct usb_pipe* (*get_pipe)(struct usb_dev *dev, struct usb_ep_descr *ep,
				char *buf, size_t len);
	void (*put_pipe)(struct usb_pipe *);
	struct usb_hcd_ops *next;
	unsigned int usb_type;
};

extern void usb_hcd_register(struct usb_hcd_ops *ops);
extern struct usb_pipe *usb_get_pipe(struct usb_dev *dev, struct usb_ep_descr *ep,
				char *buf, size_t len);
extern void usb_put_pipe(struct usb_pipe *pipe);
extern int usb_send_ctrl(struct usb_pipe *pipe, struct usb_dev_req *req, void *data);
extern struct usb_dev *usb_devpool_get(void);
extern void usb_devpool_put(struct usb_dev *);

#endif
