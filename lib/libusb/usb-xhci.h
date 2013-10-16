/******************************************************************************
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
/*
 * Definitions for XHCI Controller - Revision 1.0 (5/21/10)
 *
 */

#ifndef USB_XHCI_H
#define USB_XHCI_H

#include <stdint.h>
#include "usb-core.h"

/* 5.3 Host Controller Capability Registers
 * Table 19
 */
struct xhci_cap_regs {
	uint8_t caplength;
	uint8_t reserved;
	uint16_t hciversion;
	uint32_t hcsparams1;
	uint32_t hcsparams2;
	uint32_t hcsparams3;
	uint32_t hccparams;
	uint32_t dboff;
	uint32_t rtsoff;
} __attribute__ ((packed));

/* Table 27: Host Controller USB Port Register Set */
struct xhci_port_regs {
	uint32_t portsc;
	uint32_t portpmsc;
	uint32_t portli;
	uint32_t reserved;
} __attribute__ ((packed));

/* 5.4 Host Controller Operational Registers
 * Table 26
 */
struct xhci_op_regs {
	uint32_t usbcmd;
	uint32_t usbsts;
	uint32_t pagesize;
	uint8_t reserved[8];    /* 0C - 13 */
	uint32_t dnctrl;        /* Device notification control */
	uint64_t crcr;          /* Command ring control */
	uint8_t reserved1[16];  /* 20 - 2F */
	uint64_t dcbaap;        /* Device Context Base Address Array Pointer */
	uint8_t config;         /* Configure */
	uint8_t reserved2[964]; /* 3C - 3FF */
	/* USB Port register set */
#define XHCI_PORT_MAX 256
	struct xhci_port_regs prs[XHCI_PORT_MAX];
} __attribute__ ((packed));

/*
 * 5.5.2  Interrupter Register Set
 * Table 42: Interrupter Registers
 */
struct xhci_int_regs {
	uint32_t iman;
	uint32_t imod;
	uint32_t erstsz;
	uint32_t reserved;
	uint64_t erstba;
	uint64_t erdp;
} __attribute__ ((packed));

/* 5.5 Host Controller Runtime Registers */
struct xhci_run_regs {
	uint32_t mfindex;       /* microframe index */
	uint8_t reserved[28];
#define XHCI_IRS_MAX
	struct xhci_int_regs irs[XHCI_IRS_MAX];
} __attribute__ ((packed));

/* 5.6 Doorbell Registers*/
struct xhci_db_regs {
	uint32_t db[256];
}  __attribute__ ((packed));

struct xhci_hcd {
	struct xhci_cap_regs *cap_regs;
	struct xhci_op_regs  *op_regs;
	struct xhci_run_regs *run_regs;
	struct xhci_db_regs *db_regs;
	struct usb_hcd_dev *hcidev;
	struct usb_pipe *freelist;
	struct usb_pipe *end;
	void *pool;
	long pool_phys;
};

struct xhci_pipe {
	struct usb_pipe pipe;
	long qh_phys;
};

#endif	/* USB_XHCI_H */
