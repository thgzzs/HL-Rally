// Rally_CRC.cpp
// Code by SaRcaZm
// Some code borrowed from http://www.createwindow.com/programming/crc32/crctext.htm

// Includes
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <io.h>
#include <string.h>

#include "Rally_CRC.h"

// Class Functions
CDataCRC32::CDataCRC32 (char *szFileName)
{
	FILE *m_pFile;

	m_pFile = NULL;
	Init_CRC32_Table ();

	m_pFile = fopen (szFileName, "rb");

	if (!m_pFile)
	{
		m_pBuffer = NULL;
		return;
	}

	m_iSize = _filelength (_fileno (m_pFile));
	m_pBuffer = (byte *)malloc (m_iSize+1);
	memset (m_pBuffer, 0, m_iSize+1);
	fread (m_pBuffer, m_iSize, 1, m_pFile);

	fclose (m_pFile);
}

CDataCRC32::CDataCRC32 (byte *pData, int iSize)
{
	m_pBuffer = pData;
	m_iSize = iSize;
}

CDataCRC32::~CDataCRC32 (void)
{
	if (m_pBuffer != NULL)
		free (m_pBuffer);
}

int CDataCRC32::GetFileCRC (void)
{
	if (m_pBuffer == NULL)
		return -1;

	return Get_CRC (m_pBuffer, m_iSize);
}

int CDataCRC32::GetFileSize (void)
{
	if (m_pBuffer == NULL)
		return -1;

	return m_iSize;
}

// Call this function only once to initialize the CRC table.
void CDataCRC32::Init_CRC32_Table()
{
	ULONG ulPolynomial = 0x04c11db7;

	// 256 values representing ASCII character codes.
	for(int i = 0; i <= 0xFF; i++)
	{
		crc32_table[i]=Reflect(i, 8) << 24;
		for (int j = 0; j < 8; j++)
			crc32_table[i] = (crc32_table[i] << 1) ^ (crc32_table[i] & (1 << 31) ? ulPolynomial : 0);
		crc32_table[i] = Reflect(crc32_table[i], 32);
	}
}

ULONG CDataCRC32::Reflect(ULONG ref, char ch)
{
	ULONG value(0);

	// Swap bit 0 for bit 7, bit 1 for bit 6, etc.
	for(int i = 1; i < (ch + 1); i++)
	{
		if(ref & 1)
			value |= 1 << (ch - i);
		ref >>= 1;
	}
	return value;
}

int CDataCRC32::Get_CRC(byte *szText, int iLen)
{
	// Start out with all bits set high.
	ULONG  ulCRC(0xffffffff);
	byte* buffer = (byte *)szText;
	while(iLen--)
		ulCRC = (ulCRC >> 8) ^ crc32_table[(ulCRC & 0xFF) ^ *buffer++];

	// Exclusive OR the result with the beginning value.
	return ulCRC ^ 0xffffffff;
}
