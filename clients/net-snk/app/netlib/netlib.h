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

#ifndef _NETLIB_H_
#define _NETLIB_H_

#include <types.h>

#define ETH_MTU_SIZE     1518   /**< Maximum Transfer Unit         */
#define ETH_ALEN            6   /**< HW address length             */
#define ETHERTYPE_IP   0x0800
#define ETHERTYPE_ARP  0x0806

#define IPTYPE_ICMP         1
#define IPTYPE_UDP         17

#define UDPPORT_BOOTPS     67   /**< UDP port of BootP/DHCP-server */
#define UDPPORT_BOOTPC     68   /**< UDP port of BootP/DHCP-client */
#define UDPPORT_DNSS       53   /**< UDP port of DNS-server        */
#define UDPPORT_DNSC    32769   /**< UDP port of DNS-client        */

typedef int32_t socklen_t;

/** \struct ethhdr
 *  A header for Ethernet-packets.
 */
struct ethhdr {
	uint8_t dest_mac[ETH_ALEN];   /**< Destination HW address        */
	uint8_t src_mac[ETH_ALEN];    /**< Source HW address             */
	uint16_t type;                /**< Next level protocol type      */
};

/** \struct iphdr
 *  A header for IP-packets.
 *  For more information see RFC 791.
 */
struct iphdr {
	uint8_t ip_hlv;      /**< Header length and version of the header      */
	uint8_t ip_tos;      /**< Type of Service                              */
	uint16_t ip_len;     /**< Length in octets, inlc. this header and data */
	uint16_t ip_id;      /**< ID is used to aid in assembling framents     */
	uint16_t ip_off;     /**< Info about fragmentation (control, offset)   */
	uint8_t ip_ttl;      /**< Time to Live                                 */
	uint8_t ip_p;        /**< Next level protocol type                     */
	uint16_t ip_sum;     /**< Header checksum                              */
	uint32_t ip_src;     /**< Source IP address                            */
	uint32_t ip_dst;     /**< Destination IP address                       */
};

struct pseudo_iphdr {
	uint32_t ip_src;
	uint32_t ip_dst;
	int8_t ip_unused;
	uint8_t ip_p;
	uint16_t ip_ulen;
};

/** \struct udphdr
 *  A header for UDP-packets.
 *  For more information see RFC 768.
 */
struct udphdr {
	uint16_t uh_sport;   /**< Source port                                  */
	uint16_t uh_dport;   /**< Destinantion port                            */
	uint16_t uh_ulen;    /**< Length in octets, incl. this header and data */
	uint16_t uh_sum;     /**< Checksum                                     */
};

/** \struct btphdr
 *  A header for BootP/DHCP-messages.
 *  For more information see RFC 951 / RFC 2131.
 */
struct btphdr {
	uint8_t op;          /**< Identifies is it request (1) or reply (2)    */
	uint8_t htype;       /**< HW address type (ethernet usually)           */
	uint8_t hlen;        /**< HW address length                            */
	uint8_t hops;        /**< This info used by relay agents (not used)    */
	uint32_t xid;        /**< This ID is used to match queries and replies */
	uint16_t secs;       /**< Unused                                       */
	uint16_t unused;     /**< Unused                                       */
	uint32_t ciaddr;     /**< Client IP address (if client knows it)       */
	uint32_t yiaddr;     /**< "Your" (client) IP address                   */
	uint32_t siaddr;     /**< Next server IP address (TFTP server IP)      */
	uint32_t giaddr;     /**< Gateway IP address (used by relay agents)    */
	uint8_t chaddr[16];  /**< Client HW address                            */
	uint8_t sname[64];   /**< Server host name (TFTP server name)          */
	uint8_t file[128];   /**< Boot file name                               */
	uint8_t vend[64];    /**< Optional parameters field (DHCP-options)     */
};

struct tftphdr {
	int16_t th_opcode;
	uint16_t th_data;
};


/** \struct arphdr
 *  A header for ARP-messages, retains info about HW and proto addresses.
 *  For more information see RFC 826.
 */
struct arphdr {
	uint16_t hw_type;    /**< HW address space (1 for Ethernet)            */
	uint16_t proto_type; /**< Protocol address space                       */
	uint8_t hw_len;      /**< Byte length of each HW address               */
	uint8_t proto_len;   /**< Byte length of each proto address            */
	uint16_t opcode;     /**< Identifies is it request (1) or reply (2)    */
	uint8_t src_mac[6];  /**< HW address of sender of this packet          */
	uint32_t src_ip;     /**< Proto address of sender of this packet       */
	uint8_t dest_mac[6]; /**< HW address of target of this packet          */
	uint32_t dest_ip;    /**< Proto address of target of this packet       */
} __attribute((packed));

typedef struct {
	uint32_t own_ip;
	uint32_t server_ip;
	uint8_t own_mac[6];	// unsigned
	uint8_t server_mac[6];	// unsigned 
	int8_t filename[256];
} filename_ip_t;

int dhcp(int32_t, char *ret_buffer, filename_ip_t *, unsigned int);
void dhcp_send_release(void);

int bootp(int32_t, char *ret_buffer, filename_ip_t *, unsigned int);

typedef struct {
	uint32_t bad_tftp_packets;
	uint32_t no_packets;
	uint32_t blocks_missed;
	uint32_t blocks_received;
} tftp_err_t;

int tftp(int, filename_ip_t *, unsigned char  *, int, unsigned int, tftp_err_t *, int huge_load);
int tftp_netsave(int32_t, filename_ip_t *, uint8_t * buffer, int32_t len, 
		int use_ci, unsigned int retries, tftp_err_t * tftp_err);

#endif /* _NETLIB_H_ */
