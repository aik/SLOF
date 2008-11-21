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

/* Initialize the environment for ARP client. */
extern void arp_init(int32_t device_socket, uint8_t own_mac[], uint32_t own_ip);

/* For given IPv4 retrieves MAC via ARP */
extern int8_t arp_getmac(uint32_t dest_ip, uint8_t dest_mac[]);

/* Handles ARP-packets, which are detected by receive_ether. */
extern int8_t handle_arp(uint8_t * packet, int32_t packetsize);
