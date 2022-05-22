// Author: FragMented!

#ifndef __Rally_Zlib_H__
#define __Rally_Zlib_H__

// Zlib Includes
#ifdef _WIN32
#include "zlib/zlib.h"
// Windows Includes
#include <windows.h>
#else
#include "zlib.h"
#endif


// Class Defintion
class CZLibFuncs {
public:
	// My Funcs
	CZLibFuncs (void);
	~CZLibFuncs (void);
	bool Init (void);

#ifndef _WIN32
	int (*ZLib_Compress2) (Bytef *dest,   uLongf *destLen, const Bytef *source, uLong sourceLen, int level);
	int (*ZLib_Uncompress) (Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);
#else
	// Function Pointers
	int (_stdcall *ZLib_Compress2) (Bytef *dest,   uLongf *destLen, const Bytef *source, uLong sourceLen, int level);
	int (_stdcall *ZLib_Uncompress) (Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);

private:
	HINSTANCE m_hZLib;
#endif
};

extern CZLibFuncs ZLibFuncs;


#endif
