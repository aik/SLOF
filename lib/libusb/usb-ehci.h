/******************************************************************************
 * Copyright (c) 2007, 2012, 2013 IBM Corporation
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
 * Definitions for EHCI Controller
 *
 */

#ifndef USB_EHCI_H
#define USB_EHCI_H

#include <stdint.h>

#define FL_SIZE	1024

struct ehci_cap_regs {
	uint8_t  caplength;
	uint8_t  reserved;
	uint16_t hciversion;
	uint32_t hcsparams;
	uint32_t hccparams;
	uint64_t portroute;
} __attribute__ ((packed));

struct ehci_op_regs {
	uint32_t usbcmd;
	uint32_t usbsts;
	uint32_t usbintr;
	uint32_t frindex;
	uint32_t ctrldssegment;
	uint32_t periodiclistbase;
	uint32_t asynclistaddr;
	uint32_t reserved[9];
	uint32_t configflag;
	uint32_t portsc[0];
} __attribute__ ((packed));

struct ehci_hcd {
	struct ehci_cap_regs *cap_regs;
	struct ehci_op_regs  *op_regs;
	struct usb_hcd_dev *hcidev;
};

struct ehci_framelist {
	uint32_t fl_ptr[FL_SIZE];
} __attribute__ ((packed));

struct ehci_qtd {
	uint32_t next_qtd;
	uint32_t alt_next_qtd;
	uint32_t token;
	uint32_t buffer[5];
} __attribute__ ((packed));

struct ehci_qh {
	uint32_t qh_ptr;
	uint32_t ep_cap1;
	uint32_t ep_cap2;
	uint32_t curr_qtd;
	uint32_t next_qtd;
	uint32_t alt_next_qtd;
	uint32_t token;
	uint32_t buffer[5];
} __attribute__ ((packed));

#define EHCI_TYP_ITD	0x00
#define EHCI_TYP_QH	0x02
#define EHCI_TYP_SITD	0x04
#define EHCI_TYP_FSTN	0x06

#define CMD_ASE		(1 << 5)
#define CMD_PSE		(1 << 4)
#define CMD_FLS_MASK	(3 << 2)
#define CMD_HCRESET	(1 << 1)
#define CMD_RUN		(1 << 0)

#define QH_CAP_H	(1 << 15)
#define QH_PTR_TERM	0x0001
#define QH_SMASK_SHIFT	0
#define QH_STS_HALTED	(1 << 6)

#endif	/* USB_EHCI_H */
