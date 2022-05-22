// Rally_AntiHaxor.h
// by SaRcaZm

#ifndef __Rally_AntiHaxor_h__
#define __Rally_AntiHaxor_h__

typedef struct Const_Client_s {
	char szCarInfo[15];

	char szPlus_Forward[10];
	char szPlus_Back[10];
	char szPlus_Left[10];
	char szPlus_Right[10];

	char szMinus_Forward[10];
	char szMinus_Back[10];
	char szMinus_Left[10];
	char szMinus_Right[10];

	char szCvar_M_Pitch[10];
	char szCvar_M_Yaw[10];
	char szCvar_Cl_ForwardSpeed[20];
	char szCvar_Cl_YawSpeed[15];

	char szCvarValue_M_Pitch[8];
	char szCvarValue_M_Yaw[8];
	char szCvarValue_Cl_ForwardSpeed[8];
	char szCvarValue_Cl_YawSpeed[8];
} Const_Client_t;

typedef struct Const_HL_s {
	char szCarInfo[15];

	char szPrecache_Rocks[30];
	char szPrecache_Dirt[30];
	char szPrecache_Sand[30];
	char szPrecache_Snow[30];
	char szPrecache_Grass[30];
	char szPrecache_Smoke[30];
	char szPrecache_Bomb[30];
	char szPrecache_Checkpoint[30];
	char szPrecache_NextTime[30];
	char szPrecache_LapComplete[30];
	char szPrecache_Warning[30];
	char szPrecache_321[30];
	char szPrecache_Horn[30];
	char szPrecache_Backfire[30];
	char szPrecache_Skid[30];
	char szPrecache_Shadow[30];
	char szPrecache_Showroom[30];

	char szMsg_VGUIMenu[15];
	char szMsg_VGUIClasses[15];
	char szMsg_SetTimer[15];
	char szMsg_RaceInfo[15];
	char szMsg_FixAngle[15];
	char szMsg_Checkpoint[15];
	char szMsg_LapEnd[15];
	char szMsg_RaceEnd[15];
	char szMsg_Spectator[15];
	char szMsg_RallyScoreInfo[15];
	
	// Crem: added this to fix sarc's merge
	char szTesterName[35];
} Const_HL_t;

Const_Client_t *Get_Client_Data (Const_Client_t *pClient);
Const_HL_t *Get_HL_Data (Const_HL_t *pHL);

#endif	// __Rally_AntiHaxor_h__


