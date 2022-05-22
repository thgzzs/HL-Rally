/***********************************************************************
*
*   File: - RallyHud.cpp
*    
*   Purpose: - Draws all sprites to the HUD
*
*   Author: - FragMented! - Not really anymore
*
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*
*   Copyright 2001 FragMented and HL Rally Team
*
*
**************************************************************************
*/

#include "STDIO.H"
#include "STDLIB.H"
#include "MATH.H"

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>

#include "cvardef.h"
#include "usercmd.h"

#include "const.h"
#include "pm_defs.h"

#include "cl_entity.h"
#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "in_defs.h"
#include "camera.h"

#include "vgui_teamfortressviewport.h"
#include "vgui_scorepanel.h"
#include "com_model.h"

#include "ref_params.h"

extern "C" playermove_t *pmove;

extern cl_enginefunc_t gEngfuncs;

extern int gear;
unsigned int money;

//struct ref_params_s *pparams;

#include "rally_effects.h"
#include "rally_vguifx.h"
#include "rally_sound.h"
extern float steer_delta;

bool bFirstFinished;
float fTimeLeft;

int iPlayerCP[MAX_PLAYERS];
int iPlayerLap[MAX_PLAYERS];

// SaRcaZm - V8 - Start
extern float g_reversing;
void FixYaw ();
extern vec3_t vUpgrades[8];
// SaRcaZm - V8 - End

//
//-----------------------------------------------------
//

	int m_HUD_kmph;
	int m_HUD_colon;
	int m_HUD_hiphen;
//	int m_HUD_points;
//	int m_HUD_deaths;
//	int m_HUD_gameover;
//	int m_HUD_damage;
//	int m_HUD_critical;
//	int m_HUD_shifter;
//	int m_HUD_ball;
//	int m_HUD_speedokmph;

	int m_HUD_snumbers;


	int HealthWidth;
	int r, b, g;


	int gameoverx, gameovery;
	int secondx;

//	int time_Minutes=0;
//	signed int time_Seconds=1;
//	float time_ms=0; // Crem: Milliseconds
	float flNextDisplay=0;

//	int m_iIncrement = 0;
//
//-----------------------------------------------------
//

// SaRcaZm - V8 - Start
DECLARE_MESSAGE (m_Rally, SetTimer);
// Moved these up here from below this function
DECLARE_MESSAGE (m_Rally, RaceInfo);
DECLARE_MESSAGE (m_Rally, FixAngle);
DECLARE_MESSAGE (m_Rally, Checkpoint);
DECLARE_MESSAGE (m_Rally, LapEnd);
DECLARE_MESSAGE (m_Rally, RaceEnd);
DECLARE_MESSAGE (m_Rally, Countdown);
DECLARE_MESSAGE (m_Rally, Flashlight);

DECLARE_MESSAGE (m_Rally, Money);
DECLARE_MESSAGE (m_Rally, Horn);

DECLARE_MESSAGE (m_Rally, UserStats);
DECLARE_MESSAGE (m_Rally, RallyScore);
// SaRcaZm - V8 - End

int CurrentLap = 1;
int CurrentCheckpoint = 1;
int CurrentPlace = 1;
int TotalLaps = 3;
int TotalCheckpoints = 3;
int TotalPlayers = 1;
float bestlap = -1;

int ab_finish = 0;

float fLapStart;
float fRaceStart;
float fLastLap;
int TimeExtension = 0;
int TimeExtensionSound = 5;

// SaRcaZm - V8 - Start
bool ab_race = false;
bool hp_race = false;
// SaRcaZm - V8 - End

int CHudRally::MsgFunc_SetTimer (const char *pszName, int iSize, void *pbuf)
{
	// Parameters:
	//		1)	Global time the timer started

	BEGIN_READ( pbuf, iSize );
	float fStartTime = READ_COORD ();

	// HACK, HACK, HACK
	// For the moment we only set the timer at the beginning of the race
	// so set this as the beginning time
	if (fStartTime != -1)
	{
		fLapStart = fRaceStart = fStartTime;

		if (!hp_race)
			FixYaw ();
		else
			TimeExtension -= 2;
	}

	return 1;
}

void CAM_ToFirstPerson (void);
void CAM_ToThirdPerson (void);
float cam_prevmode = 0;
bool pers_thirdperson = true;

int CHudRally::MsgFunc_Countdown (const char *pszName, int iSize, void *pbuf)
{
	// Parameters:
	//		1)	Global time the countdown started

	BEGIN_READ (pbuf, iSize);
	//float fCountdownStartTime = READ_COORD ();

	// Play the sound file
	RallySound->fmodPlay("voc/321.wav", false, 0);

	// Display the countdown on the hud
	CRallyVGUIFX::getSingleton()->StartCountdown (READ_COORD());

	// Update the player info ready for a new race
	for (int i = 0; i < MAX_PLAYERS+1; i++)
	{
		g_PlayerExtraInfo[i].racetime = gEngfuncs.GetClientTime ();
		g_PlayerExtraInfo[i].finished = 0;
		if (g_PlayerExtraInfo[i].bestlap <= 0.0f)
			g_PlayerExtraInfo[i].bestlap = -1;
	}

	// Moved this stuff out of the loop (dunno why I ever put it in there)
	// Fix the lap number / checkpoint not resetting sometimes
	CurrentLap = 1;
	CurrentCheckpoint = 1;
	ab_finish = 0;
	// Stop both timers from running
	fLapStart = fRaceStart = fLastLap = -1;

	RallySound->fmodStartEngine ();

	gear = 0;	// Reset to neutral
	steer_delta = 0;	// Reset the steering delta

	fTimeLeft = -1;
	bFirstFinished = false;

	// SaRcaZm - V8 - Start
	// Clear out the checkpoint array
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		iPlayerCP[i] = iPlayerLap[i] = 1;
	}

	gEngfuncs.Cvar_SetValue ("cam_viewmode", cam_prevmode);
	if (pers_thirdperson)		// 3rd person
		CAM_ToThirdPerson ();
	else
		CAM_ToFirstPerson ();

	//memset (iPlayerCP, 1, sizeof (int) * MAX_PLAYERS);
	//memset (iPlayerLap, 1, sizeof (int) * MAX_PLAYERS);
	// SaRcaZm - V8 - End

	return 1;
}

int CHudRally::MsgFunc_Flashlight (const char *pszName, int iSize, void *pbuf)
{
	// Parameters:
	//		1)	Client index
	//		2)	bool HeadlightOn
	BEGIN_READ (pbuf, iSize);

	short cl = READ_BYTE ();
	short on = READ_BYTE ();

	for(unsigned short i = 0; i < MAX_PLAYERS; i++)
	{
		CRallyEffects::playerEffectsInfo_t *pl = &CRallyEffects::getSingleton()->m_PlayerList[i];

		if(pl && pl->pPlayerEnt && cl == pl->pPlayerEnt->index)
		{
			pl->bIsHeadlightOn = (on != 0);
			break;
		}
	}

	return 1;
}

int CHudRally::MsgFunc_RaceInfo (const char *pszName, int iSize, void *pbuf)
{
	// Parameters:
	//		1)	Total Laps
	//		2)	Total Checkpoints / Lap
	//		3)	Total Players on Server
	//		4)	Time Extension Time

	BEGIN_READ( pbuf, iSize );
	TotalLaps = READ_BYTE ();
	TotalCheckpoints = READ_BYTE ();
	TotalPlayers = READ_BYTE ();

	TimeExtension = READ_BYTE ();
	TimeExtensionSound = 5;

	// Find out what type of map this is
	// Get rid of the "maps/"
//	static char szHUDMap[80] = "";
	const char *level = gEngfuncs.pfnGetLevelName();
	if (!level)
		return true;
	while (*level != '/')
		level++;
	level++;

	// Reset the best lap time if a map change has occurred
//	if (stricmp (level, szHUDMap))
//	{
		if (((level[0] == 'a') || (level[0] == 'A')) && ((level[1] == 'b') || (level[1] == 'B')))
			ab_race = true;
		else
			ab_race = false;

		if (((level[0] == 'h') || (level[0] == 'H')) && ((level[1] == 'p') || (level[1] == 'P')))
			hp_race = true;
		else
			hp_race = false;

//		strcpy (szHUDMap, level);
//	}

	// Hack to take into account the extra 2 times SetTimer is called for HP_ mode
	if (hp_race)
		TimeExtension += 4;

	CRallyEffects::getSingleton()->freeSkidmarks();

	return 1;
}

// SaRcaZm - V8 - Start
extern bool fixangle;
int CHudRally::MsgFunc_FixAngle (const char *pszName, int iSize, void *pbuf)
{
	// Parameters:
	//		None

	fixangle = true;
	return 1;
}

void UpdateScoreboardPositions (int cl)
{
	int iSearch = cl;

RestartLoop:
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		// Make sure we don't compare ourselves
		if ((i != iSearch) && *g_PlayerExtraInfo[i].teamname)
		{
			// Check to see if anyone is in the same place as our updated player
			if (g_PlayerExtraInfo[i].position == g_PlayerExtraInfo[iSearch].position)
			{
				g_PlayerExtraInfo[i].position++;
				iSearch = i;
				goto RestartLoop;	// Here's the evil dodgy goto again
			}
		}
	}
}

extern "C" PM_SetStats (int index, int accel, int handling, int topspeed);

int CHudRally::MsgFunc_UserStats (const char *pszName, int iSize, void *pbuf)
{
	float accel, handling, topspeed;
	cl_entity_t *pthisplayer = gEngfuncs.GetLocalPlayer();

	BEGIN_READ( pbuf, iSize );
	accel    = READ_COORD ();
	handling = READ_COORD ();
	topspeed = READ_COORD ();

	PM_SetStats (pthisplayer->index, accel, handling, topspeed);

	return 1;
}
// SaRcaZm - V8 - End

int CHudRally::MsgFunc_Checkpoint (const char *pszName, int iSize, void *pbuf)
{
	// Parameters:
	//		1)	Player Number
	//		2)	Position
	//		3)	Time Extension Time
	short cl;

	cl_entity_t *pthisplayer = gEngfuncs.GetLocalPlayer();

	BEGIN_READ( pbuf, iSize );
	cl = READ_BYTE ();
	int place = READ_BYTE ();

	// Update the position in the extra player info array
	g_PlayerExtraInfo[cl].position = place;

	// Only play the checkpoint sound / update the current checkpoint if local player
	if (cl == pthisplayer->index)
	{
		// Play the checkpoint sound
		RallySound->fmodPlay("voc/checkpoint.wav", false, 0);

		// Update the current checkpoint and place
		CurrentCheckpoint++;
		CurrentPlace = place;

		TimeExtension += READ_BYTE ();
		TimeExtensionSound = 5;
	}

	iPlayerCP[cl]++;

	// SaRcaZm - V8 - Start
	UpdateScoreboardPositions (cl);
	CurrentPlace = g_PlayerExtraInfo[pthisplayer->index].position;
	// SaRcaZm - V8 - End

	return 1;
}

int CHudRally::MsgFunc_LapEnd (const char *pszName, int iSize, void *pbuf)
{
	// Parameters:
	//		1)	Player index
	//		2)	Position
	//		3)	Lap time
	//		4)	Time Extension Time

	cl_entity_t *pthisplayer = gEngfuncs.GetLocalPlayer();

	BEGIN_READ( pbuf, iSize );
	short cl = READ_BYTE ();
	int place = READ_BYTE ();
	float laptime = READ_COORD ();

	// Do local stuff if this is the local client
	if (cl == pthisplayer->index)
	{
		// Play the lap end sound
		RallySound->fmodPlay("voc/lapend.wav", false, 0);

		// Update the current lap and place
		CurrentLap++;
		CurrentPlace = place;
		// Reset the current checkpoint
		CurrentCheckpoint = 1;

		// Check to see if this is our new best lap
		if ((laptime < g_PlayerExtraInfo[cl].bestlap) || (g_PlayerExtraInfo[cl].bestlap < 0.0f))
		{
			g_PlayerExtraInfo[cl].bestlap = bestlap = laptime;
		}

		// Update the current lap start
		fLapStart = gEngfuncs.GetClientTime ();
		fLastLap = laptime;		// Save it for display on the HUD

		TimeExtension += READ_BYTE ();
		TimeExtensionSound = 5;
	}
	else	// Network player
	{
		if ((laptime < g_PlayerExtraInfo[cl].bestlap) || (g_PlayerExtraInfo[cl].bestlap < 0.0f))
			g_PlayerExtraInfo[cl].bestlap = laptime;
	}

	g_PlayerExtraInfo[cl].position = place;
	gViewPort->m_pScoreBoard->Update();

	iPlayerCP[cl] = 1;	// Start of a new lap
	iPlayerLap[cl]++;

	// SaRcaZm - V8 - Start
	UpdateScoreboardPositions (cl);
	CurrentPlace = g_PlayerExtraInfo[pthisplayer->index].position;
	// SaRcaZm - V8 - End

	return 1;
}

int CHudRally::MsgFunc_RaceEnd (const char *pszName, int iSize, void *pbuf)
{
	// This is now sent to all players so we need a player index
	// Parameters:
	//		1)	Player index
	//		2)	Position
	//		3)	Lap time
	//		4)	Race time
	short cl;

	cl_entity_t *pthisplayer = gEngfuncs.GetLocalPlayer();

	BEGIN_READ( pbuf, iSize );
	cl = READ_BYTE ();
	int place = READ_BYTE ();
	float laptime = READ_COORD ();
	float racetime = READ_COORD ();

	if (TimeExtension)
		place = g_PlayerExtraInfo[cl].position;

	// SaRcaZm - V8 - Start
	if (!bFirstFinished && !TimeExtension)
	{
		// Start the countdown
		bFirstFinished = true;

		fTimeLeft = fRaceStart + racetime + 30;
		cl_entity_t *pthisplayer = gEngfuncs.GetLocalPlayer();
		RallySound->fmodPlay("voc/warning30.wav", false, 0);
	}

	// Update everything on the client side if it is the local player
	if (cl == pthisplayer->index)
	{
		// Play the race end sound
		// SaRcaZm - V8 - Start
		if (place < 2)
			RallySound->fmodPlay("voc/congrats.mp3", false, 0);
		else if (place < 3)
			RallySound->fmodPlay("voc/second.mp3", false, 0);
		else if (place < 4)
			RallySound->fmodPlay("voc/third.mp3", false, 0);
		else
			RallySound->fmodPlay("voc/nexttime.wav", false, 0);
		// SaRcaZm - V8 - End

		// Reset the current checkpoint and lap and place
		CurrentCheckpoint = 1;
		CurrentLap = 1;
		CurrentPlace = place;

		// Check to see if this is our new best lap
		if ((laptime > 0) && ((laptime < g_PlayerExtraInfo[cl].bestlap) || (g_PlayerExtraInfo[cl].bestlap < 0.0f)))
		{
			g_PlayerExtraInfo[cl].bestlap = bestlap = laptime;
		}

		if (ab_race)
		{
			ab_finish = 1;
			fLastLap = laptime;		// Save it for display on the HUD
		}
		// SaRcaZm - V8 - End

		// Skid to a stop when the time runs out in time extension mode
		if (TimeExtension)
			ab_finish = 1;

		RallySound->fmodStopEngine ();
	}
	else	// Not the local, so just update the scoreboard
	{
		// Update their best lap
		if ((laptime > 0) && ((laptime < g_PlayerExtraInfo[cl].bestlap) || (g_PlayerExtraInfo[cl].bestlap < 0.0f)))
			g_PlayerExtraInfo[cl].bestlap = laptime;
	}

	g_PlayerExtraInfo[cl].finished = 1;
	g_PlayerExtraInfo[cl].racetime = racetime;
	g_PlayerExtraInfo[cl].position = place;
	gViewPort->m_pScoreBoard->Update ();

	iPlayerLap[cl] = -1;

	UpdateScoreboardPositions (cl);
	CurrentPlace = g_PlayerExtraInfo[pthisplayer->index].position;
	// SaRcaZm - V8 - End

	return 1;
}

int CHudRally::MsgFunc_Money (const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
	money = READ_SHORT ();

	return 1;
}

int CHudRally::MsgFunc_Horn (const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
	short cl = READ_BYTE ();
	cl_entity_t *plr = gEngfuncs.GetEntityByIndex (cl);

	if (plr)
	{
		for(unsigned short i = 0; i < MAX_PLAYERS; i++)
		{
			CRallyEffects::playerEffectsInfo_t *pl = &CRallyEffects::getSingleton()->m_PlayerList[i];

			if(pl && pl->pPlayerEnt && cl == pl->pPlayerEnt->index)
			{
				CRallyEffects::getSingleton()->m_PlayerList[i].bIsHorn = true;
				break;
			}
		}
	}

	return 1;
}

// SaRcaZm - V8 - Start
int CHudRally::MsgFunc_RallyScore (const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
	short cl = READ_BYTE();
	short frags = READ_SHORT();

	if ( cl > 0 && cl <= MAX_PLAYERS )
	{
		g_PlayerExtraInfo[cl].frags = frags;
		gViewPort->UpdateOnPlayerInfo();
	}

	return 1;
}
// SaRcaZm - V8 - End

#define timexstep 16

void CHudRally::DrawTime (int time_Minutes, int time_Seconds, float time_Ms, int x, int y, int r, int g, int b)
{
	if (time_Minutes != -1) {
		// Minutes
		gHUD.DrawHudNumber2(x, y, DHN_2DIGITS | DHN_DRAWZERO, time_Minutes, r, g, b, m_HUD_snumbers);

		// Colon 1
		SPR_Set(gHUD.GetSprite(m_HUD_colon), r, g, b);
		x += timexstep*2;
		SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_colon));

		x += timexstep;
		// Seconds
		gHUD.DrawHudNumber2(x, y, DHN_2DIGITS | DHN_DRAWZERO, time_Seconds, r, g, b, m_HUD_snumbers);
		
		// Append 0 to seconds
		if(time_Seconds < 10)
			gHUD.DrawHudNumber2(x, y, DHN_DRAWZERO, 0, r, g, b, m_HUD_snumbers);

		x += timexstep*2;
		// Colon 2
		SPR_Set(gHUD.GetSprite(m_HUD_colon), r, g, b);
		SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_colon));

		x += timexstep;
		// Milliseconds
		gHUD.DrawHudNumber2(x, y, DHN_2DIGITS | DHN_DRAWZERO, time_Ms, r, g, b, m_HUD_snumbers);

		// Append 0 to milliseconds
		if(time_Ms < 10)
			gHUD.DrawHudNumber2(x, y, DHN_DRAWZERO, 0, r, g, b, m_HUD_snumbers);

	} else {

		// Minutes
		x += timexstep;
		SPR_Set(gHUD.GetSprite(m_HUD_hiphen), r, g, b);
		SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_hiphen));

		// Colon 1
		x += timexstep;
		SPR_Set(gHUD.GetSprite(m_HUD_colon), r, g, b);
		SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_colon));

		// Seconds
		x += timexstep;
		SPR_Set(gHUD.GetSprite(m_HUD_hiphen), r, g, b);
		SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_hiphen));
		x += timexstep;
		SPR_Set(gHUD.GetSprite(m_HUD_hiphen), r, g, b);
		SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_hiphen));
		
		// Colon 2
		x += timexstep;
		SPR_Set(gHUD.GetSprite(m_HUD_colon), r, g, b);
		SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_colon));

		// MilliSeconds
		x += timexstep;
		SPR_Set(gHUD.GetSprite(m_HUD_hiphen), r, g, b);
		SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_hiphen));
		x += timexstep;
		SPR_Set(gHUD.GetSprite(m_HUD_hiphen), r, g, b);
		SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_hiphen));

	}

}

// Crem Start: modified this routine to allow for --:--:-- time
void CHudRally::DrawTime (float time, int x, int y, int r, int g, int b)
{
	int t_Ms, t_Seconds, t_Minutes;
	
	if (time == -1) {

		t_Ms = -1;
		t_Seconds = -1;
		t_Minutes = -1;

	} else {

		t_Ms = (time - (int) time) * 100;
		t_Seconds = ((int) time) % 60;
		t_Minutes = ((int)time) / 60;
	}

	DrawTime (t_Minutes, t_Seconds, t_Ms, x, y, r, g, b);
}

int CHudRally::Init(void)
{
	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem(this);

	// SaRcaZm - V8 - Start
	HOOK_MESSAGE (SetTimer);
	HOOK_MESSAGE (RaceInfo);
	HOOK_MESSAGE (FixAngle);	// Do this here because input.cpp does not contain a class
	HOOK_MESSAGE (Checkpoint);
	HOOK_MESSAGE (LapEnd);
	HOOK_MESSAGE (RaceEnd);
	HOOK_MESSAGE (Countdown);
	HOOK_MESSAGE (Flashlight);
	HOOK_MESSAGE (UserStats);
	HOOK_MESSAGE (Money);
	HOOK_MESSAGE (Horn);
	HOOK_MESSAGE (RallyScore);
	money = 0;
	// SaRcaZm - V8 - End

	return 1;
}

// SaRcaZm - V8 - Start
vec3_t vCheckPoints[40];	// MAX_CHECKPOINTS

void LoadCheckPoints (void)
{
	float position[3] = {0,0,0};
	cl_entity_t * pEnt = gEngfuncs.GetEntityByIndex( 0 );	// get world model
	int num = -1;

	if ( pEnt )
	{
		char			*data;
		char			keyname[256];
		int				n;
		char			token[1024];

		for (int i = 0; i < 3; i++)
		{
			for ( data = gEngfuncs.COM_ParseFile( pEnt->model->entities, token) ;	// cl.worldmodel->entities
			data && (*data) && (*token != '}');
			data = gEngfuncs.COM_ParseFile(data, token) ) // TODO check this for null pointer crashes
			{
				
				if (token[0] != '{')
					return;
				
				while (1)
				{	
					// parse key
					data = gEngfuncs.COM_ParseFile(data, token);
					if (token[0] == '}')
						break;
					if (!data)
						return;
					
					strcpy (keyname, token);
					// another hack to fix keynames with trailing spaces
					n = strlen(keyname);
					while (n && keyname[n-1] == ' ')
					{
						keyname[n-1] = 0;
						n--;
					}
					
					// parse value
					data = gEngfuncs.COM_ParseFile(data, token);
					
					if (!data)
						return;
					
					if (token[0] == '}')
						return;
					
					if (!stricmp(keyname,"classname"))
					{
						if (!stricmp(token,"info_checkpoint"))
						{
							if (num != -1)
								vCheckPoints[num] = position;
						}
					}

					if (!stricmp(keyname,"origin"))
					{
						UTIL_StringToVector(position, token);
					}

					if (!stricmp(keyname, "number"))
					{
						num = atoi (token);
					}

				} // while (1)

				if (strlen(data) < 2) { 
					break;
				}
			}
		}
	}
}

void UpdatePlaces (void)	// Called every frame
{
	float iDist[MAX_PLAYERS];
	int iPos[MAX_PLAYERS];

	memset (iPos, -1, sizeof (int) * MAX_PLAYERS);
	for (int i = 0; i < MAX_PLAYERS; i++)
		iDist[i] = -1;

	for (i = 1; i < MAX_PLAYERS; i++)
	{
		cl_entity_t *pl = gEngfuncs.GetEntityByIndex (i);
		if (!pl)
			continue;

		float dist = (vCheckPoints[iPlayerCP[i]] - pl->origin).Length ();

		// Search the array for the correct position
		int pos = -1;
		if (iPlayerLap[i] == -1)	// They have finished
			pos = g_PlayerExtraInfo[i].position;
		else
		{
			for (int j = 0; j < MAX_PLAYERS; j++)
			{
				if (iDist[j] < dist)
					break;
			}

			// Slot them into the dist array
			//memcpy (&iDist[j], &iDist[j+1], sizeof (float) * (MAX_PLAYERS - j - 1));
			for (int y = MAX_PLAYERS - 1; y > j; y--)
				iDist[y] = iDist[y - 1];

			iDist[j] = dist;
			pos = j;
		}

		// Slot them into the dist array
		//memcpy (&iPos[pos], &iPos[pos+1], sizeof (int) * (MAX_PLAYERS - pos - 1));
		for (int y = MAX_PLAYERS - 1; y > pos; y--)
			iPos[y] = iPos[y - 1];

		if (pos >= 0 && pos < MAX_PLAYERS)
			iPos[pos] = i;
	}

	// Update the scoreboard with the new info
	for (i = 0; i < MAX_PLAYERS && iPos[i] > -1; i++)
		g_PlayerExtraInfo[iPos[i]].position = i + 1;

	cl_entity_t *pl = gEngfuncs.GetLocalPlayer ();
	CurrentPlace = g_PlayerExtraInfo[pl->index].position;
}
// SaRcaZm - V8 - End

//-----------------------------------------------------

extern int g_iUser1;

int CHudRally::Draw(float flTime)
{
	if ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) || (CRallyVGUIFX::getSingleton()->GetMode () == MODE_VGUI) || (CRallyVGUIFX::getSingleton()->GetMode () == MODE_SHOWROOM))
		return 1;

	static float foldtime = 9999;
	float fcurtime = gEngfuncs.GetClientTime ();
	if (foldtime > fcurtime)	// New map
	{
		bestlap = -1;

		// Also, fix the stored model name
		memset (gViewPort->m_szModel, 0, sizeof (gViewPort->m_szModel));
		memset (gViewPort->m_szTeam, 0, sizeof (gViewPort->m_szTeam));

		// For UpdatePlaces
		//LoadCheckPoints ();

		// Clear out the upgrades
		for (int i = 0; i < 8; i++)
			vUpgrades[i] = Vector (0, 0, 0);
	}
	foldtime = fcurtime;

	int x, y, xstep;

	/*pmove->Con_DPrintf ("HUD P: %.2f, R: %.2f\n", pmove->vuser1[0], pmove->vuser1[2]);
	pmove->angles[0] = pmove->vuser1[0];
	pmove->angles[2] = pmove->vuser1[2];*/

	m_HUD_kmph = gHUD.GetSpriteIndex( "cdials" ); // kmph
	m_HUD_colon = gHUD.GetSpriteIndex( "colon" );
	m_HUD_hiphen = gHUD.GetSpriteIndex( "hiphen" );

//	m_HUD_points = gHUD.GetSpriteIndex( "points" );
//	m_HUD_deaths = gHUD.GetSpriteIndex( "deaths" );

//	m_HUD_gameover = gHUD.GetSpriteIndex( "gameover" );
//	m_HUD_critical = gHUD.GetSpriteIndex( "critical" );
//	m_HUD_damage = gHUD.GetSpriteIndex( "damage" );

//	m_HUD_shifter = gHUD.GetSpriteIndex( "shifter" );
//	m_HUD_ball = gHUD.GetSpriteIndex( "ball" );

//	m_HUD_speedokmph = gHUD.GetSpriteIndex( "cdials" );
	int m_HUD_lap = gHUD.GetSpriteIndex( "lap" );
	int m_HUD_clock = gHUD.GetSpriteIndex( "clock" );
	int m_HUD_slash = gHUD.GetSpriteIndex( "slash" );

	// Crem - new stuff here
	m_HUD_snumbers = gHUD.GetSpriteIndex( "number_0" ); // small numbers
	int m_HUD_bnumbers = gHUD.GetSpriteIndex( "bnumber_0" ); // black numbers
	int m_HUD_lgnumbers = gHUD.GetSpriteIndex( "lgnumber_0" ); // large numbers
	int m_HUD_lgsuffix = gHUD.GetSpriteIndex( "lg_st" ); // suffix (st, nd, rd, th)

	int m_HUD_slash_sm = gHUD.GetSpriteIndex( "slash_sm" );
	int m_HUD_leftbracket = gHUD.GetSpriteIndex( "leftbracket" );
	int m_HUD_rightbracket = gHUD.GetSpriteIndex( "rightbracket" );
	// More new stuff
	int m_HUD_bestlap = gHUD.GetSpriteIndex( "bestlap" );
	int m_HUD_currentlap = gHUD.GetSpriteIndex( "currentlap" );
	int m_HUD_totaltime = gHUD.GetSpriteIndex( "totaltime" );

	int m_HUD_version = gHUD.GetSpriteIndex( "version" );
	// Creme end

	int m_HUD_letter_n = gHUD.GetSpriteIndex ("letter_n");
	int	m_HUD_letter_r = gHUD.GetSpriteIndex ("letter_r");
	int m_HUD_intdials = gHUD.GetSpriteIndex ("intdials");
	int m_HUD_letter_dollars = gHUD.GetSpriteIndex ("letter_dol");

	//--------------------------


	// Guess the current place by simple distances
//	static float lastupdate = 0;
//	if (lastupdate < currenttime - 2)	// Update every 2 seconds
//	{
//		UpdatePlaces ();	// This corrupts something and crashes later
//		pmove->Con_DPrintf ("UpdatePlaces called at %f\n", currenttime);
//		lastupdate = currenttime;
//	}
	// SaRcaZm - V8 - End

	if (g_iUser1)		// Don't draw when in spectator mode
	{
		r = 255; g = 0; b = 0;
		if ((fTimeLeft > 0) && (flTime > fTimeLeft - 30))
		{
			// Countdown is on
			float frac = flTime - (int)flTime;
			DrawTime (fTimeLeft - flTime, 8, ScreenHeight - 96, r, g, b);
		}

		return 1;
	}


//--------------------------
//		if( CL_IsThirdPerson() )
//		{

			float currentspeed = Length (pmove->velocity) / 3.0f;

			if ( currentspeed < 9 )
			{
				currentspeed = 0;
			}
			else
			{
				// actual speed
			}

			int dials;
			if (cam_thirdperson)
				dials = m_HUD_kmph;
			else
				dials = m_HUD_intdials;
			// Below Draws the Speedo
			x = ScreenWidth - 224;
			y = ScreenHeight - 232;
			SPR_Set(gHUD.GetSprite(dials), 255, 255, 255);
			SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(dials));

			x = ScreenWidth - 52;
			y = ScreenHeight - 32;
			r = 255; g = 75; b = 14;
			gHUD.DrawHudNumber2(x, y, DHN_3DIGITS | DHN_DRAWZERO, currentspeed, r, g, b, m_HUD_snumbers);
//		}
		
		r = 255;
		g = 255;
		b = 255;

		xstep = 16;
		x = 25;
		y = 16;

		if (!ab_race && !hp_race)
		{
			// The word 'Lap'
			SPR_Set(gHUD.GetSprite(m_HUD_lap), r, g, b);
			SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_lap));

			x = 20;
			y = 56;

			// Current Lap
			gHUD.DrawHudNumber2(x, y, DHN_DRAWZERO, CurrentLap, r, g, b, m_HUD_lgnumbers);

			x += xstep;

			// The slash between the lap/total
			SPR_Set(gHUD.GetSprite(m_HUD_slash), r, g, b);
			SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_slash));

			x += xstep + 4;

			//. Total Laps
			gHUD.DrawHudNumber2(x, y, DHN_DRAWZERO, TotalLaps, r, g, b, m_HUD_lgnumbers);
		}

		r = 192;
		g = 192;
		b = 192;

		// Draw the current checkpoint
		// Only draw the checkpoint and count in -dev mode
//		if (gEngfuncs.pfnGetCvarFloat("developer") != 0) { 
		if (!hp_race)
		{
			r = 255; g = 75; b = 14;
				
			x = 16;
			y = 106;
			SPR_Set(gHUD.GetSprite(m_HUD_leftbracket), r, g, b);
			SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_leftbracket));
			x += 8;
			if (CurrentCheckpoint >= 10) {
				gHUD.DrawHudNumber2(x, y, DHN_2DIGITS | DHN_DRAWZERO, CurrentCheckpoint, r, g, b, m_HUD_snumbers);
				x += xstep * 2;
			} else { 
				gHUD.DrawHudNumber2(x, y, DHN_DRAWZERO, CurrentCheckpoint, r, g, b, m_HUD_snumbers);
				x += xstep;
			}
			// The slash between the lap/total
			SPR_Set(gHUD.GetSprite(m_HUD_slash_sm), r, g, b);
			SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_slash_sm));
			x += xstep;
			gHUD.DrawHudNumber2(x, y, DHN_DRAWZERO, TotalCheckpoints, r, g, b, m_HUD_snumbers);
			if (TotalCheckpoints >= 10) {
				x += xstep*2;
			} else {
				x += xstep;
			}
			SPR_Set(gHUD.GetSprite(m_HUD_rightbracket), r, g, b);
			SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_rightbracket));

			if (cam_thirdperson) {
				// SaRcaZm - V8 - Start
				// Draw the current place
				y = 16;
				int tens = CurrentPlace / 10;
				int leftover = CurrentPlace % 10;
				if (CurrentPlace < 10)
				{
					x = ScreenWidth / 2 - 24;
					gHUD.DrawHudNumber2 (x, y, DHN_DRAWZERO, CurrentPlace, 255, 255, 255, m_HUD_lgnumbers);
					x += xstep;
				}
				else
				{
					x = ScreenWidth / 2 - 32;
					gHUD.DrawHudNumber2 (x, y, DHN_DRAWZERO, tens, 255, 255, 255, m_HUD_lgnumbers);
					x += xstep;
					gHUD.DrawHudNumber2 (x, y, DHN_DRAWZERO, leftover, 255, 255, 255, m_HUD_lgnumbers);
					x += xstep;
				}

				// Suffix (st, nd, rd, th, etc)
				if ((leftover > 0) && (leftover < 4) && (tens != 1))
				{
					SPR_Set(gHUD.GetSprite(m_HUD_lgsuffix+(leftover-1)), 255, 255, 255);
					SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_lgsuffix+(leftover-1)));
				}
				else
				{
					// if 4 or greater then use a suffix of 'th' 
					// will work fine until you get above 20, when you need 
					// to do st, nd, rd again for every number ending in 1,2,3, 
					// but not having a second last digit of 1
					// (ie: 111, 112, 113 suffixes are th)
					SPR_Set(gHUD.GetSprite(m_HUD_lgsuffix+3), 255, 255, 255);
					SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_lgsuffix+3));
				}
				// SaRcaZm - V8 - End
			}


			r = 255; g = 75; b = 14;
			x = ScreenWidth - 176;

			SPR_Set(gHUD.GetSprite(m_HUD_totaltime), r, g, b);
			SPR_DrawHoles(0, x, 16, &gHUD.GetSpriteRect(m_HUD_totaltime));

			// SaRcaZm - V8 - Start
			if (ab_finish)
				DrawTime (fLastLap, x, 36, r, g, b);
			else if (fRaceStart != -1)
				DrawTime (flTime - fRaceStart, x, 36, r, g, b);
			else
				DrawTime (fRaceStart, x, 36, r, g, b);

			// The race time
			// Don't show lap time in ab_ race
			if (!ab_race)
			{
				SPR_Set (gHUD.GetSprite(m_HUD_currentlap), r, g, b);
				SPR_DrawHoles(0, x, 60, &gHUD.GetSpriteRect(m_HUD_currentlap));

				// Make the lap time stay on the screen for 4 seconds at the completion of the lap
				if ((flTime - fLapStart < 4) && (fLastLap != -1))
				{
					// Make it flash on and off
					float frac = flTime - (int)flTime;
					if ((frac < 0.4) || (frac > 0.8))
						DrawTime (fLastLap, x, 80, 128, 128, 128);
				}
				else if (fLapStart != -1)
					DrawTime (flTime - fLapStart, x, 80, r, g, b);
				else
					DrawTime (fLapStart, x, 80, r, g, b);
			}
			// SaRcaZm - V8 - End

			r = 255; g = 75; b = 14;

			// Move this down to make way for lap time
			SPR_Set(gHUD.GetSprite(m_HUD_bestlap), r, g, b);
			SPR_DrawHoles(0, x, 104, &gHUD.GetSpriteRect(m_HUD_bestlap));	// Was 60

			DrawTime (bestlap, x, 124, r, g, b);							// Was 80

			x = ScreenWidth - 76;
			y = 16;
			
			SPR_Set(gHUD.GetSprite(m_HUD_clock), r, g, b);
			SPR_DrawHoles(0, x, y, &gHUD.GetSpriteRect(m_HUD_clock));
		}
	// Crem End

		// 30 seconds left countdown
		r = 255; g = 0; b = 0;
		// SaRcaZm - V8 - Start
		if (!ab_finish && (fTimeLeft > 0) && (flTime > fTimeLeft - 30))
		// SaRcaZm - V8 - End
		{
			// Countdown is on
			float frac = flTime - (int)flTime;
			DrawTime (fTimeLeft - flTime, 8, ScreenHeight - 32, r, g, b);
		}
		// The timer
		else if (TimeExtension)
		{
			// Time extension mode
			if (fRaceStart != -1)
			{
				float timeleft = TimeExtension - (flTime - fRaceStart);
				DrawTime (timeleft, 8, ScreenHeight - 32, r, g, b);

				if ((timeleft < TimeExtensionSound) && (TimeExtensionSound > 0))
				{
					RallySound->fmodPlay("misc/harryup.wav", false, 0);
					TimeExtensionSound--;
				}
			}
			else
				DrawTime (TimeExtension, 8, ScreenHeight - 32, r, g, b);
		}


//--------------------------

		// sets colours back
		r = 192;
		g = 192;
		b = 192;

//--------------------------

	int gearx, geary;//, ballx, bally;
	gearx = ScreenWidth - 152;
	geary = ScreenHeight - 32;

	r = 255; g = 75; b = 14;
	// Two special cases, reverse and neutral
	// SaRcaZm - V8 - Start
	if ((CRallyEffects::getSingleton()->m_pCvarManual->value && (gear == -1)) || (!CRallyEffects::getSingleton()->m_pCvarManual->value && g_reversing))
	// SaRcaZm - V8 - End
	{
		SPR_Set(gHUD.GetSprite(m_HUD_letter_r), r, g, b);
		SPR_DrawHoles(0, gearx, geary, &gHUD.GetSpriteRect(m_HUD_letter_r));
	}
	else if (gear == 0)
	{
		SPR_Set(gHUD.GetSprite(m_HUD_letter_n), r, g, b);
		SPR_DrawHoles(0, gearx, geary, &gHUD.GetSpriteRect(m_HUD_letter_n));
	}
	else
		gHUD.DrawHudNumber2(gearx, geary, DHN_DRAWZERO, gear, r, g, b, m_HUD_snumbers);

	return 1;

}
//-----------------------------------------------------