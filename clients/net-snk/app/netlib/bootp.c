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

#include <types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netlib/netlib.h>
#include <time.h>

#define DEBUG 0

void
print_ip(char *ip)
{
	printf("%d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
}


/* IP header checksum calculation */
unsigned short
checksum(unsigned short *packet, int words)
{
	unsigned long checksum;
	for (checksum = 0; words > 0; words--)
		checksum += *packet++;
	checksum = (checksum >> 16) + (checksum & 0xffff);
	checksum += (checksum >> 16);
	return ~checksum;
}


static int
send_bootp(int boot_device, filename_ip_t * fn_ip)
{
	int i;
	unsigned int packetsize =
	    sizeof(struct iphdr) + sizeof(struct ethhdr) +
	    sizeof(struct udphdr) + sizeof(struct btphdr);
	unsigned char packet[packetsize];
	struct ethhdr *ethh;
	struct iphdr *iph;
	struct udphdr *udph;
	struct btphdr *btph;

	ethh = (struct ethhdr *) packet;
	iph = (struct iphdr *) ((void *) ethh + sizeof(struct ethhdr));
	udph = (struct udphdr *) ((void *) iph + sizeof(struct iphdr));
	btph = (struct btphdr *) ((void *) udph + sizeof(struct udphdr));

	memset(packet, 0, packetsize);

	memcpy(ethh->src_mac, fn_ip->own_mac, 6);
	memcpy(ethh->dest_mac, fn_ip->server_mac, 6);

	ethh->type = htons(ETHERTYPE_IP);
	iph->ip_hlv = 0x45;
	iph->ip_tos = 0;
	iph->ip_len = htons(packetsize - sizeof(struct ethhdr));
	iph->ip_id = htons(54321);
	iph->ip_off = 0;
	iph->ip_ttl = 255;
	iph->ip_p = IPTYPE_UDP;
	iph->ip_src = fn_ip->own_ip;
	iph->ip_dst = fn_ip->server_ip;
	iph->ip_sum =
	    checksum((unsigned short *) iph, sizeof(struct iphdr) >> 1);

	udph->uh_sport = htons(UDPPORT_BOOTPC);
	udph->uh_dport = htons(UDPPORT_BOOTPS);
	udph->uh_ulen = htons(sizeof(struct udphdr) + sizeof(struct btphdr));
	udph->uh_sum = 0;
	btph->op = 1;
	btph->htype = 1;
	btph->hlen = 6;
	strcpy((char *) btph->file, "bla");
	memcpy(btph->chaddr, ethh->src_mac, 6);

#if DEBUG
	printf("Sending packet\n");
	printf("Packet is ");
	for (i = 0; i < packetsize; i++)
		printf(" %02x", packet[i]);
	printf(".\n");
#endif

	i = send(boot_device, packet, packetsize, 0);
#if DEBUG
	printf("%d bytes transmitted over socket.\n", i);
#endif

	return 0;
}


static int
receive_bootp(int boot_device, filename_ip_t * fn_ip)
{
	int len, old_sum;
	unsigned int packetsize = 2000;
	unsigned char packet[packetsize];
	struct ethhdr *ethh;
	struct iphdr *iph;
	struct udphdr *udph;
	struct btphdr *btph;

	ethh = (struct ethhdr *) packet;
	iph = (struct iphdr *) (packet + sizeof(struct ethhdr));
	udph = (struct udphdr *) ((void *) iph + sizeof(struct iphdr));
	btph = (struct btphdr *) ((void *) udph + sizeof(struct udphdr));

	memset(packet, 0, packetsize);

	/* setting up a timer with a timeout of one second */
	set_timer(TICKS_SEC);

	do {

		/* let's receive a packet */
		len = recv(boot_device, packet, packetsize, 0);

#if DEBUG
		int j;
		printf("%d bytes received, %d expected \n", len, packetsize);
		if (len == 346) {
			printf("Rec packet\n");
			printf("Packet is ");
			for (j = 0; j < len; j++) {
				if (j % 16 == 0)
					printf("\n");
				printf(" %02x", packet[j]);
			}
			printf(".\n");
		}
#endif
		/* check if the ip checksum is correct */
		old_sum = iph->ip_sum;
		iph->ip_sum = 0x00;
		if (old_sum !=
		    checksum((unsigned short *) iph, sizeof(struct iphdr) >> 1))
			/* checksum failed */
			continue;
		/* is it a udp packet */
		if (iph->ip_p != IPTYPE_UDP)
			continue;
		/* check if the source port and destination port and the packet
		 * say that it is a bootp answer */
		if (udph->uh_dport != htons(UDPPORT_BOOTPC) || udph->uh_sport != htons(UDPPORT_BOOTPS))
			continue;
		/* check if it is a Boot Reply */
		if (btph->op != 2)
			continue;
		/* Comparing our mac address with the one in the bootp reply */	
		if (memcmp(fn_ip->own_mac, btph->chaddr, ETH_ALEN))
			continue;

		fn_ip->own_ip = btph->yiaddr;
		fn_ip->server_ip = btph->siaddr;
		memcpy(fn_ip->server_mac, &ethh->src_mac, 6);
		strcpy((char *) fn_ip->filename, (char *) btph->file);

#if DEBUG
		printf("\nThese are the details of the bootp reply:\n");
		printf("Our IP address: ");
		print_ip(&fn_ip->own_ip);
		printf("Next server IP address: ");
		print_ip(&fn_ip->server_ip);
		printf("Boot file name: %s\n", btph->file);
		printf("Packet is: %s\n", btph->file);
		for (j = 0; j < len; j++) {
			if (j % 16 == 0)
				printf("\n");
			printf(" %02x", packet[j]);
		}
		printf(".\n");
		printf("fn_ip->own_mac: %02x:%02x:%02x:%02x:%02x:%02x\n", 
                            fn_ip->own_mac[0], fn_ip->own_mac[1], fn_ip->own_mac[2], 
			    fn_ip->own_mac[3], fn_ip->own_mac[4], fn_ip->own_mac[5]);
		printf("Header ethh->dest_mac: %02x:%02x:%02x:%02x:%02x:%02x\n", 
                            ethh->dest_mac[0], ethh->dest_mac[1], ethh->dest_mac[2], 
			    ethh->dest_mac[3], ethh->dest_mac[4], ethh->dest_mac[5]);
		printf("Header ethh->src_mac: %02x:%02x:%02x:%02x:%02x:%02x\n", 
                            ethh->src_mac[0], ethh->src_mac[1], ethh->src_mac[2], 
			    ethh->src_mac[3], ethh->src_mac[4], ethh->src_mac[5]);
		printf("Header ethh->typ: %x\n",ethh->type); 
		printf("Header iph->ip_hlv: %x\n",iph->ip_hlv); 
		printf("Header iph->ip_len: %x\n",iph->ip_len); 
		printf("Header iph->ip_id: %x\n",iph->ip_id); 
		printf("Header iph->ip_off: %x\n",iph->ip_off); 
		printf("Header iph->ip_ttl: %x\n",iph->ip_ttl); 
		printf("Header iph->ip_p: %x\n",iph->ip_p); 
		printf("Header iph->ip_sum: %x\n",iph->ip_sum); 
		printf("Header iph->ip_src: %x\n",iph->ip_src); 
		printf("Header iph->ip_dst: %x\n",iph->ip_dst); 

		printf("Header btph->op: %x\n",btph->op); 
		printf("Header btph->htype: %x\n",btph->htype); 
		printf("Header btph->hlen: %x\n",btph->hlen); 
		printf("Header btph->hops: %x\n",btph->hops); 
		printf("Header btph->xid: %x\n",btph->xid); 
		printf("Header btph->secs: %x\n",btph->secs); 
		printf("Header btph->ciaddr: %x\n",btph->ciaddr); 
		printf("Header btph->yiaddr: %x\n",btph->yiaddr); 
		printf("Header btph->siaddr: %x\n",btph->siaddr); 
		printf("Header btph->giaddr: %x\n",btph->giaddr); 

 		printf("Header btph->chaddr: %02x:%02x:%02x:%02x:%02x:%02x:\n", 
                            btph->chaddr[0], btph->chaddr[1], btph->chaddr[2], 
			    btph->chaddr[3], btph->chaddr[4], btph->chaddr[5]);
		

#endif
		return 0;

		/* only do this for the time specified during set_timer() */
	} while (get_timer() > 0);
	return -1;
}


int
bootp(int boot_device, filename_ip_t * fn_ip, unsigned int retries)
{
	int i = (int) retries+1;
	fn_ip->own_ip = 0;

	printf("   ");

	do {
		printf("\b\b%02d", i);
		if (!i--) {
			printf("\nGiving up after %d bootp requests\n",
			       retries+1);
			return -1;
		}
		send_bootp(boot_device, fn_ip);
		/* if the timer in receive_bootp expired it will return
		 * -1 and we will just send another bootp request just
		 * in case the previous one was lost. And because we don't
		 * trust the network cable we keep on doing this 30 times */
	} while (receive_bootp(boot_device, fn_ip) != 0);
	printf("\b\b\b");
	return 0;
}
