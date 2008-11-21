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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <cfgparse.h>
#include <createcrc.h>

#define FFS_TARGET_HEADER_SIZE (4 * 8)

/* put this somewhere else */
#define FLAG_LLFW 1

unsigned int glob_rom_pos   = 0;
unsigned int glob_num_files = 0;

typedef struct {
	uint32_t high;
	uint32_t low;
} uint64s_t;

int file_exist(char *, int);
/* change done 2005-March-09 */
int write_to_file(int fd, unsigned char *buf, int size);
/* by Rolf Schaefer */

#define pad8_num(x) ((x) + (-(x) & 7))

int build_ffs(struct ffs_chain_t *fs, char *outfile)
{
	int ofdCRC;		/* change done 2005-April-07 by Rolf Schaefer */
	int ofd, ffsize, datasize, imgfd, i, cnt;
	int tokensize, hdrsize, ffile_offset, hdrbegin;
	struct ffs_header_t *hdr;
	unsigned char *ffile, c;
	struct stat fileinfo;
	uint64s_t val64;

	if (NULL == fs->first) {
		return 1;
	}
	hdr = fs->first;
	
	/* check output file and open it for creation */
	if (file_exist(outfile, 0)) {
		printf("Output file (%s) will be overwritten\n",
				outfile);
	}

	ofd 	=	open(".crc_flash", O_CREAT | O_WRONLY | O_TRUNC, 0666);
	ofdCRC 	=	open(outfile, O_CREAT | O_WRONLY | O_TRUNC, 0666);	/* change done 2005-April-07 by Rolf Schaefer */
	if (0 > ofd || 0 > ofdCRC) {
		perror(outfile);
		return 1;
	}

	while (1) {
		/* 
		 * first estimate the size of the file including header
		 * to allocate the memory in advance.
		 */
		if (NULL == hdr->linked_to) {
			memset((void*)&fileinfo, 0, sizeof(struct stat));
			if (stat(hdr->imagefile, &fileinfo) != 0) {
				perror(hdr->imagefile);
				return 1;
			}
			datasize = fileinfo.st_size;
		} else {
			datasize = 0;
		}
		ffile_offset = 0,
		tokensize    = pad8_num(strlen(hdr->token) + 1 /* ens trl 0 */);
		hdrsize      = FFS_TARGET_HEADER_SIZE + tokensize;
		ffsize       = hdrsize + pad8_num(datasize) + 8 /* -1 */;

		/* get the mem */
		ffile = (unsigned char*) malloc(ffsize);
		if (NULL == ffile) {
			perror("alloc mem for ffile");
			return 1;
		}
		memset((void*)ffile, 0 , ffsize);

		/* check if file wants a specific address */
		if ((hdr->romaddr > 0) || (0 != (hdr->flags & FLAG_LLFW))) {
			/* check if romaddress is below current position */
			if (hdr->romaddr < (glob_rom_pos + hdrsize)) {
				printf("[%s] ERROR: requested impossible "
						"romaddr of %llx\n",
						hdr->token, hdr->romaddr);
				close(ofd);
				free(ffile);
				return 1;
			}

			/* spin offset to new positon */
			if (pad8_num(hdr->romaddr) != hdr->romaddr) {
				printf("BUG!!!! pad8_num(hdr->romaddr) != hdr->romaddr\n");
				close(ofd);
				free(ffile);
				return 1;
			}
			i = hdr->romaddr - glob_rom_pos - hdrsize;
			while (0 < i) {
				c = 0;
/* change done 09.03.2005 */
				if (0 != write_to_file(ofd, &c, 1)) {
/* by Rolf Schaefer */
					printf("write failed\n");
					close(ofd);
					free(ffile);
				}
				glob_rom_pos++;
				i--;
			}
			if (glob_rom_pos != (hdr->romaddr - hdrsize)) {
				printf("BUG!!! hdr->romaddr != glob_rom_pos (%llx, %x)\n",
						hdr->romaddr, glob_rom_pos);
			}
			if (0 == glob_num_files) {
				printf("\nWARNING: The filesystem will have no "
					 "entry header !\n"
					 "         It is still usable but you need "
					 "to find\n"
					 "         the FS by yourself in the image.\n\n");
			}
		}
		hdrbegin = glob_rom_pos;
		///printf("OFFS=%6x FSIZE=%6d ", glob_rom_pos, ffsize);

		/* write header ********************************************/
		/* next addr ***********************************************/
		val64.high= 0;
		val64.low = 0;
		//printf("\n\n\nFFS_TARGET_HEADER_SIZE = %d\n", FFS_TARGET_HEADER_SIZE);
		//printf(      "FFS_TARGET_HEADER_SIZE = 0x%x\n", FFS_TARGET_HEADER_SIZE);
		//printf(      "glob_rom_pos           = 0x%x\n", glob_rom_pos);
		//printf(      "ffsize                 = %d\n", ffsize);
		//printf(      "ffsize                 = 0x%x\n", ffsize);

		if (NULL != hdr->next) {
			//printf(      "next-romaddr           = 0x%lx\n", hdr->next->romaddr);
			//printf(      "strlen(hdr->next->token= %d\n", pad8_num(strlen(hdr->next->token)));
			//printf(      "strlen(hdr->next->token= 0x%d\n", pad8_num(strlen(hdr->next->token)));
			if (hdr->next->romaddr > 0) {
				/* FIXME this is quite ugly, any other idea? */
				val64.low  = hdr->next->romaddr -
					     (FFS_TARGET_HEADER_SIZE + 
					     pad8_num(strlen(hdr->next->token)));
				val64.low -= glob_rom_pos;
				val64.low  = htonl(val64.low);
			} 
			//printf(      "val64.low              = 0x%lx\n", val64.low);

			if (0 == val64.low) {
				//next address relative to rombase
				//val64.low = htonl(glob_rom_pos + ffsize);
				//
				//changed next pointer to be relative
				//to current fileposition; needed to glue
				//different romfs-volumes together
				val64.low = htonl(ffsize);
			}
		} 
		//printf("\n\n\n");

		memcpy(ffile + ffile_offset, &val64, 8);
		glob_rom_pos += 8;
		ffile_offset += 8;

		/* length **************************************************/
		if (NULL == hdr->linked_to) {
			val64.low = htonl(datasize);
			hdr->save_data_len = datasize;
		} else {
			printf("\nBUG!!! links not supported anymore\n");
			close(ofd);
			free(ffile);
			return 1;

			/*
			if (0 == hdr->linked_to->save_data_valid) {
				printf("ERROR: this version does not support"
						"forward references in links\n");
				close(ofd);
				free(ffile);
				return 1;
			}
			val64.low = htonl(hdr->linked_to->save_data_len);
			*/
		}
		memcpy(ffile + ffile_offset, &val64, 8);
		glob_rom_pos += 8;
		ffile_offset += 8;

		/* flags ***************************************************/
		val64.low = htonl(hdr->flags);
		memcpy(ffile + ffile_offset, &val64, 8);
		glob_rom_pos += 8;
		ffile_offset += 8;

		/* datapointer *********************************************/
		if (NULL == hdr->linked_to) {
			//save-data pointer is relative to rombase
			hdr->save_data       = hdrbegin + hdrsize;
			hdr->save_data_valid = 1;
			//pointer relative to rombase:
			//val64.low            = htonl(hdr->save_data);
			//
			//changed pointers to be relative to file:
			val64.low            = htonl(hdr->save_data - hdrbegin);
		} else {
			printf("\nBUG!!! links not supported anymore\n");
			close(ofd);
			free(ffile);
			return 1;

			/*
			if (0 == hdr->linked_to->save_data_valid) {
				printf("ERROR: this version does not support"
						"forward references in links\n");
				close(ofd);
				free(ffile);
				return 1;
			}
			val64.low = htonl(hdr->linked_to->save_data);
			*/
		}
		memcpy(ffile + ffile_offset, &val64, 8);
		glob_rom_pos += 8;
		ffile_offset += 8;

		/* name (token) ********************************************/
		memset(ffile + ffile_offset, 0, tokensize);
		strcpy((char *)ffile + ffile_offset, hdr->token);
		glob_rom_pos += tokensize;
		ffile_offset += tokensize;

		/* image file **********************************************/
		if (NULL == hdr->linked_to) {
			if (! file_exist(hdr->imagefile, 1)) {
				printf("access error to file: %s\n", hdr->imagefile);
				close(ofd);
				free(ffile);
				return 1;
			}

			imgfd = open(hdr->imagefile, O_RDONLY);
			if (0 >= imgfd) {
				perror(hdr->imagefile);
				close(ofd);
				free(ffile);
				return 1;
			}

			/* now copy file to file buffer */
			cnt = 0;
			while (1) {
				i = read(imgfd, ffile + ffile_offset, ffsize-ffile_offset);
				if (i <= 0) break;
				glob_rom_pos += i;
				ffile_offset += i;
				cnt += i;
			}

			/* pad file */
			glob_rom_pos += pad8_num(datasize) - datasize;
			ffile_offset += pad8_num(datasize) - datasize;

			/* sanity check */
			if (cnt != datasize) {
				printf("BUG!!! copy error on image file [%s](e%d, g%d)\n",
						hdr->imagefile, datasize, cnt);
				close(imgfd);
				close(ofd);
				free(ffile);
				return 1;
			}
			close(imgfd);
		}

		/* limiter *************************************************/
		val64.low     = -1;
		val64.high    = -1;
		memcpy(ffile + ffile_offset, &val64, 8);
		glob_rom_pos += 8;
		ffile_offset += 8;

		/* *********************************************************/
		///printf("FLEN=%6d TOK=%-11s FILE=%s\n", 
		///		ffsize,
		///		hdr->token,
		///		hdr->imagefile);
/* change done 9.3.2005*/
		if (write_to_file(ofd, ffile, ffsize) != 0) {
			printf("Failed while processing file '%s' (size = %d bytes)\n",
			       hdr->imagefile, datasize);
			return 1;
		}
/* by Rolf Schaefer */
		free(ffile);
		hdr = hdr->next;
		glob_num_files++;

		if (NULL == hdr) break;
	}
/* change done 09.03.2005 */
	writeDataStream(ofdCRC);
/* by Rolf Schaefer */
	close(ofd);
	close(ofdCRC);

	return 0;
}

int file_exist(char *name, int errdisp)
{
	struct stat fileinfo;

	memset((void*)&fileinfo, 0, sizeof(struct stat));
	if (stat(name, &fileinfo) != 0) {
		if (0 != errdisp) {
			perror(name);
		}
		return 0;
	}
	if (S_ISREG(fileinfo.st_mode)) {
		return 1;
	}
	return 0;
}

/* changes done 2005_March-09 */
int write_to_file(int fd, unsigned char *buf, int size)
{
	int i;

	if (buildDataStream(buf, size) != 0)
		return -1;

	for ( ; size; size -= i, buf += i) {
		i = write(fd, buf, size);
		if (i < 0) {
			perror("write to image");
			return 1;
		}
	}

/*by Rolf Schaefer  */

	return 0;
}
