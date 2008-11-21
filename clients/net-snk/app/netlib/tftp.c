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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>

#include <netlib/netlib.h>
#include <netlib/icmp.h>

//#define __DEBUG__

#define BUFFER_LEN 2048
#define ACK_BUFFER_LEN 256
#define READ_BUFFER_LEN 256

#define ENOTFOUND 1
#define EACCESS   2
#define EBADOP    4
#define EBADID    5
#define ENOUSER   7
//#define EUNDEF 0
//#define ENOSPACE 3
//#define EEXISTS 6

#define RRQ   1
#define WRQ   2
#define DATA  3
#define ACK   4
#define ERROR 5
#define OACK  6

/**
 * dump_package - Prints a package.
 *
 * @package: package which is to print
 * @len:     length of the package
 */
#ifdef __DEBUG__

static void
dump_package(unsigned char *buffer, unsigned int len)
{
	int i;

	for (i = 1; i <= len; i++) {
		printf("%02x%02x ", buffer[i - 1], buffer[i]);
		i++;
		if ((i % 16) == 0)
			printf("\n");
	}
	printf("\n");
}
#endif

/* UDP header checksum calculation */

static unsigned short
checksum(unsigned short *packet, int words, unsigned short *pseudo_ip)
{
	int i;
	unsigned long checksum;
	for (checksum = 0; words > 0; words--)
		checksum += *packet++;
	if (pseudo_ip) {
		for (i = 0; i < 6; i++)
			checksum += *pseudo_ip++;
	}
	checksum = (checksum >> 16) + (checksum & 0xffff);
	checksum += (checksum >> 16);
	return ~checksum;
}


/**
 * send_rrq - Sends a read request package.
 *
 * @client:   client IPv4 address (e.g. 127.0.0.1)
 * @server:   server IPv4 address (e.g. 127.0.0.1)
 * @filename: name of the file which should be downloaded 
 */
static void
send_rrq(int boot_device, filename_ip_t * fn_ip)
{
	int i;
	unsigned char mode[] = "octet";
	unsigned char packet[READ_BUFFER_LEN];
	char *ptr;
	struct ethhdr *ethh;
	struct iphdr *ip;
	struct udphdr *udph;
	struct tftphdr *tftp;
	struct pseudo_iphdr piph = { 0 };

	memset(packet, 0, READ_BUFFER_LEN);

	ethh = (struct ethhdr *) packet;

	memcpy(ethh->src_mac, fn_ip->own_mac, 6);
	memcpy(ethh->dest_mac, fn_ip->server_mac, 6);
	ethh->type = htons(ETHERTYPE_IP);

	ip = (struct iphdr *) ((unsigned char *) ethh + sizeof(struct ethhdr));
	ip->ip_hlv = 0x45;
	ip->ip_tos = 0x00;
	ip->ip_len = sizeof(struct iphdr) + sizeof(struct udphdr)
	    + strlen((char *) fn_ip->filename) + strlen((char *) mode) + 4
	    + strlen("blksize") + strlen("1432") + 2;
	ip->ip_id = 0x0;
	ip->ip_off = 0x0000;
	ip->ip_ttl = 60;
	ip->ip_p = 17;
	ip->ip_src = fn_ip->own_ip;
	ip->ip_dst = fn_ip->server_ip;
	ip->ip_sum = 0;

	udph = (struct udphdr *) (ip + 1);
	udph->uh_sport = htons(2001);
	udph->uh_dport = htons(69);
	udph->uh_ulen = htons(sizeof(struct udphdr)
			      + strlen((char *) fn_ip->filename) + strlen((char *) mode) + 4
			      + strlen("blksize") + strlen("1432") + 2);

	tftp = (struct tftphdr *) (udph + 1);
	tftp->th_opcode = htons(RRQ);

	ptr = (char *) &tftp->th_data;
	memcpy(ptr, fn_ip->filename, strlen((char *) fn_ip->filename) + 1);

	ptr += strlen((char *) fn_ip->filename) + 1;
	memcpy(ptr, mode, strlen((char *) mode) + 1);

	ptr += strlen((char *) mode) + 1;
	memcpy(ptr, "blksize", strlen("blksize") + 1);

	ptr += strlen("blksize") + 1;
	memcpy(ptr, "1432", strlen("1432") + 1);

	piph.ip_src = ip->ip_src;
	piph.ip_dst = ip->ip_dst;
	piph.ip_p = ip->ip_p;
	piph.ip_ulen = udph->uh_ulen;

	udph->uh_sum = 0;
	udph->uh_sum =
	    checksum((unsigned short *) udph, udph->uh_ulen >> 1,
		     (unsigned short *) &piph);

	ip->ip_sum =
	    checksum((unsigned short *) ip, sizeof(struct iphdr) >> 1, 0);
	i = send(boot_device, packet, ip->ip_len + sizeof(struct ethhdr), 0);
#ifdef __DEBUG__
	printf("tftp RRQ %d bytes transmitted over socket.\n", i);
#endif
	return;
}

/**
 * send_ack - Sends a acknowlege package.
 *
 * @boot_device:
 * @fn_ip:
 * @blckno: block number
 */
static void
send_ack(int boot_device, filename_ip_t * fn_ip,
	 int blckno, unsigned short dport)
{
	int i;
	unsigned char packet[ACK_BUFFER_LEN];
	struct ethhdr *ethh;
	struct iphdr *ip;
	struct udphdr *udph;
	struct tftphdr *tftp;
	struct pseudo_iphdr piph = { 0 };

	memset(packet, 0, ACK_BUFFER_LEN);

	ethh = (struct ethhdr *) packet;
	memcpy(ethh->src_mac, fn_ip->own_mac, 6);
	memcpy(ethh->dest_mac, fn_ip->server_mac, 6);
	ethh->type = htons(ETHERTYPE_IP);

	ip = (struct iphdr *) ((unsigned char *) ethh + sizeof(struct ethhdr));
	ip->ip_hlv = 0x45;
	ip->ip_tos = 0x00;
	ip->ip_len = sizeof(struct iphdr) + sizeof(struct udphdr) + 4;
	ip->ip_id = 0;
	ip->ip_off = 0x0000;
	ip->ip_ttl = 60;
	ip->ip_p = 17;
	ip->ip_src = fn_ip->own_ip;
	ip->ip_dst = fn_ip->server_ip;

	ip->ip_sum = 0;

	udph = (struct udphdr *) (ip + 1);
	udph->uh_sport = htons(2001);
	udph->uh_dport = htons(dport);
	udph->uh_ulen = htons(sizeof(struct udphdr) + 4);

	tftp = (struct tftphdr *) (udph + 1);
	tftp->th_opcode = htons(ACK);
	tftp->th_data = htons(blckno);

	piph.ip_src = ip->ip_src;
	piph.ip_dst = ip->ip_dst;
	piph.ip_p = ip->ip_p;
	piph.ip_ulen = udph->uh_ulen;

	udph->uh_sum = 0;
	udph->uh_sum =
	    checksum((unsigned short *) udph, udph->uh_ulen >> 1,
		     (unsigned short *) &piph);

	ip->ip_sum =
	    checksum((unsigned short *) ip, sizeof(struct iphdr) >> 1, 0);

	i = send(boot_device, packet, ip->ip_len + sizeof(struct ethhdr), 0);

#ifdef __DEBUG__
	printf("tftp ACK %d bytes transmitted over socket.\n", i);
#endif

	return;
}

/**
 * send_error - Sends an error package.
 *
 * @boot_device: socket handle
 * @fn_ip:       some OSI CEP-IDs
 * @error_code:  Used sub code for error packet
 * @dport:       UDP destination port
 */
static void
send_error(int boot_device, filename_ip_t * fn_ip,
	 int error_code, unsigned short dport)
{
	int i;
	unsigned char packet[256];
	struct ethhdr *ethh;
	struct iphdr *ip;
	struct udphdr *udph;
	struct tftphdr *tftp;
	struct pseudo_iphdr piph = { 0 };

	memset(packet, 0, 256);

	ethh = (struct ethhdr *) packet;
	memcpy(ethh->src_mac, fn_ip->own_mac, 6);
	memcpy(ethh->dest_mac, fn_ip->server_mac, 6);
	ethh->type = htons(ETHERTYPE_IP);

	ip = (struct iphdr *) ((unsigned char *) ethh + sizeof(struct ethhdr));
	ip->ip_hlv = 0x45;
	ip->ip_tos = 0x00;
	ip->ip_len = sizeof(struct iphdr) + sizeof(struct udphdr) + 5;
	ip->ip_id = 0;
	ip->ip_off = 0x0000;
	ip->ip_ttl = 60;
	ip->ip_p = 17;
	ip->ip_src = fn_ip->own_ip;
	ip->ip_dst = fn_ip->server_ip;

	ip->ip_sum = 0;

	udph = (struct udphdr *) (ip + 1);
	udph->uh_sport = htons(2001);
	udph->uh_dport = htons(dport);
	udph->uh_ulen = htons(sizeof(struct udphdr) + 5);

	tftp = (struct tftphdr *) (udph + 1);
	tftp->th_opcode = htons(ERROR);
	tftp->th_data = htons(error_code);
	((char *) &tftp->th_data)[2] = 0;

	piph.ip_src = ip->ip_src;
	piph.ip_dst = ip->ip_dst;
	piph.ip_p = ip->ip_p;
	piph.ip_ulen = udph->uh_ulen;

	udph->uh_sum = 0;
	udph->uh_sum =
	    checksum((unsigned short *) udph, udph->uh_ulen >> 1,
		     (unsigned short *) &piph);

	ip->ip_sum =
	    checksum((unsigned short *) ip, sizeof(struct iphdr) >> 1, 0);

	i = send(boot_device, packet, ip->ip_len + sizeof(struct ethhdr), 0);

#ifdef __DEBUG__
	printf("tftp ERROR %d bytes transmitted over socket.\n", i);
#endif

	return;
}


static int
send_arp_reply(int boot_device, filename_ip_t * fn_ip)
{
	int i;
	unsigned int packetsize = sizeof(struct ethhdr) + sizeof(struct arphdr);
	unsigned char packet[packetsize];
	struct ethhdr *ethh;
	struct arphdr *arph;

	ethh = (struct ethhdr *) packet;
	arph = (struct arphdr *) ((void *) ethh + sizeof(struct ethhdr));

	memset(packet, 0, packetsize);

	memcpy(ethh->src_mac, fn_ip->own_mac, 6);
	memcpy(ethh->dest_mac, fn_ip->server_mac, 6);
	ethh->type = htons(ETHERTYPE_ARP);

	arph->hw_type = 1;
	arph->proto_type = 0x800;
	arph->hw_len = 6;
	arph->proto_len = 4;

	memcpy(arph->src_mac, fn_ip->own_mac, 6);
	arph->src_ip = fn_ip->own_ip;

	arph->dest_ip = fn_ip->server_ip;

	arph->opcode = 2;
#ifdef __DEBUG__
	printf("send arp reply\n");
#endif
#if 0
	printf("Sending packet\n");
	printf("Packet is ");
	for (i = 0; i < packetsize; i++)
		printf(" %2.2x", packet[i]);
	printf(".\n");
#endif

	i = send(boot_device, packet, packetsize, 0);
	return i;
}

static void
print_progress(int urgent, int received_bytes)
{
	static unsigned int i = 1;
	static int first = -1;
	static int last_bytes = 0;
	char buffer[100];
	char *ptr;

	// 1MB steps or 0x400 times or urgent 
	if(((received_bytes - last_bytes) >> 20) > 0
	|| (i & 0x3FF) == 0 || urgent) {
		if(!first) {
			sprintf(buffer, "%d KBytes", (last_bytes >> 10));
			for(ptr = buffer; *ptr != 0; ++ptr)
				*ptr = '\b';
			printf(buffer);
		}
		printf("%d KBytes", (received_bytes >> 10));
		i = 1;
		first = 0;
		last_bytes = received_bytes;
	}
	++i;
}

/**
 * get_blksize tries to extract the blksize from the OACK package
 * the TFTP returned. From RFC 1782
 * The OACK packet has the following format:
 *
 *   +-------+---~~---+---+---~~---+---+---~~---+---+---~~---+---+
 *   |  opc  |  opt1  | 0 | value1 | 0 |  optN  | 0 | valueN | 0 |
 *   +-------+---~~---+---+---~~---+---+---~~---+---+---~~---+---+
 *
 * @param buffer  the network packet
 * @param len  the length of the network packet
 * @return  the blocksize the server supports or 0 for error
 */
static int
get_blksize(unsigned char *buffer, unsigned int len)
{
	unsigned char *orig = buffer;
	/* skip all headers until tftp has been reached */
	buffer += sizeof(struct ethhdr);
	buffer += sizeof(struct iphdr);
	buffer += sizeof(struct udphdr);
	/* skip opc */
	buffer += 2;
	while (buffer < orig + len) {
		if (!memcmp(buffer, "blksize", strlen("blksize") + 1))
			return (unsigned short) strtoul((char *) (buffer +
							strlen("blksize") + 1),
							(char **) NULL, 10);
		else {
			/* skip the option name */
			buffer = (unsigned char *) strchr((char *) buffer, 0);
			if (!buffer)
				return 0;
			buffer++;
			/* skip the option value */
			buffer = (unsigned char *) strchr((char *) buffer, 0);
			if (!buffer)
				return 0;
			buffer++;
		}
	}
	return 0;
}

/**
 * this prints out some status characters
 * \|-/ for each packet received
 * A for an arp packet
 * I for an ICMP packet
 * #+* for different unexpected TFTP packets (not very good)
 */
static int
the_real_tftp(int boot_device, filename_ip_t * fn_ip, unsigned char *buffer,
	      int len, unsigned int retries, tftp_err_t * tftp_err)
{
	int i, j = 0;
	int received_len = 0;
	struct ethhdr *ethh;
	struct arphdr *arph;

	struct iphdr *ip;
	struct udphdr *udph;
	struct tftphdr *tftp;
	struct icmphdr *icmp;

	unsigned char packet[BUFFER_LEN];
	short port_number = -1;
	unsigned short block = 0;
	unsigned short blocksize = 512;
	int lost_packets = 0;

	tftp_err->bad_tftp_packets = 0;
	tftp_err->no_packets = 0;

	send_rrq(boot_device, fn_ip);

	printf("  Receiving data:  ");
	print_progress(-1, 0);

	set_timer(TICKS_SEC);
	while (j++ < 0x100000) {
		/* bad_tftp_packets are counted whenever we receive a TFTP packet
		 * which was not expected; if this gets larger than 'retries'
		 * we just exit */
		if (tftp_err->bad_tftp_packets > retries) {
			return -40;
		}
		/* no_packets counts the times we have returned from recv() without
		 * any packet received; if this gets larger than 'retries'
		 * we also just exit */
		if (tftp_err->no_packets > retries) {
			return -41;
		}
		/* don't wait longer than 0.5 seconds for packet to be recevied */
		do {
			i = recv(boot_device, packet, BUFFER_LEN, 0);
			if (i != 0)
				break;
		} while (get_timer() > 0);

		/* no packet received; no processing */
		if (i == 0) {
			/* the server doesn't seem to retry let's help out a bit */
			if (tftp_err->no_packets > 4 && port_number != -1
			    && block > 1)
				send_ack(boot_device, fn_ip, block,
					 port_number);
			tftp_err->no_packets++;
			set_timer(TICKS_SEC);
			continue;
		}
#ifndef __DEBUG__
		print_progress(0, received_len);
#endif
		ethh = (struct ethhdr *) packet;
		arph =
		    (struct arphdr *) ((void *) ethh + sizeof(struct ethhdr));
		ip = (struct iphdr *) (packet + sizeof(struct ethhdr));
		udph = (struct udphdr *) ((void *) ip + sizeof(struct iphdr));
		tftp =
		    (struct tftphdr *) ((void *) udph + sizeof(struct udphdr));
		icmp = (struct icmphdr *) ((void *) ip + sizeof(struct iphdr));

		if (memcmp(ethh->dest_mac, fn_ip->own_mac, 6) == 0) {
			set_timer(TICKS_SEC);
			tftp_err->no_packets = 0;
		}

		if (ethh->type == htons(ETHERTYPE_ARP) &&
		    arph->hw_type == 1 &&
		    arph->proto_type == 0x800 && arph->opcode == 1) {
			/* let's see if the arp request asks for our IP address
			 * else we will not answer */
			if (fn_ip->own_ip == arph->dest_ip) {
#ifdef __DEBUG__
				printf("\bA ");
#endif
				send_arp_reply(boot_device, fn_ip);
			}
			continue;
		}

		/* check if packet is an ICMP packet */
		if (ip->ip_p == PROTO_ICMP) {
#ifdef __DEBUG__
			printf("\bI ");
#endif
			i = handle_icmp(icmp);
			if (i)
				return i;
		}

		/* only IPv4 UDP packets we want */
		if (ip->ip_hlv != 0x45 || ip->ip_p != 0x11) {
#ifdef __DEBUG__
			printf("Unknown packet %x %x %x %x %x \n", ethh->type,
			       ip->ip_hlv, ip->ip_p, ip->ip_dst, fn_ip->own_ip);
#endif
			continue;
		}

		/* we only want packets for our own IP and broadcast UDP packets
		 * there will be probably never be a broadcast UDP TFTP packet
		 * but the RFC talks about it (crazy RFC) */
		if (!(ip->ip_dst == fn_ip->own_ip || ip->ip_dst == 0xFFFFFFFF))
			continue;
#ifdef __DEBUG__
		dump_package(packet, i);
#endif

		port_number = udph->uh_sport;
		if (tftp->th_opcode == htons(OACK)) {
			/* an OACK means that the server answers our blocksize request */
			blocksize = get_blksize(packet, i);
			if (!blocksize || blocksize > 1432) {
				send_error(boot_device, fn_ip, 8, port_number);
				return -8;
			}
			send_ack(boot_device, fn_ip, 0, port_number);
		} else if (tftp->th_opcode == htons(ACK)) {
			/* an ACK means that the server did not answers
			 * our blocksize request, therefore we will set the blocksize
			 * to the default value of 512 */
			blocksize = 512;
			send_ack(boot_device, fn_ip, 0, port_number);
		} else if ((unsigned char) tftp->th_opcode == ERROR) {
#ifdef __DEBUG__
			printf("tftp->th_opcode : %x\n", tftp->th_opcode);
			printf("tftp->th_data   : %x\n", tftp->th_data);
#endif
			if ((unsigned char) tftp->th_data == ENOTFOUND)	/* 1 */
				return -3;	// ERROR: file not found
			else if ((unsigned char) tftp->th_data == EACCESS)	/* 2 */
				return -4;	// ERROR: access violation
			else if ((unsigned char) tftp->th_data == EBADOP)	/* 4 */
				return -5;	// ERROR: illegal TFTP operation
			else if ((unsigned char) tftp->th_data == EBADID)	/* 5 */
				return -6;	// ERROR: unknown transfer ID
			else if ((unsigned char) tftp->th_data == ENOUSER)	/* 7 */
				return -7;	// ERROR: no such user
			return -1;	// ERROR: unknown error
		} else if (tftp->th_opcode == DATA) {
			/* DATA PACKAGE */
			if (tftp->th_data == block + 1)
				block++;
			else if (tftp->th_data == block) {
#ifdef __DEBUG__
				printf
				    ("\nTFTP: Received block %x, expected block was %x\n",
				     tftp->th_data, block + 1);
				printf("\b+ ");
#endif
				send_ack(boot_device, fn_ip, tftp->th_data,
					 port_number);
				lost_packets++;
				tftp_err->bad_tftp_packets++;
				continue;
			} else if (tftp->th_data < block) {
#ifdef __DEBUG__
				printf
				    ("\nTFTP: Received block %x, expected block was %x\n",
				     tftp->th_data, block + 1);
				printf("\b* ");
#endif
				/* This means that an old data packet appears (again);
				 * this happens sometimes if we don't answer fast enough
				 * and a timeout is generated on the server side;
				 * as we already have this packet we just ignore it */
				tftp_err->bad_tftp_packets++;
				continue;
			} else {
				tftp_err->blocks_missed = block + 1;
				tftp_err->blocks_received = tftp->th_data;
				return -42;
			}

			tftp_err->bad_tftp_packets = 0;
			/* check if our buffer is large enough */
			if (received_len + udph->uh_ulen - 12 > len)
				return -2;
			memcpy(buffer + received_len, &tftp->th_data + 1,
			       udph->uh_ulen - 12);
			send_ack(boot_device, fn_ip, tftp->th_data,
				 port_number);
			received_len += udph->uh_ulen - 12;
			/* Last packet reached if the payload of the UDP packet
			 * is smaller than blocksize + 12
			 * 12 = UDP header (8) + 4 bytes TFTP payload */
			if (udph->uh_ulen < blocksize + 12)
				break;
			/* 0xffff is the highest block number possible
			 * see the TFTP RFCs */
			if (block >= 0xffff)
				return -9;
		} else {
#ifdef __DEBUG__
			printf("Unknown packet %x\n", tftp->th_opcode);
			printf("\b# ");
#endif
			tftp_err->bad_tftp_packets++;
			continue;
		}
	}
	print_progress(-1, received_len);
	printf("\n");
	if (lost_packets)
		printf("Lost ACK packets: %d\n", lost_packets);
	return received_len;
}

int
tftp(int boot_device, filename_ip_t * fn_ip, unsigned char *buffer, int len,
     unsigned int retries, tftp_err_t * tftp_err)
{
	return the_real_tftp(boot_device, fn_ip, buffer, len, retries,
			     tftp_err);
}
