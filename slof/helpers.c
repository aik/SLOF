/******************************************************************************
 * Copyright (c) 2007, 2012, 2013 IBM Corporation
 * All rights reserved.
 * This program and the accompanying materials
 * are made available under the terms of the BSD License
 * which accompanies this distribution, and is available at
 * http://www.opensource.org/licenses/bsd-license.php
 *
 * Contributors:
 *     IBM Corporation - initial implementation
 *****************************************************************************/
/*
 * All functions concerning interface to slof
 */

#include <stdio.h>
#include <string.h>
#include "helpers.h"
#include "paflof.h"

/**
 * get msec-timer value
 * access to HW register
 * overrun will occur if boot exceeds 1.193 hours (49 days)
 *
 * @param   -
 * @return  actual timer value in ms as 32bit
 */
uint32_t SLOF_GetTimer(void)
{
	forth_eval("get-msecs");
	return (uint32_t) forth_pop();
}

void SLOF_msleep(uint32_t time)
{
	time = SLOF_GetTimer() + time;
	while (time > SLOF_GetTimer())
		cpu_relax();
}

void *SLOF_dma_alloc(long size)
{
	forth_push(size);
	forth_eval("dma-alloc");
	return (void *)forth_pop();
}

void SLOF_dma_free(void *virt, long size)
{
	forth_push((long)virt);
	forth_push(size);
	forth_eval("dma-free");
}

long SLOF_dma_map_in(void *virt, long size, int cacheable)
{
	forth_push((long)virt);
	forth_push(size);
	forth_push(cacheable);
	forth_eval("dma-map-in");
	return forth_pop();
}

void SLOF_dma_map_out(long phys, void *virt, long size)
{
	forth_push((long)virt);
	forth_push((long)phys);
	forth_push(size);
	forth_eval("dma-map-out");
}
