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


#ifndef _SOCKET_H
#define _SOCKET_H
extern int socket(int dom, int type, int proto, char *mac_addr);
extern int sendto(int fd, const void* buffer, int len,
		  int flags, const void* sock_addr, int sock_addr_len);
extern int send(int fd, void* buffer, int len, int flags);
extern int recv(int fd, void* buffer, int len, int flags);

#define htonl(x) x
#define htons(x) x

#endif
