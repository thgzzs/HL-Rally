// Rally_CryptFile.cpp
// Code by SaRcaZm

/************************************\
*      B  L  I  N  D  S  I  D  E     *
*                                    *
*  Image stego/crypto tool, please   *
*  see http://www.blindside.co.uk    *
*                                    *
*  Compile with GCC or equiv ANSI C  *
*                                    *
* John Collomosse (mapjpc@bath.ac.uk)*
* Freeware/public domain             *
*                                    *
\************************************/

// Includes
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <io.h>
#include <string.h>

#include "Rally_CryptFile.h"

// Disable warnings about type mismatches, as there are lots of them
#pragma warning (disable : 4244 4018)

// Class functions
CCryptFile::CCryptFile (char *szFileName)
{
	strcpy (m_szFileName, szFileName);
	m_pFile = NULL;
}

CCryptFile::~CCryptFile (void)
{
	if (m_pFile)
		fclose (m_pFile);
}

int CCryptFile::GetFileSize (char *szCryptFile, char *szPassword)
{
	if (GetFileSpace (MODE_READ, szPassword) == NULL)
		return 0;

	int ret = fileSize (filespace, szCryptFile);

	CleanUp ();
	return ret;
}

int CCryptFile::GetFile (char *szCryptFile, char *szPassword, int iLen, byte *pBuffer)
{
	if (GetFileSpace (MODE_READ, szPassword) == NULL)
		return 0;

	int ret = extractFile(filespace,szCryptFile, iLen, pBuffer);
	CleanUp ();
	return ret;
}

int CCryptFile::AddFile (char *szCryptFile, char *szPassword, int iLen, byte *pBuffer)
{
	if (GetFileSpace (MODE_ADD, szPassword) == NULL)
		return 0;

	addFileToArchive(filespace,pBuffer, iLen, szCryptFile);

//	if (isencrypted==true) {
		cryptoData(filespace,(unsigned char *)szPassword);
//	}
	hotspots=(LOOKUP*)calloc(1,sizeof(LOOKUP));

	if (hotspots==NULL) {
		return 0;
	}
	hotspots->dataspace=(unsigned long int*)calloc(LOOKUP_INITLEN,sizeof(unsigned long int));
	if (hotspots==NULL) {
		return 0;
	}
	hotspots->currentlen=LOOKUP_INITLEN;
	flattenBitmap(BMPworking,hotspots);
	encodeData(BMPworking,filespace,hotspots);
	outfile_handle=fopen(m_szFileName,"wb");
	writeBitmapFile(outfile_handle, BMPworking);
	fclose(outfile_handle);

	CleanUp ();
	return 1;
}

int CCryptFile::GetFreeSpace (char *szPassword)
{
	if (GetFileSpace (MODE_SPACE, szPassword) == NULL)
		return 0;

	dumpFileStats(BMPworking, filespace); 
	CleanUp ();
	return 1;
}

int CCryptFile::GetContents (char *szPassword)
{
	if (GetFileSpace (MODE_FILES, szPassword) == NULL)
		return 0;

	dumpFileDirectory(filespace);
	CleanUp ();
	return 1;
}

HEAP *CCryptFile::GetFileSpace (int mode, char *szPassword)
{
	// Initialise variables
	isencrypted = false;
	bitmap_handle=NULL;
	outfile_handle=NULL;
	BMPsource=NULL;
	BMPworking=NULL;
	filespace=NULL;

	/* Open all necessary files */
	bitmap_handle=fopen(m_szFileName,"rb");

	if (bitmap_handle==NULL)
		return NULL;

	/* Read source bitmap (this must be done for every command line variation) */
	BMPsource=(BITMAP_t*)calloc(1,sizeof(BITMAP_t));
	BMPsource->palette=(HEAP*)calloc(1,sizeof(HEAP));
	BMPsource->raster=(HEAP*)calloc(1,sizeof(HEAP));
	initialiseHeap(BMPsource->palette);
	initialiseHeap(BMPsource->raster);
	if (!readBitmapFile(bitmap_handle,BMPsource))
		return NULL;

	/* Analyse Bitmap */
	if (stricmp("BM",BMPsource->signature) || BMPsource->bitplanes!=1)
		return NULL;
	if (BMPsource->compression!=0)
		return NULL;
	if (BMPsource->bpp==1)
		return NULL;
	if (BMPsource->bpp!=4 && BMPsource->bpp!=8 && BMPsource->bpp!=24)
		return NULL;

	/* Colour depth increase if need be */
	if (BMPsource->bpp<24) {
		BMPworking=(BITMAP_t*)calloc(1,sizeof(BITMAP_t));
		if (BMPworking==NULL)
			return NULL;

		BMPworking->palette=(HEAP*)calloc(1,sizeof(HEAP));
		BMPworking->raster=(HEAP*)calloc(1,sizeof(HEAP));
		if (BMPworking->palette==NULL || BMPworking->raster==NULL)
			return NULL;
		if (!initialiseHeap(BMPworking->palette) || !initialiseHeap(BMPworking->raster))
			return NULL;

		/* Do the promote to 24bpp */
		promoteTo24(BMPsource, BMPworking);
		/* cleanup source */
		removeHeap(BMPsource->palette);
		removeHeap(BMPsource->raster);
		free(BMPsource);
		BMPsource=NULL;
	}
	else {
		/* Source was fine -> already 24 bpp */
		BMPworking=BMPsource;
		BMPsource=NULL;
	}
	
	/* Prepare BS dataspace */
	filespace=(HEAP*)calloc(1,sizeof(HEAP));
	initialiseHeap(filespace);
	switch (decodeData(BMPworking,filespace)) {
		case 0:
			break;
		case 1: removeHeap(filespace);
			initialiseHeap(filespace);                      
			if (mode==2 || mode==4)
				return NULL;
			if (mode==1) {
				if (!formatDataspace(BMPworking, filespace))
					return NULL;
			}
			break;
		case 2:
			return NULL;
			break;
		case 3:
			return NULL;
			break;
	}

	/* At this point 'filespace' is always a working BS archive (although it may be one with no files contained) */
	/* In case of Mode 3 it MAY NOT be the case that filespace is archive - it may be empty heap                 */

	/* Decrypt if necessary */
	if (filespace->heaplen!=0) {
		if (*(filespace->dataspace+1)=='E') {
			/* decrypt */
			if (strcmp(szPassword,"")==0) {
				/* get pword */
				return NULL;
			}
			cryptoData(filespace,(unsigned char *)szPassword);
			if (*(filespace->dataspace+7)=='O' && *(filespace->dataspace+8)=='K') {
				isencrypted=true;
			}
			else {
				return NULL;
			}
		}
	}

	return filespace;
}

void CCryptFile::CleanUp (void)
{
	/* Cleaup and exit */
	removeHeap(filespace);
	free(filespace);
	removeHeap(BMPworking->palette);
	free(BMPworking->palette);
	removeHeap(BMPworking->raster);
	free(BMPworking->raster);
	free(BMPworking);

	fclose (bitmap_handle);
}

/***************************** HEAP MANAGEMENT ***********************/
int CCryptFile::initialiseHeap(HEAP* heap) {
	heap->dataspace=NULL;
	heap->heapunits=0;
	heap->nextchar=NULL;
	heap->heaplen=0;

	heap->dataspace=(unsigned char*)calloc(HEAP_INITUNITS,HEAP_UNITSIZE*sizeof(unsigned char));
	if (heap->dataspace==NULL)
		return FALSE;
	else {
		heap->heapunits=HEAP_INITUNITS;
		heap->nextchar=heap->dataspace;
		return TRUE;
	}
}

int CCryptFile::expandHeap(int newunits, HEAP* heap) {
	if (newunits>heap->heapunits) {
		unsigned char*  tmpheap;
		int     i;

		tmpheap=heap->dataspace;
		heap->dataspace=(unsigned char*)calloc(newunits,HEAP_UNITSIZE*sizeof(unsigned char));
		if (heap->dataspace==NULL) {
			heap->dataspace=tmpheap;
			return FALSE;
		}
		else {
			for (i=0; i<heap->heapunits*HEAP_UNITSIZE; i++)
				heap->dataspace[i]=tmpheap[i];
			free(tmpheap);
			heap->nextchar=(heap->dataspace)+(heap->heapunits*HEAP_UNITSIZE);
			heap->heapunits=newunits;
		}
	}
	return TRUE;
}

void CCryptFile::removeHeap(HEAP* heap) {
	if (heap->dataspace!=NULL) {
		free (heap->dataspace);
		heap->heapunits=0;
		heap->dataspace=NULL;
		heap->nextchar=NULL;
		heap->heaplen=0;
	}
}

int CCryptFile::streamFileToHeap(FILE* infile, HEAP* heap) {
	unsigned char readchar;

	while (fscanf(infile,"%c",&readchar)>0) {
		if (!putToHeap(readchar,heap))
			return FALSE;
	}

	return TRUE;
}

int CCryptFile::putToHeap(unsigned char data, HEAP* heap) {

	unsigned long hopeful_pos=heap->heaplen;
	unsigned long current_pos=(heap->heapunits*HEAP_UNITSIZE);


	if (hopeful_pos >= current_pos) {
		int discrep=(int)hopeful_pos/HEAP_UNITSIZE;
		discrep++;
		if (!expandHeap(discrep, heap)) {
		       return FALSE;
		}
	}

	*(heap->nextchar)=data;
	heap->heaplen++;
	heap->nextchar++;

	return TRUE;
}

/*********************************************************************/
/******************************BMP HANDLER ROUTINES*******************/

int CCryptFile::readBitmapFile(FILE* bmpfile, BITMAP_t* datazone) {

	unsigned char tmpchar;
	int  dataread;
	int  i;
	long offsetToRaster=0;

	/* Get Signature */
	datazone->signature=(char*)calloc(3,sizeof(char));
	datazone->signature[2]='\0';
	dataread=fscanf(bmpfile,"%c",&tmpchar);
	if (dataread==1)
		datazone->signature[0]=tmpchar;
	dataread=fscanf(bmpfile,"%c",&tmpchar);
	if (dataread==1)
		datazone->signature[1]=tmpchar;

	/* Get Filesize */
	datazone->filesize=streamToLong(bmpfile);

	/* Skip reserved section */
	for (i=0; i<4; i++)
		dataread=fscanf(bmpfile,"%c",&tmpchar);

	/* Get Raster Offset */
	offsetToRaster=streamToLong(bmpfile);

	/* Skip size of infoheader (always=40 anyway) */
	for (i=0; i<4; i++)
		dataread=fscanf(bmpfile,"%c",&tmpchar);

	/* Get Bitmap width (x_size) */
	datazone->x_size=streamToLong(bmpfile);

	/* Get Bitmap height (y_size) */
	datazone->y_size=streamToLong(bmpfile);

	/* Get number bitplanes */
	dataread=fscanf(bmpfile,"%c",&tmpchar);
	if (dataread==1)
		datazone->bitplanes=((unsigned int)tmpchar*1);
	dataread=fscanf(bmpfile,"%c",&tmpchar);
	if (dataread==1)
		datazone->bitplanes+=((unsigned int)tmpchar*256);

	/* Get bits per pixel */
	dataread=fscanf(bmpfile,"%c",&tmpchar);
	if (dataread==1)
		datazone->bpp=((unsigned int)tmpchar*1);
	dataread=fscanf(bmpfile,"%c",&tmpchar);
	if (dataread==1)
		datazone->bpp+=((unsigned int)tmpchar*256);

	/* Get Compression */
	datazone->compression=streamToLong(bmpfile);

	/* Get Compressed Image Size */
	datazone->compresssize=streamToLong(bmpfile);

	/* Get X pix per metre */
	datazone->x_pix_per_metre=streamToLong(bmpfile);

	/* Get Y pix per metre */
	datazone->y_pix_per_metre=streamToLong(bmpfile);

	/* Get Colours Used */
	datazone->colours=streamToLong(bmpfile);

	/* Get num important colours*/
	datazone->cols_important=streamToLong(bmpfile);

	if (datazone->bpp<=8) {
		int numcolors=0;
		switch(datazone->bpp) {
			case 1: numcolors=1;
			case 4: numcolors=16;
			case 8: numcolors=256;
		}
		for (i=0; i<numcolors; i++) {
			unsigned char readchar;
			/* RED */
			if (fscanf(bmpfile,"%c",&readchar)>0) {
				if (!putToHeap(readchar,datazone->palette))
					return FALSE;
			}
			/* GREEN */
			if (fscanf(bmpfile,"%c",&readchar)>0) {
				if (!putToHeap(readchar,datazone->palette))
					return FALSE;
			}
			/* BLUE */
			if (fscanf(bmpfile,"%c",&readchar)>0) {
				if (!putToHeap(readchar,datazone->palette))
					return FALSE;
			}
			/* RESERVED */
			if (fscanf(bmpfile,"%c",&readchar)>0) {
				if (!putToHeap(readchar,datazone->palette))
					return FALSE;
			}
		}
	}


	if (fseek(bmpfile,offsetToRaster,SEEK_SET)==0)
		if(!streamFileToHeap(bmpfile,datazone->raster))
			return FALSE;

	return TRUE;}

void CCryptFile::promoteTo24(BITMAP_t* source, BITMAP_t* dest) {
	unsigned long int ctr;
	unsigned char*  tmpptr;
	unsigned char   tmpchar[4];
	unsigned char   split16_1,split16_2;
	unsigned int    EOLtargetpadding;       /*in bits, for 24 target*/
	unsigned long int   xpos;
	int             i;

	/* translate */
	EOLtargetpadding=calcEOLpad(24,source->x_size);

	xpos=0;
	tmpptr=source->raster->dataspace;
	for (ctr=0; ctr<source->raster->heaplen; ctr+=4) {
		switch(source->bpp) {
			case 4: /* 16colour */
				for (i=0; i<4; i++)
					tmpchar[i]=*(tmpptr++);
				for (i=0; i<4; i++) {
					split16_1=tmpchar[i];
					split16_2=tmpchar[i];
					split16_1=(split16_1&0xf0)>>4;
					split16_2=split16_2&0xf;
					if (xpos<source->x_size) {
						process24gotpixel(split16_1,source->palette->dataspace,dest->raster);
						xpos++;
					}
					if (xpos<source->x_size) {
						process24gotpixel(split16_2,source->palette->dataspace,dest->raster);
						xpos++;
					}
				}
				break;
			case 8: /* 256 cols */
				for (i=0; i<4; i++) {
					tmpchar[i]=*(tmpptr++);
				}
				for (i=0; i<4; i++) {
					if (xpos<source->x_size) {
						process24gotpixel(tmpchar[i],source->palette->dataspace,dest->raster);
						xpos++;
					}
				}
				break;
		}
		if (xpos==source->x_size) {
			/* align on source */
			xpos=0;
			/* align on 24bit target */
			for (i=0; i<EOLtargetpadding; i++)
				putToHeap('\0',dest->raster);
		}
	}

	/* sort header info out */
	dest->signature=(char*)calloc(3,sizeof(char));
	dest->signature[0]='B';
	dest->signature[1]='M';
	dest->signature[2]='\0';
	dest->compression=0;
	dest->x_pix_per_metre=source->x_pix_per_metre;
	dest->y_pix_per_metre=source->y_pix_per_metre;
	dest->bitplanes=1;
	dest->bpp=24;
	dest->x_size=source->x_size;
	dest->y_size=source->y_size;
	dest->colours=0;
	dest->cols_important=0;
	/*file sizes */
	dest->compresssize=dest->raster->heaplen;
	dest->filesize=54+(dest->compresssize);
}

void CCryptFile::process24gotpixel(unsigned char pal, unsigned char* palette, HEAP* raster) {
	/* got pixel palette entry# PAL */
	palette+=(unsigned int)pal*4;
	putToHeap(*palette,raster);
	putToHeap(*(palette+1),raster);
	putToHeap(*(palette+2),raster);

}

void CCryptFile::writeBitmapFile(FILE* outfile, BITMAP_t* source) {
	unsigned char* curspace=NULL;
	unsigned long int ctr;

	fprintf(outfile,"%c%c",source->signature[0],source->signature[1]);
	longToStream(outfile,source->filesize);
	longToStream(outfile,0);
	longToStream(outfile,54);
	longToStream(outfile,40);
	longToStream(outfile,source->x_size);
	longToStream(outfile,source->y_size);
	fprintf(outfile,"%c%c%c%c",1,0,24,0);
	longToStream(outfile,source->compression);
	longToStream(outfile,source->compresssize);
	longToStream(outfile,source->x_pix_per_metre);
	longToStream(outfile,source->y_pix_per_metre);
	longToStream(outfile,0);
	longToStream(outfile,0);
	/* stream out dataspace */

	ctr=0;
	curspace=(source->raster->dataspace);
	while (ctr<source->raster->heaplen) {
		fprintf(outfile,"%c",*(curspace++));
		ctr++;
	}
}

/*********************************************************************/
/**************************** ENCODE FUNCTIONS ***********************/
unsigned long int CCryptFile::resolveMaxEncode(BITMAP_t* bmp, LOOKUP* store) {
	/* return maximum possible storage space in BMP */

	unsigned long storage;
	unsigned long offset,tmp;
	unsigned char thisR;
	unsigned char thisG;
	unsigned char thisB;
	unsigned char lookaheadR;
	unsigned char lookaheadG;
	unsigned char lookaheadB;
	int dom,lookaheaddom;
	int keepflag,keepflag2;
	unsigned long int hits;
	PIXEL getdata, corrupta;
	unsigned long int longtmp;
	unsigned long int longtmp2;
	unsigned long int *longptrtmp;

	offset=0;
	storage=0;     /* running total */

	while (offset<(bmp->x_size*bmp->y_size)) {
		getdata=getIndexedPixel(offset, bmp);
		thisR=getdata.red;
		thisG=getdata.green;
		thisB=getdata.blue;

		dom=biggest_of_3(thisR,thisG,thisB);
		keepflag=FALSE;

		/* If 1 col varies from others by at least 3 => candidate */
		if (dom==thisG && abs(thisG-thisR)>2 && abs(thisG-thisB)>2)
			keepflag=TRUE;
		if (dom==thisB && abs(thisB-thisR)>2 && abs(thisB-thisG)>2)
			keepflag=TRUE;
		if (dom==thisR && abs(thisR-thisB)>2 && abs(thisR-thisG)>2)
			keepflag=TRUE;
		if (keepflag==FALSE) {
			offset++;
			continue;
		}

		/* only one of the 3 cols=dom here, and that col is at least
		   3 points greater than the others */

		/* see if dominant col continues on more pixels + that other
		   cols dont challenge its dominance */

		tmp=offset+1;
		hits=1;
		while (tmp<(bmp->x_size*bmp->y_size)) {
			getdata=getIndexedPixel(tmp, bmp);
			lookaheadR=getdata.red;
			lookaheadG=getdata.green;
			lookaheadB=getdata.blue;
			keepflag=FALSE;
			if (dom==thisG && lookaheadG==thisG)
				keepflag=TRUE;
			if (dom==thisR && lookaheadR==thisR)
				keepflag=TRUE;
			if (dom==thisB && lookaheadB==thisB)
				keepflag=TRUE;

			/* check dom is still dominant col by at least 4 */
			lookaheaddom=biggest_of_3(lookaheadR,lookaheadG,lookaheadB);
			keepflag2=FALSE;
			if (thisR==dom && lookaheadR==lookaheaddom && abs(lookaheadR-lookaheadG)>2 && abs(lookaheadR-lookaheadB)>2)
				keepflag2=TRUE;
			if (thisG==dom && lookaheadG==lookaheaddom && abs(lookaheadG-lookaheadR)>2 && abs(lookaheadG-lookaheadB)>2)
				keepflag2=TRUE;
			if (thisB==dom && lookaheadB==lookaheaddom && abs(lookaheadB-lookaheadR)>2 && abs(lookaheadB-lookaheadG)>2)
				keepflag2=TRUE;

			if (keepflag==FALSE || keepflag2==FALSE) {
				/* make this byte (that broke chain) much
				   different to running colour byte to prevent decode confusion */
				if (thisR==dom && lookaheadR!=thisR && abs(thisR-lookaheadR)<3) {
					/* chain was red, and broke with a minor difference*/
					corrupta.blue=lookaheadB;
					corrupta.green=lookaheadG;
					if (lookaheadR>thisR) {
						if (lookaheadR<253)
							corrupta.red=lookaheadR+3;
						else
							corrupta.red=lookaheadR-6;
					}
					else {
						if (lookaheadR<253)
							corrupta.red=lookaheadR-3;
						else
							corrupta.red=lookaheadR-6;
					}
					setIndexedPixel(tmp,bmp,corrupta);

				}
				else if (thisG==dom && lookaheadG!=thisG && abs(thisG-lookaheadG)<3) {
					/* chain was green and broke with minor diff*/
					corrupta.red=lookaheadR;
					corrupta.blue=lookaheadB;
					if (lookaheadG>thisG) {
						if (lookaheadG<253)
							corrupta.green=lookaheadG+3;
						else
							corrupta.green=lookaheadG-6;
					}
					else {
						if (lookaheadG<253)
							corrupta.green=lookaheadG-3;
						else
							corrupta.green=lookaheadG-6;
					}
					setIndexedPixel(tmp,bmp,corrupta);
				}
				else if (thisB==dom && lookaheadB!=thisB && abs(thisB-lookaheadB)<3) {
					/* chain was blue and broke with minor diff */
					corrupta.green=lookaheadG;
					corrupta.red=lookaheadR;
					if (lookaheadB>thisB) {
						if (lookaheadB<253)
							corrupta.blue=lookaheadB+3;
						else
							corrupta.blue=lookaheadB-6;
					}
					else {
						if (lookaheadB<253)
							corrupta.blue=lookaheadB-3;
						else
							corrupta.blue=lookaheadB-6;
					}
					setIndexedPixel(tmp,bmp,corrupta);
				}
				break;
			}
			hits++;
			tmp++;
		}

		if (hits>2) {
			storage+=(hits-2);
			/* store in lookup table */
			for (longtmp=offset+2; longtmp<offset+hits; longtmp++) {
				if (store->curitem>=store->currentlen) {
					longptrtmp=store->dataspace;
					store->dataspace=(unsigned long int*)calloc(2*store->currentlen,sizeof(unsigned long int));
					for (longtmp2=0; longtmp2<store->currentlen; longtmp2++) {
						*(store->dataspace+longtmp2)=*(longptrtmp+longtmp2);
					}
					free (longptrtmp);
					longptrtmp=NULL;
					store->currentlen*=2;
				}
				*(store->dataspace+(store->curitem++))=longtmp;
			}
			offset+=hits;
		}
		else {
			offset++;
		}
	}

	return (unsigned long)(storage/8);
}

unsigned int CCryptFile::biggest_of_3 (unsigned int a, unsigned int b, unsigned int c) {
	int pr1,pr2;
	if (a>b)
		pr1=a;
	else
		pr1=b;
	if (b>c)
		pr2=b;
	else
		pr2=c;
	if (pr1>pr2)
		return pr1;
	else
		return pr2;
}

/*********************************************************************/
void CCryptFile::longToStream(FILE* strm, unsigned long int x) {
	unsigned int bit4,bit3,bit2,bit1;

	bit4=(unsigned int)x/16777216;
	x=x-(unsigned long)((unsigned long)bit4*16777216);
	bit3=(unsigned int)x/65536;
	x=x-(unsigned long)((unsigned long)bit3*65536);
	bit2=(unsigned int)x/256;
	x=x-(unsigned long)((unsigned long)bit2*256);
	bit1=(unsigned int)x;

	fprintf(strm,"%c%c%c%c",bit1,bit2,bit3,bit4);
}

unsigned long int CCryptFile::streamToLong(FILE* strm) {
	unsigned char dataread;
	unsigned char tmpchar;
	unsigned long int result=0;

	dataread=fscanf(strm,"%c",&tmpchar);
	if (dataread==1)
		result+=((unsigned long int)tmpchar*1);
	dataread=fscanf(strm,"%c",&tmpchar);
	if (dataread==1)
		result+=((unsigned long int)tmpchar*256);
	dataread=fscanf(strm,"%c",&tmpchar);
	if (dataread==1)
		result+=((unsigned long int)tmpchar*65536);
	dataread=fscanf(strm,"%c",&tmpchar);
	if (dataread==1)
		result+=((unsigned long int)tmpchar*16777216);

	return result;

}

unsigned int CCryptFile::calcEOLpad(int bitlen, long linelen) {
	unsigned long int bitsperline;
	unsigned int      padder;

	bitsperline=(unsigned long)bitlen*(unsigned long)linelen;
	padder=bitsperline%32;
	padder=32-padder;

	if (padder==32)
		padder=0;

	return (int)(padder/8);
}

PIXEL CCryptFile::getIndexedPixel(unsigned long int idx, BITMAP_t* bmp) {
	unsigned long int offset;
	PIXEL    result;
	unsigned int EOLpad;
	unsigned long int x;
	unsigned long int y;

	EOLpad=calcEOLpad(24,bmp->x_size);
	x=idx%(bmp->x_size);
	y=(idx-x)/(bmp->x_size);

	y=(bmp->y_size-1)-y;

	offset=(y*((bmp->x_size*3)+EOLpad))+(x*3);

	result.blue=*(bmp->raster->dataspace+(offset++));
	result.green=*(bmp->raster->dataspace+(offset++));
	result.red=*(bmp->raster->dataspace+(offset));

	return result;
}

void CCryptFile::setIndexedPixel(unsigned long int idx, BITMAP_t* bmp, PIXEL pix) {
	unsigned long int offset;
	unsigned int EOLpad;
	unsigned long int x;
	unsigned long int y;

	EOLpad=calcEOLpad(24,bmp->x_size);
	x=idx%(bmp->x_size);
	y=(idx-x)/(bmp->x_size);

	y=(bmp->y_size-1)-y;

	offset=(y*((bmp->x_size*3)+EOLpad))+(x*3);

	*(bmp->raster->dataspace+(offset++))=pix.blue;
	*(bmp->raster->dataspace+(offset++))=pix.green;
	*(bmp->raster->dataspace+(offset))=pix.red;

}

/****************DATA CODING **********************************/
void CCryptFile::encodeData(BITMAP_t* dest, HEAP* data, LOOKUP* hotspots) {
	/* encodes data bit by bit into dest */
	/* hotspots contains actual data storage locations */

	unsigned long int i;
	unsigned long int heapitem;
	PIXEL    blititem;
	int bitshift;
	unsigned char theBool;
	unsigned char dom;

	bitshift=0;
	heapitem=0;

	for (i=0; i<hotspots->curitem; i++) {
		/* get next bit */
		theBool=getNextHeapBit(heapitem,bitshift,data);
		bitshift++;
		if (bitshift==8) {
			bitshift=0;
			heapitem++;
			if (heapitem>data->heaplen)
				break;
		}
		blititem=getIndexedPixel(*(hotspots->dataspace+i),dest);
		dom=biggest_of_3(blititem.red, blititem.green, blititem.blue);
		if (dom==blititem.red) {
			if (theBool==1) {
				/* R */
				if (dom==255)
					dom--;
				else
					dom++;
				blititem.red=dom;
			}
		}
		else if (dom==blititem.green) {
			if (theBool==1) {
				/* G */
				if (dom==255)
					dom--;
				else
					dom++;
				blititem.green=dom;
			}
		}
		else {
			if (theBool==1) {
				/* B */
				if (dom==255)
					dom--;
				else
					dom++;
				blititem.blue=dom;
			}
		}
		setIndexedPixel(*(hotspots->dataspace+i),dest,blititem);
	}
}


unsigned char CCryptFile::getNextHeapBit (unsigned long int item, int bit, HEAP* data) {
	unsigned char thisitem;
	int bitshift;

	bit=7-bit;      /* reverse bit order */

	thisitem=*(data->dataspace+item);
	bitshift=pow(2,bit);

	thisitem=thisitem&bitshift;
	thisitem=thisitem>>bit;

	return thisitem;
}

int CCryptFile::decodeData(BITMAP_t* src, HEAP* dest) {
	unsigned long offset,tmp;
	unsigned char thisR;
	unsigned char thisG;
	unsigned char thisB;
	unsigned char lookaheadR;
	unsigned char lookaheadG;
	unsigned char lookaheadB;
	int dom,lookaheaddom;
	int keepflag,keepflag2;
	unsigned long int hits;
	PIXEL getdata;
	PIXEL controlPixel, thisPixel;
	int theBool;
	unsigned long int longtmp;
	int currentBit;
	int bitshifts;

	offset=0;
	currentBit=0;
	bitshifts=0;

	while (offset<(src->x_size*src->y_size)) {
		getdata=getIndexedPixel(offset, src);
		thisR=getdata.red;
		thisG=getdata.green;
		thisB=getdata.blue;

		dom=biggest_of_3(thisR,thisG,thisB);
		keepflag=FALSE;

		/* If 1 col varies from others by at least 3 => candidate */
		if (dom==thisG && abs(thisG-thisR)>2 && abs(thisG-thisB)>2)
			keepflag=TRUE;
		if (dom==thisB && abs(thisB-thisR)>2 && abs(thisB-thisG)>2)
			keepflag=TRUE;
		if (dom==thisR && abs(thisR-thisB)>2 && abs(thisR-thisG)>2)
			keepflag=TRUE;
		if (keepflag==FALSE) {
			offset++;
			continue;
		}

		/* only one of the 3 cols=dom here, and that col is at least
		   3 points greater than the others */

		/* see if dominant col continues on more pixels(+ or - 1) and that other
		   cols dont challenge its dominance */

		tmp=offset+1;
		hits=1;
		while (tmp<(src->x_size*src->y_size)) {
			getdata=getIndexedPixel(tmp, src);
			lookaheadR=getdata.red;
			lookaheadG=getdata.green;
			lookaheadB=getdata.blue;
			keepflag=FALSE;
			if (dom==thisG && abs(lookaheadG-thisG)<2)
				keepflag=TRUE;
			if (dom==thisR && abs(lookaheadR-thisR)<2)
				keepflag=TRUE;
			if (dom==thisB && abs(lookaheadB-thisB)<2)
				keepflag=TRUE;

			/* check dom is still dominant col by at least 3 */
			lookaheaddom=biggest_of_3(lookaheadR,lookaheadG,lookaheadB);
			keepflag2=FALSE;
			if (thisR==dom && lookaheadR==lookaheaddom && abs(lookaheadR-lookaheadG)>2 && abs(lookaheadR-lookaheadB)>2)
				keepflag2=TRUE;
			if (thisG==dom && lookaheadG==lookaheaddom && abs(lookaheadG-lookaheadR)>2 && abs(lookaheadG-lookaheadB)>2)
				keepflag2=TRUE;
			if (thisB==dom && lookaheadB==lookaheaddom && abs(lookaheadB-lookaheadR)>2 && abs(lookaheadB-lookaheadG)>2)
				keepflag2=TRUE;

			if (keepflag==FALSE || keepflag2==FALSE)
				break;
			hits++;
			tmp++;
		}

		if (hits>2) {
			/* store in lookup table */
			for (longtmp=offset+2; longtmp<offset+hits; longtmp++) {
				controlPixel=getIndexedPixel(offset,src);
				thisPixel=getIndexedPixel(longtmp,src);
				dom=biggest_of_3(controlPixel.red, controlPixel.green, controlPixel.blue);
				if (dom==controlPixel.red) {
					if (thisPixel.red-dom==0)
						theBool=0;
					else
						theBool=1;
				}
				if (dom==controlPixel.green) {
					if (thisPixel.green-dom==0)
						theBool=0;
					else
						theBool=1;
				}
				if (dom==controlPixel.blue) {
					if (thisPixel.blue-dom==0)
						theBool=0;
					else
						theBool=1;
				}
				currentBit=currentBit<<1;
				currentBit+=theBool;
				bitshifts++;
				if (bitshifts==8) {
					if (!putToHeap((char)currentBit,dest))
						return 2;
					bitshifts=0;
					currentBit=0;
				}
			}
			offset+=hits;
		}
		else {
			offset++;
		}
	}

	/* decoded and in heap */
	if (dest->heaplen<13)
		return 1;
	if (!(*(dest->dataspace)=='B') || (*(dest->dataspace+1)!='S' && *(dest->dataspace+1)!='E'))
		return 1;

	if (*(dest->dataspace+6)>THIS_PROTO_VER)
		return 3;
	/* set data length by truncation */
	longtmp=0;
	longtmp+=((unsigned long int)(*(dest->dataspace+2))*1);
	longtmp+=((unsigned long int)(*(dest->dataspace+3))*256);
	longtmp+=((unsigned long int)(*(dest->dataspace+4))*65536);
	longtmp+=((unsigned long int)(*(dest->dataspace+5))*16777216);
	if (longtmp<=dest->heaplen)
		dest->heaplen=longtmp;
	else
		return 1;

	return 0;
}

/*************************** ENCRYPTION ROUTINES ***********************/
int CCryptFile::hash_func(unsigned char* password) {

	/* This hash function steps thru the string, storing the     */
	/* lowest 4 bits, and shifting the running total left 4 bits */
	/* The shift is a rotational one, e.g. the top nibble comes  */
	/* back in the bottom (LSB) nibble                           */


	unsigned int    hash_ttl=0;
	unsigned int    ptr=0;
	char            this_chr;

	while (password[ptr] != '\0') {

		/* Step through string one char at a time */
		this_chr=password[ptr++];

		/* Add lowest 4 bits of character */
		hash_ttl=hash_ttl + (this_chr & 0x0f);

		/* Rotate hash total 4 bits to left (to MSB) */
		hash_ttl = rotl4 (hash_ttl);
	}


	return (hash_ttl);
}

int CCryptFile::rotl4(int inval) {

	/* Rotate 16bit integer INVAL left by 4 bits */

	unsigned int result;
	unsigned int tmp;

	tmp = inval & 0xf000;           /* MSB nibble of the 4         */
	result = inval & 0xfff;         /* 3 LSB nibbles of the 4      */
	result <<= 4;                   /* Shift nibs 2-0 to 3-1 posns */
	tmp >>= 12;                     /* Shift nib 3 to nib 0 posn   */

	result = result + tmp;          /* Add nib 0 into 3-1 value    */
	return (result);

}

void CCryptFile::cryptoData(HEAP* data, unsigned char* password) {
	unsigned long int i;
	unsigned int      passhash;
	unsigned char     pupper,plower;
	int alternhash=FALSE;


	passhash=hash_func(password);

	pupper=passhash&0xff00;
	plower=passhash&0xff;

	*(data->dataspace+1)='E';

	for (i=7; i<data->heaplen; i++) {
		if (alternhash) {
			*(data->dataspace+i)=xor(*(data->dataspace+i),pupper);
			alternhash=FALSE;
		}
		else {
			*(data->dataspace+i)=xor(*(data->dataspace+i),plower);
			alternhash=TRUE;
		}
	}


}

unsigned char CCryptFile::xor(unsigned char A, unsigned char B) {
	unsigned char t1,t2,t3;

	t1=A|B;
	t2=A&B;
	t3=t1-t2;
	return ~t3;

}

/************************************POW******************************/

int CCryptFile::pow (int a, int b) {

	int result=1;
	int lp;

	for (lp=b; lp>0; lp--) {
		result=result*a;
	}

       return result;

}

/*******************************STRICMP*******************************/
int CCryptFile::stricmp (const char* a, const char* b) {

	int i;
	char acmp,bcmp;

	if (strlen(a)!=strlen(b))
		return 1;

	for (i=0; i<strlen(a); i++) {
		acmp=a[i];
		bcmp=b[i];
		if (acmp>='a' && acmp<='z')
			acmp=acmp-0x20;
		if (bcmp>='a' && bcmp<='z')
			bcmp=bcmp-0x20;
		if (acmp!=bcmp)
			return 1;
	}
	return 0;


} 

/**********************ARCHIVE MANAGER*******************************/
int CCryptFile::formatDataspace(BITMAP_t* BMPworking, HEAP* filespace) {

	LOOKUP*         hotspots;
	unsigned long int       bit1,bit2,bit3,bit4;
	unsigned long int       maxencode,longtemp,i;

	putToHeap('B',filespace);
	putToHeap('S',filespace);
	putToHeap('\0',filespace);      /* archive total length inc all hdr:- changed later LONG */
	putToHeap('\0',filespace);
	putToHeap('\0',filespace);
	putToHeap('\0',filespace);
	putToHeap(THIS_PROTO_VER,filespace);
	putToHeap('O',filespace);
	putToHeap('K',filespace);
	putToHeap('\0',filespace);      /* length of this file block inc filename hdr */
	putToHeap('\0',filespace);
	putToHeap('\0',filespace);
	putToHeap('\0',filespace);

	hotspots=(LOOKUP*)calloc(1,sizeof(LOOKUP));
	if (hotspots==NULL)
		return FALSE;

	hotspots->dataspace=(unsigned long int*)calloc(LOOKUP_INITLEN,sizeof(unsigned long int));
	if (hotspots==NULL)
		return FALSE;

	hotspots->currentlen=LOOKUP_INITLEN;
	maxencode=resolveMaxEncode(BMPworking, hotspots);

	if (maxencode<13)
		return FALSE;

	longtemp=maxencode;
	bit4=(unsigned int)longtemp/16777216;
		longtemp=longtemp-(unsigned long)((unsigned long)bit4*16777216);
	bit3=(unsigned int)longtemp/65536;
		longtemp=longtemp-(unsigned long)((unsigned long)bit3*65536);
	bit2=(unsigned int)longtemp/256;
		longtemp=longtemp-(unsigned long)((unsigned long)bit2*256);
	bit1=(unsigned int)longtemp;
	*(filespace->dataspace+2)=bit1;
	*(filespace->dataspace+3)=bit2;
	*(filespace->dataspace+4)=bit3;
	*(filespace->dataspace+5)=bit4;

	for (i=filespace->heaplen; i<maxencode; i++)
		putToHeap('\0',filespace);

	/* encode blank data into bitmap */
	encodeData(BMPworking,filespace,hotspots);

	return TRUE;
}


void CCryptFile::dumpFileDirectory(HEAP* filespace) {

	unsigned long int offset,filelen;
	unsigned long int filectr,bytectr;
	int filenamelen;

	offset=9;
	filectr=0;
	bytectr=0;

	printf("\nFilename          Size (bytes)\n--------          ------------\n");
	for (;;) {
		filelen=0;
		filelen+=((unsigned long int)(*(filespace->dataspace+(offset++)))*1);
		filelen+=((unsigned long int)(*(filespace->dataspace+(offset++)))*256);
		filelen+=((unsigned long int)(*(filespace->dataspace+(offset++)))*65536);
		filelen+=((unsigned long int)(*(filespace->dataspace+(offset++)))*16777216);
		if (filelen==0) 
			break;
		filenamelen=strlen((const char*)filespace->dataspace+offset)+1;
		printf("%-18s%lu\n",filespace->dataspace+offset,filelen-filenamelen);
		offset=offset+filelen;  
		filectr++;
		bytectr+=filelen-filenamelen;
	}
	if (filectr==0)
		printf("No files found in archive!\n");
	else
		printf("\nTotal %lu byte(s), in %lu file(s)\n",bytectr,filectr);
}


void CCryptFile::dumpFileStats(BITMAP_t* BMPworking, HEAP* filespace) {

	unsigned long int offset,filelen,totalsize;
	unsigned long int filectr,bytectr;
	unsigned long int maxencode;
	float             perc_used;
	LOOKUP*           hotspots;

	if (filespace->heaplen==0) {
		hotspots=(LOOKUP*)calloc(1,sizeof(LOOKUP));             
		hotspots->dataspace=(unsigned long int*)calloc(LOOKUP_INITLEN,sizeof(unsigned long int));
		hotspots->currentlen=LOOKUP_INITLEN;
		maxencode=resolveMaxEncode(BMPworking, hotspots);
		free(hotspots);

		printf("The predicted storage capacity for this image is %lu byte(s) [%d Kb]\n",maxencode,(int)((float)maxencode/1024));
	}
	else {

		totalsize=0;
		totalsize+=((unsigned long int)(*(filespace->dataspace+2))*1);
		totalsize+=((unsigned long int)(*(filespace->dataspace+3))*256);
		totalsize+=((unsigned long int)(*(filespace->dataspace+4))*65536);
		totalsize+=((unsigned long int)(*(filespace->dataspace+5))*16777216);

		offset=9;
		filectr=0;
		bytectr=0;
	
		for (;;) {
			filelen=0;
			filelen+=((unsigned long int)(*(filespace->dataspace+(offset++)))*1);
			filelen+=((unsigned long int)(*(filespace->dataspace+(offset++)))*256);
			filelen+=((unsigned long int)(*(filespace->dataspace+(offset++)))*65536);
			filelen+=((unsigned long int)(*(filespace->dataspace+(offset++)))*16777216);
			bytectr+=filelen+4;
			if (filelen==0) 
				break;
			offset=offset+filelen;  
			filectr++;                      
		}

		bytectr+=9;     

		perc_used=(float)100/totalsize;
		perc_used=perc_used*(float)(totalsize-bytectr);

		printf("\nArchive Found - Capacity %lu byte(s) [%lu Kb]\n\n",totalsize,totalsize/1024);
		printf("Total files - %lu, using %lu byte(s)\n",filectr,bytectr);
		printf("Freespace remaining: %lu byte(s) [%d%%]\n",totalsize-bytectr,(int)perc_used);
	}
}


void CCryptFile::addFileToArchive(HEAP* filespace, byte *buffer, int len, char* filename) {

	HEAP*   filedump;
	int     i;
	unsigned long int offset,filelen,freespace,ilong;
	unsigned long int lastfile,lastfile2;
	unsigned int bit4,bit3,bit2,bit1;

	offset=9;
	
	for (;;) {
		filelen=0;
		filelen+=((unsigned long int)(*(filespace->dataspace+(offset++)))*1);
		filelen+=((unsigned long int)(*(filespace->dataspace+(offset++)))*256);
		filelen+=((unsigned long int)(*(filespace->dataspace+(offset++)))*65536);
		filelen+=((unsigned long int)(*(filespace->dataspace+(offset++)))*16777216);
		lastfile=offset-4;
		if (filelen==0) 
			break;
		if (stricmp(filename,(const char*)(filespace->dataspace+offset))==0)
			return;

		offset=offset+filelen;  
	}

	freespace=(filespace->heaplen)-(lastfile+4);
	
	filedump=(HEAP*)calloc(1,sizeof(HEAP));
	initialiseHeap(filedump);

	/* Write filename */
	for (i=0; i<strlen(filename); i++) {
		putToHeap(filename[i],filedump);
	}
	putToHeap('\0',filedump);

	/* Write file */
	// Add the buffer to the heap
	for (i = 0; i < len; i++)
	{
		if (!putToHeap(*(buffer + i),filedump))
			return;
	}

	if (filedump->heaplen==(strlen(filename)+1)) {
		return;
	}

	/* Write next file length (end of chain) */
	putToHeap('\0',filedump);
	putToHeap('\0',filedump);
	putToHeap('\0',filedump);
	putToHeap('\0',filedump);

	if (freespace<filedump->heaplen) {
		return;
	}

	for (ilong=0; ilong<filedump->heaplen; ilong++) {
		*(filespace->dataspace+offset+ilong)=*(filedump->dataspace+ilong);
	}

	lastfile2=filedump->heaplen-4;
	bit4=(unsigned int)lastfile2/16777216;
	lastfile2=lastfile2-(unsigned long)((unsigned long)bit4*16777216);
	bit3=(unsigned int)lastfile2/65536;
	lastfile2=lastfile2-(unsigned long)((unsigned long)bit3*65536);
	bit2=(unsigned int)lastfile2/256;
	lastfile2=lastfile2-(unsigned long)((unsigned long)bit2*256);
	bit1=(unsigned int)lastfile2;

	*(filespace->dataspace+lastfile)=bit1;
	*(filespace->dataspace+lastfile+1)=bit2;
	*(filespace->dataspace+lastfile+2)=bit3;
	*(filespace->dataspace+lastfile+3)=bit4;

	removeHeap(filedump);
	free(filedump); 
}

int CCryptFile::extractFile(HEAP* filespace, char* criteria, int len, byte *buffer) {

	unsigned long int offset,filelen,filectr;
	unsigned long int extractctr,i;
	int               alwaysoverwrite;
	int               extractIt;
	int               wasprompt;
	int               filenamelen;

	offset=9;
	extractctr=0;
	filectr=0;
	alwaysoverwrite=FALSE;

	for (;;) {
		filelen=0;
		filelen+=((unsigned long int)(*(filespace->dataspace+(offset++)))*1);
		filelen+=((unsigned long int)(*(filespace->dataspace+(offset++)))*256);
		filelen+=((unsigned long int)(*(filespace->dataspace+(offset++)))*65536);
		filelen+=((unsigned long int)(*(filespace->dataspace+(offset++)))*16777216);
		if (filelen==0) 
			break;
		filectr++;
		if (criteria==NULL || stricmp((const char*)filespace->dataspace+offset,criteria)==0) {
			/* extract this file */
			extractIt=TRUE;
			wasprompt=FALSE;
			if (extractIt) {
				/* Do the actual extraction */
				extractctr++;
				//outfile=fopen((const char *)filespace->dataspace+offset,"wb");
				filenamelen=strlen((const char *)filespace->dataspace+offset)+1;
				while ((*(filespace->dataspace+offset))!='\0') {
					offset++;
				}
				offset++;
				//int j = 0;
				// Make sure the buffer has enough room
				if (filelen-filenamelen > len)
					return FALSE;

				for (i=0; i<filelen-filenamelen; i++)
					*(buffer + i) = *(filespace->dataspace+offset+i);
					//j += sprintf(buffer + j,"%c",*(filespace->dataspace+offset+i));
				//fclose(outfile);
				offset=offset+(filelen-filenamelen);
			}
			else {
				offset+=filelen;  
			}
		}
		else {
			offset+=filelen;
		}
	}


	if (filectr==0) {
		return FALSE;
	}
	else {
		if (extractctr==0)
			return FALSE;
	}
	return extractctr;
}

int CCryptFile::checkExists(char* filename) {
	FILE* tester;

	if ((tester=fopen(filename,"rb"))) {
		fclose(tester);
		return TRUE;
	}

	return FALSE;
}

int CCryptFile::fileSize (HEAP* filespace, char* criteria) {

	unsigned long int offset,filelen,filectr;
	unsigned long int extractctr;
	int               alwaysoverwrite;
	int               extractIt;
	int               wasprompt;
	int               filenamelen;

	offset=9;
	extractctr=0;
	filectr=0;
	alwaysoverwrite=FALSE;

	for (;;) {
		filelen=0;
		filelen+=((unsigned long int)(*(filespace->dataspace+(offset++)))*1);
		filelen+=((unsigned long int)(*(filespace->dataspace+(offset++)))*256);
		filelen+=((unsigned long int)(*(filespace->dataspace+(offset++)))*65536);
		filelen+=((unsigned long int)(*(filespace->dataspace+(offset++)))*16777216);
		if (filelen==0) 
			break;
		filectr++;
		if (criteria==NULL || stricmp((const char*)filespace->dataspace+offset,criteria)==0) {
			/* extract this file */
			extractIt=TRUE;
			wasprompt=FALSE;
			if (extractIt) {
				/* Do the actual extraction */
				extractctr++;
				//outfile=fopen((const char *)filespace->dataspace+offset,"wb");
				filenamelen=strlen((const char *)filespace->dataspace+offset)+1;

				return filelen-filenamelen;
			}
			else {
				offset+=filelen;  
			}
		}
		else {
			offset+=filelen;
		}
	}

	return 0;
}

void CCryptFile::flattenBitmap(BITMAP_t* src, LOOKUP* store) {

	/* passes over data as if reading it - but just flattens all data to '0' bits and
	   compiles a hotspots list */

	unsigned long offset,tmp;
	unsigned char thisR;
	unsigned char thisG;
	unsigned char thisB;
	unsigned char lookaheadR;
	unsigned char lookaheadG;
	unsigned char lookaheadB;
	int dom,lookaheaddom;
	int keepflag,keepflag2;
	unsigned long int hits;
	PIXEL getdata;
	PIXEL controlPixel, thisPixel;
	unsigned long int longtmp;
	unsigned long int longtmp2;
	unsigned long int *longptrtmp;

	offset=0;

	while (offset<(src->x_size*src->y_size)) {
		getdata=getIndexedPixel(offset, src);
		thisR=getdata.red;
		thisG=getdata.green;
		thisB=getdata.blue;

		dom=biggest_of_3(thisR,thisG,thisB);
		keepflag=FALSE;

		/* If 1 col varies from others by at least 3 => candidate */
		if (dom==thisG && abs(thisG-thisR)>2 && abs(thisG-thisB)>2)
			keepflag=TRUE;
		if (dom==thisB && abs(thisB-thisR)>2 && abs(thisB-thisG)>2)
			keepflag=TRUE;
		if (dom==thisR && abs(thisR-thisB)>2 && abs(thisR-thisG)>2)
			keepflag=TRUE;
		if (keepflag==FALSE) {
			offset++;
			continue;
		}

		/* only one of the 3 cols=dom here, and that col is at least
		   3 points greater than the others */

		/* see if dominant col continues on more pixels(+ or - 1) and that other
		   cols dont challenge its dominance */

		tmp=offset+1;
		hits=1;
		while (tmp<(src->x_size*src->y_size)) {
			getdata=getIndexedPixel(tmp, src);
			lookaheadR=getdata.red;
			lookaheadG=getdata.green;
			lookaheadB=getdata.blue;
			keepflag=FALSE;
			if (dom==thisG && abs(lookaheadG-thisG)<2)
				keepflag=TRUE;
			if (dom==thisR && abs(lookaheadR-thisR)<2)
				keepflag=TRUE;
			if (dom==thisB && abs(lookaheadB-thisB)<2)
				keepflag=TRUE;

			/* check dom is still dominant col by at least 3 */
			lookaheaddom=biggest_of_3(lookaheadR,lookaheadG,lookaheadB);
			keepflag2=FALSE;
			if (thisR==dom && lookaheadR==lookaheaddom && abs(lookaheadR-lookaheadG)>2 && abs(lookaheadR-lookaheadB)>2)
				keepflag2=TRUE;
			if (thisG==dom && lookaheadG==lookaheaddom && abs(lookaheadG-lookaheadR)>2 && abs(lookaheadG-lookaheadB)>2)
				keepflag2=TRUE;
			if (thisB==dom && lookaheadB==lookaheaddom && abs(lookaheadB-lookaheadR)>2 && abs(lookaheadB-lookaheadG)>2)
				keepflag2=TRUE;

			if (keepflag==FALSE || keepflag2==FALSE)
				break;
			hits++;
			tmp++;
		}

		if (hits>2) {
			for (longtmp=offset+2; longtmp<offset+hits; longtmp++) {
				controlPixel=getIndexedPixel(offset,src);
				thisPixel=getIndexedPixel(longtmp,src);
				dom=biggest_of_3(controlPixel.red, controlPixel.green, controlPixel.blue);
				if (dom==controlPixel.red) {
					thisPixel.red=controlPixel.red;
				}
				if (dom==controlPixel.green) {
					thisPixel.green=controlPixel.green;
				}
				if (dom==controlPixel.blue) {
					thisPixel.blue=controlPixel.blue;
				}
				setIndexedPixel(longtmp,src,thisPixel);

				/* store in lookup table */
				if (store->curitem>=store->currentlen) {
					longptrtmp=store->dataspace;
					store->dataspace=(unsigned long int*)calloc(2*store->currentlen,sizeof(unsigned long int));
					for (longtmp2=0; longtmp2<store->currentlen; longtmp2++) {
						*(store->dataspace+longtmp2)=*(longptrtmp+longtmp2);
					}
					free (longptrtmp);
					longptrtmp=NULL;
					store->currentlen*=2;
				}
				*(store->dataspace+(store->curitem++))=longtmp;
			}
			offset+=hits;
		}
		else {
			offset++;
		}
	}

}
