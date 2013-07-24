
/******************************************************************************
 * Copyright (c) 2006, 2012, 2013 IBM Corporation
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
 * prototypes for libusb implementation used in libusb.code
 */

#ifndef __LIBUSB_H
#define __LIBUSB_H

/*******************************************/
/* SLOF:  USB-OHCI-REGISTER                */
/*******************************************/
extern void usb_ohci_register(void);
/*******************************************/
/* SLOF:  USB-HCD-INIT                     */
/*******************************************/
extern void usb_hcd_init(void *hcidev);

#endif

