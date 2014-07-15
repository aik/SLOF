/******************************************************************************
 * Copyright (c) 2004, 2011 IBM Corporation
 * All rights reserved.
 * This program and the accompanying materials
 * are made available under the terms of the BSD License
 * which accompanies this distribution, and is available at
 * http://www.opensource.org/licenses/bsd-license.php
 *
 * Contributors:
 *     IBM Corporation - initial implementation
 *****************************************************************************/

#include <netdriver_int.h>
#include <kernel.h>
#include <of.h>
#include <rtas.h> 
#include <libelf.h>
#include <cpu.h> /* flush_cache */
#include <unistd.h> /* open, close, read, write */
#include <stdio.h>
#include <pci.h>
#include "modules.h"

snk_module_t * cimod_check_and_install(void);

extern snk_module_t of_module, ci_module;

extern char __client_start[];

snk_module_t *snk_modules[MODULES_MAX];

/* Load module and call init code.
   Init code will check, if module is responsible for device.
   Returns -1, if not responsible for device, 0 otherwise.
*/

void
modules_init(void)
{
	int i;

	snk_modules[0] = &of_module;

	/* Setup Module List */
	for(i=1; i<MODULES_MAX; ++i) {
		snk_modules[i] = 0;
	}

	/* Try to init client-interface module (it's built-in, not loadable) */
	for(i=0; i<MODULES_MAX; ++i) {
		if(snk_modules[i] == 0) {
			snk_modules[i] = cimod_check_and_install();
			break;
		}
	}
}

void
modules_term(void)
{
	int i;

	/* remove all modules */
	for(i=0; i<MODULES_MAX; ++i) {
		if(snk_modules[i] && snk_modules[i]->running != 0) {
			snk_modules[i]->term();
		}
		snk_modules[i] = 0;
	}
}

snk_module_t *
get_module_by_type(int type) {
	int i;

	for(i=0; i<MODULES_MAX; ++i) {
		if(snk_modules[i] && snk_modules[i]->type == type) {
			return snk_modules[i];
		}
	}
	return 0;
}
