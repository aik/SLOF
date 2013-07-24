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
#include "usb-ehci.h"
#include "tools.h"

#undef EHCI_DEBUG
//#define EHCI_DEBUG
#ifdef EHCI_DEBUG
#define dprintf(_x ...) printf(_x)
#else
#define dprintf(_x ...)
#endif

#ifdef EHCI_DEBUG
static void dump_ehci_regs(struct ehci_hcd *ehcd)
{
	struct ehci_cap_regs *cap_regs;
	struct ehci_op_regs *op_regs;

	cap_regs = ehcd->cap_regs;
	op_regs = ehcd->op_regs;

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
#endif

static int ehci_hcd_init(struct ehci_hcd *ehcd)
{
	uint32_t usbcmd;
	uint32_t time;
	struct ehci_framelist *fl;
	struct ehci_qh *qh_intr, *qh_async;
	int i;
	long fl_phys = 0, qh_intr_phys = 0, qh_async_phys;

	/* Reset the host controller */
	time = SLOF_GetTimer() + 250;
	usbcmd = read_reg32(&ehcd->op_regs->usbcmd);
	write_reg32(&ehcd->op_regs->usbcmd, (usbcmd & ~(CMD_PSE | CMD_ASE)) | CMD_HCRESET);
	while (time > SLOF_GetTimer())
		cpu_relax();
	usbcmd = read_reg32(&ehcd->op_regs->usbcmd);
	if (usbcmd & CMD_HCRESET) {
		printf("usb-ehci: reset failed\n");
		return -1;
	}

	/* Initialize periodic list */
	fl = SLOF_dma_alloc(sizeof(*fl));
	if (!fl) {
		printf("usb-ehci: Unable to allocate frame list\n");
		goto fail;
	}
	fl_phys = SLOF_dma_map_in(fl, sizeof(*fl), true);
	dprintf("fl %p, fl_phys %lx\n", fl, fl_phys);

	/* TODO: allocate qh pool */
	qh_intr = SLOF_dma_alloc(sizeof(*qh_intr));
	if (!qh_intr) {
		printf("usb-ehci: Unable to allocate interrupt queue head\n");
		goto fail_qh_intr;
	}
	qh_intr_phys = SLOF_dma_map_in(qh_intr, sizeof(*qh_intr), true);
	dprintf("qh_intr %p, qh_intr_phys %lx\n", qh_intr, qh_intr_phys);

	memset(qh_intr, 0, sizeof(*qh_intr));
	qh_intr->qh_ptr = QH_PTR_TERM;
	qh_intr->ep_cap2 = cpu_to_le32(0x01 << QH_SMASK_SHIFT);
	qh_intr->next_qtd = qh_intr->alt_next_qtd = QH_PTR_TERM;
	qh_intr->token = cpu_to_le32(QH_STS_HALTED);
	for (i = 0; i < FL_SIZE; i++)
		fl->fl_ptr[i] = cpu_to_le32(qh_intr_phys | EHCI_TYP_QH);
	write_reg32(&ehcd->op_regs->periodiclistbase, fl_phys);

	/* Initialize async list */
	qh_async = SLOF_dma_alloc(sizeof(*qh_async));
	if (!qh_async) {
		printf("usb-ehci: Unable to allocate async queue head\n");
		goto fail_qh_async;
	}
	qh_async_phys = SLOF_dma_map_in(qh_async, sizeof(*qh_async), true);
	dprintf("qh_async %p, qh_async_phys %lx\n", qh_async, qh_async_phys);

	memset(qh_async, 0, sizeof(*qh_async));
	qh_async->qh_ptr = cpu_to_le32(qh_async_phys | EHCI_TYP_QH);
	qh_async->ep_cap1 = cpu_to_le32(QH_CAP_H);
	qh_async->next_qtd = qh_async->alt_next_qtd = QH_PTR_TERM;
	qh_async->token = cpu_to_le32(QH_STS_HALTED);
	write_reg32(&ehcd->op_regs->asynclistaddr, qh_async_phys);

	write_reg32(&ehcd->op_regs->usbcmd, usbcmd | CMD_ASE | CMD_RUN);
	write_reg32(&ehcd->op_regs->configflag, 1);

	return 0;

fail_qh_async:
	SLOF_dma_map_out(qh_intr_phys, qh_intr, sizeof(*qh_intr));
	SLOF_dma_free(qh_intr, sizeof(*qh_intr));
fail_qh_intr:
	SLOF_dma_map_out(fl_phys, fl, sizeof(*fl));
	SLOF_dma_free(fl, sizeof(*fl));
fail:
	return -1;
}

static void ehci_init(struct usb_hcd_dev *hcidev)
{
	struct ehci_hcd *ehcd;

	printf("  EHCI: Initializing\n");
	dprintf("%s: device base address %p\n", __func__, hcidev->base);

	ehcd = SLOF_dma_alloc(sizeof(*ehcd));
	if (!ehcd) {
		printf("usb-ehci: Unable to allocate memory\n");
		return;
	}

	hcidev->priv = ehcd;
	ehcd->hcidev = hcidev;
	ehcd->cap_regs = (struct ehci_cap_regs *)(hcidev->base);
	ehcd->op_regs = (struct ehci_op_regs *)(hcidev->base +
						read_reg8(&ehcd->cap_regs->caplength));
#ifdef EHCI_DEBUG
	dump_ehci_regs(ehcd);
#endif
	ehci_hcd_init(ehcd);
	//ehci_hub_check_ports(ehcd);
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
