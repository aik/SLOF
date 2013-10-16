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
#include "usb-xhci.h"
#include "tools.h"
#include "paflof.h"

#undef XHCI_DEBUG
//#define XHCI_DEBUG
#ifdef XHCI_DEBUG
#define dprintf(_x ...) do { printf("%s: ", __func__); printf(_x); } while (0)
#else
#define dprintf(_x ...) do { } while (0)
#endif

static void dump_xhci_regs(struct xhci_hcd *xhcd)
{
#ifdef XHCI_DEBUG
	struct xhci_cap_regs *cap;
	struct xhci_op_regs *op;
	struct xhci_run_regs *run;

	cap = xhcd->cap_regs;
	op = xhcd->op_regs;
	run = xhcd->run_regs;

	dprintf("\n");
	dprintf(" - CAPLENGTH           %02X\n", read_reg8 (&cap->caplength));
	dprintf(" - HCIVERSION          %04X\n", read_reg16(&cap->hciversion));
	dprintf(" - HCSPARAMS1          %08X\n", read_reg32(&cap->hcsparams1));
	dprintf(" - HCSPARAMS2          %08X\n", read_reg32(&cap->hcsparams2));
	dprintf(" - HCSPARAMS3          %08X\n", read_reg32(&cap->hcsparams3));
	dprintf(" - HCCPARAMS           %08X\n", read_reg32(&cap->hccparams));
	dprintf(" - DBOFF               %08X\n", read_reg32(&cap->dboff));
	dprintf(" - RTSOFF              %08X\n", read_reg32(&cap->rtsoff));
	dprintf("\n");

	dprintf(" - USBCMD              %08X\n", read_reg32(&op->usbcmd));
	dprintf(" - USBSTS              %08X\n", read_reg32(&op->usbsts));
	dprintf(" - PAGESIZE            %08X\n", read_reg32(&op->pagesize));
	dprintf(" - DNCTRL              %08X\n", read_reg32(&op->dnctrl));
	dprintf(" - CRCR              %016llX\n", read_reg64(&op->crcr));
	dprintf(" - DCBAAP            %016llX\n", read_reg64(&op->dcbaap));
	dprintf(" - CONFIG              %08X\n", read_reg32(&op->config));
	dprintf("\n");

	dprintf(" - MFINDEX             %08X\n", read_reg32(&run->mfindex));
	dprintf("\n");
#endif
}

static void xhci_init(struct usb_hcd_dev *hcidev)
{
	struct xhci_hcd *xhcd;

	printf("  XHCI: Initializing\n");
	dprintf("%s: device base address %p\n", __func__, hcidev->base);

	hcidev->base = (void *)((uint64_t)hcidev->base & ~7);
	xhcd = SLOF_alloc_mem(sizeof(*xhcd));
	if (!xhcd) {
		printf("usb-xhci: Unable to allocate memory\n");
		return;
	}
	memset(xhcd, 0, sizeof(*xhcd));

	hcidev->nextaddr = 1;
	hcidev->priv = xhcd;
	xhcd->hcidev = hcidev;
	xhcd->cap_regs = (struct xhci_cap_regs *)(hcidev->base);
	xhcd->op_regs = (struct xhci_op_regs *)(hcidev->base +
						read_reg8(&xhcd->cap_regs->caplength));
	xhcd->run_regs = (struct xhci_run_regs *)(hcidev->base +
						read_reg32(&xhcd->cap_regs->rtsoff));
	xhcd->db_regs = (struct xhci_db_regs *)(hcidev->base +
						read_reg32(&xhcd->cap_regs->dboff));

#ifdef XHCI_DEBUG
	dump_xhci_regs(xhcd);
#endif
}

struct usb_hcd_ops xhci_ops = {
	.name          = "xhci-hcd",
	.init          = xhci_init,
	.usb_type      = USB_XHCI,
	.next          = NULL,
};

void usb_xhci_register(void)
{
	usb_hcd_register(&xhci_ops);
}
