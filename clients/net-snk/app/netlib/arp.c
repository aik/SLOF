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

/*>>>>>>>>>>>>>>>>>>>>> DEFINITIONS & DECLARATIONS <<<<<<<<<<<<<<<<<<<<<<*/

#include <types.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netlib/netlib.h>
#include <netlib/netbase.h>
#include <netlib/arp.h>
#include <time.h>

/* ARP Message types */
#define ARP_REQUEST            1
#define ARP_REPLY              2


/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>> PROTOTYPES <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

static void
arp_send_request(uint32_t dest_ip);

static void
arp_send_reply(uint32_t src_ip, uint8_t * src_mac);

static void
fill_arphdr(uint8_t * packet, uint8_t opcode,
            uint8_t * src_mac, uint32_t src_ip,
            uint8_t * dest_mac, uint32_t dest_ip);


/*>>>>>>>>>>>>>>>>>>>>>>>>>>>> LOCAL VARIABLES <<<<<<<<<<<<<<<<<<<<<<<<<<*/

static uint8_t  ether_packet[ETH_MTU_SIZE];
static int32_t  arp_device_socket = 0;
static uint8_t  arp_own_mac[]     = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint32_t arp_own_ip        = 0;
static uint8_t  arp_result_mac[]  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


/*>>>>>>>>>>>>>>>>>>>>>>>>>>>> IMPLEMENTATION <<<<<<<<<<<<<<<<<<<<<<<<<<<*/

/**
 * ARP: Initialize the environment for ARP client.
 *      To perform ARP-requests use the function arp_getmac.
 *
 * @param  device_socket a socket number used to send and recieve packets
 * @param  own_mac       client hardware-address (MAC)
 * @param  own_ip        client IPv4 address (e.g. 127.0.0.1)
 * @see                  arp_getmac
 */
void
arp_init(int32_t device_socket, uint8_t own_mac[], uint32_t own_ip) {
	arp_device_socket = device_socket;
	memcpy(arp_own_mac, own_mac, 6);
	arp_own_ip = own_ip;
}

/**
 * ARP: For given IPv4 retrieves MAC via ARP (makes several attempts)
 *
 * @param  dest_ip   IP for the ARP-request
 * @param  dest_mac  In case of SUCCESS stores MAC
 *                   In case of FAULT stores zeros (0.0.0.0.0.0).
 * @return           TRUE - MAC successfuly retrieved;
 *                   FALSE - error condition occurs.
 */
int8_t
arp_getmac(uint32_t dest_ip, uint8_t dest_mac[]) {
	// this counter is used so that we abort after 30 ARP request
	// int32_t i;

	// clearing the result buffer to detect an ARP answer
	memset(arp_result_mac, 0, 6);

//	for(i = 0; i<30; ++i) {
		arp_send_request(dest_ip);

		// setting up a timer with a timeout of one second
		set_timer(TICKS_SEC);

		do {
			receive_ether();
			if (arp_result_mac[0] != 0 || arp_result_mac[1] != 0 ||
			    arp_result_mac[2] != 0 || arp_result_mac[3] != 0 ||
			    arp_result_mac[4] != 0 || arp_result_mac[5] != 0) {
				memcpy(dest_mac, arp_result_mac, 6);
				return 1; // no error
			}
		} while (get_timer() > 0);

		// time is up 
//	}

//	printf("\nGiving up after %d ARP requests\n", i);
	return 0; // error
}

/**
 * ARP: Sends an ARP-request package.
 *      For given IPv4 retrieves MAC via ARP (makes several attempts)
 *
 * @param  dest_ip   IP of the host which MAC should be obtained
 */
static void
arp_send_request(uint32_t dest_ip) {
	uint8_t   dest_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

	memset(ether_packet, 0, sizeof(struct ethhdr) + sizeof(struct arphdr));

	fill_arphdr(&ether_packet[sizeof(struct ethhdr)], ARP_REQUEST,
	            arp_own_mac, arp_own_ip, dest_mac, dest_ip);
	fill_ethhdr(&ether_packet[0], ETHERTYPE_ARP, arp_own_mac, dest_mac);

	PRINT_SENDING(ether_packet,
	              sizeof(struct ethhdr) + sizeof(struct arphdr));

	send(arp_device_socket, ether_packet,
	     sizeof(struct ethhdr) + sizeof(struct arphdr), 0);
}

/**
 * ARP: Sends an ARP-reply package.
 *      This package is used to serve foreign requests (in case IP in
 *      foreign request matches our host IP).
 *
 * @param  src_ip    requester IP address (foreign IP)
 * @param  src_mac   requester MAC address (foreign MAC)
 */
static void
arp_send_reply(uint32_t src_ip, uint8_t * src_mac) {
	memset(ether_packet, 0, sizeof(struct ethhdr) + sizeof(struct arphdr));

	fill_ethhdr(&ether_packet[0], ETHERTYPE_ARP, arp_own_mac, src_mac);
	fill_arphdr(&ether_packet[sizeof(struct ethhdr)], ARP_REPLY,
	            arp_own_mac, arp_own_ip, src_mac, src_ip);

	PRINT_SENDING(ether_packet,
	              sizeof(struct ethhdr) + sizeof(struct arphdr));

	send(arp_device_socket, ether_packet,
	     sizeof(struct ethhdr) + sizeof(struct arphdr), 0);
};

/**
 * ARP: Creates ARP package. Places ARP-header in a packet and fills it
 *      with corresponding information.
 *      <p>
 *      Use this function with similar functions for other network layers
 *      (fill_ethhdr).
 *
 * @param  packet      Points to the place where ARP-header must be placed.
 * @param  opcode      Identifies is it request (ARP_REQUEST)
 *                     or reply (ARP_REPLY) package.
 * @param  src_mac     sender MAC address
 * @param  src_ip      sender IP address
 * @param  dest_mac    receiver MAC address
 * @param  dest_ip     receiver IP address
 * @see                arphdr
 * @see                fill_ethhdr
 */
static void
fill_arphdr(uint8_t * packet, uint8_t opcode,
	    uint8_t * src_mac, uint32_t src_ip,
	    uint8_t * dest_mac, uint32_t dest_ip) {
	struct arphdr * arph = (struct arphdr *) packet;

	arph -> hw_type = htons(1);
	arph -> proto_type = htons(ETHERTYPE_IP);
	arph -> hw_len = 6;
	arph -> proto_len = 4;
	arph -> opcode = htons(opcode);

	memcpy(arph->src_mac, src_mac, 6);
	arph->src_ip = htonl(src_ip);
	memcpy(arph->dest_mac, dest_mac, 6);
	arph->dest_ip = htonl(dest_ip);
}

/**
 * ARP: Handles ARP-messages according to Receive-handle diagram.
 *      Sets arp_result_mac for given dest_ip (see arp_getmac).
 *
 * @param  packet     ARP-packet to be handled
 * @param  packetsize length of the packet
 * @return            ZERO - packet handled successfully;
 *                    NON ZERO - packet was not handled (e.g. bad format)
 * @see               arp_getmac
 * @see               receive_ether
 * @see               arphdr
 */
int8_t
handle_arp(uint8_t * packet, int32_t packetsize) {
	struct arphdr * arph = (struct arphdr *) packet;

	if (packetsize < sizeof(struct arphdr))
		return -1; // Packet is too small

	if (arph -> hw_type != htons(1) || arph -> proto_type != htons(ETHERTYPE_IP))
		return -1; // Unknown hardware or unsupported protocol

	if (arph -> dest_ip != htonl(arp_own_ip))
		return -1; // receiver IP doesn't match our IP

	switch(htons(arph -> opcode)) {
	case ARP_REQUEST:
		// foreign request
		if(arp_own_ip != 0)
			arp_send_reply(htonl(arph->src_ip), arph -> src_mac);
		return 0; // no error
	case ARP_REPLY:
		// if it is for us -> fill server MAC
		if (!memcmp(arp_own_mac, arph -> dest_mac, 6))
			memcpy(arp_result_mac, arph -> src_mac, 6);
		return 0; // no error
	default:
		break;
	}
	return -1; // Invalid message type
}
