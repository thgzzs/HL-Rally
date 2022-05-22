// Rally_CryptFile.h
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

#ifndef __Rally_CryptFile_h__
#define __Rally_CryptFile_h__

// Definitions
#define HEAP_UNITSIZE           (102400)
#define HEAP_INITUNITS          (1)
#define LOOKUP_INITLEN          (2048)

#undef FALSE
#undef TRUE

#define FALSE                   (0)
#define TRUE                    (-1)
#define PATH_SEP                ('\\')    /* OS Specific Path Seperator */
#define THIS_PROTO_VER          (1)

/* Heap Structure (memory storage class for uchars) */
typedef struct heapstruct {
  unsigned int   heapunits;               /* Heap size to nearest 'unit' */
  unsigned char  *dataspace;              /* Dataspace where data stored */
  unsigned char  *nextchar;               /* Pointer to end of dataspace */
  unsigned long  heaplen;                 /* Length of data in heap */
} HEAP;

typedef struct lookupstruct {
	unsigned long int*      dataspace;
	unsigned long int       currentlen;
	unsigned long int       curitem;
} LOOKUP;

/* Bitmap Structure */
typedef struct bmpstruct {
  char* signature;
  long  filesize;
  long  x_size;
  long  y_size;
  int   bitplanes;
  int   bpp;
  long  compression;
  long  compresssize;
  long  x_pix_per_metre;
  long  y_pix_per_metre;
  long  colours;
  long  cols_important;
  HEAP* palette;
  HEAP* raster;
} BITMAP_t;

typedef struct pixstruct {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
} PIXEL;

typedef enum {
	MODE_ADD = 1,
	MODE_READ,
	MODE_SPACE,
	MODE_FILES
} modes_t;

typedef unsigned char byte;

class CCryptFile {
public:
	CCryptFile (char *szFileName);
	~CCryptFile (void);

	int GetFileSize (char *szCryptFile, char *szPassword);
	int GetFile (char *szCryptFile, char *szPassword, int iLen, byte *pBuffer);
	int AddFile (char *szCryptFile, char *szPassword, int iLen, byte *pBuffer);
	int GetFreeSpace (char *szPassword);
	int GetContents (char *szPassword);

private:
	HEAP *GetFileSpace (int MODE, char *szPassword);
	void CleanUp (void);

	/* Function prototypes */
	int             initialiseHeap          (HEAP*);
	int             expandHeap              (int, HEAP*);
	void            removeHeap              (HEAP*);
	int             putToHeap               (unsigned char,HEAP*);
	int             streamFileToHeap        (FILE*, HEAP*);
	int             readBitmapFile          (FILE*, BITMAP_t*);
	void            promoteTo24             (BITMAP_t*, BITMAP_t*);
	unsigned long   resolveMaxEncode        (BITMAP_t*, LOOKUP*);
	void            writeBitmapFile         (FILE*, BITMAP_t*);
	unsigned int    biggest_of_3            (unsigned int,unsigned int,unsigned int);
	void            longToStream            (FILE*,unsigned long int);
	unsigned long   streamToLong            (FILE*);
	void            process24gotpixel       (unsigned char, unsigned char*,HEAP*);
	unsigned int    calcEOLpad              (int, long);
	PIXEL           getIndexedPixel         (unsigned long int, BITMAP_t*);
	void            encodeData              (BITMAP_t*, HEAP*, LOOKUP*);
	void            setIndexedPixel         (unsigned long int, BITMAP_t*, PIXEL);
	unsigned char   getNextHeapBit          (unsigned long int, int, HEAP*);
	int             decodeData              (BITMAP_t*, HEAP*);
	void            cryptoData              (HEAP* data, unsigned char* key);
	unsigned char   xor                     (unsigned char, unsigned char);
	int             rotl4                   (int);
	int             hash_func               (unsigned char*);
	int             pow                     (int,int);
	int             stricmp                 (const char*, const char*);
	int             formatDataspace         (BITMAP_t*, HEAP*);
	void            dumpFileDirectory       (HEAP*);
	void            dumpFileStats           (BITMAP_t*, HEAP*);
	void            addFileToArchive        (HEAP*, byte*, int, char*);
	int             checkExists             (char*);
	int             extractFile             (HEAP*, char*, int, byte*);
	int				fileSize				(HEAP* filespace, char* criteria);
	void            flattenBitmap           (BITMAP_t*, LOOKUP*);

	// Variables
	FILE *m_pFile;
	char m_szFileName[80];

	FILE*                   bitmap_handle;
	FILE*                   outfile_handle;
	BITMAP_t*                 BMPsource;
	BITMAP_t*                 BMPworking;
	HEAP*                   filespace;
	LOOKUP*                 hotspots;
	unsigned long int       i;
	bool                    isencrypted;
};

#endif //__Rally_CryptFile_h__