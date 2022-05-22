// HL Includes
#include "hud.h"
#include "cl_util.h"
#include "rally_fmod.h"

CFModFuncs::CFModFuncs (void)
{
	m_hFMod = NULL;
}


bool CFModFuncs::Init (void)
{
	char fmodlib[256];

	sprintf( fmodlib, "%s/fmod.dll", gEngfuncs.pfnGetGameDirectory());

	// replace forward slashes with backslashes
	for( int i=0; i < 256; i++ )
	{
		if( fmodlib[i] == '/' )
			fmodlib[i] = '\\';
	}

	m_hFMod = LoadLibrary( fmodlib );

	if( m_hFMod != NULL )
	{
		// FMod Functions
		(FARPROC&)FSOUND_Init = GetProcAddress(m_hFMod, "_FSOUND_Init@12");
		(FARPROC&)FSOUND_SetOutput = GetProcAddress(m_hFMod, "_FSOUND_SetOutput@4");
		(FARPROC&)FSOUND_SetBufferSize = GetProcAddress(m_hFMod, "_FSOUND_SetBufferSize@4");
		(FARPROC&)FSOUND_SetDriver = GetProcAddress(m_hFMod, "_FSOUND_SetDriver@4");
		(FARPROC&)FSOUND_SetMinHardwareChannels = GetProcAddress(m_hFMod, "_FSOUND_SetMinHardwareChannels@4");
		(FARPROC&)FSOUND_SetSFXMasterVolume = GetProcAddress(m_hFMod, "_FSOUND_SetSFXMasterVolume@4");
		(FARPROC&)FSOUND_Update = GetProcAddress(m_hFMod, "_FSOUND_Update@0");
		(FARPROC&)FSOUND_Close = GetProcAddress(m_hFMod, "_FSOUND_Close@0");

		// Channel Functions
		(FARPROC&)FSOUND_GetMaxChannels = GetProcAddress(m_hFMod, "_FSOUND_GetMaxChannels@0");
		(FARPROC&)FSOUND_SetVolume = GetProcAddress(m_hFMod, "_FSOUND_SetVolume@8");
		(FARPROC&)FSOUND_SetVolumeAbsolute = GetProcAddress(m_hFMod, "_FSOUND_SetVolumeAbsolute@8");
		(FARPROC&)FSOUND_SetFrequency = GetProcAddress(m_hFMod, "_FSOUND_SetFrequency@8");
		(FARPROC&)FSOUND_GetFrequency = GetProcAddress(m_hFMod, "_FSOUND_GetFrequency@4");
		(FARPROC&)FSOUND_SetMute = GetProcAddress(m_hFMod, "_FSOUND_SetMute@8");
		(FARPROC&)FSOUND_IsPlaying = GetProcAddress(m_hFMod, "_FSOUND_IsPlaying@4");
		(FARPROC&)FSOUND_SetPaused = GetProcAddress(m_hFMod, "_FSOUND_SetPaused@8");
		(FARPROC&)FSOUND_GetPaused = GetProcAddress(m_hFMod, "_FSOUND_GetPaused@4");
		(FARPROC&)FSOUND_SetPan = GetProcAddress(m_hFMod, "_FSOUND_SetPan@8");

		// Stream Functions
		(FARPROC&)FSOUND_Stream_Open = GetProcAddress(m_hFMod, "_FSOUND_Stream_Open@16");
		(FARPROC&)FSOUND_Stream_Play = GetProcAddress(m_hFMod, "_FSOUND_Stream_Play@8");
		(FARPROC&)FSOUND_Stream_Stop = GetProcAddress(m_hFMod, "_FSOUND_Stream_Stop@4");
		(FARPROC&)FSOUND_Stream_Close = GetProcAddress(m_hFMod, "_FSOUND_Stream_Close@4");
		(FARPROC&)FSOUND_Stream_GetLengthMs = GetProcAddress (m_hFMod, "_FSOUND_Stream_GetLengthMs@4");
		(FARPROC&)FSOUND_Stream_GetLength = GetProcAddress (m_hFMod, "_FSOUND_Stream_GetLength@4");
		(FARPROC&)FSOUND_Stream_GetTime = GetProcAddress (m_hFMod, "_FSOUND_Stream_GetTime@4");
		(FARPROC&)FSOUND_Stream_SetTime = GetProcAddress (m_hFMod, "_FSOUND_Stream_SetTime@8");
		(FARPROC&)FSOUND_Stream_GetOpenState = GetProcAddress (m_hFMod, "_FSOUND_Stream_GetOpenState@4");
		(FARPROC&)FSOUND_Stream_GetMode = GetProcAddress (m_hFMod, "_FSOUND_Stream_GetMode@4");
		(FARPROC&)FSOUND_Stream_GetNumTagFields = GetProcAddress (m_hFMod, "_FSOUND_Stream_GetNumTagFields@8");
		(FARPROC&)FSOUND_Stream_GetTagField = GetProcAddress (m_hFMod, "_FSOUND_Stream_GetTagField@24");

		// Internet Stream Functions
		(FARPROC&)FSOUND_Stream_Net_SetBufferProperties = GetProcAddress (m_hFMod, "_FSOUND_Stream_Net_SetBufferProperties@12");
		(FARPROC&)FSOUND_Stream_Net_SetMetadataCallback = GetProcAddress (m_hFMod, "_FSOUND_Stream_Net_SetMetadataCallback@12");

		// 3D Sound Functions
		(FARPROC&)FSOUND_3D_SetDopplerFactor = GetProcAddress(m_hFMod, "_FSOUND_3D_SetDopplerFactor@4");
		(FARPROC&)FSOUND_3D_SetDistanceFactor = GetProcAddress(m_hFMod, "_FSOUND_3D_SetDistanceFactor@4");
		(FARPROC&)FSOUND_3D_SetRolloffFactor = GetProcAddress(m_hFMod, "_FSOUND_3D_SetRolloffFactor@4");
		(FARPROC&)FSOUND_3D_SetAttributes = GetProcAddress(m_hFMod, "_FSOUND_3D_SetAttributes@12");
		(FARPROC&)FSOUND_3D_Listener_SetAttributes = GetProcAddress(m_hFMod, "_FSOUND_3D_Listener_SetAttributes@32");

		// Sample Functions
		(FARPROC&)FSOUND_Sample_Load = GetProcAddress(m_hFMod, "_FSOUND_Sample_Load@20");
		(FARPROC&)FSOUND_PlaySound = GetProcAddress(m_hFMod, "_FSOUND_PlaySound@8");
		(FARPROC&)FSOUND_StopSound = GetProcAddress(m_hFMod, "_FSOUND_StopSound@4");

		if (!FSOUND_Init || !FSOUND_SetOutput || !FSOUND_SetBufferSize || !FSOUND_SetDriver ||
			!FSOUND_SetMinHardwareChannels || !FSOUND_SetSFXMasterVolume || !FSOUND_Update ||
			!FSOUND_Close || !FSOUND_GetMaxChannels ||
			!FSOUND_SetVolume || !FSOUND_SetVolumeAbsolute || !FSOUND_SetFrequency || !FSOUND_SetMute ||
			!FSOUND_IsPlaying || !FSOUND_SetPaused || !FSOUND_GetPaused || !FSOUND_SetPan ||
			!FSOUND_Stream_Open || !FSOUND_Stream_Play || !FSOUND_Stream_Stop ||
			!FSOUND_Stream_Close || !FSOUND_Stream_GetLengthMs || !FSOUND_Stream_GetTime ||
			!FSOUND_Stream_SetTime || !FSOUND_Stream_GetOpenState ||
			!FSOUND_Stream_GetNumTagFields || !FSOUND_Stream_GetTagField ||
			!FSOUND_3D_SetDopplerFactor || !FSOUND_3D_SetDistanceFactor ||
			!FSOUND_3D_SetRolloffFactor || !FSOUND_3D_SetAttributes || !FSOUND_3D_Listener_SetAttributes ||
			!FSOUND_Sample_Load || !FSOUND_PlaySound || !FSOUND_StopSound)
		{
			FreeLibrary( m_hFMod );
			m_hFMod = NULL;
		}
	}

	return (m_hFMod != NULL);
}

CFModFuncs::~CFModFuncs (void)
{
	if( m_hFMod )
	{
		FSOUND_Close ();

		FreeLibrary (m_hFMod);
		m_hFMod = NULL;
	}
}
