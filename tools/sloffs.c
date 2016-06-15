/******************************************************************************
 * Copyright (c) 2008, 2009 Adrian Reber
 * All rights reserved.
 * This program and the accompanying materials
 * are made available under the terms of the BSD License
 * which accompanies this distribution, and is available at
 * http://www.opensource.org/licenses/bsd-license.php
 *
 * Contributors:
 *     Adrian Reber - initial implementation
 *****************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <byteswap.h>
#include <getopt.h>

#include <calculatecrc.h>
#include <crclib.h>

#define VERSION 1

#ifdef _BIG_ENDIAN
#define cpu_to_be64(x)  (x)
#define be64_to_cpu(x)  (x)
#define be16_to_cpu(x)  (x)
#define be32_to_cpu(x)  (x)
#else
#define cpu_to_be64(x)  bswap_64(x)
#define be64_to_cpu(x)  bswap_64(x)
#define be16_to_cpu(x)  bswap_16(x)
#define be32_to_cpu(x)  bswap_32(x)
#endif


/* no board dependencies wanted here, let's hardcode SLOF's
 * magic strings here */

#define FLASHFS_MAGIC "magic123"
#define FLASHFS_PLATFORM_MAGIC "JS2XBlade"
#define FLASHFS_PLATFORM_REVISION "1"

/* there seems to be no structure defined anywhere in the code
 * which resembles the actual sloffs/romfs file header;
 * so defining it here for now */

struct sloffs {
	uint64_t next;
	uint64_t len;
	uint64_t flags;
	uint64_t data;
	char *name;
};

static struct sloffs *
next_file(struct sloffs *sloffs)
{
	return (struct sloffs *)((unsigned char *)sloffs +
				 be64_to_cpu(sloffs->next));
}

static struct sloffs *
find_file(const void *data, const char *name)
{
	struct sloffs *sloffs = (struct sloffs *)data;

	for (;;) {
		if (!strcmp((char *)&sloffs->name, name))
			return sloffs;

		if (be64_to_cpu(sloffs->next) == 0)
			break;
		sloffs = next_file(sloffs);
	}
	return NULL;
}

static void
sloffs_dump(const void *data)
{
	struct stH *header;
	struct sloffs *sloffs;
	int i;
	uint64_t crc;
	uint64_t *datetmp;

	/* find the "header" file with all the information about
	 * the flash image */
	sloffs = find_file(data, "header");
	if (!sloffs) {
		printf("sloffs file \"header\" not found. aborting...\n");
		return;
	}

	header = (struct stH *)((unsigned char *)sloffs +
				be64_to_cpu(sloffs->data));

	if (memcmp(FLASHFS_MAGIC, header->magic, strlen(FLASHFS_MAGIC))) {
		printf("sloffs magic not found. "
		       "probably not a valid SLOF flash image. aborting...\n");
		return;
	}
	printf("  Magic       : %s\n", header->magic);
	printf("  Platform    : %s\n", header->platform_name);
	printf("  Version     : %s\n", header->version);
	/* there is a bug in the date position;
	 * it should be at header->date, but it is at (header->date + 2) */
	printf("  Build Date  : ");
	datetmp = (void *)header->date;
	if (be64_to_cpu(*datetmp)) {
	    printf("%04x", be16_to_cpu(*(uint16_t *)(header->date + 2)));
	    printf("-%02x", *(uint8_t *)(header->date + 4));
	    printf("-%02x", *(uint8_t *)(header->date + 5));
	    printf(" %02x:", *(uint8_t *)(header->date + 6));
	    printf("%02x", *(uint8_t *)(header->date + 7));
	} else {
	    printf("N/A");
	}
	printf("\n");
	printf("  Modify Date : ");
	datetmp = (void *)header->mdate;
	if (be64_to_cpu(*datetmp)) {
	    printf("%04x", be16_to_cpu(*(uint16_t *)(header->mdate + 2)));
	    printf("-%02x", *(uint8_t *)(header->mdate + 4));
	    printf("-%02x", *(uint8_t *)(header->mdate + 5));
	    printf(" %02x:", *(uint8_t *)(header->mdate + 6));
	    printf("%02x", *(uint8_t *)(header->mdate + 7));
	} else {
	    printf("N/A");
	}
	printf("\n");
	printf("  Image Length: %ld", be64_to_cpu(header->flashlen));
	printf(" (0x%lx) bytes\n", be64_to_cpu(header->flashlen));
	printf("  Revision    : %s\n", header->platform_revision);
	crc = be64_to_cpu(header->ui64CRC);
	printf("  Header CRC  : 0x%016lx CRC check: ", crc);
	crc = calCRCword((unsigned char *)data, be64_to_cpu(sloffs->len), 0);
	if (!crc)
		printf("[OK]");
	else
		printf("[FAILED]");
	printf("\n");

	crc = be64_to_cpu(header->flashlen);
	crc = *(uint64_t *)(unsigned char *)(data + crc - 8);
	crc = be64_to_cpu(crc);
	printf("  Image CRC   : 0x%016lx CRC check: ", crc);
	crc = calCRCword((unsigned char *)data, be64_to_cpu(header->flashlen), 0);
	if (!crc)
		printf("[OK]");
	else
		printf("[FAILED]");
	printf("\n");

	/* count number of files */
	sloffs = (struct sloffs *)data;
	i = 0;
	for (;;) {
		i++;
		if (be64_to_cpu(sloffs->next) == 0)
			break;
		sloffs = next_file(sloffs);
	}
	printf("  Files       : %d\n", i);
}

static void
sloffs_list(const void *data)
{
	struct sloffs *sloffs = (struct sloffs *)data;
	const char *name_header = "File Name";
	unsigned int i;
	unsigned int max;
	unsigned int line;

	/* find largest name */
	max = strlen(name_header);;
	for (;;) {
		if (max < strlen((char *)&sloffs->name))
			max = strlen((char *)&sloffs->name);

		if (be64_to_cpu(sloffs->next) == 0)
			break;
		sloffs = next_file(sloffs);
	}

	/* have at least two spaces between name and size column */
	max += 2;

	/* header for listing */
	line = printf("   Offset      ");
	line += printf("%s", name_header);
	for (i = 0; i < max - strlen(name_header); i++)
		line += printf(" ");
	line += printf("Size                ");
	line += printf("Flags\n");
	printf("   ");
	for (i = 0; i <= line; i++)
		printf("=");
	printf("\n");

	sloffs = (struct sloffs *)data;
	for (;;) {
		printf("   0x%08lx", (void *)sloffs - (void *)data);
		printf("  %s", (char *)&sloffs->name);
		for (i = 0; i < max - strlen((char *)&sloffs->name); i++)
			printf(" ");

		printf("%07ld ", be64_to_cpu(sloffs->len));
		printf("(0x%06lx)", be64_to_cpu(sloffs->len));
		printf("  0x%08lx\n", be64_to_cpu(sloffs->flags));

		if (be64_to_cpu(sloffs->next) == 0)
			break;
		sloffs = next_file(sloffs);
	}
}

static void
usage(void)
{
	printf("sloffs lists or changes a SLOF flash image\n\n");
	printf("Usage:\n");
	printf("  sloffs [OPTION]... [FILE]\n\n");
	printf("Options:\n");
	printf("  -h, --help             show this help, then exit\n");
	printf("  -l, --list             list all files in the flash image\n");
	printf("  -v, --version          print the version, then exit\n");
	printf("  -d, --dump             dump the information from the header\n");
	printf("\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	int fd;
	void *file;
	struct stat stat;
	const struct option loption[] = {
		{ "help", 0, NULL, 'h' },
		{ "list", 0, NULL, 'l' },
		{ "version", 0, NULL, 'v' },
		{ "dump", 0, NULL, 'd' },
		{ 0, 0, 0, 0 }
	};
	const char *soption = "dhlv";
	int c;
	char mode = 0;

	for (;;) {
		c = getopt_long(argc, argv, soption, loption, NULL);
		if (c == -1)
			break;
		switch (c) {
		case 'l':
			mode = 'l';
			break;
		case 'v':
			printf("sloffs (version %d)\n", VERSION);
			exit(0);
		case 'd':
			mode = 'd';
			break;
		case 'h':
		default:
			usage();
		}
	}

	if (optind >= argc)
		usage();

	fd = open(argv[optind], O_RDONLY);

	if (fd == -1) {
		perror(argv[optind]);
		exit(1);
	}

	fstat(fd, &stat);
	file = mmap(NULL, stat.st_size, PROT_READ, MAP_SHARED, fd, 0);

	switch (mode) {
	case 'l':
		sloffs_list(file);
		break;
	case 'd':
		sloffs_dump(file);
		break;
	}

	munmap(file, stat.st_size);
	close(fd);
	return 0;
}
