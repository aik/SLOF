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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <cfgparse.h>
#include <time.h>
#include <calculatecrc.h>
#include <product.h>
#include <byteswap.h>
#include <endian.h>

int createHeaderImage(void);
unsigned int calCRCEthernet32(unsigned char *TextPtr, unsigned long int TextLength, unsigned int AccumCRC);
int createCRCParameter(uint64_t *ui64RegisterMask, unsigned int *iRegisterLength);
uint64_t calCRCbyte(unsigned char *TextPtr, uint32_t Residual, uint64_t AccumCRC);
uint64_t calCRCword(unsigned char *TextPtr, uint32_t Residual, uint64_t AccumCRC);
uint64_t checkCRC(unsigned char *TextPtr, uint32_t Residual, uint64_t AccumCRC);
int insert64bit(uint64_t ui64CRC, uint32_t uliPosition);

static uint64_t ui64globalFileSize=0;	// file length in bytes
static unsigned char pucFileStream[4400000];	// space for the file stream >= 4MB + 4bytes
static uint64_t ui64globalHeaderSize=0;	// header length in bytes
static int iglobalHeaderFlag = 1;		// flag to filter detect the header in buildDataStream()
static uint64_t ui64Generator1;
/*----------------------------------------------------------------------*/
/*									*/
/*  Build the file image and store it as Data Stream of bytes		*/
/*  calculate a first CRC for the first file and 			*/
/*  chatch the position of this CRC 					*/
/*----------------------------------------------------------------------*/
int buildDataStream(unsigned char *pucbuf, int size)
{
	if (ui64globalFileSize + size > sizeof(pucFileStream)) {
		printf("Error: File size is too big!\n");
		return -1;
	}

	for( ; size; size -=1, pucbuf +=1) {

		pucFileStream[ui64globalFileSize] = *pucbuf;		
		ui64globalFileSize++;					
	} 
	if(iglobalHeaderFlag == 1) {							// catch header

		ui64globalHeaderSize = ui64globalFileSize;	
		iglobalHeaderFlag = 0;	
	}					

	return 0;
}
/*--------------------------------------------------------------------------------- */
/*                                                                      			+
+ 	write Header.img				                                    			+
+  								                                 					+
+	note: 	use insert64bit to write all uint64_t variables because of				+								
+			Big Endian - Little Endian problem between Intel CPU and PowerPC CPU 	*/
/* -------------------------------------------------------------------------------- */
int createHeaderImage(void)
{
	int iCounter;
	uint64_t ui64RomAddr, ui64DataAddr, ui64FlashlenAddr;
	time_t caltime;
	struct tm  *tm;
	char *pcVersion;
	char dastr[16];
	unsigned long long da;

	union {
		unsigned char pcArray[FLASHFS_HEADER_DATA_SIZE];
		struct stH stHeader;
	} uHeader;

	memset(uHeader.pcArray,0x00, FLASHFS_HEADER_DATA_SIZE);		// initialize Header

	/* read driver info */
	if(NULL != (pcVersion = getenv("DRIVER_NAME"))) {
		strncpy(uHeader.stHeader.version, pcVersion,16);
	}
	else if (NULL != (pcVersion = getenv("USER")) ) {
		strncpy(uHeader.stHeader.version, pcVersion,16);
	}
	else if(pcVersion == NULL) {
		strncpy(uHeader.stHeader.version, "No known user!",16 );
	}
	
	/* read time and write it into data stream */
	if ( (caltime = time(NULL)) ==-1) {
		printf("time error\n");
	}
	if ( (tm = localtime(&caltime)) == NULL) {
		printf("local time error\n");
	}

	// length must be 13 instead 12 because of terminating NUL. Therefore uH.stH.platform_revison
	// must be writen later to overwrite the terminating NUL
	if(strftime(dastr, 15, "0x%Y%m%d%H%M", tm) == 0) {
		printf("strftime error\n");
	}
	da=strtoll(dastr,NULL, 16);
	#if __BYTE_ORDER == __LITTLE_ENDIAN
	da=__bswap_64(da) >> 16;
	#else
	da = da << 16;
	#endif
	memcpy(uHeader.stHeader.date, &da, 8);

	strcpy(uHeader.stHeader.magic,FLASHFS_MAGIC);                   // write Magic value into data stream
	strcpy(uHeader.stHeader.platform_name,FLASHFS_PLATFORM_MAGIC);  // write platform name into data stream
	strcpy(uHeader.stHeader.platform_revision,FLASHFS_PLATFORM_REVISION); // write platform recision into data stream

	
	/* fill end of file info (8 bytes of FF) into data stream */
	uHeader.stHeader.ui64FileEnd = (uint64_t)(0xFFFFFFFF);				
	uHeader.stHeader.ui64FileEnd = (uHeader.stHeader.ui64FileEnd << 32) + (uint64_t)0xFFFFFFFF;

	/* read address of next file and address of header date, both are 64 bit values */ 
	ui64RomAddr 	= 0;
	ui64DataAddr 	= 0;
	for(iCounter = 0; iCounter < 8; iCounter++) {

		ui64RomAddr 	= ( ui64RomAddr << 8 ) + pucFileStream[FLASHFS_ROMADDR + iCounter];		// addr of next file
		ui64DataAddr	= ( ui64DataAddr << 8) + pucFileStream[FLASHFS_DATADDR + iCounter];		// addr of header data
	}

	/* calculate final flash-header-size and flash-file-size */
    ui64globalHeaderSize = (uint32_t)ui64DataAddr + (uint32_t)FLASHFS_HEADER_DATA_SIZE;     // calculate end addr of header
    ui64globalHeaderSize    -=8;                // cut 64 bit to place CRC for File-End
    ui64globalFileSize      += 8;               // add 64 bit to place CRC behind File-End

	if( ui64globalHeaderSize < ui64RomAddr ) {
			memset( &pucFileStream[ui64DataAddr], 0, (ui64RomAddr-ui64DataAddr));		// fill free space in Header with zeros
			// place data to header 
			memcpy( &pucFileStream[ui64DataAddr], uHeader.pcArray, FLASHFS_HEADER_DATA_SIZE); // insert header data

			// insert header length into data stream
			ui64FlashlenAddr = (uint64_t)FLASHFS_HEADER_SIZE_ADDR;
			insert64bit(ui64globalHeaderSize, ui64FlashlenAddr );
			
			// insert flash length into data stream          
            ui64FlashlenAddr = ((uint64_t)ui64DataAddr +(uint64_t)FLASHFS_FILE_SIZE_ADDR );
            insert64bit(ui64globalFileSize, ui64FlashlenAddr);

			// insert zeros as placeholder for CRC 
			insert64bit(0, (ui64globalHeaderSize -8) );
			insert64bit(0, (ui64globalFileSize -8) );

			return 0;
	}
	else {
		printf("%s\n","--- Header File to long");
		return 1;
	}
}
/*--------------------------------------------------------------------- */
/*																		*/
/*  calculate standart ethernet 32 bit CRC 										*/
/*  generator polynome is 0x104C11DB7   								*/
/*  this algorithm can be used for encoding and decoding				*/
/*----------------------------------------------------------------------*/
unsigned int calCRCEthernet32(unsigned char *TextPtr, unsigned long int TextLength, unsigned int AccumCRC) {
   const unsigned int CrcTableHigh[16] = {
      0x00000000, 0x4C11DB70, 0x9823B6E0, 0xD4326D90,
      0x34867077, 0x7897AB07, 0xACA5C697, 0xE0B41DE7,
      0x690CE0EE, 0x251D3B9E, 0xF12F560E, 0xBD3E8D7E,
      0x5D8A9099, 0x119B4BE9, 0xC5A92679, 0x89B8FD09
   };
   const unsigned CrcTableLow[16] = {
      0x00000000, 0x04C11DB7, 0x09823B6E, 0x0D4326D9,
      0x130476DC, 0x17C56B6B, 0x1A864DB2, 0x1E475005,
      0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61,
      0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD
   };

   unsigned char*  Buffer   =  TextPtr;
   unsigned long int  Residual = TextLength;


   while ( Residual > 0 ) {
      unsigned int Temp = ((AccumCRC >> 24) ^ *Buffer) & 0x000000ff;
      AccumCRC <<= 8;
      AccumCRC ^= CrcTableHigh[ Temp/16 ];
      AccumCRC ^= CrcTableLow[ Temp%16 ];
      ++Buffer;
      --Residual;
   }
   return AccumCRC;
}
/*------------------------------------------------------- calculate CRC */
/*																		*/
/*																		*/
/*----------------------------------------------------------------------*/                                                   
/*  create CRC Parameter:  CRC Polynome, Shiftregister Mask and length	*/
//    ui64Generator[0] = 0;
//    ui64Generator[1] = 0x42F0E1EB;
//   ui64Generator[1] = (ui64Generator[1] << 32) + 0xA9EA3693;
//    iRegisterLength = 63;
//    ui64RegisterMask =  0xffffffff;
//    ui64RegisterMask = ((ui64RegisterMask) << 32) + 0xffffffff;
/* 	ucl=0x00000000ffffffff = Mask for 32 bit LSFR to cut down number of bits 
	in the variable to get the same length as LFSR

	il = length of LSFR = degree of generator polynom reduce il by one to calculate the degree 
	of the highest register in LSFR 
	
	Examples:
	CRC-16 for Tap: 		x16 + x15 + x2 + 1	
		generator = 0x8005, 	il = 16,	ucl = 0x000000000000FFFF

	CRC-16 for Floppy:		x16 + x12 + x5 +1
		generator = 0x1021,	il = 16,	ucl = 0x000000000000FFFF

	CRC-32 for Ethernet:	x32 + x26 + x23 + x22 + x16 + x12 + x11 + x10 + x8 + x7 + x5 + x4 + x2 + x + 1
		generator = 0x04C11DB7,	il = 32,	ucl = 0x00000000FFFFFFFF

	CRC-64 SP-TrEMBL	x64 + x4 + x3 + x + 1 (maximal-length LFSR)
		renerator = 0x1B,	il = 64,	ucl = 0xFFFFFFFFFFFFFFFF

    CRC-64 improved	  	x64 + x63 + x61 + x59 + x58 + x56 + x55 + x52 + x49 + x48 + x47 + x46+ x44 +
						x41 + x37 + x36 + x34 + x32 + x31 + x28 + x26 + x23 + x22 + x19 + x16 + x13 +
						x12 + x10 + x9 + x6 + x4 + x3 + 1 (see http://www.cs.ud.ac.uk/staff/D.Jones/crcbote.pdf)
        grenerator = 0xAD93D23594C9362D,  il = 64,    ucl = 0xFFFFFFFFFFFFFFFF

	CRC-64 DLT1 spec	x64 + x62 + x57 + x55 + x54 + x53 + x52 + x47 + x46 + x45 + x40 + x39 + x38 + x37 +
						x35 + x33 + x32 + x31 + x29 + x27 + x24 + x23 + x22 + x21 + x19 + x17 + x13 + x12 +
						x10 + x9 + x7 + x4 + x + 1 
						(see http://www.ecma-international.org/publications/files/ECMA-ST/Ecma-182.pdf  -> page63)
		generator = 0x42F0E1EBA9EA3693 

	CRC-64 from internet G(x)= 1006003C000F0D50B
*/

/*--------------------------------------------------------------------- */
int createCRCParameter(uint64_t *ui64RegisterMask, unsigned int *uiRegisterLength)
{
	enum Generators {Tape_16, Floppy_16, Ethernet_32, SPTrEMBL_64, SPTrEMBL_improved_64,DLT1_64}; 
	enum Generators Generator;

	Generator = CRC_METHODE;
	switch  (Generator){
		case Tape_16:{
	        *ui64RegisterMask =  0x0000ffff;
        	ui64Generator1 = 0x00008005;
        	*uiRegisterLength = 16;
			break;
		}
		case Floppy_16: {
	        *ui64RegisterMask =  0x0000ffff;
        	ui64Generator1 = 0x00001021;
        	*uiRegisterLength = 16;
			break;
		}
		case Ethernet_32: {
	        *ui64RegisterMask =  0xffffffff;
        	ui64Generator1 = 0x04C11DB7;
        	*uiRegisterLength = 32;
			break;
		}
		case SPTrEMBL_64: {
	        *ui64RegisterMask =  0xffffffff;
    	    *ui64RegisterMask = ((*ui64RegisterMask) << 32) + 0xffffffff;
        	ui64Generator1 = 0x0000001B;
        	*uiRegisterLength = 64;
			break;
		}
		case SPTrEMBL_improved_64: {
        	*ui64RegisterMask =  0xffffffff;
        	*ui64RegisterMask = ((*ui64RegisterMask) << 32) + 0xffffffff;
        	ui64Generator1 = 0xAD93D235;
        	ui64Generator1 = (ui64Generator1 << 32) + 0x94C9362D;
        	*uiRegisterLength = 64;
			break;
		}
		case DLT1_64:{
    	*ui64RegisterMask =  0xffffffff;
    	*ui64RegisterMask = ((*ui64RegisterMask) << 32) + 0xffffffff;
    	ui64Generator1 = 0x42F0E1EB;
    	ui64Generator1 = (ui64Generator1 << 32) + 0xA9EA3693;
		*uiRegisterLength = 64;
		break;
		}
	}
    (*uiRegisterLength)--;

	return 0;
}
/*------------------------------------------------ create CRC Parameter */
/*                                                                   	*/
/*  Check CRC by using Linear Feadback Shift Register (LFSR)   		*/
/*----------------------------------------------------------------------*/
uint64_t calCRCbyte(unsigned char *cPtr, uint32_t ui32NoWords, uint64_t AccumCRC) {

    uint64_t ui64Mask, ui64Generator0;	
	uint8_t ui8Buffer;
	unsigned int uiRegisterLength;
    int iShift;

	createCRCParameter(&ui64Mask, &uiRegisterLength);	

	ui8Buffer = (*cPtr);
    while ( ui32NoWords > 0) {
        for (iShift = 7;iShift >= 0; iShift--) {

            ui64Generator0  =  (AccumCRC  >> uiRegisterLength);
            AccumCRC        <<= 1;
          	ui64Generator0  &=  0x01;
            ui64Generator0  =   (0-ui64Generator0);
            AccumCRC        ^=  (ui64Generator1 & ui64Generator0);
        }
        AccumCRC ^= ui8Buffer;
        AccumCRC &= ui64Mask;
        ui32NoWords -= 1;
        cPtr += 1;
		ui8Buffer = (*cPtr);
    }
    return AccumCRC;
}
/*--------------------------------------------------------------checkCRC */
/*                                                                   	*/
/*  Check CRC by using Linear Feadback Shift Register (LFSR)   		*/
/*----------------------------------------------------------------------*/
uint64_t calCRCword(unsigned char *cPtr, uint32_t ui32NoWords, uint64_t AccumCRC) {

    uint64_t ui64Mask, ui64Generator0;	
	uint16_t ui16Buffer;
	unsigned int uiRegisterLength;
    int iShift;

	createCRCParameter(&ui64Mask, &uiRegisterLength);	

	if( (ui32NoWords%2) != 0) {		// if Data string does not end at word boundery add one byte
 		ui32NoWords++;
		cPtr[ui32NoWords] = 0;
	}
	ui16Buffer = ( (*(cPtr+0)) * 256) + (*(cPtr+1));
    while ( ui32NoWords > 0) {
        for (iShift = 15;iShift >= 0; iShift--) {
            ui64Generator0  =   (AccumCRC >> uiRegisterLength);
            AccumCRC        <<= 1;
            ui64Generator0  &=  0x01;
            ui64Generator0  =   (0-ui64Generator0);
            AccumCRC        ^=  (ui64Generator1 & ui64Generator0);
        }
        AccumCRC ^= ui16Buffer;
        AccumCRC &= ui64Mask;
        ui32NoWords -= 2;
        cPtr += 2;
		ui16Buffer = ( (*(cPtr+0)) * 256) + (*(cPtr+1));
    }
    return AccumCRC;
}
/*------------------------------------------------------------- checkCRCnew */
/*																			*/
/*																			*/
/*------------------------------------------------------------------------- */
uint64_t checkCRC(unsigned char *cPtr, uint32_t ui32NoWords, uint64_t AccumCRC) {

	enum Generators {Ethernet_32};
	enum Generators Generator;
	uint64_t ui64Buffer = AccumCRC;

	Generator = CRC_METHODE;

	switch (Generator) {
		case Ethernet_32: {
		// (ui32NoWords - 4),no need of 4 bytes 0x as with shift-register method
		AccumCRC =  calCRCEthernet32( cPtr, (ui32NoWords-4), AccumCRC ); 
		break;
		}
		default :{
			AccumCRC =  calCRCword( cPtr, ui32NoWords, AccumCRC);
			break;
		}
	}
	
	if( calCRCbyte(cPtr, ui32NoWords, ui64Buffer) !=  AccumCRC) {
		printf("\n --- big Endian - small Endian problem --- \n");
		AccumCRC--;
	}
	
	return( AccumCRC);
}
	

/*--------------------------------------------------------------------- CRC */
/*  																		*/
/*  Insert 64 bit  as Big Endian											*/
/*	into DataStream starting at (uli)Position								*/
/*------------------------------------------------------------------------- */
int insert64bit(uint64_t ui64CRC, uint32_t ui32Position)
{

	pucFileStream[ui32Position]  	= (char) ((ui64CRC >> 56) & 0x00FF);
	pucFileStream[ui32Position+1] 	= (char) ((ui64CRC >> 48) & 0x00FF);
    pucFileStream[ui32Position+2]    = (char) ((ui64CRC >> 40) & 0x00FF);
    pucFileStream[ui32Position+3]    = (char) ((ui64CRC >> 32) & 0x00FF);

    pucFileStream[ui32Position+4]    = (char) ((ui64CRC >> 24) & 0x00FF);
    pucFileStream[ui32Position+5]    = (char) ((ui64CRC >> 16) & 0x00FF);
    pucFileStream[ui32Position+6]    = (char) ((ui64CRC >>  8) & 0x00FF);
    pucFileStream[ui32Position+7]    = (char)  (ui64CRC & 0x00FF);

	return 0;
}
/*----------------------------------------------------------- insertCRC */
/*									*/
/*  insert header and file CRC into data stream				*/
/*  do CRC check on header and file					*/
/*  write data stream to disk						*/
/*----------------------------------------------------------------------*/
int writeDataStream(int iofd)
{
	uint64_t ui64FileCRC=0, ui64HeaderCRC=0, ui64RegisterMask;
	unsigned int uiRegisterLength;

	if(0 != createHeaderImage()) {
		return 1;
	}

	createCRCParameter( &ui64RegisterMask, &uiRegisterLength);

/* calculate CRC---------------------------------------------------- */
	ui64HeaderCRC = checkCRC(pucFileStream, ui64globalHeaderSize, 0);
	insert64bit(ui64HeaderCRC, (ui64globalHeaderSize-8) );

	ui64FileCRC = checkCRC(pucFileStream, ui64globalFileSize, 0);
	insert64bit(ui64FileCRC, (ui64globalFileSize -8) );

/* report CRCs and proof if CRCs are correct implemented  */
	//printf("\n");
	//printf("%s%X%X\n","Generator = 0x1",(uint32_t)(ui64Generator1>>32),(uint32_t)ui64Generator1);

    //printf("%s%x%s%u%-9s%s%X%X\n","header size = 0x",  (uint32_t)ui64globalHeaderSize, "/", (uint32_t)ui64globalHeaderSize," bytes","file CRC   = 0x",(uint32_t)(ui64HeaderCRC >> 32),(uint32_t)ui64HeaderCRC);

    //printf("Firmware Header CRC = 0x%08x\n", (uint32_t)ui64HeaderCRC); 
    //printf("Flash Image CRC     = 0x%08x\n", (uint32_t)ui64FileCRC); 

	//printf("%s%x%s%u%-9s%s%X%X\n","flash size = 0x", (uint32_t)ui64globalFileSize, "/", (uint32_t)ui64globalFileSize," bytes","file CRC   = 0x",(uint32_t)(ui64FileCRC >> 32),(uint32_t)ui64FileCRC);

/* check CRC-implementation */
	ui64HeaderCRC = calCRCword( pucFileStream, ui64globalHeaderSize, 0);
	ui64FileCRC = calCRCword( pucFileStream, ui64globalFileSize, 0);

    //printf("%s%X%X\n","header CRC check = 0x", (uint32_t)(ui64HeaderCRC >> 32),(uint32_t)ui64HeaderCRC);
    //printf("%s%X%X\n","flash CRC check = 0x", (uint32_t)(ui64FileCRC >> 32),(uint32_t)ui64FileCRC);

	if( (ui64HeaderCRC == 0) && (ui64FileCRC == 0) ) {
		//printf("\n%s\n","CRCs correct implemented. ---> Data will be written to disk.");
		/* write file image to disk */
		if (0 < write(iofd, pucFileStream, ui64globalFileSize)) {
			return 0;
		}
		else {
			printf("<< write failed >>\n");
			return -1;
		}

	}
	else {
		printf("\n\n %s \n %s \n\n","CRCs not correct implemented."," ---> Data will not be written do disk.");
		return -1;
	}
}
/*----------------------------------------------------- writeDataStream */


