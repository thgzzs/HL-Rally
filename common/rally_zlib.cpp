// HL Includes
#include <stdio.h>
#include "rally_zlib.h"

CZLibFuncs ZLibFuncs;

CZLibFuncs::CZLibFuncs (void)
{
#ifdef _WIN32
	m_hZLib = NULL;
#endif
	ZLib_Compress2 = NULL;
	ZLib_Uncompress = NULL;
}

bool CZLibFuncs::Init (void)
{
#if _WIN32

	char szZLib[] = "hlrally\\zlib.dll";

	m_hZLib = LoadLibrary( szZLib );

	if( m_hZLib != NULL )
	{

		// Sample Functions
		(FARPROC&)ZLib_Compress2 = GetProcAddress (m_hZLib, "compress2");
		(FARPROC&)ZLib_Uncompress = GetProcAddress (m_hZLib, "uncompress");

		if (!ZLib_Compress2)
		{
			FreeLibrary( m_hZLib );
			m_hZLib = NULL;
		}
	}

	return (m_hZLib != NULL);

#else

	ZLib_Compress2 = compress2;
	ZLib_Uncompress = uncompress;

#endif
}

CZLibFuncs::~CZLibFuncs (void)
{
#ifdef _WIN32
	if( m_hZLib )
	{
		FreeLibrary (m_hZLib);
		m_hZLib = NULL;
	}
#endif
}
