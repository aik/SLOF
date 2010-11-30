/******************************************************************************
 * Copyright (c) 2004, 2008 IBM Corporation
 * All rights reserved.
 * This program and the accompanying materials
 * are made available under the terms of the BSD License
 * which accompanies this distribution, and is available at
 * http://www.opensource.org/licenses/bsd-license.php
 *
 * Contributors:
 *     IBM Corporation - initial implementation
 *****************************************************************************/


#ifndef MY_ENDIAN_H
#define MY_ENDIAN_H

#include <stdint.h>

extern inline uint16_t bswap_16 (uint16_t x);
extern inline uint32_t bswap_32 (uint32_t x);
extern inline uint64_t  bswap_64 (uint64_t x);
#define CPU_BIG_ENDIAN

#ifndef CPU_BIG_ENDIAN
#define cpu_to_le64(x)  (x)
#define cpu_to_le32(x)  (x)
#define cpu_to_le16(x)  (x)
#define cpu_to_be16(x)  bswap_16(x)
#define cpu_to_be32(x)  bswap_32(x)
#define le64_to_cpu(x)  (x)
#define le32_to_cpu(x)  (x)
#define le16_to_cpu(x)  (x)
#define be32_to_cpu(x)  bswap_32(x)
#else
#define cpu_to_le64(x)  bswap_64(x)
#define cpu_to_le32(x)  bswap_32(x)
#define cpu_to_le16(x)  bswap_16(x)
#define cpu_to_be16(x)  (x)
#define cpu_to_be32(x)  (x)
#define le64_to_cpu(x)  bswap_64(x)
#define le32_to_cpu(x)  bswap_32(x)
#define le16_to_cpu(x)  bswap_16(x)
#define be32_to_cpu(x)  (x)
#endif

#endif
