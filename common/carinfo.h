//
// Carinfo.h
//
// Reads information about the cars from a file
//
// by SaRcaZm
//
#if !defined ( CARINFO_H )
#define CARINFO_H
#if defined( _WIN32 )
#pragma once
#endif

typedef unsigned char byte;

class CCarInfo {
public:
	CCarInfo (void);
	CCarInfo (char *filename);
	CCarInfo (char *filecontents, int length);
	~CCarInfo (void);

	// Pointer setting
	void resetPointer (void);

	// Data retrieval
	void getNextManufacturer (char *szManufacturerName);
	void getNextModel (char *szModelName);
	void getNextAttributeString (char *szAttributeName, char *szAttributeValue);

	void loadFromFile(char *filename);

private:
	void skipWhiteSpace (void);

	char *m_pData;
	char m_szFileName[80];
	char *m_pPointer;
};

#endif // CARINFO_H
