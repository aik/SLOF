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

#ifndef _ICMP_H_
#define _ICMP_H_

#include <netlib/netlib.h>

/* RFC 792 - Internet Control Message Protocol

  http://www.faqs.org/rfcs/rfc792.html

Summary of Message Types
    0  Echo Reply
    3  Destination Unreachable
    4  Source Quench
    5  Redirect
    8  Echo
   11  Time Exceeded
   12  Parameter Problem
   13  Timestamp
   14  Timestamp Reply
   15  Information Request
   16  Information Reply
*/

#define PROTO_ICMP 1

#define ICMP_TYPE_DEST_UNREACHABLE 3

#define ICMP_NET_UNREACHABLE 0
#define ICMP_HOST_UNREACHABLE 1
#define ICMP_PROTOCOL_UNREACHABLE 2
#define ICMP_PORT_UNREACHABLE 3
#define ICMP_FRAGMENTATION_NEEDED 4
#define ICMP_SOURCE_ROUTE_FAILED 5

struct icmphdr {
	unsigned char type;
	unsigned char code;
	unsigned short int checksum;
	union {
		/* for type 3 "Destination Unreachable" */
		unsigned int unused;
		/* for type 0 and 8 */
		struct echo {
			unsigned short int id;
			unsigned short int seq;
		} echo;
	} options;
	union {
		/* payload for destination unreachable */
		struct dun {
			unsigned char iphdr[20];
			unsigned char data[64];
		} dun;
		/* payload for echo or echo reply */
		/* maximum size supported is 84 */
		unsigned char data[84];
	} payload;
};

int handle_icmp(struct icmphdr *);
int echo_request(int, filename_ip_t *, unsigned int);

#endif /* _ICMP_H_ */
