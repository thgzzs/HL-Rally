//
// Carinfo.cpp
//
// Reads information about the cars from a file
//
// by SaRcaZm, FragMented
//

#include <stdlib.h>

#ifdef _WIN32
#	include <io.h>
#else
#	include <sys/io.h>
#	include <sys/stat.h>
#	include <unistd.h>
#endif

#include <stdio.h>
#include <string.h>

#include "carinfo.h"


// Constructor
CCarInfo::CCarInfo (void)
{
	loadFromFile("temp_parse.txt");
}

// Constructor
CCarInfo::CCarInfo (char *filecontents, int length)
{/*
	m_pData = (char *) malloc (length+1);
	memcpy (m_pData, filecontents, length);
	m_pData[length] = 0;
	resetPointer();*/

	FILE *pFileTmp = fopen("hlrally/tmp_info.txt", "wt");
	fwrite(filecontents, length, 1, pFileTmp);
	fclose(pFileTmp);

	loadFromFile("tmp_info.txt");
}

// Constructor
CCarInfo::CCarInfo (char *filename)
{
	loadFromFile(filename);
}

CCarInfo::~CCarInfo (void)
{
	// Free the data
	free (m_pData);
}

// Load carinfo from file
void CCarInfo::loadFromFile(char *filename)
{
	char szRelFile[80];

	// Open the file
	strcpy (m_szFileName, filename);

#ifdef _WIN32
	sprintf (szRelFile, "hlrally\\%s", m_szFileName);
#else
	sprintf (szRelFile, "hlrally/%s", m_szFileName);
#endif

	FILE *pFile = fopen (szRelFile, "rt");	// Open in read text mode

	if (pFile == NULL)
		return;

#ifdef _WIN32
	int iFileSize = _filelength (_fileno (pFile));
#else
	struct stat fileStat;
	stat( szRelFile, &fileStat );
	int iFileSize = fileStat.st_size;
	printf( "carinfo filesize %d\n", iFileSize );
#endif

	// Read the entire file
	m_pData = (char *) malloc (iFileSize+1);
	memset (m_pData, 0, iFileSize);
	fread (m_pData, 1, iFileSize, pFile);
    m_pData[iFileSize] = 0;
	fclose (pFile);

	// Set the pointer to the beginning of the file
	resetPointer();
}

// Reset the pointer to the beginning of the file
void CCarInfo::resetPointer (void)
{
	m_pPointer = m_pData;
}

// Skip whitespace in the file
void CCarInfo::skipWhiteSpace (void)
{
	// Read until the end of the line
	while (m_pPointer && *m_pPointer && *m_pPointer != '\n')
		m_pPointer++;

	m_pPointer++;
}

// Gets the name of the next manufacturer in the file
void CCarInfo::getNextManufacturer (char *szManufacturerName)
{
	// Set up for failure
	*szManufacturerName = 0;

	// Skip until we hit the beginning delimiter ('[')
	while (m_pPointer && *m_pPointer && *m_pPointer != '[')
		m_pPointer++;

	if(!m_pPointer)		// If we have hit the end, return
		return;

	m_pPointer++;		// Skip the delimeter

	// Read until we hit the end delimeter (']')
	while (m_pPointer && *m_pPointer && *m_pPointer != ']')
		*szManufacturerName++ = *m_pPointer++;

	*szManufacturerName = 0;	// Terminate the string
}

// Gets the name of the next model in the file
void CCarInfo::getNextModel (char *szModelName)
{
	// Set up for failure
	*szModelName = 0;

	// Skip until we hit the beginning delimiter ('{')
	while (m_pPointer && *m_pPointer && *m_pPointer != '{' && *m_pPointer != '[')
		m_pPointer++;

	if(!m_pPointer || m_pPointer[0] == '[')	// If we have hit the end or a new manufacterer, just return
		return;

	m_pPointer++;	// Skip the delimeter

	// Read until we hit the end delimeter ('}')
	while (m_pPointer && *m_pPointer && *m_pPointer != '}' && *m_pPointer != '[')
		*szModelName++ = *m_pPointer++;

	*szModelName = 0;	// Terminate the string
}

// Get the name and value (in string format) of the next attribute in the file
void CCarInfo::getNextAttributeString (char *szAttributeName, char *szAttributeValue)
{
	// Set up for failure
	*szAttributeName = 0;
	*szAttributeValue = 0;

	// Skip all the whitespace before the attribute
	skipWhiteSpace ();

	// Start reading the attribute name
	while (m_pPointer && *m_pPointer && *m_pPointer != '=' && *m_pPointer != '[' && *m_pPointer != '{')
		*szAttributeName++ = *m_pPointer++;

	szAttributeName[0] = 0;	// Terminate the string

	// If we have hit the end or a new model / manufacturer, just return
	if (!m_pPointer || !*m_pPointer || *m_pPointer == '{' || *m_pPointer == '[')
		return;

	m_pPointer++;	// Skip the = sign

	// Now, read the attribute value
	while (*m_pPointer != 0 && *m_pPointer != '\n' && *m_pPointer != '[' && *m_pPointer != '{')
		*szAttributeValue++ = *m_pPointer++;

	*szAttributeName = 0;
	*szAttributeValue = 0;	// Terminate the string
}
