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
#ifndef CFGPARSE_H
#define CFGPARSE_H

struct ffs_chain_t {
	struct ffs_header_t *first;
};

struct ffs_header_t {
	unsigned long long flags;
	unsigned long long romaddr;
	char  *token;
	char  *imagefile;
	int    imagefile_length;
	struct ffs_header_t *linked_to;
	struct ffs_header_t *next;
	unsigned long long save_data;
	unsigned long long save_data_len;
	int save_data_valid;
};

int find_next_entry(int file, struct ffs_chain_t *chain);
int inbetween_white(char *s, int max, char **start, char **end, char **next);
void debug_print_range(char *s, char *e);
void free_chain_memory(struct ffs_chain_t *chain);
int add_header(struct ffs_chain_t *, struct ffs_header_t *);
int find_entry_by_token(struct ffs_chain_t *, char *, struct ffs_header_t **);
void dump_fs_contents(struct ffs_chain_t *chain);
void find_duplicates(struct ffs_chain_t *chain);

int build_ffs(struct ffs_chain_t *fs, char *outfile);
#endif
