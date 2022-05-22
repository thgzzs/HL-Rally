// Rally_CRC.h
// Code by SaRcaZm

#ifndef __Rally_CRC_h__
#define __Rally_CRC_h__

typedef unsigned long ULONG;
typedef unsigned char byte;

class CDataCRC32 {
public:
	CDataCRC32 (char *szFileName);
	CDataCRC32 (byte *pData, int iSize);
	~CDataCRC32 (void);

	GetFileCRC (void);
	GetFileSize (void);

private:
	void Init_CRC32_Table(void);
	ULONG Reflect(ULONG ref, char ch);
	int Get_CRC(byte *szText, int iLen);

	byte *m_pBuffer;
	int m_iSize;
	ULONG crc32_table[256]; // Lookup table array
};

#endif //__Rally_CRC_h__