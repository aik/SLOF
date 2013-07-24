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

#include "usb.h"
#include "usb-core.h"
#include "usb-ehci.h"
#include "tools.h"

struct ehci_controller ehci_cntl;
#undef EHCI_DEBUG
//#define EHCI_DEBUG
#ifdef EHCI_DEBUG
#define dprintf(_x ...) printf(_x)
#else
#define dprintf(_x ...)
#endif

static void dump_ehci_regs(void)
{
	struct ehci_cap_regs *cap_regs;
	struct ehci_op_regs *op_regs;

	cap_regs = ehci_cntl.cap_regs;
	op_regs = ehci_cntl.op_regs;

	dprintf("\n - CAPLENGTH           %02X", read_reg8(&cap_regs->caplength));
	dprintf("\n - HCIVERSION          %04X", read_reg16(&cap_regs->hciversion));
	dprintf("\n - HCSPARAMS           %08X", read_reg32(&cap_regs->hcsparams));
	dprintf("\n - HCCPARAMS           %08X", read_reg32(&cap_regs->hccparams));
	dprintf("\n - HCSP_PORTROUTE      %016llX", read_reg64(&cap_regs->portroute));
	dprintf("\n");

	dprintf("\n - USBCMD              %08X", read_reg32(&op_regs->usbcmd));
	dprintf("\n - USBSTS              %08X", read_reg32(&op_regs->usbsts));
	dprintf("\n - USBINTR             %08X", read_reg32(&op_regs->usbintr));
	dprintf("\n - FRINDEX             %08X", read_reg32(&op_regs->frindex));
	dprintf("\n - CTRLDSSEGMENT       %08X", read_reg32(&op_regs->ctrldssegment));
	dprintf("\n - PERIODICLISTBASE    %08X", read_reg32(&op_regs->periodiclistbase));
	dprintf("\n - ASYNCLISTADDR       %08X", read_reg32(&op_regs->asynclistaddr));
	dprintf("\n - CONFIGFLAG          %08X", read_reg32(&op_regs->configflag));
	dprintf("\n - PORTSC              %08X", read_reg32(&op_regs->portsc[0]));
	dprintf("\n");
}

static void ehci_init(struct usb_hcd_dev *hcidev)
{
	printf("  EHCI: Initializing\n");
	dprintf("%s: device base address %p\n", __func__, hcidev->base);
	ehci_cntl.cap_regs = (struct ehci_cap_regs *)(hcidev->base);
	ehci_cntl.op_regs = (struct ehci_op_regs *)(hcidev->base +
						    read_reg8(&ehci_cntl.cap_regs->caplength));
	dump_ehci_regs();
}

static void ehci_detect(void)
{

}

static void ehci_disconnect(void)
{

}

static int ehci_send_ctrl(struct usb_pipe *pipe, struct usb_dev_req *req, void *data)
{
	return false;
}

static struct usb_pipe *ehci_get_pipe(struct usb_dev *dev, struct usb_ep_descr *ep,
				char *buf, size_t len)
{
	return NULL;
}

static void ehci_put_pipe(struct usb_pipe *pipe)
{

}

struct usb_hcd_ops ehci_ops = {
	.name        = "ehci-hcd",
	.init        = ehci_init,
	.detect      = ehci_detect,
	.disconnect  = ehci_disconnect,
	.get_pipe    = ehci_get_pipe,
	.put_pipe    = ehci_put_pipe,
	.send_ctrl   = ehci_send_ctrl,
	.usb_type    = USB_EHCI,
	.next        = NULL,
};

void usb_ehci_register(void)
{
	usb_hcd_register(&ehci_ops);
}
