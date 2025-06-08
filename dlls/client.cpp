/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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
// Robin, 4-22-98: Moved set_suicide_frame() here from player.cpp to allow us to 
//				   have one without a hardcoded player.mdl in tf_client.cpp

/*

===== client.cpp ========================================================

  client/server game specific stuff

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "player.h"
#include "spectator.h"
#include "client.h"
#include "soundent.h"
#include "gamerules.h"
#include "customentity.h"
#include "weapons.h"
#include "weaponinfo.h"
#include "usercmd.h"
#include "netadr.h"
#include "rally_bot.h"
#include "const.h"
#include <stdio.h>

// FragMented
#include "../common/rally_zlib.h"

// SaRcaZm - V3 - Start
#include "game.h"
#include "carinfo.h"
#include "rally_websocket.h"
//edict_t *EntSelectSpawnPoint( CBaseEntity *pPlayer );
// SaRcaZm - V3 - End
// SaRcaZm - V7 - Start
extern cvar_t joinmidrace;
edict_t *listenserver = NULL;
// SaRcaZm - V7 - End
// SaRcaZm - V8 - Start
float pers_numlaps, pers_newlaps;
float pers_racemode, pers_newmode;
float pers_direction, pers_newdirection;
extern int gmsgUserStats;
// SaRcaZm - V8 - End

extern edict_t *EntSelectSpawnPoint( CBaseEntity *pPlayer );

extern DLL_GLOBAL ULONG		g_ulModelIndexPlayer;
extern DLL_GLOBAL BOOL		g_fGameOver;
extern DLL_GLOBAL int		g_iSkillLevel;
extern DLL_GLOBAL ULONG		g_ulFrameCount;

extern void CopyToBodyQue(entvars_t* pev);
extern int giPrecacheGrunt;
extern int gmsgSayText;

extern int CountPlayers(); // FragMented

void LinkUserMessages( void );
/*
 * used by kill command and disconnect command
 * ROBIN: Moved here from player.cpp, to allow multiple player models
 */
void set_suicide_frame(entvars_t* pev)
{       
	if (!FStrEq(STRING(pev->model), "models/player.mdl"))
		return; // allready gibbed

//	pev->frame		= $deatha11;
	pev->solid		= SOLID_NOT;
	pev->movetype	= MOVETYPE_TOSS;
	pev->deadflag	= DEAD_DEAD;
	pev->nextthink	= -1;
}


/*
===========
ClientConnect

called when a player connects to a server
============
*/
// hero
#ifdef _WIN32
#include <process.h>
#else
#include <pthread.h>
#endif

char pName[32];
char pAddress[32];
int score, wonid;

// hero
#ifdef _WIN32
unsigned _stdcall RequestClientCon (void *arg)
#else
void *RequestClientCon (void *arg)
#endif
{
	// SaRcaZm - V7 - Start
	char myBuf[256];
	sprintf(myBuf, "GET /players/record.php?wonid=%i&ip=%s&n=%s&plr=%i HTTP/1.1\nHost: www.hlrally.net\nUser-Agent: HL RALLY\n\n", wonid, pAddress, pName, CountPlayers()+1 );

	if ((CVAR_GET_FLOAT("sv_lan") || strstr(myBuf, "wonid=-1") || strstr(myBuf, "loopback") || strstr(myBuf, "127.0.0.1")) || (CVAR_GET_FLOAT("sv_php_stats") == 0))
	// SaRcaZm - V7 - End
	{
		// hero
		#ifdef _WIN32
			_endthreadex (0);
		#else
			pthread_exit (NULL);
		#endif
		return 0;
	}

	CRallySock RallySock;
	ALERT(at_console, RallySock.Socket_Connect(myBuf).c_str());
	ALERT(at_console, RallySock.Socket_ReadLn().c_str());
	RallySock.SocketClose();

	// hero
	#ifdef _WIN32
		_endthreadex (0);
	#else
		pthread_exit (NULL);
	#endif

	return 0;
}



BOOL ClientConnect( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ]  )
{
	if (g_fGameOver)
		return false;

	// SaRcaZm - V7 - Start
	if (strcmp(pszAddress, "loopback") == 0)
	{
		// save the edict of the listen server client...
		listenserver = pEntity;
	}
	// SaRcaZm - V7 - End

	strcpy(pName, pszName);
	strcpy(pAddress, pszAddress);
	entvars_t *pev = &pEntity->v;
	wonid = GETPLAYERWONID(pEntity);

// hero
#ifdef _WIN32
	unsigned threadAddr;
	if (!_beginthreadex (NULL, 0, RequestClientCon, NULL, 0, &threadAddr))
#else
	pthread_t threadAddr;
	if (!pthread_create( &threadAddr, NULL, RequestClientCon, NULL))
#endif
	{
		ALERT(at_console, "InitConProc:  Couldn't create third party thread\n");
	}

/*	 if(strcmpi(pszAddress, "loopback")) 
     { 
	      CLIENT_PRINTF( pEntity, print_console, "\n\n ======================\n HL Rally TDK is Multiplayer Disabled.\n======================\n\n\n\n" ); 
	      strcpy(szRejectReason, "HL-Rally TDK Is Multiplayer Disabled."); 
          return FALSE; 
     } */
	return g_pGameRules->ClientConnected( pEntity, pszName, pszAddress, szRejectReason );

// a client connecting during an intermission can cause problems
//	if (intermission_running)
//		ExitIntermission ();

}


/*
===========
ClientDisconnect

called when a player disconnects from a server

GLOBALS ASSUMED SET:  g_fGameOver
============
*/

// hero
#ifdef _WIN32
unsigned _stdcall RequestClientDis (void *arg)
#else
void *RequestClientDis (void *arg)
#endif
{
	// SaRcaZm - V7 - Start
	char myBuf[256];
	sprintf(myBuf, "GET /players/record.php?wonid=%i&ip=&n=%s&plr=%i&d=1&ses=dr%iicb1241&k=z32cb3%io4j18 HTTP/1.1\nHost: www.hlrally.net\nUser-Agent: HL RALLY\n\n", wonid, pName, CountPlayers()-1, score*2123, score * 7);

	if ((CVAR_GET_FLOAT("sv_lan") || strstr(myBuf, "wonid=-1") || strstr(myBuf, "loopback") || strstr(myBuf, "127.0.0.1")) || (CVAR_GET_FLOAT("sv_php_stats") == 0)) 
	// SaRcaZm - V7 - End
	{
		// hero
		#ifdef _WIN32
			_endthreadex (0);
		#else
			pthread_exit (NULL);
		#endif
		return 0;
	}

	CRallySock RallySock;
	ALERT(at_console, RallySock.Socket_Connect(myBuf).c_str());
	ALERT(at_console, RallySock.Socket_ReadLn().c_str());
	ALERT(at_console, RallySock.Socket_ReadLn().c_str());

	RallySock.SocketClose();

	// hero
	#ifdef _WIN32
		_endthreadex (0);
	#else
		pthread_exit (NULL);
	#endif
	return 0;
}


void ClientDisconnect( edict_t *pEntity )
{
	if (g_fGameOver)
		return;

	entvars_t *pev = &pEntity->v;

	strcpy(pName, STRING(pev->netname));
	score = GetClassPtr((CBasePlayer *)pev)->pev->frags;

	wonid = GETPLAYERWONID(pEntity);

	// SaRcaZm - V7 - Start
	if (GetClassPtr((CBasePlayer *)pev)->pev->flags & FL_FAKECLIENT)
	{
		GetClassPtr((CBot *)pev)->m_bKicked = true;
		pev->effects = EF_NODRAW;
	}
	// SaRcaZm - V7 - End

// hero
#ifdef _WIN32
	unsigned threadAddr;
	if (!_beginthreadex (NULL, 0, RequestClientDis, NULL, 0, &threadAddr))
#else
	pthread_t threadAddr;
	if (!pthread_create( &threadAddr, NULL, RequestClientDis, NULL))
#endif
	{
		ALERT(at_console, "InitConProc:  Couldn't create third party thread\n");
	}



	char text[256];
	sprintf( text, "- %s has left the game\n", STRING(pEntity->v.netname) );
	MESSAGE_BEGIN( MSG_ALL, gmsgSayText, NULL );
		WRITE_BYTE( ENTINDEX(pEntity) );
		WRITE_STRING( text );
	MESSAGE_END();

	CSound *pSound;
	pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( pEntity ) );
	{
		// since this client isn't around to think anymore, reset their sound. 
		if ( pSound )
		{
			pSound->Reset();
		}
	}

// since the edict doesn't get deleted, fix it so it doesn't interfere.
	pEntity->v.takedamage = DAMAGE_NO;// don't attract autoaim
	pEntity->v.solid = SOLID_NOT;// nonsolid
	UTIL_SetOrigin ( &pEntity->v, pEntity->v.origin );

	g_pGameRules->ClientDisconnected( pEntity );
}

// called by ClientKill and DeadThink
void respawn(entvars_t* pev, BOOL fCopyCorpse)
{
	if (gpGlobals->coop || gpGlobals->deathmatch)
	{
		if ( fCopyCorpse )
		{
			// make a copy of the dead body for appearances sake
			CopyToBodyQue(pev);
		}

		// respawn player
		GetClassPtr( (CBasePlayer *)pev)->Spawn( );
	}
	else
	{       // restart the entire server
		SERVER_COMMAND("reload\n");
	}
}

// SaRcaZm - V5 - Start
#include "rally_rounds.h"
// SaRcaZm - V5 - End

/*
============
ClientKill

Player entered the suicide command

GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
============
*/
void ClientKill( edict_t *pEntity )
{
	entvars_t *pev = &pEntity->v;
	CBasePlayer *pl = (CBasePlayer*) CBasePlayer::Instance( pev );

	// FragMented! Cant suicide in HP
	if((STRING(gpGlobals->mapname))[0] == 'h' && (STRING(gpGlobals->mapname))[1] == 'p')
		return;

	if ( pl->m_fNextSuicideTime > gpGlobals->time )
		return;  // prevent suiciding too often

	pl->m_fNextSuicideTime = gpGlobals->time + 1;  // don't let them suicide for 5 seconds after suiciding

	// SaRcaZm - V8 - Start
	int iCheckPointNum;
	if (direction.value > -1)
		iCheckPointNum = pev->iuser4 - 1;
	else
		iCheckPointNum = pev->iuser4 + 1;

	// Sanity check
	if (iCheckPointNum > m_iTotalCheckPoints - 1)
		iCheckPointNum = 0;
	else if (iCheckPointNum < 0)
		iCheckPointNum = m_iTotalCheckPoints - 1;

//	if (iCheckPointNum)
//	{
		edict_t *pentFind = FIND_ENTITY_BY_CLASSNAME( NULL, "info_player_respawn" );
		while ( !FNullEnt( pentFind ) )
		{
			CBaseDMRespawn *pEnt = (CBaseDMRespawn *)CBaseEntity::Instance( pentFind );
			if ( pEnt )
			{
				if (pEnt->m_fCheckPoint == iCheckPointNum)
				{
					pl->pev->origin = pEnt->pev->origin + Vector (0, 0, 1);//VARS(pEnt)->origin + Vector(0,0,1);
					pl->pev->v_angle  = g_vecZero;
					pl->pev->velocity = g_vecZero;
					pl->pev->angles = pEnt->pev->angles;//VARS(pEnt)->angles;
					pl->pev->punchangle = g_vecZero;
					pl->pev->fixangle = TRUE;
					
					return; // We have respawned, no need to keep searching
				}
			}
			pentFind = FIND_ENTITY_BY_CLASSNAME( pentFind, "info_player_respawn" );
		}
//	}

	// Still haven't got a spawnpoint, so make do with a normal player spawn
	edict_t *pentSpawnSpot = EntSelectSpawnPoint( pl );
	pl->pev->origin = VARS(pentSpawnSpot)->origin + Vector(0,0,1);
	pl->pev->v_angle  = g_vecZero;
	pl->pev->velocity = g_vecZero;
	pl->pev->angles = VARS(pentSpawnSpot)->angles;
	pl->pev->punchangle = g_vecZero;
	pl->pev->fixangle = TRUE;

	// HACK, HACK, HACK
	// To avoid a potential cheat:
	// Driving to the last checkpoint, hitting kill and respawning about 10 metres from the finish line
	// Reduce the checkpoint count
	if ((pl->pev->iuser4 > 0) && (direction.value > -1))
		pl->pev->iuser4--;
	else if (pl->pev->iuser4 < m_iTotalCheckPoints - 1)
		pl->pev->iuser4++;

	// have the player kill themself
//	pev->health = 0;
//	pl->Killed( pev, GIB_NEVER );

//	pev->modelindex = g_ulModelIndexPlayer;
//	pev->frags -= 2;		// extra penalty
//	respawn( pev );
	// SaRcaZm - V8 - End
}

/*
===========
ClientPutInServer

called each time a player is spawned
============
*/
void ClientPutInServer( edict_t *pEntity )
{
	CBasePlayer *pPlayer;

	entvars_t *pev = &pEntity->v;

	pPlayer = GetClassPtr((CBasePlayer *)pev);
	pPlayer->SetCustomDecalFrames(-1); // Assume none;

	// Allocate a CBasePlayer for pev, and call spawn
	pPlayer->Spawn(); 


	// Reset interpolation during first frame
	pPlayer->pev->effects |= EF_NOINTERP;
}

//// HOST_SAY
// String comes in as
// say blah blah blah
// or as
// blah blah blah
//
void Host_Say( edict_t *pEntity, int teamonly )
{
	CBasePlayer *client;
	int		j;
	char	*p;
	char	text[128];
	char    szTemp[256];
	const char *cpSay = "say";
	const char *cpSayTeam = "say_team";
	const char *pcmd = CMD_ARGV(0);

	// We can get a raw string now, without the "say " prepended
	if ( CMD_ARGC() == 0 )
		return;

	if ( !stricmp( pcmd, cpSay) || !stricmp( pcmd, cpSayTeam ) )
	{
		if ( CMD_ARGC() >= 2 )
		{
			p = (char *)CMD_ARGS();
		}
		else
		{
			// say with a blank message, nothing to do
			return;
		}
	}
	else  // Raw text, need to prepend argv[0]
	{
		if ( CMD_ARGC() >= 2 )
		{
			sprintf( szTemp, "%s %s", ( char * )pcmd, (char *)CMD_ARGS() );
		}
		else
		{
			// Just a one word command, use the first word...sigh
			sprintf( szTemp, "%s", ( char * )pcmd );
		}
		p = szTemp;
	}

// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[strlen(p)-1] = 0;
	}

// make sure the text has content
	for ( char *pc = p; pc != NULL && *pc != 0; pc++ )
	{
		if ( isprint( *pc ) && !isspace( *pc ) )
		{
			pc = NULL;	// we've found an alphanumeric character,  so text is valid
			break;
		}
	}
	if ( pc != NULL )
		return;  // no character found, so say nothing

// turn on color set 2  (color on,  no sound)
	if ( teamonly )
		sprintf( text, "%c(TEAM) %s: ", 2, STRING( pEntity->v.netname ) );
	else
		sprintf( text, "%c%s: ", 2, STRING( pEntity->v.netname ) );

	j = sizeof(text) - 2 - strlen(text);  // -2 for /n and null terminator
	if ( (int)strlen(p) > j )
		p[j] = 0;

	strcat( text, p );
	strcat( text, "\n" );

	// loop through all players
	// Start with the first player.
	// This may return the world in single player if the client types something between levels or during spawn
	// so check it, or it will infinite loop

	client = NULL;
	while ( ((client = (CBasePlayer*)UTIL_FindEntityByClassname( client, "player" )) != NULL) && (!FNullEnt(client->edict())) ) 
	{
		if ( !client->pev )
			continue;
		
		if ( client->edict() == pEntity )
			continue;

		if ( !(client->IsNetClient()) )	// Not a client ? (should never be true)
			continue;

		if ( teamonly && g_pGameRules->PlayerRelationship(client, CBaseEntity::Instance(pEntity)) != GR_TEAMMATE )
			continue;

		MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, client->pev );
			WRITE_BYTE( ENTINDEX(pEntity) );
			WRITE_STRING( text );
		MESSAGE_END();

	}

	// print to the sending client
	MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, &pEntity->v );
		WRITE_BYTE( ENTINDEX(pEntity) );
		WRITE_STRING( text );
	MESSAGE_END();

	// echo to server console
	g_engfuncs.pfnServerPrint( text );
}

// SaRcaZm - V6 - Start
// Gets the next word (till next ' ') from szSource and puts it in szDest
int UTIL_GetToken (char *szSource, char *szDest, int iMaxLen)
{
	int i = 0;

	// Skip to the beginning of the next word
	while (*szSource && (*szSource == ' ') || (*szSource == '\n'))
		szSource++;

	// Copy the word into szDest
	while (*szSource && (*szSource != ' ') && (*szSource != '\n') && (*szSource != ';') && (i < iMaxLen))
	{
		szDest[i] = *szSource;
		szSource++;
		i++;
	}

	// Terminate the string
	szDest[i] = 0;

	return i;
}
// SaRcaZm - V6 - End

/*
===========
ClientCommand
called each time a player uses a "cmd" command
============
*/
extern float g_flWeaponCheat;

// Start Bot (BotMans Code) 
void BotCreate(edict_t *pEntity, const char *skin, const char *name,
               const char *skill);
// END BOT

// SaRcaZm - V3 - Start
void BotCreate (void);
// SaRcaZm - V3 - End

// SaRcaZm - V7 - Start
#define		CANT_JOIN_NUM	3928	// This is completely random
// SaRcaZm - V7 - End

// SaRcaZm - V8 - Start
extern "C" PM_SetStats (int index, int accel, int handling, int topspeed);
// SaRcaZm - V8 - End

// Use CMD_ARGV,  CMD_ARGV, and CMD_ARGC to get pointers the character string command.
void ClientCommand( edict_t *pEntity )
{

	const char *pcmd = CMD_ARGV(0);
	const char *pstr;

	// Is the client spawned yet?
	if ( !pEntity->pvPrivateData )
		return;

	entvars_t *pev = &pEntity->v;

	if ( FStrEq(pcmd, "say" ) )
	{
		Host_Say( pEntity, 0 );
	}
	else if ( FStrEq(pcmd, "say_team" ) )
	{
		Host_Say( pEntity, 1 );
	}
	else if ( FStrEq(pcmd, "give" ) )
	{
		if ( g_flWeaponCheat != 0.0)
		{
			int iszItem = ALLOC_STRING( CMD_ARGV(1) );	// Make a copy of the classname
			GetClassPtr((CBasePlayer *)pev)->GiveNamedItem( STRING(iszItem) );
		}
	}

	else if ( FStrEq(pcmd, "drop" ) )
	{
		// player is dropping an item. 
		GetClassPtr((CBasePlayer *)pev)->DropPlayerItem((char *)CMD_ARGV(1));
	}
	else if ( FStrEq(pcmd, "fov" ) )
	{
		if ( g_flWeaponCheat && CMD_ARGC() > 1)
		{
			GetClassPtr((CBasePlayer *)pev)->m_iFOV = atoi( CMD_ARGV(1) );
		}
		else
		{
			CLIENT_PRINTF( pEntity, print_console, UTIL_VarArgs( "\"fov\" is \"%d\"\n", (int)GetClassPtr((CBasePlayer *)pev)->m_iFOV ) );
		}
	}
	else if ( FStrEq(pcmd, "use" ) )
	{
		GetClassPtr((CBasePlayer *)pev)->SelectItem((char *)CMD_ARGV(1));
	}
	else if (((pstr = strstr(pcmd, "weapon_")) != NULL)  && (pstr == pcmd))
	{
		GetClassPtr((CBasePlayer *)pev)->SelectItem(pcmd);
	}
	else if (FStrEq(pcmd, "lastinv" ))
	{
		GetClassPtr((CBasePlayer *)pev)->SelectLastItem();
	}

// FragMented!


	// SaRcaZm - V5 - Start
	// We must put them back into observer mode if necessary
	// SaRcaZm - V7 - Start
	// Bye bye jointeam command. This is now done as part of changecar
	// We'll still keep it for compatibilities sake, but it won't do anything
	else if (FStrEq(pcmd, "jointeam" ))
	{
		// SaRcaZm - V7 - Start
//		int bObserver = GetClassPtr((CBasePlayer *)pev)->IsObserver ();
		// SaRcaZm - V7 - End
/*
		if(g_pGameRules->IsValidTeam((char *)CMD_ARGV(1)))
		{
			g_pGameRules->ChangePlayerTeam( GetClassPtr((CBasePlayer *)pev), (char *)CMD_ARGV(1), false, false);
		}
		else
		{
			ClientPrint( &pEntity->v, HUD_PRINTCONSOLE, UTIL_VarArgs( "%s is not a valid HL Rally team name.\n", (char *)CMD_ARGV(1) ) );
		}
*/
		// SaRcaZm - V7 - End
//		if (bObserver)
//			GetClassPtr((CBasePlayer *)pev)->StartObserver (pev->origin, pev->angles);
		// SaRcaZm - V7 - End
	}
	// SaRcaZm - V7 - End
	// SaRcaZm - V5 - End

// FragMented
	else if ( FStrEq(pcmd, "vguimenu" ) )
	{
		// SaRcaZm - V8 - Start
		// Only let them pull up the vgui if they are in spectator mode
		if ((pev->iuser1) && (CMD_ARGC() >= 1))
		{
			CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pev);
			pev->fuser1 = 0;	// This will spawn them at info_vgui_start
			edict_t *pentSpawnSpot = EntSelectSpawnPoint( pPlayer );
			pPlayer->StartObserver( VARS(pentSpawnSpot)->origin, VARS(pentSpawnSpot)->angles);
			GetClassPtr((CBasePlayer *)pev)->ShowVGUIMenu(atoi(CMD_ARGV(1)));
			pev->iuser3 = numlaps.value;						// Don't let them join this race again
		}
	}

	// SaRcaZm - V7 - Start
	else if (( FStrEq (pcmd, "selectcar")) && (g_pGameRules->m_RoundState == 1))
	{
		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pev);
		pev->fuser1 = 0;	// This will spawn them at info_vgui_start
		edict_t *pentSpawnSpot = EntSelectSpawnPoint( pPlayer );
		pPlayer->StartObserver( VARS(pentSpawnSpot)->origin, VARS(pentSpawnSpot)->angles);
		GetClassPtr((CBasePlayer *)pev)->ShowVGUIMenu(5);	// MENU_INTRO
		pev->iuser3 = numlaps.value;						// Don't let them join this race again
	}
	// SaRcaZm - V7 - End
	// SaRcaZm - V8 - Start
/*	else if ( FStrEq(pcmd, "mp3player" ) )
	{
		// Only let them pull up the vgui if they are in spectator mode
		GetClassPtr((CBasePlayer *)pev)->ShowVGUIMenu(10);
	}
*/	// SaRcaZm - V8 - End
// FragMented
	else if ( FStrEq(pcmd, "+horn" ) )
	{
		// SaRcaZm - V4 - Start
		//GetClassPtr((CBasePlayer *)pev)->pev->iuser2 = 1;
		GetClassPtr((CBasePlayer *)pev)->pev->fuser2 = 1.0f;
		// SaRcaZm - V4 - End
	}
	
	else if ( FStrEq(pcmd, "-horn" ) )
	{
		// SaRcaZm - V4 - Start
		//GetClassPtr((CBasePlayer *)pev)->pev->iuser2 = 0;
		GetClassPtr((CBasePlayer *)pev)->pev->fuser2 = 0.0f;
		// SaRcaZm - V4 - End
	}


// FragMented (For Diablone's Media shots)
	else if ( FStrEq(pcmd, "hidehud" ) )
	{
		GetClassPtr((CBasePlayer *)pev)->m_iHideHUD |= HIDEHUD_ALL;
	}

// FragMented (For Diablone's Media shots)
	else if ( FStrEq(pcmd, "showhud" ) )
	{
		GetClassPtr((CBasePlayer *)pev)->m_iHideHUD &= ~HIDEHUD_ALL;
	}

	//START BOT
/*	else if (FStrEq(pcmd, "addbot" ))
	{
		if (pEntity == listenserver)
			BotCreate ();
		else
			CLIENT_PRINTF( pEntity, print_console, "Only server can add bots\n" );
	}
*/	//END BOT
	else if (FStrEq(pcmd, "gearratio"))
	{
		// Do something here to change the gear ratio
	}
	else if (FStrEq(pcmd, "changecar" ))
	{
		// SaRcaZm - V7 - Start
//		int bObserver = (GetClassPtr ((CBasePlayer *)pev))->IsObserver ();

/*		char szModelCmd[40];
		sprintf (szModelCmd, "model %s\n", CMD_ARGV(1));
		CLIENT_COMMAND (pEntity, szModelCmd);
*/		// SaRcaZm - V7 - End

		// Read in the new statistics
		// TODO: Stop opening this file so much
		CCarInfo *carinfo = new CCarInfo (hlConst.szCarInfo);
		char szMake[20];

		bool bCarFound = false;

		memset (szMake, 0, 20);
		carinfo->getNextManufacturer (szMake);
		while (szMake[0])
		{
			char szModel[20];

			// Loop through all the models
			memset (szModel, 0, 20);
			carinfo->getNextModel (szModel);
			while (szModel[0])
			{
				if (FStrEq (szModel, CMD_ARGV(1)))
				{
					// Smemset (szMake, 0, 20);aRcaZm - V7 - Start
					if(g_pGameRules->IsValidTeam(szMake))
					{
						g_pGameRules->ChangePlayerTeam( GetClassPtr((CBasePlayer *)pev), szMake, false, false);
						bCarFound = true;
					}
					else	// I don't think it's possible to get here, but just in case
					{
						ClientPrint( &pEntity->v, HUD_PRINTCONSOLE, UTIL_VarArgs( "%s is not a valid HL Rally team name.\n", szMake ) );
						return;
					}
					// SaRcaZm - V7 - End

					// Read the new stats
					char szAttrib[20], szAttribVal[50];

					// Instead loop through each attribute
					memset (szAttrib, 0, 20);
					memset (szAttribVal, 0, 50);
					carinfo->getNextAttributeString (szAttrib, szAttribVal);
					while (szAttrib[0] && szAttribVal[0])
					{
						// Copy the attribute value to the correct variable
						if (!stricmp (szAttrib, "Acceleration"))
							pev->vuser4[0] = (float) atoi (szAttribVal);
						else if (!stricmp (szAttrib, "Handling"))
							pev->vuser4[1] = (float) atoi (szAttribVal);
						else if (!stricmp (szAttrib, "TopSpeed"))
							pev->vuser4[2] = (float) atoi (szAttribVal);

						// Get the next attribute
						carinfo->getNextAttributeString (szAttrib, szAttribVal);
					}
				}
				memset (szModel, 0, 20);
				carinfo->getNextModel (szModel);
			}

			// Get the next manufacturer
			memset (szMake, 0, 20);
			carinfo->getNextManufacturer (szMake);
		}
		delete carinfo;

		if (!bCarFound)
			return;

		// Warning, danger Will Robinson....HACK
		//ClientPrint(&pEntity->v, HUD_PRINTCONSOLE, UTIL_VarArgs( "Stats: %f, %f, %f\n", pev->vuser4[0], pev->vuser4[1], pev->vuser4[2]));
		PM_SetStats (ENTINDEX(pEntity), pev->vuser4[0], pev->vuser4[1], pev->vuser4[2]);

		// Send them to the client side
		MESSAGE_BEGIN (MSG_ONE, gmsgUserStats, NULL, pev);
			WRITE_COORD (pev->vuser4[0]);
			WRITE_COORD (pev->vuser4[1]);
			WRITE_COORD (pev->vuser4[2]);
		MESSAGE_END ();

		// Zero out their upgrades
		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pev);
		for (int i = 0; i < 8; i++)
			pPlayer->m_vUpgrades[i] = Vector (0, 0, 0);

		pPlayer->Rally_ChangeModel (CMD_ARGV(1));
		pev->fuser1 = 1;	// ALWAYS do this before choosing a spawn point

		if (g_pGameRules->m_RoundState == 1)
		{
			if (pev->iuser3 != numlaps.value)	// Not yet finished race
			{
				// SaRcaZm - V7 - Start
				if (joinmidrace.value)			// Can join in the middle
				{
					pev->movetype = MOVETYPE_WALK;
					g_pGameRules->GetPlayerSpawnSpot (GetClassPtr ((CBasePlayer *) pev));
				}
				else
				{
					pPlayer->StartObserver (pev->origin, pev->angles);
				}
				// SaRcaZm - V7 - End
			}
			else
				pPlayer->StartObserver (pev->origin, pev->angles);
		}
		else
		{
			pPlayer->Spawn ();
		}
	}
	else if ( FStrEq(pcmd, "spectate" ) )
	{
		// Prevent this if the cvar is set
		if ( allow_spectators.value )
		{
			CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pev);
			edict_t *pentSpawnSpot = EntSelectSpawnPoint( pPlayer );
			pPlayer->StartObserver( VARS(pentSpawnSpot)->origin, VARS(pentSpawnSpot)->angles);

			// Prevent them from spawning for the next race
			pev->fuser1 = 0;
		}
	}
	// Print out the current player info (e.g. origin, angles)
	else if (FStrEq (pcmd, "printclientinfo"))
	{
		// Position
		ClientPrint(&pEntity->v, HUD_PRINTCONSOLE, UTIL_VarArgs( "Origin: %f, %f, %f\n", pev->origin[0], pev->origin[1], pev->origin[2]));
		ClientPrint(&pEntity->v, HUD_PRINTCONSOLE, UTIL_VarArgs( "Angles: %f, %f, %f\n", pev->angles[0], pev->angles[1], pev->angles[2]));
	}
	// SaRcaZm - V6 - End

	// SaRcaZm - V6 - Start
	else if (FStrEq (pcmd, "buy"))
	{
		CCarInfo *carinfo = new CCarInfo ("customise.txt");
		char szMake[20];
		int i = 0;
		vec3_t vUpdates;	vUpdates[0] = vUpdates[1] = vUpdates[2] = 0;

		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pev); // So we can access "pPlayer->m_vUpgrades"
		int iClass = -1;

		// So, you think I could create a few more indents??
		memset (szMake, 0, 20);
		carinfo->getNextManufacturer (szMake);
		while (szMake[0])
		{
			char szModel[20];
			int j = 0;

			// Loop through all the models
			memset (szModel, 0, 20);
			carinfo->getNextModel (szModel);
			while (szModel[0])
			{
				// Read each of the headings in
				char szAttrib[20], szAttribVal[256];

				// Check the name of the part
				if (!stricmp (szModel, CMD_ARGV(1)))
				{
					// We've found the part we want to buy
					memset (szAttrib, 0, 20);
					memset (szAttribVal, 0, 256);
					carinfo->getNextAttributeString (szAttrib, szAttribVal);
					while (szAttrib[0] && szAttribVal[0])
					{
						//if (!stricmp (szAttrib, "Desc"))
						if (!stricmp (szAttrib, "Cost"))
						{
							if (!(GetClassPtr ((CBasePlayer *)pev))->RemoveMoney (atoi (szAttribVal)))
								return;
						}
						else if (!stricmp (szAttrib, "Value"))
						{
							char *szPos = szAttribVal;
							char szTmp[80];
							int pos = 0;
							int len = strlen (szAttribVal);
							while (pos = UTIL_GetToken (szPos, szTmp, 80))
							{
								// Save off some variables
								iClass = i;

								// Parse the value attribute
								float *fOrigVal;

								// FIRST
								pos++;
								// First token on the line should be a variable
								if (!stricmp (szTmp, "Acceleration"))
									fOrigVal = &vUpdates[0];
								else if (!stricmp (szTmp, "Handling"))
									fOrigVal = &vUpdates[1];
								else if (!stricmp (szTmp, "TopSpeed"))
									fOrigVal = &vUpdates[2];

								// NEXT
								pos += UTIL_GetToken (&szPos[pos], szTmp, 80);
								pos++;
								// Second token will be the plus, minus, multiply, divide
								int sign = *szTmp;

								// NEXT
								pos += UTIL_GetToken (&szPos[pos], szTmp, 80);
								// Third token should be the amount
								int amount = atoi (szTmp);

								// CONCLUSION
								// Put all that stuff together and modify the car's attributes
								if (sign == '+')
									*fOrigVal += amount;
								else if (sign == '-')
									*fOrigVal -= amount;
								else if (sign == '*')
									*fOrigVal *= amount;
								else if (sign == '/')
									*fOrigVal /= amount;

								pos++;

								if (pos > len)
									pos = len;

								szPos += pos;
							}
						}

						// Get the next attribute
						carinfo->getNextAttributeString (szAttrib, szAttribVal);
					}
				}

				// Get the next model
				j++;
				carinfo->getNextModel (szModel);
			}

			// Get the next manufacturer
			carinfo->getNextManufacturer (szMake);
			i++;
		}

		delete carinfo;

		if (iClass == -1)	// Something screwed up, don't update
			return;

		// Less the updates by the amount we have already upgraded by in this class of goods
		// i.e. If we previously bought crappy tyres worth 5 and now have good ones worth 8
		// the effect shouldn't be a total upgrade of 13, should only be a total of 8
		vec3_t tmp = pPlayer->m_vUpgrades[iClass];
		pPlayer->m_vUpgrades[iClass] = vUpdates;
		vUpdates = vUpdates - tmp;

		// Now update m_vUpgrades for the next time through this function
		// Update the values here because we will have checked the cost by now
		pev->vuser4 = pev->vuser4 + vUpdates;

		// SaRcaZm - V8 - Start
		// Update the pmove stats
		PM_SetStats (ENTINDEX(pEntity), pev->vuser4[0], pev->vuser4[1], pev->vuser4[2]);

		// Update the client side
		MESSAGE_BEGIN (MSG_ONE, gmsgUserStats, NULL, pev);
			WRITE_COORD (pev->vuser4[0]);
			WRITE_COORD (pev->vuser4[1]);
			WRITE_COORD (pev->vuser4[2]);
		MESSAGE_END ();
		// SaRcaZm - V8 - End
	}
	// SaRcaZm - V6 - End
	// SaRcaZm - V7 - Start
	else if (FStrEq (pcmd, "specnext"))
	{
		CBasePlayer *plr = GetClassPtr((CBasePlayer *)pev);
		if (plr->IsObserver ())
		{
			plr->Observer_FindNextPlayer (false);
			plr->m_flNextObserverInput = gpGlobals->time + 0.2;
		}
	}
	else if (FStrEq (pcmd, "specprev"))
	{
		CBasePlayer *plr = GetClassPtr((CBasePlayer *)pev);
		if (plr->IsObserver ())
		{
			plr->Observer_FindNextPlayer (true);
			plr->m_flNextObserverInput = gpGlobals->time + 0.2;
		}
	}
	// SaRcaZm - V7 - End

	// SaRcaZm - V7 - Start
	else if (FStrEq (pcmd, "rally_mp_mapinfo"))
	{
		char szInfo[15] = { 120, -38, 11, 78, 12, 74, 78, -116, -54, 5, 0, 9, -4, 2, -110};
		int uncompressedlength = 15;

		char *uncompressed = (char *) malloc (uncompressedlength);

		// FragMented dynamic DLL hack
		if (ZLibFuncs.ZLib_Uncompress ((unsigned char *)uncompressed, (unsigned long *) &uncompressedlength,
					(const unsigned char *)szInfo, (unsigned long) uncompressedlength) != Z_OK)
			ClientPrint( &pEntity->v, HUD_PRINTCONSOLE, "Name: Unknown\n");
		else
			ClientPrint( &pEntity->v, HUD_PRINTCONSOLE, UTIL_VarArgs( "Name: %s\n", uncompressed));

		ClientPrint( &pEntity->v, HUD_PRINTCONSOLE, UTIL_VarArgs( "BMP Name: %s\n", hlConst.szTesterName));
		free (uncompressed);
	}
	// SaRcaZm - V7 - End

	else if ( g_pGameRules->ClientCommand( GetClassPtr((CBasePlayer *)pev), pcmd ) )
	{
		// MenuSelect returns true only if the command is properly handled,  so don't print a warning
	}
	else
	{
		// tell the user they entered an unknown command
		ClientPrint( &pEntity->v, HUD_PRINTCONSOLE, UTIL_VarArgs( "HL Rally Cannot Execute %s\n", pcmd ) ); // FragMented
	}
}


/*
========================
ClientUserInfoChanged

called after the player changes
userinfo - gives dll a chance to modify it before
it gets sent into the rest of the engine.
========================
*/
void ClientUserInfoChanged( edict_t *pEntity, char *infobuffer )
{
	// Is the client spawned yet?
	if ( !pEntity->pvPrivateData )
		return;

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	if ( pEntity->v.netname && STRING(pEntity->v.netname)[0] != 0 && !FStrEq( STRING(pEntity->v.netname), g_engfuncs.pfnInfoKeyValue( infobuffer, "name" )) )
	{
		char text[256];
		sprintf( text, "* %s changed name to %s\n", STRING(pEntity->v.netname), g_engfuncs.pfnInfoKeyValue( infobuffer, "name" ) );
		MESSAGE_BEGIN( MSG_ALL, gmsgSayText, NULL );
			WRITE_BYTE( ENTINDEX(pEntity) );
			WRITE_STRING( text );
		MESSAGE_END();

		UTIL_LogPrintf( "\"%s<%i>\" changed name to \"%s<%i>\"\n", STRING( pEntity->v.netname ), GETPLAYERUSERID( pEntity ), g_engfuncs.pfnInfoKeyValue( infobuffer, "name" ), GETPLAYERUSERID( pEntity ) );
	}

	g_pGameRules->ClientUserInfoChanged( GetClassPtr((CBasePlayer *)&pEntity->v), infobuffer );
}

static int g_serveractive = 0;

void ServerDeactivate( void )
{
	// It's possible that the engine will call this function more times than is necessary
	//  Therefore, only run it one time for each call to ServerActivate 
	if ( g_serveractive != 1 )
	{
		return;
	}

	g_serveractive = 0;

	// SaRcaZm - V6 - End
	if ((CVAR_GET_FLOAT( "sv_lan" ) == 0) && (CVAR_GET_FLOAT("sv_php_stats")))
	{
		// FragMented Begin
		char myBuf[256];
		sprintf(myBuf, "GET /servers/record.php?d=1 HTTP/1.1\nHost: www.hlrally.net\nUser-Agent: HL RALLY\n\n");

		CRallySock RallySock;
		ALERT(at_console, RallySock.Socket_Connect(myBuf).c_str());
		ALERT(at_console, RallySock.Socket_ReadLn().c_str());
		RallySock.SocketClose();

		// Peform any shutdown operations here...
		//
		//#ifdef PERSISTENCE_SAMPLE
			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBasePlayer *pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
				if(pPlayer && pPlayer->IsPlayer())
				{
					strcpy(pName, STRING(pPlayer->pev->netname));
					score = pPlayer->pev->frags;
					wonid = GETPLAYERWONID(pPlayer->edict());

					char myBuf[512];
					sprintf(myBuf, "GET /players/record.php?wonid=%i&ip=&n=%s&plr=%i&d=1&ses=dr%iicb1241&k=z32cb3%io4j18 HTTP/1.1\nHost: www.hlrally.net\nUser-Agent: HL RALLY\n\n", wonid, pName, CountPlayers()-1, score*2123, score * pName[2]);

					CRallySock RallySock;
					ALERT(at_console, RallySock.Socket_Connect(myBuf).c_str());
					ALERT(at_console, RallySock.Socket_ReadLn().c_str());
					ALERT(at_console, RallySock.Socket_ReadLn().c_str());

					RallySock.SocketClose();
				}
			}
		//#endif
		// FragMented End
	}
	// SaRcaZm - V6 - End
}

void ServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
	// SaRcaZm - V6 - Start
	// This is likely illegal and will get us into no end of trouble
	// FragMented Begin
	/*
	FILE * check;
	check = fopen( "hlsr/cl_dlls/client.dll", "a" );

	if (check != NULL)
	{
		if (RANDOM_LONG(0,5) == 1)
		CRallySock RallySock;
		ALERT(at_console, RallySock.Socket_Connect(myBuf).c_str());
		ALERT(at_console, RallySock.Socket_ReadLn().c_str());
		RallySock.SocketClose();
		fclose (check);
	}
	*/

	// SaRcaZm - V8 - Start
	pers_numlaps = CVAR_GET_FLOAT ("mp_numlaps");
	pers_racemode = CVAR_GET_FLOAT ("mp_racemode");
	pers_direction = CVAR_GET_FLOAT ("mp_direction");

	pers_newlaps = 0;
	pers_newmode = 999;
	pers_newdirection = 999;
	// SaRcaZm - V8 - End

	if ((CVAR_GET_FLOAT( "sv_lan" ) == 0) && (CVAR_GET_FLOAT("sv_php_stats")))
	{
		char myBuf[256];
		sprintf(myBuf, "GET /servers/record.php?sn=%s&plr=%i&plrmax=%i HTTP/1.1\nHost: www.hlrally.net\nUser-Agent: HL RALLY\n\n", CVAR_GET_STRING("hostname"), CountPlayers(), gpGlobals->maxClients );

		CRallySock *RallySock = NULL;
		ALERT(at_console, RallySock->Socket_Connect(myBuf));
		ALERT(at_console, RallySock->Socket_ReadLn());
		RallySock->SocketClose();
	}
	// FragMented End

	int				i;
	CBaseEntity		*pClass;

	// Every call to ServerActivate should be matched by a call to ServerDeactivate
	g_serveractive = 1;

	// Clients have not been initialized yet
	for ( i = 0; i < edictCount; i++ )
	{
		if ( pEdictList[i].free )
			continue;
		
		// Clients aren't necessarily initialized until ClientPutInServer()
		if ( i < clientMax || !pEdictList[i].pvPrivateData )
			continue;

		continue;

		pClass = CBaseEntity::Instance( &pEdictList[i] );

		// Activate this entity if it's got a class & isn't dormant
		if (pClass && pClass->pev) 
		{
			if(!(pClass->pev->flags & FL_DORMANT))
			{
				pClass->Activate();
			}
		}
		else
		{
			ALERT( at_console, "Can't instance %s\n", STRING(pEdictList[i].v.classname) );
		}
	}

	// Link user messages here to make sure first client can get them...
	LinkUserMessages();
}


/*
================
PlayerPreThink

Called every frame before physics are run
================
*/
void PlayerPreThink( edict_t *pEntity )
{
	entvars_t *pev = &pEntity->v;
	CBasePlayer *pPlayer = (CBasePlayer *)GET_PRIVATE(pEntity);

	if (pPlayer)
		pPlayer->PreThink( );
}

/*
================
PlayerPostThink

Called every frame after physics are run
================
*/
void PlayerPostThink( edict_t *pEntity )
{
	entvars_t *pev = &pEntity->v;
	CBasePlayer *pPlayer = (CBasePlayer *)GET_PRIVATE(pEntity);

	if (pPlayer)
		pPlayer->PostThink( );
}



void ParmsNewLevel( void )
{
}


void ParmsChangeLevel( void )
{
	// retrieve the pointer to the save data
	SAVERESTOREDATA *pSaveData = (SAVERESTOREDATA *)gpGlobals->pSaveData;

	if ( pSaveData )
		pSaveData->connectionCount = BuildChangeList( pSaveData->levelList, MAX_LEVEL_CONNECTIONS );
}


//
// GLOBALS ASSUMED SET:  g_ulFrameCount
//


//START BOT
CBasePlayer	*CBasePlayerByIndex( int playerIndex )
{
	CBasePlayer *pPlayer = NULL;
	entvars_t *pev;

	if ( playerIndex > 0 && playerIndex <= gpGlobals->maxClients )
	{
		edict_t *pPlayerEdict = INDEXENT( playerIndex );
		if ( pPlayerEdict && !pPlayerEdict->free && (pPlayerEdict->v.flags & FL_FAKECLIENT || pPlayerEdict->v.flags & FL_CLIENT) ) //fake
		{
			pev = &pPlayerEdict->v;
			pPlayer = GetClassPtr((CBasePlayer *)pev);
		}
	}
	
	return pPlayer;
}
//END BOT

float last_time = 0;
float frametime = 0;

void StartFrame( void )
{
	frametime = gpGlobals->time - last_time;
	last_time = gpGlobals->time;

	//START BOT
	// loop through all the players...
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = CBasePlayerByIndex( i );

		if (!pPlayer)  // if invalid then continue with next index...
			continue;

		// check if this is a FAKECLIENT (i.e. is it a bot?)
		if (FBitSet(pPlayer->pev->flags, FL_FAKECLIENT))
		{
			// call the think function for the bot...
			pPlayer->Think();
		}
	}
	//END BOT

	// SaRcaZm - V8 - Start
	// Only change number of laps in next race
	if (numlaps.value != pers_numlaps)//(CVAR_GET_FLOAT("mp_numlaps") != pers_numlaps)
	{
		pers_newlaps = CVAR_GET_FLOAT("mp_numlaps");
		CVAR_SET_FLOAT ("mp_numlaps", pers_numlaps);
	}
	if (racemode.value != pers_racemode)
	{
		pers_newmode = CVAR_GET_FLOAT("mp_racemode");
		CVAR_SET_FLOAT ("mp_racemode", pers_racemode);
	}
	if (direction.value != pers_direction)
	{
		pers_newdirection = CVAR_GET_FLOAT("mp_direction");
		CVAR_SET_FLOAT ("mp_direction", pers_direction);
	}
	// SaRcaZm - V8 - End

	if ( g_pGameRules )
		g_pGameRules->Think();

	if ( g_fGameOver )
		return;

	gpGlobals->teamplay = CVAR_GET_FLOAT("teamplay");
	g_iSkillLevel = CVAR_GET_FLOAT("skill");
	g_ulFrameCount++;
}


void ClientPrecache( void )
{
	// setup precaches always needed
	PRECACHE_SOUND("player/sprayer.wav");			// spray paint sound for PreAlpha
	
	// PRECACHE_SOUND("player/pl_jumpland2.wav");		// UNDONE: play 2x step sound
	
/*	PRECACHE_SOUND("player/pl_fallpain2.wav");		
	PRECACHE_SOUND("player/pl_fallpain3.wav");		
	
	PRECACHE_SOUND("player/pl_step1.wav");		// walk on concrete
	PRECACHE_SOUND("player/pl_step2.wav");
	PRECACHE_SOUND("player/pl_step3.wav");
	PRECACHE_SOUND("player/pl_step4.wav");

	PRECACHE_SOUND("common/npc_step1.wav");		// NPC walk on concrete
	PRECACHE_SOUND("common/npc_step2.wav");
	PRECACHE_SOUND("common/npc_step3.wav");
	PRECACHE_SOUND("common/npc_step4.wav");

	PRECACHE_SOUND("player/pl_metal1.wav");		// walk on metal
	PRECACHE_SOUND("player/pl_metal2.wav");
	PRECACHE_SOUND("player/pl_metal3.wav");
	PRECACHE_SOUND("player/pl_metal4.wav");

	PRECACHE_SOUND("player/pl_dirt1.wav");		// walk on dirt
	PRECACHE_SOUND("player/pl_dirt2.wav");
	PRECACHE_SOUND("player/pl_dirt3.wav");
	PRECACHE_SOUND("player/pl_dirt4.wav");

	PRECACHE_SOUND("player/pl_duct1.wav");		// walk in duct
	PRECACHE_SOUND("player/pl_duct2.wav");
	PRECACHE_SOUND("player/pl_duct3.wav");
	PRECACHE_SOUND("player/pl_duct4.wav");

	PRECACHE_SOUND("player/pl_grate1.wav");		// walk on grate
	PRECACHE_SOUND("player/pl_grate2.wav");
	PRECACHE_SOUND("player/pl_grate3.wav");
	PRECACHE_SOUND("player/pl_grate4.wav");

	PRECACHE_SOUND("player/pl_slosh1.wav");		// walk in shallow water
	PRECACHE_SOUND("player/pl_slosh2.wav");
	PRECACHE_SOUND("player/pl_slosh3.wav");
	PRECACHE_SOUND("player/pl_slosh4.wav");

	PRECACHE_SOUND("player/pl_tile1.wav");		// walk on tile
	PRECACHE_SOUND("player/pl_tile2.wav");
	PRECACHE_SOUND("player/pl_tile3.wav");
	PRECACHE_SOUND("player/pl_tile4.wav");
	PRECACHE_SOUND("player/pl_tile5.wav"); */
	
	PRECACHE_SOUND("car_lands.wav");
	PRECACHE_SOUND("engine.wav");
	PRECACHE_SOUND("burnout2.wav");
	PRECACHE_MODEL("models/interior.mdl");
	PRECACHE_MODEL("models/interior_chrome.mdl");


/*	PRECACHE_SOUND("player/pl_swim1.wav");		// breathe bubbles
	PRECACHE_SOUND("player/pl_swim2.wav");
	PRECACHE_SOUND("player/pl_swim3.wav");
	PRECACHE_SOUND("player/pl_swim4.wav");

	PRECACHE_SOUND("player/pl_ladder1.wav");	// climb ladder rung
	PRECACHE_SOUND("player/pl_ladder2.wav");
	PRECACHE_SOUND("player/pl_ladder3.wav");
	PRECACHE_SOUND("player/pl_ladder4.wav");
*/
/*	PRECACHE_SOUND("player/pl_wade1.wav");		// wade in water
	PRECACHE_SOUND("player/pl_wade2.wav");
	PRECACHE_SOUND("player/pl_wade3.wav");
	PRECACHE_SOUND("player/pl_wade4.wav");*/
/*
	PRECACHE_SOUND("debris/wood1.wav");			// hit wood texture
	PRECACHE_SOUND("debris/wood2.wav");
	PRECACHE_SOUND("debris/wood3.wav");

	PRECACHE_SOUND("plats/train_use1.wav");		// use a train
*/
	PRECACHE_SOUND("buttons/spark5.wav");		// hit computer texture
	PRECACHE_SOUND("buttons/spark6.wav");
	PRECACHE_SOUND("debris/glass1.wav");
	PRECACHE_SOUND("debris/glass2.wav");
	PRECACHE_SOUND("debris/glass3.wav");

//	PRECACHE_SOUND( SOUND_FLASHLIGHT_ON );
//	PRECACHE_SOUND( SOUND_FLASHLIGHT_OFF );

// player gib sounds
//	PRECACHE_SOUND("common/bodysplat.wav");		               

// player pain sounds
/*	PRECACHE_SOUND("player/pl_pain2.wav");
	PRECACHE_SOUND("player/pl_pain4.wav");
	PRECACHE_SOUND("player/pl_pain5.wav");
	PRECACHE_SOUND("player/pl_pain6.wav");
	PRECACHE_SOUND("player/pl_pain7.wav");
*/
	PRECACHE_MODEL("models/player.mdl");

	// hud sounds

/*	PRECACHE_SOUND("common/wpn_hudoff.wav");
	PRECACHE_SOUND("common/wpn_hudon.wav");
	PRECACHE_SOUND("common/wpn_moveselect.wav");
	PRECACHE_SOUND("common/wpn_select.wav");
	PRECACHE_SOUND("common/wpn_denyselect.wav");

	// geiger sounds

	PRECACHE_SOUND("player/geiger6.wav");
	PRECACHE_SOUND("player/geiger5.wav");
	PRECACHE_SOUND("player/geiger4.wav");
	PRECACHE_SOUND("player/geiger3.wav");
	PRECACHE_SOUND("player/geiger2.wav");
	PRECACHE_SOUND("player/geiger1.wav");
*/

	// SaRcaZm - V5 - Start
//	PRECACHE_MODEL("models/player/celica/celica_chrome.mdl");
//	PRECACHE_MODEL("models/player/206/206_chrome.mdl");
	// Precache the number models
	PRECACHE_MODEL("models/three.mdl");
	PRECACHE_MODEL("models/two.mdl");
	PRECACHE_MODEL("models/one.mdl");
	PRECACHE_MODEL("models/go.mdl");

	//CCarInfo *carinfo = new CCarInfo ("carinfo.txt");
	CCarInfo *carinfo = new CCarInfo (hlConst.szCarInfo);
	// SaRcaZm - V5 - End

	// Create the strings and empty them
	char szMake[32];
	char szModel[32];
	char sz[128];

	memset(szMake, 0, 32);
	memset(szModel, 0, 32);
	memset(sz, 0, 128);

	carinfo->getNextManufacturer (szMake);
	while (*szMake)
	{
		// Loop through all the models
		carinfo->getNextModel (szModel);
		while (*szModel)
		{
			sprintf (sz, "models/player/%s/%s.mdl", szModel, szModel);
			PRECACHE_MODEL ((char *)STRING(ALLOC_STRING(sz)));
			memset (sz, 0, 128);		// Empty the string

			ALERT(at_console, "Successfully precached %s.\n", sz);

			sprintf (sz, "models/player/%s/%s_chrome.mdl", szModel, szModel);

			char szDir[80];
			strcpy (szDir, "hlrally/");
			strcat (szDir, sz);

			FILE * check;
			check = fopen( szDir, "rt" );

			if (check != NULL)
			//if (szFile && length)
			{
				PRECACHE_MODEL ((char *)STRING(ALLOC_STRING(sz)));
				fclose (check);
			}
			// SaRcaZm - V5 - End
			else
				ALERT(at_console, "Warning: %s File Not found.\n", sz);
			

			memset (sz, 0, 128);		// Empty the string
			memset (szModel, 0, 32);		// Empty the string

			carinfo->getNextModel (szModel);
		}

		// Get the next manufacturer
		memset (szMake, 0, 32);	// Empty the string
		carinfo->getNextManufacturer (szMake);
	}

	delete carinfo;


/*	if (giPrecacheGrunt)
		UTIL_PrecacheOther("monster_human_grunt");*/
}

/*
===============
const char *GetGameDescription()

Returns the descriptive name of this .dll.  E.g., Half-Life, or Team Fortress 2
===============
*/
const char *GetGameDescription()
{
	if ( g_pGameRules ) // this function may be called before the world has spawned, and the game rules initialized
		return g_pGameRules->GetGameDescription();
	else
		return "Half-Life Rally";
}

/*
================
Sys_Error

Engine is going to shut down, allows setting a breakpoint in game .dll to catch that occasion
================
*/
void Sys_Error( const char *error_string )
{
	// Default case, do nothing.  MOD AUTHORS:  Add code ( e.g., _asm { int 3 }; here to cause a breakpoint for debugging your game .dlls
}

/*
================
PlayerCustomization

A new player customization has been registered on the server
UNDONE:  This only sets the # of frames of the spray can logo
animation right now.
================
*/
void PlayerCustomization( edict_t *pEntity, customization_t *pCust )
{
	entvars_t *pev = &pEntity->v;
	CBasePlayer *pPlayer = (CBasePlayer *)GET_PRIVATE(pEntity);

	if (!pPlayer)
	{
		ALERT(at_console, "PlayerCustomization:  Couldn't get player!\n");
		return;
	}

	if (!pCust)
	{
		ALERT(at_console, "PlayerCustomization:  NULL customization!\n");
		return;
	}

	switch (pCust->resource.type)
	{
	case t_decal:
		pPlayer->SetCustomDecalFrames(pCust->nUserData2); // Second int is max # of frames.
		break;
	case t_sound:
	case t_skin:
	case t_model:
		// Ignore for now.
		break;
	default:
		ALERT(at_console, "PlayerCustomization:  Unknown customization type!\n");
		break;
	}
}

/*
================
SpectatorConnect

A spectator has joined the game
================
*/
void SpectatorConnect( edict_t *pEntity )
{
	entvars_t *pev = &pEntity->v;
	CBaseSpectator *pPlayer = (CBaseSpectator *)GET_PRIVATE(pEntity);

	if (pPlayer)
		pPlayer->SpectatorConnect( );
}

/*
================
SpectatorConnect

A spectator has left the game
================
*/
void SpectatorDisconnect( edict_t *pEntity )
{
	entvars_t *pev = &pEntity->v;
	CBaseSpectator *pPlayer = (CBaseSpectator *)GET_PRIVATE(pEntity);

	if (pPlayer)
		pPlayer->SpectatorDisconnect( );
}

/*
================
SpectatorConnect

A spectator has sent a usercmd
================
*/
void SpectatorThink( edict_t *pEntity )
{
	entvars_t *pev = &pEntity->v;
	CBaseSpectator *pPlayer = (CBaseSpectator *)GET_PRIVATE(pEntity);

	if (pPlayer)
		pPlayer->SpectatorThink( );
}


////////////////////////////////////////////////////////
// PAS and PVS routines for client messaging
//

/*
================
SetupVisibility

A client can have a separate "view entity" indicating that his/her view should depend on the origin of that
view entity.  If that's the case, then pViewEntity will be non-NULL and will be used.  Otherwise, the current
entity's origin is used.  Either is offset by the view_ofs to get the eye position.

From the eye position, we set up the PAS and PVS to use for filtering network messages to the client.  At this point, we could
 override the actual PAS or PVS values, or use a different origin.

NOTE:  Do not cache the values of pas and pvs, as they depend on reusable memory in the engine, they are only good for this one frame
================
*/
void SetupVisibility( edict_t *pViewEntity, edict_t *pClient, unsigned char **pvs, unsigned char **pas )
{
	Vector org;
	edict_t *pView = pClient;

	// Find the client's PVS
	if ( pViewEntity )
	{
		pView = pViewEntity;
	}

	// SaRcaZm - V3 - Start
	// Tracking Spectators use the visibility of their target
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );
	if ( (pPlayer->pev->iuser2 != 0) && (pPlayer->m_hObserverTarget != NULL) )
	{
		pView = pPlayer->m_hObserverTarget->edict();
	}
	// SaRcaZm - V3 - End

	org = pView->v.origin + pView->v.view_ofs;
	if ( pView->v.flags & FL_DUCKING )
	{
		org = org + ( VEC_HULL_MIN - VEC_DUCK_HULL_MIN );
	}

	*pvs = ENGINE_SET_PVS ( (float *)&org );
	*pas = ENGINE_SET_PAS ( (float *)&org );
}

#include "entity_state.h"

/*
AddToFullPack

Return 1 if the entity state has been filled in for the ent and the entity 
will be propagated to the client, 0 otherwise

state is the server maintained copy of the state info that is transmitted to 
the client a MOD could alter values copied into state to send the "host" a 
different look for a particular entity update, etc. e and ent are the entity 
that is being added to the update, if 1 is returned host is the player's 
edict of the player whom we are sending the update to player is 1 if the 
ent/e is a player and 0 otherwise pSet is either the PAS or PVS that we 
previous set up.  We can use it to ask the engine to filter the entity 
against the PAS or PVS. we could also use the pas/ pvs that we set in 
SetupVisibility, if we wanted to.  Caching the value is valid in that case, 
but still only for the current frame
*/
int AddToFullPack( struct entity_state_s *state, int e, edict_t *ent, edict_t *host, int hostflags, int player, unsigned char *pSet )
{
	int					i;

	// don't send if flagged for NODRAW and it's not the host getting the message
	if ( ( ent->v.effects == EF_NODRAW ) &&
		 ( ent != host ) )
		return 0;

	// Ignore ents without valid / visible models
	if ( !ent->v.modelindex || !STRING( ent->v.model ) )
		return 0;

	// Don't send spectators to other players
	if ( ( ent->v.flags & FL_SPECTATOR ) && ( ent != host ) )
	{
		return 0;
	}

	// Ignore if not the host and not touching a PVS/PAS leaf
	// If pSet is NULL, then the test will always succeed and the entity will be added to the update
	if ( ent != host )
	{
		if ( !ENGINE_CHECK_VISIBILITY( (const struct edict_s *)ent, pSet ) )
		{
			return 0;
		}
	}


	// Don't send entity to local client if the client says it's predicting the entity itself.
	if ( ent->v.flags & FL_SKIPLOCALHOST )
	{
		if ( ( hostflags & 1 ) && ( ent->v.owner == host ) )
			return 0;
	}
	
	if ( host->v.groupinfo )
	{
		UTIL_SetGroupTrace( host->v.groupinfo, GROUP_OP_AND );

		// Should always be set, of course
		if ( ent->v.groupinfo )
		{
			if ( g_groupop == GROUP_OP_AND )
			{
				if ( !(ent->v.groupinfo & host->v.groupinfo ) )
					return 0;
			}
			else if ( g_groupop == GROUP_OP_NAND )
			{
				if ( ent->v.groupinfo & host->v.groupinfo )
					return 0;
			}
		}

		UTIL_UnsetGroupTrace();
	}

	memset( state, 0, sizeof( *state ) );

	// Assign index so we can track this entity from frame to frame and
	//  delta from it.
	state->number	  = e;
	state->entityType = ENTITY_NORMAL;
	
	// Flag custom entities.
	if ( ent->v.flags & FL_CUSTOMENTITY )
	{
		state->entityType = ENTITY_BEAM;
	}

	// 
	// Copy state data
	//

	// Round animtime to nearest millisecond
	state->animtime   = (int)(1000.0 * ent->v.animtime ) / 1000.0;

	memcpy( state->origin, ent->v.origin, 3 * sizeof( float ) );
	memcpy( state->angles, ent->v.angles, 3 * sizeof( float ) );
	memcpy( state->mins, ent->v.mins, 3 * sizeof( float ) );
	memcpy( state->maxs, ent->v.maxs, 3 * sizeof( float ) );

	memcpy( state->startpos, ent->v.startpos, 3 * sizeof( float ) );
	memcpy( state->endpos, ent->v.endpos, 3 * sizeof( float ) );

	state->impacttime = ent->v.impacttime;
	state->starttime = ent->v.starttime;

	state->modelindex = ent->v.modelindex;
		
	state->frame      = ent->v.frame;

	state->skin       = ent->v.skin;
	state->effects    = ent->v.effects;

	// This non-player entity is being moved by the game .dll and not the physics simulation system
	//  make sure that we interpolate it's position on the client if it moves
	if ( !player &&
		 ent->v.animtime &&
		 ent->v.velocity[ 0 ] == 0 && 
		 ent->v.velocity[ 1 ] == 0 && 
		 ent->v.velocity[ 2 ] == 0 )
	{
		state->eflags |= EFLAG_SLERP;
	}

	state->scale	  = ent->v.scale;
	state->solid	  = ent->v.solid;
	state->colormap   = ent->v.colormap;
	state->movetype   = ent->v.movetype;
	state->sequence   = ent->v.sequence;
	state->framerate  = ent->v.framerate;
	state->body       = ent->v.body;

	for (i = 0; i < 4; i++)
	{
		state->controller[i] = ent->v.controller[i];
	}

	for (i = 0; i < 2; i++)
	{
		state->blending[i]   = ent->v.blending[i];
	}

	state->rendermode    = ent->v.rendermode;
	state->renderamt     = ent->v.renderamt; 
	state->renderfx      = ent->v.renderfx;
	state->rendercolor.r = ent->v.rendercolor[0];
	state->rendercolor.g = ent->v.rendercolor[1];
	state->rendercolor.b = ent->v.rendercolor[2];

	// Crem
	state->fuser1 = ent->v.fuser1;

	state->aiment = 0;
	if ( ent->v.aiment )
	{
		state->aiment = ENTINDEX( ent->v.aiment );
	}

	state->owner = 0;
	if ( ent->v.owner )
	{
		int owner = ENTINDEX( ent->v.owner );
		
		// Only care if owned by a player
		if ( owner >= 1 && owner <= gpGlobals->maxClients )
		{
			state->owner = owner;	
		}
	}

	// HACK:  Somewhat...
	// Class is overridden for non-players to signify a breakable glass object ( sort of a class? )
	if ( !player )
	{
		state->playerclass  = ent->v.playerclass;
	}

	// Special stuff for players only
	if ( player )
	{
		memcpy( state->basevelocity, ent->v.basevelocity, 3 * sizeof( float ) );

		state->weaponmodel  = MODEL_INDEX( STRING( ent->v.weaponmodel ) );
		state->gaitsequence = ent->v.gaitsequence;
		state->spectator = ent->v.flags & FL_SPECTATOR;
		state->friction     = ent->v.friction;

		state->gravity      = ent->v.gravity;
//		state->team			= ent->v.team;
//		state->playerclass  = ent->v.playerclass;
		state->usehull      = ( ent->v.flags & FL_DUCKING ) ? 1 : 0;
		state->health		= ent->v.health;
	}

	return 1;
}

// defaults for clientinfo messages
#define	DEFAULT_VIEWHEIGHT	28

/*
===================
CreateBaseline

Creates baselines used for network encoding, especially for player data 
since players are not spawned until connect time.
===================
*/
void CreateBaseline( int player, int eindex, struct entity_state_s *baseline, struct edict_s *entity, int playermodelindex, vec3_t player_mins, vec3_t player_maxs )
{
	baseline->origin		= entity->v.origin;
	baseline->angles		= entity->v.angles;
	baseline->frame			= entity->v.frame;
	baseline->skin			= (short)entity->v.skin;

	// render information
	baseline->rendermode	= (byte)entity->v.rendermode;
	baseline->renderamt		= (byte)entity->v.renderamt;
	baseline->rendercolor.r	= (byte)entity->v.rendercolor[0];
	baseline->rendercolor.g	= (byte)entity->v.rendercolor[1];
	baseline->rendercolor.b	= (byte)entity->v.rendercolor[2];
	baseline->renderfx		= (byte)entity->v.renderfx;

	if ( player )
	{
		baseline->mins			= player_mins;
		baseline->maxs			= player_maxs;

		baseline->colormap		= eindex;
		baseline->modelindex	= playermodelindex;
		baseline->friction		= 1.0;
		baseline->movetype		= MOVETYPE_WALK;

		baseline->scale			= entity->v.scale;
		baseline->solid			= SOLID_SLIDEBOX;
		baseline->framerate		= 1.0;
		baseline->gravity		= 1.0;

	}
	else
	{
		baseline->mins			= entity->v.mins;
		baseline->maxs			= entity->v.maxs;

		baseline->colormap		= 0;
		baseline->modelindex	= entity->v.modelindex;//SV_ModelIndex(pr_strings + entity->v.model);
		baseline->movetype		= entity->v.movetype;

		baseline->scale			= entity->v.scale;
		baseline->solid			= entity->v.solid;
		baseline->framerate		= entity->v.framerate;
		baseline->gravity		= entity->v.gravity;
	}
}

typedef struct
{
	char name[32];
	int	 field;
} entity_field_alias_t;

#define FIELD_ORIGIN0			0
#define FIELD_ORIGIN1			1
#define FIELD_ORIGIN2			2
#define FIELD_ANGLES0			3
#define FIELD_ANGLES1			4
#define FIELD_ANGLES2			5

static entity_field_alias_t entity_field_alias[]=
{
	{ "origin[0]",			0 },
	{ "origin[1]",			0 },
	{ "origin[2]",			0 },
	{ "angles[0]",			0 },
	{ "angles[1]",			0 },
	{ "angles[2]",			0 },
};

void Entity_FieldInit( struct delta_s *pFields )
{
	entity_field_alias[ FIELD_ORIGIN0 ].field		= DELTA_FINDFIELD( pFields, entity_field_alias[ FIELD_ORIGIN0 ].name );
	entity_field_alias[ FIELD_ORIGIN1 ].field		= DELTA_FINDFIELD( pFields, entity_field_alias[ FIELD_ORIGIN1 ].name );
	entity_field_alias[ FIELD_ORIGIN2 ].field		= DELTA_FINDFIELD( pFields, entity_field_alias[ FIELD_ORIGIN2 ].name );
	entity_field_alias[ FIELD_ANGLES0 ].field		= DELTA_FINDFIELD( pFields, entity_field_alias[ FIELD_ANGLES0 ].name );
	entity_field_alias[ FIELD_ANGLES1 ].field		= DELTA_FINDFIELD( pFields, entity_field_alias[ FIELD_ANGLES1 ].name );
	entity_field_alias[ FIELD_ANGLES2 ].field		= DELTA_FINDFIELD( pFields, entity_field_alias[ FIELD_ANGLES2 ].name );
}

/*
==================
Entity_Encode

Callback for sending entity_state_t info over network. 
FIXME:  Move to script
==================
*/
void Entity_Encode( struct delta_s *pFields, const unsigned char *from, const unsigned char *to )
{
	entity_state_t *f, *t;

	int localplayer = 0;
	static int initialized = 0;

	if ( !initialized )
	{
		Entity_FieldInit( pFields );
		initialized = 1;
	}

	f = (entity_state_t *)from;
	t = (entity_state_t *)to;

	// Never send origin to local player, it's sent with more resolution in clientdata_t structure
	localplayer =  ( t->number - 1 ) == ENGINE_CURRENT_PLAYER();
	if ( localplayer )
	{
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN2 ].field );
	}

	if ( ( t->impacttime != 0 ) && ( t->starttime != 0 ) )
	{
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN2 ].field );

		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ANGLES0 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ANGLES1 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ANGLES2 ].field );
	}

	if ( ( t->movetype == MOVETYPE_FOLLOW ) &&
		 ( t->aiment != 0 ) )
	{
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN2 ].field );
	}
	else if ( t->aiment != f->aiment )
	{
		DELTA_SETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_SETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_SETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN2 ].field );
	}
}

static entity_field_alias_t player_field_alias[]=
{
	{ "origin[0]",			0 },
	{ "origin[1]",			0 },
	{ "origin[2]",			0 },
};

void Player_FieldInit( struct delta_s *pFields )
{
	player_field_alias[ FIELD_ORIGIN0 ].field		= DELTA_FINDFIELD( pFields, player_field_alias[ FIELD_ORIGIN0 ].name );
	player_field_alias[ FIELD_ORIGIN1 ].field		= DELTA_FINDFIELD( pFields, player_field_alias[ FIELD_ORIGIN1 ].name );
	player_field_alias[ FIELD_ORIGIN2 ].field		= DELTA_FINDFIELD( pFields, player_field_alias[ FIELD_ORIGIN2 ].name );
}

/*
==================
Player_Encode

Callback for sending entity_state_t for players info over network. 
==================
*/
void Player_Encode( struct delta_s *pFields, const unsigned char *from, const unsigned char *to )
{
	entity_state_t *f, *t;
	int localplayer = 0;
	static int initialized = 0;

	if ( !initialized )
	{
		Player_FieldInit( pFields );
		initialized = 1;
	}

	f = (entity_state_t *)from;
	t = (entity_state_t *)to;

	// Never send origin to local player, it's sent with more resolution in clientdata_t structure
	localplayer =  ( t->number - 1 ) == ENGINE_CURRENT_PLAYER();
	if ( localplayer )
	{
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN2 ].field );
	}

	if ( ( t->movetype == MOVETYPE_FOLLOW ) &&
		 ( t->aiment != 0 ) )
	{
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN2 ].field );
	}
	else if ( t->aiment != f->aiment )
	{
		DELTA_SETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_SETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_SETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN2 ].field );
	}
}

#define CUSTOMFIELD_ORIGIN0			0
#define CUSTOMFIELD_ORIGIN1			1
#define CUSTOMFIELD_ORIGIN2			2
#define CUSTOMFIELD_ANGLES0			3
#define CUSTOMFIELD_ANGLES1			4
#define CUSTOMFIELD_ANGLES2			5
#define CUSTOMFIELD_SKIN			6
#define CUSTOMFIELD_SEQUENCE		7
#define CUSTOMFIELD_ANIMTIME		8

entity_field_alias_t custom_entity_field_alias[]=
{
	{ "origin[0]",			0 },
	{ "origin[1]",			0 },
	{ "origin[2]",			0 },
	{ "angles[0]",			0 },
	{ "angles[1]",			0 },
	{ "angles[2]",			0 },
	{ "skin",				0 },
	{ "sequence",			0 },
	{ "animtime",			0 },
};

void Custom_Entity_FieldInit( struct delta_s *pFields )
{
	custom_entity_field_alias[ CUSTOMFIELD_ORIGIN0 ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ORIGIN0 ].name );
	custom_entity_field_alias[ CUSTOMFIELD_ORIGIN1 ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ORIGIN1 ].name );
	custom_entity_field_alias[ CUSTOMFIELD_ORIGIN2 ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ORIGIN2 ].name );
	custom_entity_field_alias[ CUSTOMFIELD_ANGLES0 ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANGLES0 ].name );
	custom_entity_field_alias[ CUSTOMFIELD_ANGLES1 ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANGLES1 ].name );
	custom_entity_field_alias[ CUSTOMFIELD_ANGLES2 ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANGLES2 ].name );
	custom_entity_field_alias[ CUSTOMFIELD_SKIN ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_SKIN ].name );
	custom_entity_field_alias[ CUSTOMFIELD_SEQUENCE ].field= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_SEQUENCE ].name );
	custom_entity_field_alias[ CUSTOMFIELD_ANIMTIME ].field= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANIMTIME ].name );
}

/*
==================
Custom_Encode

Callback for sending entity_state_t info ( for custom entities ) over network. 
FIXME:  Move to script
==================
*/
void Custom_Encode( struct delta_s *pFields, const unsigned char *from, const unsigned char *to )
{
	entity_state_t *f, *t;
	int beamType;
	static int initialized = 0;

	if ( !initialized )
	{
		Custom_Entity_FieldInit( pFields );
		initialized = 1;
	}

	f = (entity_state_t *)from;
	t = (entity_state_t *)to;

	beamType = t->rendermode & 0x0f;
		
	if ( beamType != BEAM_POINTS && beamType != BEAM_ENTPOINT )
	{
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ORIGIN0 ].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ORIGIN1 ].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ORIGIN2 ].field );
	}

	if ( beamType != BEAM_POINTS )
	{
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANGLES0 ].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANGLES1 ].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANGLES2 ].field );
	}

	if ( beamType != BEAM_ENTS && beamType != BEAM_ENTPOINT )
	{
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_SKIN ].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_SEQUENCE ].field );
	}

	// animtime is compared by rounding first
	// see if we really shouldn't actually send it
	if ( (int)f->animtime == (int)t->animtime )
	{
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANIMTIME ].field );
	}
}

/*
=================
RegisterEncoders

Allows game .dll to override network encoding of certain types of entities and tweak values, etc.
=================
*/
void RegisterEncoders( void )
{
	DELTA_ADDENCODER( "Entity_Encode", Entity_Encode );
	DELTA_ADDENCODER( "Custom_Encode", Custom_Encode );
	DELTA_ADDENCODER( "Player_Encode", Player_Encode );
}

int GetWeaponData( struct edict_s *player, struct weapon_data_s *info )
{
#if defined( CLIENT_WEAPONS )
	int i;
	weapon_data_t *item;
	entvars_t *pev = &player->v;
	CBasePlayer *pl = ( CBasePlayer *) CBasePlayer::Instance( pev );
	CBasePlayerWeapon *gun;
	
	ItemInfo II;

	memset( info, 0, 32 * sizeof( weapon_data_t ) );

	if ( !pl )
		return 1;

	// go through all of the weapons and make a list of the ones to pack
	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		if ( pl->m_rgpPlayerItems[ i ] )
		{
			// there's a weapon here. Should I pack it?
			CBasePlayerItem *pPlayerItem = pl->m_rgpPlayerItems[ i ];

			while ( pPlayerItem )
			{
				gun = (CBasePlayerWeapon *)pPlayerItem->GetWeaponPtr();
				if ( gun && gun->UseDecrement() )
				{
					// Get The ID.
					memset( &II, 0, sizeof( II ) );
					gun->GetItemInfo( &II );

					if ( II.iId >= 0 && II.iId < 32 )
					{
						item = &info[ II.iId ];
						
						item->m_iId						= II.iId;
						item->m_iClip					= gun->m_iClip;

						item->m_flTimeWeaponIdle		= max( gun->m_flTimeWeaponIdle, -0.001 );
						item->m_flNextPrimaryAttack		= max( gun->m_flNextPrimaryAttack, -0.001 );
						item->m_flNextSecondaryAttack	= max( gun->m_flNextSecondaryAttack, -0.001 );
						item->m_fInReload				= gun->m_fInReload;
					}
				}
				pPlayerItem = pPlayerItem->m_pNext;
			}
		}
	}
#else
	memset( info, 0, 32 * sizeof( weapon_data_t ) );
#endif
	return 1;
}

/*
=================
UpdateClientData

Data sent to current client only
engine sets cd to 0 before calling.
=================
*/
void UpdateClientData ( const struct edict_s *ent, int sendweapons, struct clientdata_s *cd )
{
	cd->flags			= ent->v.flags;
	cd->health			= ent->v.health;

	cd->viewmodel		= MODEL_INDEX( STRING( ent->v.viewmodel ) );
	cd->waterlevel		= ent->v.waterlevel;
	cd->watertype		= ent->v.watertype;
	cd->weapons			= ent->v.weapons;

	// Vectors
	cd->origin			= ent->v.origin;
	cd->velocity		= ent->v.velocity;
	cd->view_ofs		= ent->v.view_ofs;
	cd->punchangle		= ent->v.punchangle;

	cd->bInDuck			= ent->v.bInDuck;
	cd->flTimeStepSound = ent->v.flTimeStepSound;
	cd->flDuckTime		= ent->v.flDuckTime;
	cd->flSwimTime		= ent->v.flSwimTime;
	cd->waterjumptime	= ent->v.teleport_time;

	strcpy( cd->physinfo, ENGINE_GETPHYSINFO( ent ) );

	cd->maxspeed		= ent->v.maxspeed;
	cd->fov				= ent->v.fov;
	cd->weaponanim		= ent->v.weaponanim;

	cd->pushmsec		= ent->v.pushmsec;

	// Crem
	cd->fuser1			= ent->v.fuser1;

	// SaRcaZm - V3 - Start
	// Spectator
	cd->iuser1			= ent->v.iuser1;
	cd->iuser2			= ent->v.iuser2;
	// SaRcaZm - V3 - End

	// SaRcaZm - V4 - Start
	cd->vuser4			= ent->v.vuser4;
	// SaRcaZm - V4 - End

#if defined( CLIENT_WEAPONS )
	if ( sendweapons )
	{
		entvars_t *pev = (entvars_t *)&ent->v;
		CBasePlayer *pl = ( CBasePlayer *) CBasePlayer::Instance( pev );

		if ( pl )
		{
			cd->m_flNextAttack	= pl->m_flNextAttack;

			if ( pl->m_pActiveItem )
			{
				CBasePlayerWeapon *gun;
				gun = (CBasePlayerWeapon *)pl->m_pActiveItem->GetWeaponPtr();
				if ( gun && gun->UseDecrement() )
				{
					ItemInfo II;
					memset( &II, 0, sizeof( II ) );
					gun->GetItemInfo( &II );

					cd->m_iId = II.iId;
				}
			}
		}
	}
#endif
}

/*
=================
CmdStart

We're about to run this usercmd for the specified player.  We can set up groupinfo and masking here, etc.
This is the time to examine the usercmd for anything extra.  This call happens even if think does not.
=================
*/
void CmdStart( const edict_t *player, const struct usercmd_s *cmd, unsigned int random_seed )
{
	entvars_t *pev = (entvars_t *)&player->v;
	CBasePlayer *pl = ( CBasePlayer *) CBasePlayer::Instance( pev );

	if( !pl )
		return;

	if ( pl->pev->groupinfo != 0 )
	{
		UTIL_SetGroupTrace( pl->pev->groupinfo, GROUP_OP_AND );
	}

	pl->random_seed = random_seed;
}

/*
=================
CmdEnd

Each cmdstart is exactly matched with a cmd end, clean up any group trace flags, etc. here
=================
*/
void CmdEnd ( const edict_t *player )
{
	entvars_t *pev = (entvars_t *)&player->v;
	CBasePlayer *pl = ( CBasePlayer *) CBasePlayer::Instance( pev );

	if( !pl )
		return;
	if ( pl->pev->groupinfo != 0 )
	{
		UTIL_UnsetGroupTrace();
	}
}

/*
================================
ConnectionlessPacket

 Return 1 if the packet is valid.  Set response_buffer_size if you want to send a response packet.  Incoming, it holds the max
  size of the response_buffer, so you must zero it out if you choose not to respond.
================================
*/
int	ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size )
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

/*
================================
GetHullBounds

  Engine calls this to enumerate player collision hulls, for prediction.  Return 0 if the hullnumber doesn't exist.
================================
*/
int GetHullBounds( int hullnumber, float *mins, float *maxs )
{
	int iret = 0;

	switch ( hullnumber )
	{
	case 0:				// Normal player
//		mins = VEC_HULL_MIN;
//		maxs = VEC_HULL_MAX;
		mins = Vector( 0, 0, 0 );
		maxs = Vector( 0, 0, 0 );
		iret = 1;
		break;
	case 1:				// Crouched player
//		mins = VEC_DUCK_HULL_MIN;
//		maxs = VEC_DUCK_HULL_MAX;
		mins = Vector( 0, 0, 0 );
		maxs = Vector( 0, 0, 0 );
		iret = 1;
		break;
	case 2:				// Point based hull
		mins = Vector( 0, 0, 0 );
		maxs = Vector( 0, 0, 0 );
		iret = 1;
		break;
	}

	return iret;
}

/*
================================
CreateInstancedBaselines

Create pseudo-baselines for items that aren't placed in the map at spawn time, but which are likely
to be created during play ( e.g., grenades, ammo packs, projectiles, corpses, etc. )
================================
*/
void CreateInstancedBaselines ( void )
{
	int iret = 0;
	entity_state_t state;

	memset( &state, 0, sizeof( state ) );

	// Create any additional baselines here for things like grendates, etc.
	// iret = ENGINE_INSTANCE_BASELINE( pc->pev->classname, &state );

	// Destroy objects.
	//UTIL_Remove( pc );
}

/*
================================
InconsistentFile

One of the ENGINE_FORCE_UNMODIFIED files failed the consistency check for the specified player
 Return 0 to allow the client to continue, 1 to force immediate disconnection ( with an optional disconnect message of up to 256 characters )
================================
*/
int	InconsistentFile( const edict_t *player, const char *filename, char *disconnect_message )
{
	// Server doesn't care?
	if ( CVAR_GET_FLOAT( "mp_consistency" ) != 1 )
		return 0;

	// Default behavior is to kick the player
	sprintf( disconnect_message, "Server is enforcing file consistency for %s\n", filename );

	// Kick now with specified disconnect message.
	return 1;
}

/*
================================
AllowLagCompensation

 The game .dll should return 1 if lag compensation should be allowed 
 ( could also just set  the sv_unlag cvar. Most games right now should return 0, 
 until client-side weapon prediction code is written and tested for them 
 ( note you can predict weapons, but not do lag compensation, too,  if you want.
================================
*/
int AllowLagCompensation( void )
{
	return 0;
}

