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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <cfgparse.h>

#undef DEBUGGING

#ifdef DEBUGGING
#define dprintf(_x...) printf(_x)
#else
#define dprintf(_x...)
#endif

void print_usage()
{
	printf("args: description_file output_file\n\n");
}

int main (int argc, char *argv[])
{
	int conf_file, rc;
	struct ffs_chain_t ffs_chain;

	if (argc != 3) {
		print_usage();
		return EXIT_FAILURE;
	}

	dprintf("ROMFS FILESYSTEM CREATION V0.1 (bad parser)\n");
	dprintf("Build directory structure...\n");

	memset((void*) &ffs_chain, 0, sizeof(struct ffs_chain_t));
	conf_file = open(argv[1], O_RDONLY);
	if (0 >= conf_file) {
		perror("load config file:");
		return EXIT_FAILURE;
	}

	while (0 == (rc = find_next_entry(conf_file, &ffs_chain)));

	if (1 >= rc) {
#ifdef DEBUGGING
		dump_fs_contents(&ffs_chain);
#endif
		dprintf("Build ffs and write to image file...\n");
		if (build_ffs(&ffs_chain, argv[2]) != 0) {
			fprintf(stderr, "build ffs failed\n");
			rc = EXIT_FAILURE;
		} else {
			rc = EXIT_SUCCESS;
		}
	} else {
		fprintf(stderr, "flash cannot be built due to config errors\n");
		rc = EXIT_FAILURE;
	}

	/* Check if there are any duplicate entries in the image: */
	find_duplicates(&ffs_chain);

	free_chain_memory(&ffs_chain);
	close(conf_file);
	dprintf("\n");

	/* If the build failed, remove the target image file */
	if (rc == EXIT_FAILURE) {
		unlink(argv[2]);
	}

	return rc;
}

