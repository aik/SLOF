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

#ifndef _NETDRIVER_INT_H
#define _NETDRIVER_INT_H
#include <stddef.h>

/* Constants for different kinds of IOCTL requests
 */

#define SIOCETHTOOL           0x1000

typedef struct {
	unsigned int addr;
	unsigned int size;
	int type;
} bar_t;

typedef struct {
	unsigned long long puid;
	unsigned int bus;
	unsigned int devfn;
	unsigned int vendor_id;
	unsigned int device_id;
	unsigned int revision_id;
	unsigned int class_code;
	bar_t bars[6];
	unsigned int interrupt_line;
} pci_config_t;

typedef int (*net_init_t) (pci_config_t * conf, char *mac_addr);
typedef int (*net_term_t) (pci_config_t * conf);
typedef int (*net_receive_t) (pci_config_t * conf, char *buffer, int len);
typedef int (*net_xmit_t) (pci_config_t * conf, char *buffer, int len);
typedef int (*net_ioctl_t) (pci_config_t * conf, int request, void *data);

typedef struct {
	int version;
	net_init_t net_init;
	net_term_t net_term;
	net_receive_t net_receive;
	net_xmit_t net_xmit;
	net_ioctl_t net_ioctl;
} snk_module_t;


typedef int (*print_t) (const char *, ...);
typedef void (*us_delay_t) (unsigned int);
typedef void (*ms_delay_t) (unsigned int);
typedef int (*pci_config_read_t) (long long puid, int size,
				  int bus, int devfn, int offset);
typedef int (*pci_config_write_t) (long long puid, int size,
				   int bus, int devfn, int offset, int value);
typedef void *(*malloc_aligned_t) (size_t, int);
typedef unsigned int (*io_read_t) (void *, size_t);
typedef int (*io_write_t) (void *, unsigned int, size_t);
typedef unsigned int (*romfs_lookup_t) (const char *name, void **addr);
typedef void (*translate_addr_t) (unsigned long *);

typedef struct {
	int version;
	print_t print;
	us_delay_t us_delay;
	ms_delay_t ms_delay;
	pci_config_read_t pci_config_read;
	pci_config_write_t pci_config_write;
	malloc_aligned_t k_malloc_aligned;
	io_read_t io_read;
	io_write_t io_write;
	romfs_lookup_t k_romfs_lookup;
	translate_addr_t translate_addr;
} snk_kernel_t;


/* special structure and constants for IOCTL requests of type ETHTOOL
 */

#define ETHTOOL_GMAC         0x03
#define ETHTOOL_SMAC         0x04
#define ETHTOOL_VERSION      0x05

typedef struct {
	int idx;
	char address[6];
} ioctl_ethtool_mac_t;

typedef struct {
	unsigned int length;
	char *text;
} ioctl_ethtool_version_t;


/* default structure and constants for IOCTL requests
 */

#define IF_NAME_SIZE 0xFF

typedef struct {
	char if_name[IF_NAME_SIZE];
	int subcmd;
	union {
		ioctl_ethtool_mac_t mac;
		ioctl_ethtool_version_t version;
	} data;
} ioctl_net_data_t;

/* Entry of module */
snk_module_t *module_init(snk_kernel_t * snk_kernel_int,
			  pci_config_t * pciconf);
#endif				/* _NETDRIVER_INT_H */
