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

/* Network debug switches */

/* main debug switch, without this switch the others don't work */
// #define NET_DEBUG

/* show received data */
// #define NET_SHOW_RECV

/* show transmitted data */
// #define NET_SHOW_XMIT

#include <types.h>

/* Initializes basic network enviroment and ARP-client */
extern void netbase_init(int32_t device_socket, uint8_t * own_mac, uint32_t own_ip);

/* Sets routing parameters */
extern int8_t net_setrouter(uint32_t router_ip, uint32_t subnet_mask);

/* For given IP retrieves MAC address (or sets router MAC) */
extern int8_t net_iptomac(uint32_t dest_ip, uint8_t dest_mac[]);

/* Receives and handles packets, according to Receive-handle diagram */
extern int32_t receive_ether(void);

/* fills ethernet header */
extern void fill_ethhdr(uint8_t * packet, uint16_t eth_type,
                        uint8_t * src_mac, uint8_t * dest_mac);

/* fills ip header */
extern void fill_iphdr(uint8_t * packet, uint16_t packetsize,
                       uint8_t ip_proto, uint32_t ip_src, uint32_t ip_dst);

/* fills udp header */
extern void fill_udphdr(uint8_t *packet, uint16_t packetsize,
                        uint16_t src_port, uint16_t dest_port);

#ifdef NET_DEBUG


/* DEBUG: Dumps ethernet-packet with appropriate header names */
extern void net_print_packet(uint8_t * packet, uint16_t packetsize);


#define NET_DEBUG_PRINTF(format, ...) printf(format, ## __VA_ARGS__)


/* Prints message and IP in the following format: "DDD.DDD.DDD.DDD" */
#define PRINT_MSGIP(msg, ip) \
	do { \
		printf("%s%03d.%03d.%03d.%03d\n", msg, \
		       (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, \
		       (ip >> 8) & 0xFF,  (ip >> 0) & 0xFF); \
	} while (0)


/* Prints message and MAC in the following format "XX.XX.XX.XX.XX.XX" */
#define PRINT_MSGMAC(msg, mac) \
	do { \
		printf("%s%02X:%02X:%02X:%02X:%02X:%02X\n", msg, \
		       mac[0], mac[1], mac[2], \
		       mac[3], mac[4], mac[5]); \
	} while (0)


/* Prints packet header in the following form: "msg: XX XX XX ... XX" */
#define PRINT_HDR(msg, hdr, hdrsize) \
	do { \
		uint16_t i, j; \
		printf("%s", msg); \
		for (i = 0; i < hdrsize; ) { \
			for (j = 0; j < 20; j++, i++) \
				printf("%2x ", * (hdr + i)); \
				printf("\n\t\t"); \
			} \
		printf("\n"); \
	} while (0)


#ifdef NET_SHOW_XMIT 

/* Prints "sending" packet with approp. header names */
#define PRINT_SENDING(packet, packetsize) \
	do { \
		printf("\nSending packet, size:\t%d\n", packetsize); \
		net_print_packet(packet, packetsize); \
	} while (0)

#else

#define PRINT_SENDING(packet, packetsize)

#endif

#ifdef NET_SHOW_RECV

/* Prints "received" packet with approp. header names */
#define PRINT_RECEIVED(packet, packetsize) \
	do { \
		printf("\nReceived packet, size:\t%d\n", packetsize); \
		net_print_packet(packet, packetsize); \
	} while (0)

#else

#define PRINT_RECEIVED(packet, packetsize)

#endif

#else // NET_DEBUG not defined

#define NET_DEBUG_PRINTF(format, ...)

#define PRINT_MSGIP(msg, ip)

#define PRINT_MSGMAC(msg, mac)

#define PRINT_HDR(msg, hdr, hdrsize)

#define NET_PRINT_PACKET(packet, packetsize)

#define PRINT_SENDING(packet, packetsize)

#define PRINT_RECEIVED(packet, packetsize)

#endif
