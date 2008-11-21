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

#include <netlib/icmp.h>
#include <netlib/arp.h>
#include <netlib/netbase.h>
#include <sys/socket.h>
#include <string.h>
#include <time.h>

/* from bootp.c */
unsigned short checksum(unsigned short *, int);

int
handle_icmp(struct icmphdr *icmp)
{
	/* we currently only handle type 3 (detination unreachable)
	 * ICMP messages */
	if (icmp->type != 3)
		return 0;
	return -icmp->code - 10;
}

static void
fill_icmp_echo_data(unsigned char *data, int length)
{
	char *filler = "SLOF";
	if (length <= 0)
		return;
	do {
		memcpy(data, filler,
		       (length >= strlen(filler)) ? strlen(filler) : length);
		data += strlen(filler);
	} while ((length -= strlen(filler)) > 0);
}

int
echo_request(int device, filename_ip_t * fn_ip, unsigned int timeout)
{
	unsigned int packetsize =
	    sizeof(struct ethhdr) + sizeof(struct iphdr) +
	    sizeof(struct icmphdr);
	unsigned char packet[packetsize * 2];
	struct icmphdr *icmp;
	int i;
	struct iphdr *ip;
	struct ethhdr *ethh;
	memset(packet, 0, packetsize);
	fill_ethhdr(packet, htons(ETHERTYPE_IP), fn_ip->own_mac,
		    fn_ip->server_mac);
	ethh = (struct ethhdr *) packet;
	fill_iphdr(packet + sizeof(struct ethhdr),
		   sizeof(struct iphdr) + sizeof(struct icmphdr), IPTYPE_ICMP,
		   fn_ip->own_ip, fn_ip->server_ip);
	icmp =
	    (struct icmphdr *) ((void *) (packet + sizeof(struct ethhdr)) +
				sizeof(struct iphdr));
	ip = (struct iphdr *) (packet + sizeof(struct ethhdr));
	icmp->type = 8;
	icmp->code = 0;
	icmp->checksum = 0;
	icmp->options.echo.id = 0xd476;
	icmp->options.echo.seq = 1;
	fill_icmp_echo_data(icmp->payload.data, sizeof(icmp->payload.data));
	icmp->checksum =
	    checksum((unsigned short *) icmp, sizeof(struct icmphdr) >> 1);
	send(device, packet, packetsize, 0);
	set_timer(TICKS_SEC / 10 * timeout);
	do {
		memset(packet, 0, packetsize);
		i = recv(device, packet, packetsize * 2, 0);
		if (i == 0)
			continue;
		if (ethh->type == htons(ETHERTYPE_ARP)) {
			handle_arp(packet + sizeof(struct ethhdr), i);
			continue;
		}
		if (ip->ip_p != PROTO_ICMP)
			continue;
		if (icmp->type != 0)
			continue;
		if (icmp->options.echo.id != 0xd476)
			continue;
		if (icmp->options.echo.seq != 1)
			continue;
		return 0;
	} while (get_timer() > 0);
	return -1;
}
