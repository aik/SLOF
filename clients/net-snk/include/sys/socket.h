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

#include "systemcall.h"

int socket(int, int, int, char *);
int sendto(int, const void *, int, int, const void *, int);
int send(int, void *, int, int);
int recv(int, void *, int, int);

#define htonl(x) x
#define htons(x) x

#endif

