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

struct ehci_controller {
	struct ehci_cap_regs *cap_regs;
	struct ehci_op_regs  *op_regs;
};

#endif	/* USB_EHCI_H */
