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

#include <of.h>
#include <rtas.h>
#include <pci.h>
#include <kernel.h>
#include <types.h>
#include <string.h>
#include <cpu.h>

/* Application entry point .*/
extern int _start(unsigned char *arg_string, long len);

unsigned long exception_stack_frame;

ihandle_t fd_array[32];
pci_config_t pci_device_array[32];

extern uint64_t tb_freq;

void term_net_driver(void);

static int
init_io()
{
	phandle_t chosen = of_finddevice("/chosen");

	if (chosen == -1)
		return -1;

	of_getprop(chosen, "stdin", &fd_array[0], sizeof(ihandle_t));
	of_getprop(chosen, "stdout", &fd_array[1], sizeof(ihandle_t));

	if (of_write(fd_array[1], " ", 1) < 0)
		return -2;

	return 0;
}

static char save_vector[0x4000];
extern char _lowmem_start;
extern char _lowmem_end;
extern char __client_start;
extern char __client_end;

static void
copy_exception_vectors()
{
	char *dest;
	char *src;
	int len;

	dest = save_vector;
	src = (char *) 0x200;
	len = &_lowmem_end - &_lowmem_start;
	memcpy(dest, src, len);

	dest = (char *) 0x200;
	src = &_lowmem_start;
	memcpy(dest, src, len);
	flush_cache(dest, len);
}

static void
restore_exception_vectors()
{
	char *dest;
	char *src;
	int len;

	dest = (char *) 0x200;
	src = save_vector;
	len = &_lowmem_end - &_lowmem_start;
	memcpy(dest, src, len);
	flush_cache(dest, len);
}

static long memory_size;

static void
checkmemorysize()
{
	char buf[255];
	phandle_t ph;
	memory_size = 0;

	struct reg {
		long adr;
		long size;
	} reg;

	ph = of_peer(0);	// begin from root-node
	ph = of_child(ph);	// get a child

	while (ph != 0) {
		if (of_getprop(ph, "device_type", buf, 255) != -1) {
			/* if device_type == memory */
			if (strcmp(buf, "memory") == 0)
				if (of_getprop(ph, "reg", &reg, 16) != -1) {
					memory_size += reg.size;
				}
		}
		ph = of_peer(ph);	// get next siblings
	}
}

long
getmemsize()
{
	return memory_size;
}

static void
get_timebase()
{
	unsigned int timebase;
	phandle_t cpu;
	phandle_t cpus = of_finddevice("/cpus");

	if (cpus == -1)
		return;

	cpu = of_child(cpus);

	if (cpu == -1)
		return;

	of_getprop(cpu, "timebase-frequency", &timebase, 4);
	tb_freq = (uint64_t) timebase;
}

int
_start_kernel(unsigned long p0, unsigned long p1)
{
	int rc,claim_rc;
	size_t _client_start = (size_t)&__client_start;
	size_t _client_size = (size_t)&__client_end - (size_t)&__client_start;

	claim_rc=(int)(long)of_claim((void *)(long)_client_start, _client_size, 0);

	if (init_io() <= -1)
		return -1;
	copy_exception_vectors();

	checkmemorysize();
	get_timebase();
	rtas_init();
	rc = _start((unsigned char *) p0, p1);

	term_net_driver();

	restore_exception_vectors();
	if (claim_rc >= 0) {
		of_release((void *)(long)_client_start, _client_size);
	}
	return rc;
}


void
exception_forward(void)
{
	restore_exception_vectors();
	undo_exception();
}
