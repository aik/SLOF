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


/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> ALGORITHMS <<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

/** \file netbase.c <pre>
 * *********************** Receive-handle diagram *************************
 *
 *                      Note: Every layer calls out required upper layer
 * lower
 *                 Receive packet (receive_ether)
 *  |                    |
 *  |                    |
 *  |                 Ethernet (handle_ether)
 *  |                    |
 *  |        +-----------+---------+
 *  |        |                     |
 *  |       ARP (handle_arp)       IP (handle_ip)   
 *  |                              |    
 *  |                              |
 *  |                             UDP (handle_udp)
 *  |                              |
 *  V             +----------------+-----------+
 *                |                            |
 * upper        DNS (handle_dns)      BootP / DHCP (handle_bootp_client)
 * 
 * ************************************************************************
 * </pre> */


/*>>>>>>>>>>>>>>>>>>>>>>> DEFINITIONS & DECLARATIONS <<<<<<<<<<<<<<<<<<<<*/

#include <types.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <netlib/netlib.h>
#include <netlib/netbase.h>
#include <netlib/arp.h>
#include <netlib/dhcp.h>
#include <netlib/dns.h>


/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>> PROTOTYPES <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

static int8_t
handle_ip(uint8_t * packet, int32_t packetsize);

static int8_t
handle_udp(uint8_t * packet, int32_t packetsize);

static unsigned short
checksum(unsigned short *packet, int words);


/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>> LOCAL VARIABLES <<<<<<<<<<<<<<<<<<<<<<<<<*/

static uint8_t  ether_packet[ETH_MTU_SIZE];
static int32_t  net_device_socket = 0;
static uint8_t  net_own_mac[6];
static uint32_t net_own_ip        = 0;

/* Routing parameters */
static uint32_t net_router_ip     = 0;
static uint8_t  net_router_mac[]  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint32_t net_subnet_mask   = 0;



/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>> IMPLEMENTATION <<<<<<<<<<<<<<<<<<<<<<<<<<*/

/**
 * NET: Initializes basic network enviroment and ARP-client.
 *
 * @param  device_socket a socket number used to send and recieve packets
 * @param  own_mac       own hardware-address (MAC)
 * @param  own_ip        own IPv4 address (e.g. 127.0.0.1)
 */
void
netbase_init(int32_t device_socket, uint8_t * own_mac, uint32_t own_ip) {
	net_device_socket = device_socket;
	if (own_mac)
		memcpy(net_own_mac, own_mac, 6);
	else
		memset(net_own_mac, 0, 6);
	net_own_ip = own_ip;
	arp_init(device_socket, own_mac, own_ip);
}

/**
 * NET: Sets routing parameters. 
 *      For more information see net_iptomac function.
 *
 * @param  router_ip     IP address of the router
 * @param  subnet_mask   Subnet mask
 * @see                  net_iptomac
 * @return               TRUE - router was installed;
 *                       FALSE - error condition occurs.
 */
int8_t
net_setrouter(uint32_t router_ip, uint32_t subnet_mask) {
	net_subnet_mask = subnet_mask;
	net_router_ip = router_ip;

	if(router_ip != 0) {
		PRINT_MSGIP("\nRouter IP:\t\t", net_router_ip);
		if (arp_getmac(net_router_ip, net_router_mac)) {
			PRINT_MSGMAC("Router MAC:\t\t", net_router_mac);
			return 1;
		}
	}

	memset(net_router_mac, 0, 6);
	NET_DEBUG_PRINTF("WARNING:\t\tCan't obtain router MAC!\n");
	return 0;
}

/**
 * NET: For given IP retrieves MAC address (or sets router MAC). 
 *      To set routing parameters use net_setrouter function.
 *
 * @param  dest_ip   IP address of the host
 * @param  dest_mac  in case of SUCCESS stores MAC address of the host; 
 *                   in case of FAULT filled with zeros.
 * @see              net_setrouter
 * @return           TRUE - destination MAC address was obtained;
 *                   FALSE - can't obtain destination MAC address
 */
int8_t
net_iptomac(uint32_t dest_ip, uint8_t dest_mac[]) {
	/* check if dest_ip and own_ip are in the same subnet */
	if ((net_subnet_mask & net_own_ip) ==
	    (net_subnet_mask & dest_ip)) {
		/* In the same subnet - use ARP to obtain DNS-server MAC */
		if (!arp_getmac(dest_ip, dest_mac)) {
			NET_DEBUG_PRINTF("\nWARNING:\t\tCan't retrieve MAC!\n");
			memset(dest_mac, 0, 6);
			return 0;
		}
		return 1;
	}

	/* In different subnets - check if router is presented */
	if (net_router_mac[0] != 0 || net_router_mac[1] != 0
	 || net_router_mac[2] != 0 || net_router_mac[3] != 0
	 || net_router_mac[4] != 0 || net_router_mac[5] != 0) {
		/* Use router MAC-address for DNS-connections */
		memcpy(dest_mac, net_router_mac, 6);
		return 1;
	}

	NET_DEBUG_PRINTF("\nWARNING:\t\tCan't obtain MAC (router is not presented)!\n");
	return 0;
}

/**
 * NET: Receives an ethernet-packet and handles it according to
 *      Receive-handle diagram.
 *
 * @return           ZERO - packet was handled or no packets received;
 *                   NON ZERO - error condition occurs.
 */
int32_t
receive_ether(void) {
	int32_t bytes_received;
	struct ethhdr * ethh;

	memset(ether_packet, 0, ETH_MTU_SIZE);
	bytes_received = recv(net_device_socket, ether_packet, ETH_MTU_SIZE, 0);

	if (!bytes_received) // No messages
		return 0;

	PRINT_RECEIVED(ether_packet, bytes_received);

	if (bytes_received < sizeof(struct ethhdr))
		return -1; // packet is too small

	ethh = (struct ethhdr *) ether_packet;

	switch (htons(ethh -> type)) {
	case ETHERTYPE_IP:
		return handle_ip(ether_packet + sizeof(struct ethhdr),
		                 bytes_received - sizeof(struct ethhdr));
	case ETHERTYPE_ARP:
		return handle_arp(ether_packet + sizeof(struct ethhdr),
		                  bytes_received - sizeof(struct ethhdr));
	default:
		break;
	}
	return -1; // unknown protocol
}

/**
 * NET: Handles IP-packets according to Receive-handle diagram.
 *
 * @param  ip_packet  IP-packet to be handled
 * @param  packetsize Length of the packet
 * @return            ZERO - packet handled successfully;
 *                    NON ZERO - packet was not handled (e.g. bad format)
 * @see               receive_ether
 * @see               iphdr
 */
static int8_t
handle_ip(uint8_t * ip_packet, int32_t packetsize) {
	struct iphdr * iph;
	int32_t old_sum;

	if (packetsize < sizeof(struct iphdr))
		return -1; // packet is too small

	iph = (struct iphdr * ) ip_packet;

	old_sum = iph -> ip_sum;
	iph -> ip_sum = 0;
	if (old_sum != checksum((uint16_t *) iph, sizeof (struct iphdr) >> 1))
		return -1; // Wrong IP checksum

	switch (iph -> ip_p) {
	case IPTYPE_UDP:
		return handle_udp(ip_packet + sizeof(struct iphdr),
		                  iph -> ip_len - sizeof(struct iphdr));
	default:
		break;
	}
	return -1; // Unknown protocol
}

/**
 * NET: Handles UDP-packets according to Receive-handle diagram.
 *
 * @param  udp_packet UDP-packet to be handled
 * @param  packetsize Length of the packet
 * @return            ZERO - packet handled successfully;
 *                    NON ZERO - packet was not handled (e.g. bad format)
 * @see               receive_ether
 * @see               udphdr
 */
static int8_t
handle_udp(uint8_t * udp_packet, int32_t packetsize) {
	struct udphdr * udph = (struct udphdr *) udp_packet;

	if (packetsize < sizeof(struct udphdr))
		return -1; // packet is too small

	switch (htons(udph -> uh_dport)) {
	case UDPPORT_BOOTPC:
		if (udph -> uh_sport == htons(UDPPORT_BOOTPS))
			return handle_dhcp(udp_packet + sizeof(struct udphdr),
			                    packetsize - sizeof(struct udphdr));
		else
			return -1;
	case UDPPORT_DNSC:
		if (udph -> uh_sport == htons(UDPPORT_DNSS))
			return handle_dns(udp_packet + sizeof(struct udphdr),
			                  packetsize - sizeof(struct udphdr));
		else
			return -1;
	default:
		return -1;
	}
}

/**
 * NET: Creates Ethernet-packet. Places Ethernet-header in a packet and
 *      fills it with corresponding information.
 *      <p>
 *      Use this function with similar functions for other network layers
 *      (fill_arphdr, fill_iphdr, fill_udphdr, fill_dnshdr, fill_btphdr).
 *
 * @param  packet      Points to the place where eth-header must be placed.
 * @param  eth_type    Type of the next level protocol (e.g. IP or ARP).
 * @param  src_mac     Sender MAC address
 * @param  dest_mac    Receiver MAC address
 * @see                ethhdr
 * @see                fill_arphdr
 * @see                fill_iphdr
 * @see                fill_udphdr
 * @see                fill_dnshdr
 * @see                fill_btphdr
 */
void
fill_ethhdr(uint8_t * packet, uint16_t eth_type,
            uint8_t * src_mac, uint8_t * dest_mac) {
	struct ethhdr * ethh = (struct ethhdr *) packet;

	ethh -> type = htons(eth_type);
	memcpy(ethh -> src_mac, src_mac, 6);
	memcpy(ethh -> dest_mac, dest_mac, 6);
}

/**
 * NET: Creates IP-packet. Places IP-header in a packet and fills it
 *      with corresponding information.
 *      <p>
 *      Use this function with similar functions for other network layers
 *      (fill_ethhdr, fill_udphdr, fill_dnshdr, fill_btphdr).
 *
 * @param  packet      Points to the place where IP-header must be placed.
 * @param  packetsize  Size of the packet in bytes incl. this hdr and data.
 * @param  ip_proto    Type of the next level protocol (e.g. UDP).
 * @param  ip_src      Sender IP address
 * @param  ip_dst      Receiver IP address
 * @see                iphdr
 * @see                fill_ethhdr
 * @see                fill_udphdr
 * @see                fill_dnshdr
 * @see                fill_btphdr
 */
void
fill_iphdr(uint8_t * packet, uint16_t packetsize,
           uint8_t ip_proto, uint32_t ip_src, uint32_t ip_dst) {
	struct iphdr * iph = (struct iphdr *) packet;

	iph -> ip_hlv = 0x45;
	iph -> ip_tos = 0x10;
	iph -> ip_len = htons(packetsize);
	iph -> ip_id = htons(0);
	iph -> ip_off = 0;
	iph -> ip_ttl = 0xFF;
	iph -> ip_p = ip_proto;
	iph -> ip_src = htonl(ip_src);
	iph -> ip_dst = htonl(ip_dst);
	iph -> ip_sum = 0;
	iph -> ip_sum = checksum((unsigned short *) iph, sizeof(struct iphdr) >> 1);
}

/**
 * NET: Creates UDP-packet. Places UDP-header in a packet and fills it
 *      with corresponding information.
 *      <p>
 *      Use this function with similar functions for other network layers
 *      (fill_ethhdr, fill_iphdr, fill_dnshdr, fill_btphdr).
 *
 * @param  packet      Points to the place where UDP-header must be placed.
 * @param  packetsize  Size of the packet in bytes incl. this hdr and data.
 * @param  src_port    UDP source port
 * @param  dest_port   UDP destination port
 * @see                udphdr
 * @see                fill_ethhdr
 * @see                fill_iphdr
 * @see                fill_dnshdr
 * @see                fill_btphdr
 */
void
fill_udphdr(uint8_t * packet, uint16_t packetsize,
            uint16_t src_port, uint16_t dest_port) {
	struct udphdr * udph = (struct udphdr *) packet;

	udph -> uh_sport = htons(src_port);
	udph -> uh_dport = htons(dest_port);
	udph -> uh_ulen = htons(packetsize);
	udph -> uh_sum = htons(0);
}

/**
 * NET: Calculates checksum for IP header.
 *
 * @param  packet     Points to the IP-header
 * @param  words      Size of the packet in words incl. IP-header and data.
 * @return            Checksum
 * @see               iphdr
 */
static unsigned short
checksum(unsigned short * packet, int words) {
	unsigned long checksum;

	for (checksum = 0; words > 0; words--)
		checksum += *packet++;
	checksum = (checksum >> 16) + (checksum & 0xffff);
	checksum += (checksum >> 16);

	return ~checksum;
}


#ifdef NET_DEBUG              // Printing for debugging purposes

/**
 * DEBUG: Dumps ethernet-packet with appropriate header names.
 *
 * @param  packet     Points to the ethernet-packet.
 * @param  packetsize Size of the packet in bytes.
 */
void
net_print_packet(uint8_t * packet, uint16_t packetsize) {
	struct ethhdr * ethh = (struct ethhdr *) packet;
	struct iphdr * iph;
	struct udphdr * udph;
	struct btphdr * btph;
	struct dnshdr * dnsh;
	struct arphdr * arph;

	PRINT_HDR("ETH:\t\t", (uint8_t *) ethh, sizeof(struct ethhdr));

	switch (htons(ethh -> type)) {
	case ETHERTYPE_IP :
		iph = (struct iphdr *) (packet + sizeof(struct ethhdr));
		PRINT_HDR("IP:\t\t", (uint8_t *) iph, sizeof(struct iphdr));
		switch (iph -> ip_p) {
		case IPTYPE_UDP:
			udph = (struct udphdr *) (packet +
			       sizeof(struct ethhdr) +
			       sizeof(struct iphdr));
			PRINT_HDR("UDP:\t\t", (uint8_t *) udph, sizeof(struct udphdr));
			switch (htons(udph -> uh_dport)) {
			case UDPPORT_BOOTPC: case UDPPORT_BOOTPS:
				btph = (struct btphdr *) (packet +
				       sizeof(struct ethhdr) +
				       sizeof(struct iphdr) +
				       sizeof(struct udphdr));
				PRINT_HDR("DHCP/BootP:\t", (uint8_t *) btph,
				          packetsize - sizeof(struct ethhdr) -
				          sizeof(struct iphdr) -
				          sizeof(struct udphdr));
				break;
			case UDPPORT_DNSC: case UDPPORT_DNSS:
				dnsh = (struct dnshdr *) (packet +
				       sizeof(struct ethhdr) +
				       sizeof(struct iphdr) +
				       sizeof(struct udphdr));
				PRINT_HDR("DNS:\t\t", (uint8_t *) dnsh,
				          packetsize - sizeof(struct ethhdr) -
				          sizeof(struct iphdr) -
				          sizeof(struct udphdr));
				break;
			}
			break;
		}
		break;
	case ETHERTYPE_ARP:
		arph = (struct arphdr *) (packet + sizeof(struct ethhdr));
		PRINT_HDR("ARP:\t\t", (uint8_t *) arph, sizeof(struct arphdr));
		break;
	}
}

#endif
