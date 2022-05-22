/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
//  cdll_int.c
//
// this implementation handles the linking of the engine to the DLL
//

#include "hud.h"
#include "cl_util.h"
#include "netadr.h"
#include "vgui_schememanager.h"

extern "C"
{
#include "pm_shared.h"
float CL_CrashAmount;
}

#include "pm_defs.h"
extern "C" playermove_t *pmove;


#include <string.h>
#include "hud_servers.h"
#include "vgui_int.h"
#include "interface.h"
#include "ITrackerUser.h"

#define DLLEXPORT __declspec( dllexport )

extern "C" void Global_DLLEXPORT_Info( float time, const char *func_name );

cl_enginefunc_t gEngfuncs;
CHud gHUD;
TeamFortressViewport *gViewPort = NULL;

HINTERFACEMODULE g_hTrackerModule = NULL;
ITrackerUser *g_pTrackerUser = NULL;

float fPlayerAngles;

void InitInput (void);
void EV_HookEvents( void );
void IN_Commands( void );

// SaRcaZm - V5 - Start
Const_Client_t clientConst;
// SaRcaZm - V5 - End
// SaRcaZm - V8 - Start
void mp3Frame (double time);
#include "rally_sound.h"
CRallySound *RallySound = NULL;
// SaRcaZm - V8 - End

// FragMented
#include "../common/rally_zlib.h"

/*
========================== 
    Initialize

Called when the DLL is first loaded.
==========================
*/
extern "C" 
{
int		DLLEXPORT Initialize( cl_enginefunc_t *pEnginefuncs, int iVersion );
int		DLLEXPORT HUD_VidInit( void );
int		DLLEXPORT HUD_Init( void );
int		DLLEXPORT HUD_Redraw( float flTime, int intermission );
int		DLLEXPORT HUD_UpdateClientData( client_data_t *cdata, float flTime );
int		DLLEXPORT HUD_Reset ( void );
void	DLLEXPORT HUD_PlayerMove( struct playermove_s *ppmove, int server );
void	DLLEXPORT HUD_PlayerMoveInit( struct playermove_s *ppmove );
char	DLLEXPORT HUD_PlayerMoveTexture( char *name );
int		DLLEXPORT HUD_ConnectionlessPacket( struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size );
int		DLLEXPORT HUD_GetHullBounds( int hullnumber, float *mins, float *maxs );
void	DLLEXPORT HUD_Frame( double time );
void	DLLEXPORT HUD_VoiceStatus(int entindex, qboolean bTalking);
void	DLLEXPORT HUD_DirectorEvent(unsigned char command, unsigned int firstObject, unsigned int secondObject, unsigned int flags);
}

/*
================================
HUD_GetHullBounds

  Engine calls this to enumerate player 
  collision hulls, for prediction. 
  Return 0 if the hullnumber doesn't exist.
================================
*/

int DLLEXPORT HUD_GetHullBounds( int hullnumber, float *mins, float *maxs )
{
	// SaRcaZm - V7 - Start
	// Update from Creme:
/*	int iret = 0;

	switch ( hullnumber )
	{
	case 0:				// Normal player
*/      mins[0] = mins[1] = -16;
        mins[2] = -36;
        maxs[0] = maxs[1] = -16;
        maxs[2] = 36;
        return 1;
/*      break;
	case 1:				// Crouched player
//		mins = Vector(-5, -5, -5 );
//		maxs = Vector(5, 5, 18 );
//		mins = Vector( 0, 0, 0 );
//		maxs = Vector( 0, 0, 0 );

        mins[0] = mins[1] = -8;
        mins[2] = -36;
        maxs[0] = maxs[1] = 8;
        maxs[2] = 8;

		iret = 1;
		break;
	case 2:				// Point based hull
//		mins = Vector( 0, 0, 0 );
//		maxs = Vector( 0, 0, 0 );

        mins[0] = mins[1] = -8;
        mins[2] = -36;
        maxs[0] = maxs[1] = 8;
        maxs[2] = 8;

		iret = 1;
		break;
	}

	return iret;
*/	// SaRcaZm - V7 - End
}

/*
================================
HUD_ConnectionlessPacket

 Return 1 if the packet is valid.  
 Set response_buffer_size if you 
 want to send a response packet.  
 Incoming, it holds the max
  size of the response_buffer, 
  so you must zero it out if you 
  choose not to respond.
================================
*/
int	DLLEXPORT HUD_ConnectionlessPacket( struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size )
{
	// Parse stuff from args
	int max_buffer_size = *response_buffer_size;

	// Zero it out since we aren't going to respond.
	// If we wanted to response, we'd write data into response_buffer
	*response_buffer_size = 0;

	// Since we don't listen for anything here, just respond that it's a bogus message
	// If we didn't reject the message, we'd return 1 for success instead.
	return 0;
}

void DLLEXPORT HUD_PlayerMoveInit( struct playermove_s *ppmove )
{
	PM_Init( ppmove );
}

char DLLEXPORT HUD_PlayerMoveTexture( char *name )
{
	return PM_FindTextureType( name );
}

// SaRcaZm - V8 - Start
float crashsound = -1;
// SaRcaZm - V8 - End
void DLLEXPORT HUD_PlayerMove( struct playermove_s *ppmove, int server )
{
	Global_DLLEXPORT_Info(gEngfuncs.GetClientTime(), "HUD_PlayerMove");

	PM_Move( ppmove, server );
	// SaRcaZm - V8 - Start
	if (CL_CrashAmount && (crashsound < pmove->time - 300))	// Don't play too often
	{
		if (CL_CrashAmount > -100)
			RallySound->fmodPlay3D ("hitwall1.wav", false, 0, 0.75, pmove->origin, pmove->velocity);
		else
			RallySound->fmodPlay3D ("hitwall2.wav", false, 0, 0.75, pmove->origin, pmove->velocity);

		crashsound = pmove->time;
	}
	// SaRcaZm - V8 - End
}

int DLLEXPORT Initialize( cl_enginefunc_t *pEnginefuncs, int iVersion )
{
	gEngfuncs = *pEnginefuncs;

	if (iVersion != CLDLL_INTERFACE_VERSION)
		return 0;

	// FragMented
	if(!ZLibFuncs.Init())
		gEngfuncs.Con_DPrintf("** \n** HL Rally could not load ZLib ** \n**\n");

	// FragMented
	RallySound = new CRallySound ();
	RallySound->fmodInit();
	RallySound->fmodPlay("sound\\init.mp3", false, 0);

	memcpy(&gEngfuncs, pEnginefuncs, sizeof(cl_enginefunc_t));

	EV_HookEvents();
	// get tracker interface, if any
	char szDir[512];
	if (!gEngfuncs.COM_ExpandFilename("Bin/TrackerUI.dll", szDir, sizeof(szDir)))
	{
		g_pTrackerUser = NULL;
		g_hTrackerModule = NULL;
		return 1;
	}

	g_hTrackerModule = Sys_LoadModule(szDir);
	CreateInterfaceFn trackerFactory = Sys_GetFactory(g_hTrackerModule);
	if (!trackerFactory)
	{
		g_pTrackerUser = NULL;
		g_hTrackerModule = NULL;
		return 1;
	}

	g_pTrackerUser = (ITrackerUser *)trackerFactory(TRACKERUSER_INTERFACE_VERSION, NULL);
	return 1;
}


/*
==========================
	HUD_VidInit

Called when the game initializes
and whenever the vid_mode is changed
so the HUD can reinitialize itself.
==========================
*/

int DLLEXPORT HUD_VidInit( void )
{
	gHUD.VidInit();

	VGui_Startup();

	return 1;
}

/*
==========================
	HUD_Init

Called whenever the client connects
to a server.  Reinitializes all 
the hud variables.
==========================
*/

int DLLEXPORT HUD_Init( void )
{
	// SaRcaZm - V5 - Start
	// Anti-Haxor Stuff
	Get_Client_Data (&clientConst);
	// SaRcaZm - V5 - End

	InitInput();
	gHUD.Init();
	Scheme_Init();
	return 1;
}


/*
==========================
	HUD_Redraw

called every screen frame to
redraw the HUD.
===========================
*/
float g_time;

// SaRcaZm - V7 - Start
extern vec3_t realvieworg;
// SaRcaZm - V7 - End

int DLLEXPORT HUD_Redraw( float time, int intermission )
{
	Global_DLLEXPORT_Info(gEngfuncs.GetClientTime(), "HUD_Redraw");

	gHUD.Redraw( time, intermission );

	// Crem: Whats the damn time again?
	g_time = time;

	// Update the 3D sound
	cl_entity_s *ent = gEngfuncs.GetLocalPlayer ();
	RallySound->fmodFrame (realvieworg, ent->curstate.velocity, ent->angles);

	return 1;
}


/*
==========================
	HUD_UpdateClientData

called every time shared client
dll/engine data gets changed,
and gives the cdll a chance
to modify the data.

returns 1 if anything has been changed, 0 otherwise.
==========================
*/

int DLLEXPORT HUD_UpdateClientData(client_data_t *pcldata, float flTime )
{
	Global_DLLEXPORT_Info(gEngfuncs.GetClientTime(), "HUD_UpdateClientData");

	IN_Commands();

	return gHUD.UpdateClientData(pcldata, flTime );
}

/*
==========================
	HUD_Reset

Called at start and end of demos to 
restore to "non"HUD state.
==========================
*/

int DLLEXPORT HUD_Reset( void )
{
	gHUD.VidInit();
	return 1;
}

/*
==========================
HUD_Frame

Called by engine every frame that client .dll is loaded
==========================
*/

float old_g_global_test = 0;
extern "C" float g_global_test;

static float SoundMuted = 0;

void DLLEXPORT HUD_Frame( double time )
{

	// Creme: set mute if nothing much happened this frame
	// ie: we are looking at a console screen or something bad happened.
	// notes: g_global_test+=1 incurs for at least 15 functions 
	//        each frame, so if its low, something is wrong.

	float MuteIt = g_global_test - old_g_global_test;

	// SaRcaZm - V8 - Start
	if (MuteIt < 5)
	{
		if (SoundMuted == 0)
		{
			RallySound->fmodStopEngine ();
			RallySound->fmodStopRemoteEngines ();
			SoundMuted = 1;
			//gEngfuncs.Con_DPrintf("** \n** Muted fmod noise! function count: %1.0f ** \n**\n", MuteIt);
		}
	}
	else
	{
		if (SoundMuted != 0) 
		{
			RallySound->fmodStartEngine ();
			SoundMuted = 0;
			//gEngfuncs.Con_DPrintf("** \n** UN-Muted fmod noise! function count: %1.0f ** \n**\n", MuteIt);
		}

		// If the sound isn't muted, add the other player's engines
		RallySound->fmodPlayRemoteEngines ();
	}

	mp3Frame (time);
	// SaRcaZm - V8 - End

	old_g_global_test = g_global_test;
	
	Global_DLLEXPORT_Info(gEngfuncs.GetClientTime(), "HUD_Frame\n");

	ServersThink( time );

	GetClientVoiceMgr()->Frame(time);
}


/*
==========================
HUD_VoiceStatus

Called when a player starts or stops talking.
==========================
*/

void DLLEXPORT HUD_VoiceStatus(int entindex, qboolean bTalking)
{
	GetClientVoiceMgr()->UpdateSpeakerStatus(entindex, bTalking);
}

/*
==========================
HUD_DirectorEvent

Called when a director event message was received
==========================
*/

void DLLEXPORT HUD_DirectorEvent(unsigned char command, unsigned int firstObject, unsigned int secondObject, unsigned int flags)
{
	 gHUD.m_Spectator.DirectorEvent(command, firstObject, secondObject, flags);
}


