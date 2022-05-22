// Rally_AntiHaxor.cpp
// Does the CRC checks on the DLLS
// by SaRcaZm

// Includes
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#ifdef _WIN32
#include <io.h>
#else
#include <sys/io.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <string.h>

#include "Rally_AntiHaxor.h"
#include "Rally_CryptFile.h"
#include "Rally_CRC.h"

#ifdef FILE_CHECK

#define CLIENT_DLL_NAME	"cl_dlls\\client.dll"
#define HL_DLL_NAME		"dlls\\hlrally.dll"
#define BMP_NAME		"gfx\\shell\\splash.bmp"

#else

#define CLIENT_DLL_NAME	"hlrally\\cl_dlls\\client.dll"
#define HL_DLL_NAME		"hlrally\\dlls\\hlrally.dll"
#define BMP_NAME		"hlrally\\gfx\\shell\\splash.bmp"

#endif	// FILE_CHECK

#define NO_CRC_CHECK	// Uncomment this line to skip the CRC check

// Look at all of those #ifdef's
// The things I will do to avoid writing 2 identical procedures...
#ifdef FILE_CHECK
#define NO_CRC_CHECK	// Include the below functions as Fill_xxx_Data
#endif	// FILE_CHECK

#ifdef NO_CRC_CHECK

#ifdef FILE_CHECK
Const_Client_t *Fill_Client_Data (Const_Client_t *pClient)
#else
Const_Client_t *Get_Client_Data (Const_Client_t *pClient)
#endif	// FILE_CHECK
{
	strcpy (pClient->szCarInfo, "carinfo.txt");

	strcpy (pClient->szPlus_Forward,  "+forward");
	strcpy (pClient->szPlus_Back,     "+back");
	strcpy (pClient->szPlus_Left,     "+left");
	strcpy (pClient->szPlus_Right,    "+right");

	strcpy (pClient->szMinus_Forward, "-forward");
	strcpy (pClient->szMinus_Back,    "-back");
	strcpy (pClient->szMinus_Left,    "-left");
	strcpy (pClient->szMinus_Right,   "-right");

	strcpy (pClient->szCvar_Cl_ForwardSpeed, "cl_forwardspeed");
	strcpy (pClient->szCvar_Cl_YawSpeed,     "cl_yawspeed");
	strcpy (pClient->szCvar_M_Pitch,         "m_pitch");
	strcpy (pClient->szCvar_M_Yaw,           "m_yaw");

	strcpy (pClient->szCvarValue_Cl_ForwardSpeed, "10000");
	strcpy (pClient->szCvarValue_Cl_YawSpeed,     "500"); // Changed from 210
	strcpy (pClient->szCvarValue_M_Pitch,         "0.022");
	strcpy (pClient->szCvarValue_M_Yaw,           "0.022");

	return pClient;
}

#ifdef FILE_CHECK
Const_HL_t *Fill_HL_Data (Const_HL_t *pHL)
#else
Const_HL_t *Get_HL_Data (Const_HL_t *pHL)
#endif	// FILE_CHECK
{
	strcpy (pHL->szCarInfo, "carinfo.txt");

	strcpy (pHL->szPrecache_Rocks,       "sprites/r_rocks.spr");
	strcpy (pHL->szPrecache_Dirt,        "sprites/r_dirt.spr");
	strcpy (pHL->szPrecache_Sand,        "sprites/r_sand.spr");
	strcpy (pHL->szPrecache_Snow,        "sprites/r_snow.spr");
	strcpy (pHL->szPrecache_Grass,       "sprites/r_grass.spr");
	strcpy (pHL->szPrecache_Smoke,       "sprites/r_smoke.spr");
	strcpy (pHL->szPrecache_Bomb,        "sprites/hp_bomb.spr");
	strcpy (pHL->szPrecache_Checkpoint,  "voc/checkpoint.wav");
	strcpy (pHL->szPrecache_NextTime,    "voc/nexttime.wav");
	strcpy (pHL->szPrecache_LapComplete, "voc/lapcomplete.wav");
	strcpy (pHL->szPrecache_Warning,     "voc/warning30.wav");
	strcpy (pHL->szPrecache_321,         "voc/321.wav");
	strcpy (pHL->szPrecache_Horn,        "horn.wav");
	strcpy (pHL->szPrecache_Backfire,    "backfire.wav");
	strcpy (pHL->szPrecache_Skid,        "sprites/r_skid.spr");
	strcpy (pHL->szPrecache_Shadow,      "sprites/r_shadow.spr");
	strcpy (pHL->szPrecache_Showroom,    "sprites/showroom_f.spr");

	strcpy (pHL->szMsg_VGUIMenu,       "VGUIMenu");
	strcpy (pHL->szMsg_VGUIClasses,    "VGUIClasses");
	strcpy (pHL->szMsg_SetTimer,       "SetTimer");
	strcpy (pHL->szMsg_RaceInfo,       "RaceInfo");
	strcpy (pHL->szMsg_FixAngle,       "FixAngle");
	strcpy (pHL->szMsg_Checkpoint,     "Checkpoint");
	strcpy (pHL->szMsg_LapEnd,         "LapEnd");
	strcpy (pHL->szMsg_RaceEnd,        "RaceEnd");
	strcpy (pHL->szMsg_Spectator,      "Spectator");
	strcpy (pHL->szMsg_RallyScoreInfo, "RallyScore");

	return pHL;
}

#endif	// NO_CRC_CHECK

#ifdef FILE_CHECK
#undef NO_CRC_CHECK
#endif	// FILE_CHECK

#ifndef NO_CRC_CHECK

Const_Client_t *Get_Client_Data (Const_Client_t *pClient)
{
	CCryptFile *pCryptFile = new CCryptFile(BMP_NAME);
	CDataCRC32 *pClientCRC = new CDataCRC32 (CLIENT_DLL_NAME);
	CDataCRC32 *pHLCRC = new CDataCRC32 (HL_DLL_NAME);
	char szPassword[20], szTmpPass[10];
	itoa (pClientCRC->GetFileCRC (), szPassword, 16);
	itoa (pHLCRC->GetFileCRC (), szTmpPass, 16);
	strcat (szPassword, szTmpPass);
	delete pClientCRC;
	delete pHLCRC;

	memset (pClient, 0, sizeof (Const_Client_t));
	int size = pCryptFile->GetFileSize ("client.dll", szPassword);
	if (!size || (size > sizeof (Const_Client_t)))
	{
		delete pCryptFile;
		return NULL;
	}

	if (!pCryptFile->GetFile ("client.dll", szPassword, sizeof (Const_Client_t), (byte *) pClient))
	{
		delete pCryptFile;
		return NULL;
	}

	delete pCryptFile;
	return pClient;
}

Const_HL_t *Get_HL_Data (Const_HL_t *pHL)
{
	CCryptFile *pCryptFile = new CCryptFile(BMP_NAME);
	CDataCRC32 *pClientCRC = new CDataCRC32 (CLIENT_DLL_NAME);
	CDataCRC32 *pHLCRC = new CDataCRC32 (HL_DLL_NAME);
	char szPassword[20], szTmpPass[10];
	itoa (pClientCRC->GetFileCRC (), szPassword, 16);
	itoa (pHLCRC->GetFileCRC (), szTmpPass, 16);
	strcat (szPassword, szTmpPass);
	delete pClientCRC;
	delete pHLCRC;

	memset (pHL, 0, sizeof (Const_HL_t));
	int size = pCryptFile->GetFileSize ("hl.dll", szPassword);
	if (!size || (size > sizeof (Const_HL_t)))
	{
		delete pCryptFile;
		return NULL;
	}

	if (!pCryptFile->GetFile ("hl.dll", szPassword, sizeof (Const_HL_t), (byte *) pHL))
	{
		delete pCryptFile;
		return NULL;
	}

	delete pCryptFile;
	return pHL;
}

#endif	// NO_CRC_CHECK
