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


#include "endian.h"

inline uint16_t bswap_16 (uint16_t x)
{
    return ((x&0xff00) >> 8) | ((x&0xff) << 8);
}
                                                                                
inline uint32_t bswap_32 (uint32_t x)
{
    return bswap_16((x&0xffff0000) >> 16) | (bswap_16(x&0xffff) << 16);
}
                                                                                
inline uint64_t  bswap_64 (uint64_t x)
{
    return (unsigned long long) bswap_32((x&0xffffffff00000000ULL) >> 32) |
    (unsigned long long) bswap_32(x&0xffffffffULL) << 32;
}
