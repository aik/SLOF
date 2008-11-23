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

/* this is elf.fs rewritten in C */

#include <string.h>
#include <cpu.h>
#include <libelf.h>

struct ehdr {
	unsigned int ei_ident;
	unsigned char ei_class;
	unsigned char ei_data;
	unsigned char ei_version;
	unsigned char ei_pad[9];
	unsigned short e_type;
	unsigned short e_machine;
	unsigned int e_version;
	unsigned int e_entry;
	unsigned int e_phoff;
	unsigned int e_shoff;
	unsigned int e_flags;
	unsigned short e_ehsize;
	unsigned short e_phentsize;
	unsigned short e_phnum;
	unsigned short e_shentsize;
	unsigned short e_shnum;
	unsigned short e_shstrndx;
};

struct phdr {
	unsigned int p_type;
	unsigned int p_offset;
	unsigned int p_vaddr;
	unsigned int p_paddr;
	unsigned int p_filesz;
	unsigned int p_memsz;
	unsigned int p_flags;
	unsigned int p_align;
};

struct ehdr64 {
	unsigned int ei_ident;
	unsigned char ei_class;
	unsigned char ei_data;
	unsigned char ei_version;
	unsigned char ei_pad[9];
	unsigned short e_type;
	unsigned short e_machine;
	unsigned int e_version;
	unsigned long e_entry;
	unsigned long e_phoff;
	unsigned long e_shoff;
	unsigned int e_flags;
	unsigned short e_ehsize;
	unsigned short e_phentsize;
	unsigned short e_phnum;
	unsigned short e_shentsize;
	unsigned short e_shnum;
	unsigned short e_shstrndx;
};

struct phdr64 {
	unsigned int p_type;
	unsigned int p_flags;
	unsigned long p_offset;
	unsigned long p_vaddr;
	unsigned long p_paddr;
	unsigned long p_filesz;
	unsigned long p_memsz;
	unsigned long p_align;
};

#define VOID(x) (void *)((unsigned long)x)

static void
load_segment(unsigned long *file_addr, struct phdr *phdr)
{
	unsigned long src = phdr->p_offset + (unsigned long) file_addr;
	/* copy into storage */
	memmove(VOID(phdr->p_vaddr), VOID(src), phdr->p_filesz);

	/* clear bss */
	memset(VOID(phdr->p_vaddr + phdr->p_filesz), 0,
	       phdr->p_memsz - phdr->p_filesz);

	if (phdr->p_memsz) {
		flush_cache(VOID(phdr->p_vaddr), phdr->p_memsz);
	}
}

static unsigned int
load_segments(unsigned long *file_addr)
{
	struct ehdr *ehdr = (struct ehdr *) file_addr;
	/* Calculate program header address */
	struct phdr *phdr =
	    (struct phdr *) (((unsigned char *) file_addr) + ehdr->e_phoff);
	int i;
	/* loop e_phnum times */
	for (i = 0; i <= ehdr->e_phnum; i++) {
		/* PT_LOAD ? */
		if (phdr->p_type == 1) {
			/* copy segment */
			load_segment(file_addr, phdr);
		}
		/* step to next header */
		phdr =
		    (struct phdr *) (((unsigned char *) phdr) +
				     ehdr->e_phentsize);
	}
	return ehdr->e_entry;
}

static void
load_segment64(unsigned long *file_addr, struct phdr64 *phdr64)
{
	unsigned long src = phdr64->p_offset + (unsigned long) file_addr;
	/* copy into storage */
	memmove(VOID(phdr64->p_vaddr), VOID(src), phdr64->p_filesz);

	/* clear bss */
	memset(VOID(phdr64->p_vaddr + phdr64->p_filesz), 0,
	       phdr64->p_memsz - phdr64->p_filesz);

	if (phdr64->p_memsz) {
		flush_cache(VOID(phdr64->p_vaddr), phdr64->p_memsz);
	}
}

static unsigned long
load_segments64(unsigned long *file_addr)
{
	struct ehdr64 *ehdr64 = (struct ehdr64 *) file_addr;
	/* Calculate program header address */
	struct phdr64 *phdr64 =
	    (struct phdr64 *) (((unsigned char *) file_addr) + ehdr64->e_phoff);
	int i;
	/* loop e_phnum times */
	for (i = 0; i <= ehdr64->e_phnum; i++) {
		/* PT_LOAD ? */
		if (phdr64->p_type == 1) {
			/* copy segment */
			load_segment64(file_addr, phdr64);
		}
		/* step to next header */
		phdr64 =
		    (struct phdr64 *) (((unsigned char *) phdr64) +
				       ehdr64->e_phentsize);
	}
	return ehdr64->e_entry;
}

#if __BYTE_ORDER == __BIG_ENDIAN
#define cpu_to_be32(x)  (x)
#else
#define cpu_to_be32(x)  bswap_32(x)
#endif

/**
 * elf_check_file tests if the file at file_addr is
 * a correct endian, ELF PPC executable
 * @param file_addr  pointer to the start of the ELF file
 * @return           the class (1 for 32 bit, 2 for 64 bit)
 *                   -1 if it is not an ELF file
 *                   -2 if it has the wrong endianess
 *                   -3 if it is not an ELF executable
 *                   -4 if it is not for PPC
 */
static int
elf_check_file(unsigned long *file_addr)
{
	struct ehdr *ehdr = (struct ehdr *) file_addr;
	/* check if it is an ELF image at all */
	if (cpu_to_be32(ehdr->ei_ident) != 0x7f454c46)
		return -1;

	/* endian check */
#if __BYTE_ORDER == __BIG_ENDIAN
	if (ehdr->ei_data != 2)
		/* not a big endian image */
#else
	if (ehdr->ei_data == 2)
		/* not a little endian image */
#endif
		return -2;

	/* check if it is an ELF executable */
	if (ehdr->e_type != 2)
		return -3;

	/* check if it is a PPC ELF executable */
	if (ehdr->e_machine != 0x14 && ehdr->e_machine != 0x15)
		return -4;

	return ehdr->ei_class;
}

/**
 * load_elf_file tries to load the ELF file specified in file_addr
 *
 * it first checks if the file is a PPC ELF executable and then loads
 * the segments depending if it is a 64bit or 32 bit ELF file
 *
 * @param file_addr  pointer to the start of the elf file
 * @param entry      pointer where the ELF loader will store
 *                   the entry point
 * @return           1 for a 32 bit file
 *                   2 for a 64 bit file
 *                   anything else means an error during load
 */
int
load_elf_file(unsigned long *file_addr, unsigned long *entry)
{
	int type = elf_check_file(file_addr);
	switch (type) {
	case 1:
		*entry = load_segments(file_addr);
		break;
	case 2:
		*entry = load_segments64(file_addr);
		break;
	}
	return type;
}
