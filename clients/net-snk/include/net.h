/******************************************************************************
 * Copyright (c) 2004, 2007 IBM Corporation
 * All rights reserved.
 * This program and the accompanying materials
 * are made available under the terms of the BSD License
 * which accompanies this distribution, and is available at
 * http://www.opensource.org/licenses/bsd-license.php
 *
 * Contributors:
 *     IBM Corporation - initial implementation
 *****************************************************************************/


#ifndef _NET_H
#define _NET_H
#include <pci.h>

typedef struct {
    int device_id;
    int vendor_id;

    int (*net_init) (pci_config_t *pciconf, char *mac_addr);
    int (*net_term) (pci_config_t *pciconf);
    int (*net_transmit) (pci_config_t *pciconf, char *buffer, int len);
    int (*net_receive) (pci_config_t *pciconf, char *buffer, int len);
    int (*net_ioctl) (pci_config_t *pciconf, int request, void* data);

} net_driver_t;

#endif
