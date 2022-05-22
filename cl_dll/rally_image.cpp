//
// Rally_Image.cpp
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <malloc.h>

#include "mmgr.h"

// N.B. I haven't include "rally_vgui.h" for a reason
typedef unsigned char byte;

int fgetLittleShort (FILE *f)
{
	byte	b1, b2;

	b1 = fgetc(f);
	b2 = fgetc(f);

	return (short)(b1 + b2*256);
}

int fgetLittleLong (FILE *f)
{
	byte	b1, b2, b3, b4;

	b1 = fgetc(f);
	b2 = fgetc(f);
	b3 = fgetc(f);
	b4 = fgetc(f);

	return b1 + (b2<<8) + (b3<<16) + (b4<<24);
}

unsigned long RGBAto24 (byte r, byte g, byte b, byte a)
{
	return (a << 24) | (r << 16) | (g << 8) | b;
}

void _24toRGBA (unsigned long l, byte *r, byte *g, byte* b, byte *a)
{
	*a = (byte) (l >> 24);
	*r = (byte) ((l >> 16) & 0xFF);
	*g = (byte) ((l >> 8) & 0xFF);
	*b = (byte) (l & 0xFF);
}

/*
=========================================================

TARGA LOADING

=========================================================
*/
typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;



byte *LoadTGA (char *filename, int *width, int *height)
{
	int				columns, rows, numPixels;
	byte			*pixbuf;
	int				row, column;

	TargaHeader		targa_header;
	byte			*targa_rgba = NULL;

	FILE *fin = fopen (filename, "rb");
	if (!fin)
		return NULL;

	targa_header.id_length = fgetc(fin);
	targa_header.colormap_type = fgetc(fin);
	targa_header.image_type = fgetc(fin);

	targa_header.colormap_index = fgetLittleShort(fin);
	targa_header.colormap_length = fgetLittleShort(fin);
	targa_header.colormap_size = fgetc(fin);
	targa_header.x_origin = fgetLittleShort(fin);
	targa_header.y_origin = fgetLittleShort(fin);
	targa_header.width = fgetLittleShort(fin);
	targa_header.height = fgetLittleShort(fin);
	targa_header.pixel_size = fgetc(fin);
	targa_header.attributes = fgetc(fin);

	if (targa_header.image_type!=2 
		&& targa_header.image_type!=10)
		return NULL;

	if (targa_header.colormap_type !=0 
		|| (targa_header.pixel_size!=32 && targa_header.pixel_size!=24))
		return NULL;

	*width = (int) targa_header.width;
	*height = (int) targa_header.height;

	columns = targa_header.width;
	rows = targa_header.height;
	numPixels = columns * rows;

	targa_rgba = (byte *) malloc (numPixels*4);
	
	if (targa_header.id_length != 0)
		fseek(fin, targa_header.id_length, SEEK_CUR);  // skip TARGA image comment
	
	if (targa_header.image_type==2) {  // Uncompressed, RGB images
		for(row=rows-1; row>=0; row--) {
			pixbuf = targa_rgba + row*columns*4;
			for(column=0; column<columns; column++) {
				unsigned char red,green,blue,alphabyte;
				switch (targa_header.pixel_size) {
					case 24:
							blue = getc(fin);
							green = getc(fin);
							red = getc(fin);
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = 255;
							break;
					case 32:
							blue = getc(fin);
							green = getc(fin);
							red = getc(fin);
							alphabyte = getc(fin);
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = alphabyte;
							break;
				}
			}
		}
	}
	else if (targa_header.image_type==10) {   // Runlength encoded RGB images
		unsigned char red,green,blue,alphabyte,packetHeader,packetSize,j;
		for(row=rows-1; row>=0; row--) {
			pixbuf = targa_rgba + row*columns*4;
			for(column=0; column<columns; ) {
				packetHeader=getc(fin);
				packetSize = 1 + (packetHeader & 0x7f);
				if (packetHeader & 0x80) {        // run-length packet
					switch (targa_header.pixel_size) {
						case 24:
								blue = getc(fin);
								green = getc(fin);
								red = getc(fin);
								alphabyte = 255;
								break;
						case 32:
								blue = getc(fin);
								green = getc(fin);
								red = getc(fin);
								alphabyte = getc(fin);
								break;
					}
	
					for(j=0;j<packetSize;j++) {
						*pixbuf++=red;
						*pixbuf++=green;
						*pixbuf++=blue;
						*pixbuf++=alphabyte;
						column++;
						if (column==columns) { // run spans across rows
							column=0;
							if (row>0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row*columns*4;
						}
					}
				}
				else {                            // non run-length packet
					for(j=0;j<packetSize;j++) {
						switch (targa_header.pixel_size) {
							case 24:
									blue = getc(fin);
									green = getc(fin);
									red = getc(fin);
									*pixbuf++ = red;
									*pixbuf++ = green;
									*pixbuf++ = blue;
									*pixbuf++ = 255;
									break;
							case 32:
									blue = getc(fin);
									green = getc(fin);
									red = getc(fin);
									alphabyte = getc(fin);
									*pixbuf++ = red;
									*pixbuf++ = green;
									*pixbuf++ = blue;
									*pixbuf++ = alphabyte;
									break;
						}
						column++;
						if (column==columns) { // pixel packet run spans across rows
							column=0;
							if (row>0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row*columns*4;
						}						
					}
				}
			}
			breakOut:;
		}
	}
	
	fclose(fin);

	return targa_rgba;
}

/*
=========================================================

BITMAP LOADING

=========================================================
*/
#define BITMAP_MAGIC_NUMBER 19778	// 'BM'

#define BI_RGB          0
#define BI_RLE8         1
#define BI_RLE4         2
#define BI_BITFIELDS    3

typedef struct tagBitmapFileHeader {
	unsigned short bfType;
	unsigned int bfSize;
	unsigned int bfReserved;
	unsigned int bfOffBits;
} BitmapFileHeader;

typedef struct tagBitmapInfoHeader {
	unsigned int biSize;
	int biWidth;
	int biHeight;
	unsigned short biPlanes;
	unsigned short biBitCount;
	unsigned long biCompression;
	unsigned int biSizeImage;
	int biXPelsPerMeter;
	int biYPelsPerMeter;
	unsigned int biClrUsed;
	unsigned int biClrImportant;
} BitmapInfoHeader;

typedef struct tagRGBQuad {
	byte rgbBlue;
	byte rgbGreen;
	byte rgbRed;
	byte rgbReserved;
} RGBQuad;

byte *LoadBMP (char *filename, int *width, int *height)
{
	int	columns, rows, numPixels;
	BitmapInfoHeader bmih;
	BitmapFileHeader bmfh;
	RGBQuad colours[256];
	unsigned char *rgb;

	FILE *fin = fopen (filename, "rb");
	if (!fin)
		return NULL;

	// Read the header
	bmfh.bfType = fgetLittleShort (fin);
	bmfh.bfSize = fgetLittleLong (fin);
	bmfh.bfReserved = fgetLittleLong (fin);
	bmfh.bfOffBits = fgetLittleLong (fin);

	if (bmfh.bfType != BITMAP_MAGIC_NUMBER)
	{
		fclose (fin);
		return NULL;
	}

	bmih.biSize = fgetLittleLong (fin);
	bmih.biWidth = fgetLittleLong (fin);
	bmih.biHeight = fgetLittleLong (fin);
	bmih.biPlanes = fgetLittleShort (fin);
	bmih.biBitCount = fgetLittleShort (fin);
	bmih.biCompression = fgetLittleLong (fin);
	bmih.biSizeImage = fgetLittleLong (fin);
	bmih.biXPelsPerMeter = fgetLittleLong (fin);
	bmih.biYPelsPerMeter = fgetLittleLong (fin);
	bmih.biClrUsed = fgetLittleLong (fin);
	bmih.biClrImportant = fgetLittleLong (fin);

	// We only deal with certain bitmaps
	if ((bmih.biBitCount < 8) || (bmih.biCompression > BI_RLE8))
	{
		fclose (fin);
		return NULL;
	}

	// Calculate some variables
	columns = bmih.biWidth;
	*width = columns = bmih.biWidth;;

	*height = rows = bmih.biHeight;
	numPixels = columns * rows;
    int numColours = 1 << bmih.biBitCount;
	int dataSize = columns * rows * (bmih.biBitCount / 8);

	// Load the pallette if necessary
	if (bmih.biBitCount == 8)
	{
		for (int i = 0; i < numColours; i++)
		{
			colours[i].rgbBlue		= fgetc (fin);
			colours[i].rgbGreen		= fgetc (fin);
			colours[i].rgbRed		= fgetc (fin);
			colours[i].rgbReserved	= fgetc (fin);
		}
    }

	// Read the pixels
	rgb = (unsigned char *)malloc (dataSize);

	if (rgb == NULL)
	{
		fclose (fin);
		return NULL;
	}

	fread (rgb, sizeof(char), dataSize, fin);
	fclose (fin);

    // Calculate the witdh of the final image in bytes
	int byteWidth, padWidth;
    byteWidth = padWidth = (int)(columns * (bmih.biBitCount / 8));

    // Adjust the width for padding as necessary
	while (padWidth % 4 != 0)
		padWidth++;
	int offset = padWidth - byteWidth;

	//allocate the buffer for the final image data
	int outputSize = rows * columns * 3;
	unsigned char *data = (unsigned char *)malloc (outputSize);

	//exit if there is not enough memory
	if (data == NULL)
	{
		free (rgb);
		return NULL;
	}

	// Convert the image to RGB
	if (bmih.biBitCount == 8)
	{
		byte *full_rgb = NULL;
		// Read the RLE encoded image
		if (bmih.biCompression == BI_RLE8)
		{
			int line = rows - 1, pos;
			bool bEndFile = false, bEndLine;
			byte count, val, *pointer = rgb, *full_rgb_pointer;

			full_rgb = (byte *)malloc (rows * columns);
			if (full_rgb == NULL)
			{
				free (rgb);
				return NULL;
			}

			full_rgb_pointer = full_rgb;
			while (!bEndFile)
			{
				bEndLine = false;
				pos = 0;

				while (!bEndFile && !bEndLine)
				{
					count = *pointer++;
					val = *pointer++;

					if (count > 0)	// Repeat 'val' 'count' times
					{
						for (int j = 0; j < count; j++, pos++)
							*full_rgb_pointer++ = val;
					}
					else	// Special escape sequence
					{
						switch (val)
						{
							case 0:		// End of Line
								bEndLine = true;
								break;

							case 1:		// End of File
								bEndFile = true;
								break;

							case 2:		// Displace Picture
								count = *pointer++;
								val = *pointer++;
								break;

							default:	// Read Absolute
								for (int j = 0; j < val; j++, pos++)
									*full_rgb_pointer++ = *pointer++;

								if (j % 2 == 1)	// Must end on word boundary
									pointer++;
								break;
						}
					}

					if (pos - 1 > columns)
						bEndLine = true;
				}

				// Check for the end of the file
				line--;
				if (line < 0)
					bEndFile = true;
			}

			// Clean up
			byte *tmp = rgb;
			rgb = full_rgb;
			free (tmp);
		}

		// Load the image
		int j, iAdd;
		if (height > 0)
		{
			j = 0;
			iAdd = 3;
		}
		else
		{
			j = outputSize - 3;
			iAdd = -3;
		}

		int iLineLen = 0;
		for (int i = 0; i < dataSize; i++)
		{
			//jump over the padding at the start of a new line
			if ((iLineLen > 0) && (iLineLen % byteWidth == 0) && (bmih.biCompression != BI_RLE8))
			{
				i += offset;
				iLineLen = 0;
			}

			//transfer the data
			*(data + j) = colours[*(rgb + i)].rgbRed;
			*(data + j + 1) = colours[*(rgb + i)].rgbGreen;
			*(data + j + 2) = colours[*(rgb + i)].rgbBlue;
			j += iAdd;
			iLineLen++;
		}
	}
	else if (bmih.biBitCount == 24)
	{
		int j, iAdd;

		if (height > 0)
		{
			j = 0;
			iAdd = 3;
		}
		else
		{
			//count backwards so you start at the front of the image
			//here you can start from the back of the file or the front,
			//after the header  The only problem is that some programs
			//will pad not only the data, but also the file size to
			//be divisible by 4 bytes.
			j = outputSize - 3;
			iAdd = -3;
		}

		int iLineLen = 0;
		for (int i = 0; i < dataSize; i += 3)
		{
			//jump over the padding at the start of a new line
			if ((iLineLen > 0) && (iLineLen % byteWidth == 0))
			{
				i += offset;
				iLineLen = 0;
			}
			
			//transfer the data
			*(data + j + 2) = *(rgb + i);
			*(data + j + 1) = *(rgb + i + 1);
			*(data + j) = *(rgb + i + 2);
			j += iAdd;
			iLineLen += 3;
		}
	}
	else if (bmih.biBitCount == 32)
	{
		int j, iAdd;
		if (height > 0)
		{
			j = 0;
			iAdd = 3;
		}
		else
		{
			j = outputSize - 3;
			iAdd = -3;
		}

		int iLineLen = 0;
		for (int i = 0; i < dataSize; i += 4)
		{
			//jump over the padding at the start of a new line
			if ((iLineLen > 0) && (iLineLen % byteWidth == 0))
			{
				i += offset;
				iLineLen = 0;
			}

			//transfer the data
			*(data + j + 2) = *(rgb + i);
			*(data + j + 1) = *(rgb + i + 1);
			*(data + j) = *(rgb + i + 2);
			j += 3;
			iLineLen += 4;
		}
	}

	// Clean up
	free (rgb);
	return data;
}

/*
=========================================================

IMAGE SCALING

=========================================================
*/


/* Averaging pixel values */

/*
// gray scale
inline unsigned char average(unsigned char a, unsigned char b)
{
	return (unsigned char)( ((int)a + (int)b) >> 1);
}

// 24-bit RGB (a pixel is packed in a 32-bit integer)
inline unsigned long average(unsigned long a, unsigned long b)
{
	return ((a & 0xfefefeffUL) + (b & 0xfefefeffUL)) >> 1;
}
*/

#define average(a, b)   ( ((((a) ^ (b)) & 0xfffefefeL) >> 1) + ((a) & (b)) )

/* Smooth Bresenham, template-based */
void ScaleLineAvg(unsigned long *Target, unsigned long *Source, int SrcWidth, int TgtWidth)
{
	int NumPixels = TgtWidth;
	int IntPart = SrcWidth / TgtWidth;
	int FractPart = SrcWidth % TgtWidth;
	int Mid = TgtWidth / 2;
	int E = 0;
	int skip;
	unsigned long p;

	skip = (TgtWidth < SrcWidth) ? 0 : TgtWidth / (2*SrcWidth) + 1;
	NumPixels -= skip;

	while (NumPixels-- > 0)
	{
		p = *Source;

		if (E >= Mid)
			p = average(p, *(Source+1));

		*Target++ = p;
		Source += IntPart;
		E += FractPart;

		if (E >= TgtWidth)
		{
			E -= TgtWidth;
			Source++;
		} /* if */
	} /* while */

	while (skip-- > 0)
		*Target++ = *Source;
}

/*  Smooth 2D scaling */
void ScaleRectAvg(unsigned long *Target, unsigned long *Source, int SrcWidth, int SrcHeight, int TgtWidth, int TgtHeight)
{
	int NumPixels = TgtHeight;
	int IntPart = (SrcHeight / TgtHeight) * SrcWidth;
	int FractPart = SrcHeight % TgtHeight;
	int Mid = TgtHeight / 2;
	int E = 0;
	int skip;
	unsigned long *ScanLine, *ScanLineAhead;
	unsigned long *PrevSource = NULL;
	unsigned long *PrevSourceAhead = NULL;

	skip = (TgtHeight < SrcHeight) ? 0 : TgtHeight / (2*SrcHeight) + 1;
	NumPixels -= skip;

	ScanLine = (unsigned long *)malloc(TgtWidth*sizeof(unsigned long));
	ScanLineAhead = (unsigned long *)malloc(TgtWidth*sizeof(unsigned long));

	while (NumPixels-- > 0)
	{
		if (Source != PrevSource)
		{
			if (Source == PrevSourceAhead)
			{
				/* the next scan line has already been scaled and stored in
				 * ScanLineAhead; swap the buffers that ScanLine and ScanLineAhead
				 * point to
				 */
				unsigned long *tmp = ScanLine;
				ScanLine = ScanLineAhead;
				ScanLineAhead = tmp;
			}
			else
			{
				ScaleLineAvg(ScanLine, Source, SrcWidth, TgtWidth);
			} /* if */

			PrevSource = Source;
		} /* if */

		if (E >= Mid && PrevSourceAhead != Source+SrcWidth)
		{
			int x;
			ScaleLineAvg(ScanLineAhead, Source+SrcWidth, SrcWidth, TgtWidth);

			for (x = 0; x < TgtWidth; x++)
				ScanLine[x] = average(ScanLine[x], ScanLineAhead[x]);

			PrevSourceAhead = Source + SrcWidth;
		} /* if */

		memcpy(Target, ScanLine, TgtWidth*sizeof(unsigned long));
		Target += TgtWidth;
		Source += IntPart;
		E += FractPart;

		if (E >= TgtHeight)
		{
			E -= TgtHeight;
			Source += SrcWidth;
		} /* if */
	} /* while */

	if (skip > 0 && Source != PrevSource)
		ScaleLineAvg(ScanLine, Source, SrcWidth, TgtWidth);

	while (skip-- > 0)
	{
		memcpy(Target, ScanLine, TgtWidth*sizeof(unsigned long));
		Target += TgtWidth;
	} /* while */

	free(ScanLine);
	free(ScanLineAhead);
}
