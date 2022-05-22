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
// teamplay_gamerules.cpp
//
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
 
#include	"skill.h"
#include	"game.h"
#include	"items.h"
#include	"voice_gamemgr.h"

// FragMented!
#include	"../common/rally_zlib.h"
#include "rally_rounds.h"

extern DLL_GLOBAL CGameRules	*g_pGameRules;
extern DLL_GLOBAL BOOL	g_fGameOver;
extern int gmsgDeathMsg;	// client dll messages
extern int gmsgScoreInfo;
extern int gmsgMOTD;
extern int gmsgServerName;

// SaRcaZm - V3 - Start
extern int gmsgSpectator;
// SaRcaZm - V3 - End

// SaRcaZm - V6 - Start
extern int gmsgTextFiles;
// SaRcaZm - V6 - End

extern int g_teamplay;

#define ITEM_RESPAWN_TIME	30
#define WEAPON_RESPAWN_TIME	20
#define AMMO_RESPAWN_TIME	20

float g_flIntermissionStartTime = 0;

CVoiceGameMgr	g_VoiceGameMgr;

class CMultiplayGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool		CanPlayerHearPlayer(CBasePlayer *pListener, CBasePlayer *pTalker)
	{
/*		if ( g_teamplay )
		{
			if ( g_pGameRules->PlayerRelationship( pListener, pTalker ) != GR_TEAMMATE )
			{
				return false;
			}
		}
*/
		return true;
	}
};
static CMultiplayGameMgrHelper g_GameMgrHelper;

// SaRcaZm - V2 - Start
void UpdateRaceInfo (void);
// SaRcaZm - V2 - End

//*********************************************************
// Rules for the half-life multiplayer game.
//*********************************************************

CHalfLifeMultiplay :: CHalfLifeMultiplay()
{
	g_VoiceGameMgr.Init(&g_GameMgrHelper, gpGlobals->maxClients);

	RefreshSkillData();
	m_flIntermissionEndTime = 0;
	g_flIntermissionStartTime = 0;
	
	// 11/8/98
	// Modified by YWB:  Server .cfg file is now a cvar, so that 
	//  server ops can run multiple game servers, with different server .cfg files,
	//  from a single installed directory.
	// Mapcyclefile is already a cvar.

	// 3/31/99
	// Added lservercfg file cvar, since listen and dedicated servers should not
	// share a single config file. (sjb)
	if ( IS_DEDICATED_SERVER() )
	{
		// dedicated server
		char *servercfgfile = (char *)CVAR_GET_STRING( "servercfgfile" );

		if ( servercfgfile && servercfgfile[0] )
		{
			char szCommand[256];
			
			ALERT( at_console, "Executing dedicated server config file\n" );
			sprintf( szCommand, "exec %s\n", servercfgfile );
			SERVER_COMMAND( szCommand );
		}
	}
	else
	{
		// listen server
		char *lservercfgfile = (char *)CVAR_GET_STRING( "lservercfgfile" );

		if ( lservercfgfile && lservercfgfile[0] )
		{
			char szCommand[256];
			
			ALERT( at_console, "Executing listen server config file\n" );
			sprintf( szCommand, "exec %s\n", lservercfgfile );
			SERVER_COMMAND( szCommand );
		}
	}

	// FragMented
	if(!ZLibFuncs.Init())
		UTIL_ClientPrintAll (HUD_PRINTNOTIFY, "Couldn't init ZLIB\n");

}

BOOL CHalfLifeMultiplay::ClientCommand( CBasePlayer *pPlayer, const char *pcmd )
{
	if(g_VoiceGameMgr.ClientCommand(pPlayer, pcmd))
		return TRUE;

	return CGameRules::ClientCommand(pPlayer, pcmd);
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::RefreshSkillData( void )
{
// load all default values
	CGameRules::RefreshSkillData();

// override some values for multiplay.

	// suitcharger
	gSkillData.suitchargerCapacity = 30;

	// Crowbar whack
	gSkillData.plrDmgCrowbar = 25;

	// Glock Round
	gSkillData.plrDmg9MM = 12;

	// 357 Round
	gSkillData.plrDmg357 = 40;

	// MP5 Round
	gSkillData.plrDmgMP5 = 12;

	// M203 grenade
	gSkillData.plrDmgM203Grenade = 100;

	// Shotgun buckshot
	gSkillData.plrDmgBuckshot = 20;// fewer pellets in deathmatch

	// Crossbow
	gSkillData.plrDmgCrossbowClient = 20;

	// RPG
	gSkillData.plrDmgRPG = 120;

	// Egon
	gSkillData.plrDmgEgonWide = 20;
	gSkillData.plrDmgEgonNarrow = 10;

	// Hand Grendade
	gSkillData.plrDmgHandGrenade = 100;

	// Satchel Charge
	gSkillData.plrDmgSatchel = 120;

	// Tripmine
	gSkillData.plrDmgTripmine = 150;

	// hornet
	gSkillData.plrDmgHornet = 10;
}

// longest the intermission can last, in seconds
#define MAX_INTERMISSION_TIME		120

extern cvar_t timeleft, fragsleft;

extern cvar_t mp_chattime;

//=========================================================
//=========================================================
void CHalfLifeMultiplay :: Think ( void )
{
	g_VoiceGameMgr.Update(gpGlobals->frametime);

	///// Check game rules /////
	static int last_frags;
	static int last_time;

	int frags_remaining = 0;
	int time_remaining = 0;

	if ( g_fGameOver )   // someone else quit the game already
	{
		// bounds check
		int time = (int)CVAR_GET_FLOAT( "mp_chattime" );
		if ( time < 10 )
			CVAR_SET_STRING( "mp_chattime", "10" );
		else if ( time > MAX_INTERMISSION_TIME )
			CVAR_SET_STRING( "mp_chattime", UTIL_dtos1( MAX_INTERMISSION_TIME ) );

		m_flIntermissionEndTime = g_flIntermissionStartTime + mp_chattime.value;

		// check to see if we should change levels now
		if ( m_flIntermissionEndTime < gpGlobals->time )
		{
			if ( m_iEndIntermissionButtonHit  // check that someone has pressed a key, or the max intermission time is over
				|| ( ( g_flIntermissionStartTime + MAX_INTERMISSION_TIME ) < gpGlobals->time) ) 
				ChangeLevel(); // intermission is over
		}

		return;
	}

	float flTimeLimit = timelimit.value * 60; //9010; // FragMented!
	float flFragLimit = fraglimit.value;

	time_remaining = (int)(flTimeLimit ? ( flTimeLimit - gpGlobals->time ) : 0);
	
	if ( flTimeLimit != 0 && gpGlobals->time >= flTimeLimit &&
		((g_pGameRules->m_RoundState == ROUND_NOTSTARTED) || (g_pGameRules->m_RoundState == ROUND_COUNTDOWN)))
	{
		GoToIntermission();
		return;
	}

	if ( flFragLimit )
	{
		int bestfrags = 9999;
		int remain;

		// check if any player is over the frag limit
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

			if ( pPlayer && pPlayer->pev->frags >= flFragLimit &&
				((g_pGameRules->m_RoundState == ROUND_NOTSTARTED) || (g_pGameRules->m_RoundState == ROUND_COUNTDOWN)))
			{
				GoToIntermission();
				return;
			}


			if ( pPlayer )
			{
				remain = flFragLimit - pPlayer->pev->frags;
				if ( remain < bestfrags )
				{
					bestfrags = remain;
				}
			}

		}
		frags_remaining = bestfrags;
	}

	// Updates when frags change
	if ( frags_remaining != last_frags )
	{
		g_engfuncs.pfnCvar_DirectSet( &fragsleft, UTIL_VarArgs( "%i", frags_remaining ) );
	}

	// Updates once per second
	if ( timeleft.value != last_time )
	{
		g_engfuncs.pfnCvar_DirectSet( &timeleft, UTIL_VarArgs( "%i", time_remaining ) );
	}

	last_frags = frags_remaining;
	last_time  = time_remaining;
}


//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::IsMultiplayer( void )
{
	return TRUE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::IsDeathmatch( void )
{
	return TRUE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::IsCoOp( void )
{
	return gpGlobals->coop;
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	if ( !pWeapon->CanDeploy() )
	{
		// that weapon can't deploy anyway.
		return FALSE;
	}

	if ( !pPlayer->m_pActiveItem )
	{
		// player doesn't have an active item!
		return TRUE;
	}

	if ( !pPlayer->m_pActiveItem->CanHolster() )
	{
		// can't put away the active item.
		return FALSE;
	}

	if ( pWeapon->iWeight() > pPlayer->m_pActiveItem->iWeight() )
	{
		return TRUE;
	}

	return FALSE;
}

BOOL CHalfLifeMultiplay :: GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon )
{

	CBasePlayerItem *pCheck;
	CBasePlayerItem *pBest;// this will be used in the event that we don't find a weapon in the same category.
	int iBestWeight;
	int i;

	iBestWeight = -1;// no weapon lower than -1 can be autoswitched to
	pBest = NULL;

	if ( !pCurrentWeapon->CanHolster() )
	{
		// can't put this gun away right now, so can't switch.
		return FALSE;
	}

	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		pCheck = pPlayer->m_rgpPlayerItems[ i ];

		while ( pCheck )
		{
			if ( pCheck->iWeight() > -1 && pCheck->iWeight() == pCurrentWeapon->iWeight() && pCheck != pCurrentWeapon )
			{
				// this weapon is from the same category. 
				if ( pCheck->CanDeploy() )
				{
					if ( pPlayer->SwitchWeapon( pCheck ) )
					{
						return TRUE;
					}
				}
			}
			else if ( pCheck->iWeight() > iBestWeight && pCheck != pCurrentWeapon )// don't reselect the weapon we're trying to get rid of
			{
				//ALERT ( at_console, "Considering %s\n", STRING( pCheck->pev->classname ) );
				// we keep updating the 'best' weapon just in case we can't find a weapon of the same weight
				// that the player was using. This will end up leaving the player with his heaviest-weighted 
				// weapon. 
				if ( pCheck->CanDeploy() )
				{
					// if this weapon is useable, flag it as the best
					iBestWeight = pCheck->iWeight();
					pBest = pCheck;
				}
			}

			pCheck = pCheck->m_pNext;
		}
	}

	// if we make it here, we've checked all the weapons and found no useable 
	// weapon in the same catagory as the current weapon. 
	
	// if pBest is null, we didn't find ANYTHING. Shouldn't be possible- should always 
	// at least get the crowbar, but ya never know.
	if ( !pBest )
	{
		return FALSE;
	}

	pPlayer->SwitchWeapon( pBest );

	return TRUE;
}

//=========================================================
//=========================================================

extern cvar_t joinmidrace;

BOOL CHalfLifeMultiplay :: ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ] )
{
	// SaRcaZm - V7 - Start
	// If there is only one player in the race currently, restart it
	int numplayers = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *plr = UTIL_PlayerByIndex( i );

		if ( plr )
		{
			numplayers++;
		}
	}

	// Restart the current race if one is going
	if ((numplayers == 2) && (g_pGameRules->m_RoundState == 1) && !joinmidrace.value)
		g_pGameRules->RoundEnd (TIME_UP, "");
	// SaRcaZm - V7 - End

	g_VoiceGameMgr.ClientConnected(pEntity);

	// SaRcaZm - V3 - Start
	// Set the timer on their hud
	if ((m_RoundState == ROUND_STARTED) && pEntity)
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pEntity );

		if (pPlayer)
		{
			// SaRcaZm - V7 - Start
			// Reset their position so it doesn't screw with the spawning
			pPlayer->pev->iuser4 = 0;
			// SaRcaZm - V7 - End

			MESSAGE_BEGIN( MSG_ONE, gmsgSetTimer, NULL, pPlayer->pev );
				// SaRcaZm - V6 - Start
				//WRITE_COORD (gpGlobals->time - pPlayer->pev->fuser4);
				WRITE_COORD (gpGlobals->time - fGlobalRaceStartTime);
				// SaRcaZm - V6 - End
				// SaRcaZm - V5 - Start
				//WRITE_SHORT (1);
				// SaRcaZm - V5 - End
			MESSAGE_END();
		}
	}
	// SaRcaZm - V3 - End

	// SaRcaZm - V7 - Start
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(pEntity);
	if (pPlayer)
		memset (pPlayer->m_szModel, 0, 40);
	// SaRcaZm - V7 - End

	return TRUE;
}

extern int gmsgSayText;
extern int gmsgGameMode;

void CHalfLifeMultiplay :: UpdateGameMode( CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgGameMode, NULL, pPlayer->edict() );
		WRITE_BYTE( 0 );  // game mode none
	MESSAGE_END();
}

void CHalfLifeMultiplay :: InitHUD( CBasePlayer *pl )
{
	// notify other clients of player joining the game
	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( "%s has joined the game\n", 
		( pl->pev->netname && STRING(pl->pev->netname)[0] != 0 ) ? STRING(pl->pev->netname) : "unconnected" ) );

	// team match?
	if ( g_teamplay )
	{
		UTIL_LogPrintf( "\"%s<%i><%u><%s>\" entered the game\n",  
			STRING( pl->pev->netname ), 
			GETPLAYERUSERID( pl->edict() ),
			GETPLAYERWONID( pl->edict() ),
			g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pl->edict() ), "model" ) );
	}
	else
	{
		UTIL_LogPrintf( "\"%s<%i><%u><%i>\" entered the game\n",  
			STRING( pl->pev->netname ), 
			GETPLAYERUSERID( pl->edict() ),
			GETPLAYERWONID( pl->edict() ),
			GETPLAYERUSERID( pl->edict() ) );
	}

	UpdateGameMode( pl );

	// sending just one score makes the hud scoreboard active;  otherwise
	// it is just disabled for single play
	// SaRcaZm - V8 - Start
	MESSAGE_BEGIN( MSG_ONE, gmsgScoreInfo, NULL, pl->edict() );
		WRITE_BYTE( ENTINDEX(pl->edict()) );
		WRITE_SHORT( 0 );
//		WRITE_SHORT( 0 );
//		WRITE_SHORT( 0 );
//		WRITE_SHORT( 0 );
		WRITE_BYTE( 0 );
	MESSAGE_END();
	// SaRcaZm - V8 - End

	// SaRcaZm - V4 - Start
	// Make sure they don't spawn till they pick a car
	pl->pev->fuser1 = 0.0f;
	// SaRcaZm - V4 - End

	// SaRcaZm - V6 - Start
	// Give them some money to play with at the beginning of the level
	pl->ResetMoney ();
	pl->AddMoney (600);
	pl->UpdateMoney ();

	// Now gets zipped up and sent with the rest of the text files
	//SendMOTDToClient( pl->edict() ); // Overlay MOTD FragMented!

	int aLength, bLength, cLength, dLength, length;
	char *aFileList = (char*)LOAD_FILE_FOR_ME( (char *)CVAR_GET_STRING( "motdfile" ), &aLength );
	char *bFileList = (char*)LOAD_FILE_FOR_ME( "anim.txt",        &bLength );
	char *cFileList = (char*)LOAD_FILE_FOR_ME( hlConst.szCarInfo, &cLength );
	char *dFileList = (char*)LOAD_FILE_FOR_ME( "customise.txt",   &dLength );
	length = aLength + bLength + cLength + dLength + 4;	// Three '\0' to seperate the files
	char *tmpfile = (char *)malloc (length);

	int pos = aLength;
	memcpy (tmpfile, aFileList, aLength);
	tmpfile[pos] = '\0';	pos += 1;
	memcpy (tmpfile + pos, bFileList, bLength);	pos += bLength;
	tmpfile[pos] = '\0';	pos += 1;
	memcpy (tmpfile + pos, cFileList, cLength);	pos += cLength;
	tmpfile[pos] = '\0';	pos += 1;
	memcpy (tmpfile + pos, dFileList, dLength);	pos += dLength;
	tmpfile[pos] = '\0';
	SendTextFileToClient (pl->edict(), tmpfile, length, gmsgTextFiles);

	FREE_FILE( aFileList );
	FREE_FILE( bFileList );
	FREE_FILE( cFileList );
	FREE_FILE( dFileList );
	free (tmpfile);
	// SaRcaZm - V6 - End

	// SaRcaZm - V5 - Start
	// You have no idea how long this took me to track down Frag
	//pl->ShowVGUIMenu(2); // Send Team Menu FragMented!
	// SaRcaZm - V5 - End

	// loop through all active players and send their score info to the new client
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		// FIXME:  Probably don't need to cast this just to read m_iDeaths
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );

		if ( plr )
		{
			// SaRcaZm - V8 - Start
			MESSAGE_BEGIN( MSG_ONE, gmsgScoreInfo, NULL, pl->edict() );
				WRITE_BYTE( i );	// client number
				WRITE_SHORT( plr->pev->frags );
				//WRITE_SHORT( plr->m_iDeaths );
				//WRITE_SHORT( 0 );
				//WRITE_SHORT( GetTeamIndex( plr->m_szTeamName ) + 1 );
				WRITE_BYTE( GetTeamIndex( plr->m_szTeamName ) + 1 );
			MESSAGE_END();
			// SaRcaZm - V8 - End

			// SaRcaZm - V3 - Start
			// Send their spectator state
			MESSAGE_BEGIN( MSG_ONE, gmsgSpectator, NULL, pl->edict() );  
				WRITE_BYTE( i );
				WRITE_BYTE( (plr->pev->iuser1 != 0) );
			MESSAGE_END();
			// SaRcaZm - V3 - End
		}
	}

	if ( g_fGameOver )
	{
		MESSAGE_BEGIN( MSG_ONE, SVC_INTERMISSION, NULL, pl->edict() );
		MESSAGE_END();
	}

	// SaRcaZm - V2 - Start
	// Let everyone know there is a new player on the server
	//UpdateRaceInfo ();
	// SaRcaZm - V2 - End
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay :: ClientDisconnected( edict_t *pClient )
{
	if ( pClient )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );

		if ( pPlayer )
		{
			FireTargets( "game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0 );

			// team match?
			if ( g_teamplay )
			{
				UTIL_LogPrintf( "\"%s<%i><%u><%s>\" disconnected\n",  
					STRING( pPlayer->pev->netname ), 
					GETPLAYERUSERID( pPlayer->edict() ),
					GETPLAYERWONID( pPlayer->edict() ),
					g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model" ) );
			}
			else
			{
				UTIL_LogPrintf( "\"%s<%i><%u><%i>\" disconnected\n",  
					STRING( pPlayer->pev->netname ), 
					GETPLAYERUSERID( pPlayer->edict() ),
					GETPLAYERWONID( pPlayer->edict() ),
					GETPLAYERUSERID( pPlayer->edict() ) );
			}

			pPlayer->pev->deadflag = DEAD_DEAD;
			pPlayer->pev->health = 0;


			if(pPlayer->HasNamedPlayerItem("weapon_rallyhp"))
			{
				CBasePlayer *hpPlayer = NULL;
				bool notGiven = true;

				int i = 0; // Stop infinite loop
				while(notGiven && i < 1024 && hpPlayer != pPlayer)
				{
					hpPlayer = (CBasePlayer *)UTIL_PlayerByIndex(RANDOM_LONG(1, 32));

					if(hpPlayer->pev && hpPlayer->IsAlive())
					{
						hpPlayer->GiveNamedItem("weapon_rallyhp");
						notGiven = false;

						char sz[256];
						sprintf(sz, "Bomb teleport! Now on %s [%s]!\n", STRING(hpPlayer->pev->netname), hpPlayer->m_szTeamName);
						UTIL_SayTextAll(sz, hpPlayer);
					}
					i++;
				}
			}

			pPlayer->RemoveAllItems( TRUE );// destroy all of the players weapons and items

			// SaRcaZm - V3 - Start
			// Tell all clients this player isn't a spectator anymore
			MESSAGE_BEGIN( MSG_ALL, gmsgSpectator );  
				WRITE_BYTE( ENTINDEX(pClient) );
				WRITE_BYTE( 0 );
			MESSAGE_END();

			CBasePlayer *client = NULL;
			while ( ((client = (CBasePlayer*)UTIL_FindEntityByClassname( client, "player" )) != NULL) && (!FNullEnt(client->edict())) ) 
			{
				if ( !client->pev )
					continue;
				if ( client == pPlayer )
					continue;

				// If a spectator was chasing this player, move him/her onto the next player
				if ( client->m_hObserverTarget == pPlayer )
				{
					int iMode = client->pev->iuser1;
					client->pev->iuser1 = 0;
					client->m_hObserverTarget = NULL;
					client->Observer_SetMode( iMode );
				}
			}
			// SaRcaZm - V3 - End
		}
	}

	// SaRcaZm - V3 - Start
	// If there are no players left, end the round
	int bPlayers = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ ) 
	{ 
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i ); 
		if (plr)
		{
			bPlayers++;
		}
	}
	// Check to see if we are the only one left on the server
	if (bPlayers == 1)
		RoundEnd (0, NULL);
	// SaRcaZm - V3 - End
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay :: FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	int iFallDamage = (int)falldamage.value;

	switch ( iFallDamage )
	{
	case 1://progressive
		pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
		return pPlayer->m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
		break;
	default:
	case 0:// fixed
		return 10;
		break;
	}
} 

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker )
{
	return TRUE;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay :: PlayerThink( CBasePlayer *pPlayer )
{
	if ( g_fGameOver )
	{
		// check for button presses
//		if ( pPlayer->m_afButtonPressed & ( IN_DUCK | IN_ATTACK | IN_ATTACK2 | IN_USE | IN_JUMP ) )
			m_iEndIntermissionButtonHit = TRUE;

		// clear attack/use commands from player
		pPlayer->m_afButtonPressed = 0;
		pPlayer->pev->button = 0;
		pPlayer->m_afButtonReleased = 0;
	}
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay :: PlayerSpawn( CBasePlayer *pPlayer )
{
	BOOL		addDefault;
	CBaseEntity	*pWeaponEntity = NULL;

	pPlayer->pev->weapons |= (1<<WEAPON_SUIT);
	
	addDefault = TRUE;

	while ( pWeaponEntity = UTIL_FindEntityByClassname( pWeaponEntity, "game_player_equip" ))
	{
		pWeaponEntity->Touch( pPlayer );
		addDefault = FALSE;
	}

	// SaRcaZm - V8 - Start
	pPlayer->fABSpectateTime = -1;
	// SaRcaZm - V8 - End

/*
FragMented
	if ( addDefault )
	{
		pPlayer->GiveNamedItem( "weapon_crowbar" );
		pPlayer->GiveNamedItem( "weapon_9mmhandgun" );
		pPlayer->GiveAmmo( 68, "9mm", _9MM_MAX_CARRY );// 4 full reloads
	}
*/
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay :: FPlayerCanRespawn( CBasePlayer *pPlayer )
{
	return TRUE;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay :: FlPlayerSpawnTime( CBasePlayer *pPlayer )
{
	return gpGlobals->time;//now!
}

BOOL CHalfLifeMultiplay :: AllowAutoTargetCrosshair( void )
{
	return 1;
}

//=========================================================
// IPointsForKill - how many points awarded to anyone
// that kills this player?
//=========================================================
int CHalfLifeMultiplay :: IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	return 1;
}


//=========================================================
// PlayerKilled - someone/something killed this player
//=========================================================
void CHalfLifeMultiplay :: PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	DeathNotice( pVictim, pKiller, pInflictor );

	pVictim->m_iDeaths += 1;


	FireTargets( "game_playerdie", pVictim, pVictim, USE_TOGGLE, 0 );
	CBasePlayer *peKiller = NULL;
	CBaseEntity *ktmp = CBaseEntity::Instance( pKiller );
	if ( ktmp && (ktmp->Classify() == CLASS_PLAYER) )
		peKiller = (CBasePlayer*)ktmp;

	if ( pVictim->pev == pKiller )  
	{  // killed self
		pKiller->frags -= 1;
	}
	else if ( ktmp && ktmp->IsPlayer() )
	{
		// if a player dies in a deathmatch game and the killer is a client, award the killer some points
		pKiller->frags += IPointsForKill( peKiller, pVictim );
		
		FireTargets( "game_playerkill", ktmp, ktmp, USE_TOGGLE, 0 );
	}
	else
	{  // killed by the world
		pKiller->frags -= 1;
	}

	// update the scores
	// killed scores
	// SaRcaZm - V8 - Start
	/*MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( ENTINDEX(pVictim->edict()) );
		WRITE_SHORT( pVictim->pev->frags );
		WRITE_SHORT( pVictim->m_iDeaths );
		WRITE_SHORT( 0 );
		WRITE_SHORT( GetTeamIndex( pVictim->m_szTeamName ) + 1 );
	MESSAGE_END();*/
	pVictim->AddPoints (0, true);
	// SaRcaZm - V8 - End

	// killers score, if it's a player
	CBaseEntity *ep = CBaseEntity::Instance( pKiller );
	if ( ep && ep->Classify() == CLASS_PLAYER )
	{
		CBasePlayer *PK = (CBasePlayer*)ep;

		// SaRcaZm - V8 - Start
		/*MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
			WRITE_BYTE( ENTINDEX(PK->edict()) );
			WRITE_SHORT( PK->pev->frags );
			WRITE_SHORT( PK->m_iDeaths );
			WRITE_SHORT( 0 );
			WRITE_SHORT( GetTeamIndex( PK->m_szTeamName) + 1 );
		MESSAGE_END();*/
		PK->AddPoints (0, true);
		// SaRcaZm - V8 - End

		// let the killer paint another decal as soon as he'd like.
		PK->m_flNextDecalTime = gpGlobals->time;
	}

}

//=========================================================
// Deathnotice. 
//=========================================================
void CHalfLifeMultiplay::DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pevInflictor )
{
	// Work out what killed the player, and send a message to all clients about it
	CBaseEntity *Killer = CBaseEntity::Instance( pKiller );

	const char *killer_weapon_name = "world";		// by default, the player is killed by the world
	int killer_index = 0;
	
	// Hack to fix name change
	char *tau = "tau_cannon";
	char *gluon = "gluon gun";

	if ( pKiller->flags & FL_CLIENT )
	{
		killer_index = ENTINDEX(ENT(pKiller));
		
		if ( pevInflictor )
		{
			if ( pevInflictor == pKiller )
			{
				// If the inflictor is the killer,  then it must be their current weapon doing the damage
				CBasePlayer *pPlayer = (CBasePlayer*)CBaseEntity::Instance( pKiller );
				
				if ( pPlayer->m_pActiveItem )
				{
					killer_weapon_name = pPlayer->m_pActiveItem->pszName();
				}
			}
			else
			{
				killer_weapon_name = STRING( pevInflictor->classname );  // it's just that easy
			}
		}
	}
	else
	{
		killer_weapon_name = STRING( pevInflictor->classname );
	}

	// strip the monster_* or weapon_* from the inflictor's classname
	if ( strncmp( killer_weapon_name, "weapon_", 7 ) == 0 )
		killer_weapon_name += 7;
	else if ( strncmp( killer_weapon_name, "monster_", 8 ) == 0 )
		killer_weapon_name += 8;
	else if ( strncmp( killer_weapon_name, "func_", 5 ) == 0 )
		killer_weapon_name += 5;

	MESSAGE_BEGIN( MSG_ALL, gmsgDeathMsg );
		WRITE_BYTE( killer_index );						// the killer
		WRITE_BYTE( ENTINDEX(pVictim->edict()) );		// the victim
		WRITE_STRING( killer_weapon_name );		// what they were killed by (should this be a string?)
	MESSAGE_END();

	// replace the code names with the 'real' names
	if ( !strcmp( killer_weapon_name, "egon" ) )
		killer_weapon_name = gluon;
	else if ( !strcmp( killer_weapon_name, "gauss" ) )
		killer_weapon_name = tau;

	if ( pVictim->pev == pKiller )  
	{
		// killed self

		// team match?
		if ( g_teamplay )
		{
			UTIL_LogPrintf( "\"%s<%i><%u><%s>\" committed suicide with \"%s\"\n",  
				STRING( pVictim->pev->netname ), 
				GETPLAYERUSERID( pVictim->edict() ),
				GETPLAYERWONID( pVictim->edict() ),
				g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pVictim->edict() ), "model" ),
				killer_weapon_name );		
		}
		else
		{
			UTIL_LogPrintf( "\"%s<%i><%u><%i>\" committed suicide with \"%s\"\n",  
				STRING( pVictim->pev->netname ), 
				GETPLAYERUSERID( pVictim->edict() ),
				GETPLAYERWONID( pVictim->edict() ),
				GETPLAYERUSERID( pVictim->edict() ),
				killer_weapon_name );		
		}
	}
	else if ( pKiller->flags & FL_CLIENT )
	{
		// team match?
		if ( g_teamplay )
		{
			UTIL_LogPrintf( "\"%s<%i><%u><%s>\" killed \"%s<%i><%u><%s>\" with \"%s\"\n",  
				STRING( pKiller->netname ),
				GETPLAYERUSERID( ENT(pKiller) ),
				GETPLAYERWONID( ENT(pKiller) ),
				g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( ENT(pKiller) ), "model" ),
				STRING( pVictim->pev->netname ),
				GETPLAYERUSERID( pVictim->edict() ),
				GETPLAYERWONID( pVictim->edict() ),
				g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pVictim->edict() ), "model" ),
				killer_weapon_name );
		}
		else
		{
			UTIL_LogPrintf( "\"%s<%i><%u><%i>\" killed \"%s<%i><%u><%i>\" with \"%s\"\n",  
				STRING( pKiller->netname ),
				GETPLAYERUSERID( ENT(pKiller) ),
				GETPLAYERWONID( ENT(pKiller) ),
				GETPLAYERUSERID( ENT(pKiller) ),
				STRING( pVictim->pev->netname ),
				GETPLAYERUSERID( pVictim->edict() ),
				GETPLAYERWONID( pVictim->edict() ),
				GETPLAYERUSERID( pVictim->edict() ),
				killer_weapon_name );
		}
	}
	else
	{ 
		// killed by the world

		// team match?
		if ( g_teamplay )
		{
			UTIL_LogPrintf( "\"%s<%i><%u><%s>\" committed suicide with \"%s\" (world)\n",
				STRING( pVictim->pev->netname ), 
				GETPLAYERUSERID( pVictim->edict() ), 
				GETPLAYERWONID( pVictim->edict() ),
				g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pVictim->edict() ), "model" ),
				killer_weapon_name );		
		}
		else
		{
			UTIL_LogPrintf( "\"%s<%i><%u><%i>\" committed suicide with \"%s\" (world)\n",
				STRING( pVictim->pev->netname ), 
				GETPLAYERUSERID( pVictim->edict() ), 
				GETPLAYERWONID( pVictim->edict() ),
				GETPLAYERUSERID( pVictim->edict() ),
				killer_weapon_name );		
		}
	}

	MESSAGE_BEGIN( MSG_SPEC, SVC_HLTV );
		WRITE_BYTE ( DRC_EVENT );	// player killed
		WRITE_SHORT( ENTINDEX(pVictim->edict()) );	// index number of primary entity
		if (pevInflictor)
			WRITE_SHORT( ENTINDEX(ENT(pevInflictor)) );	// index number of secondary entity
		else
			WRITE_SHORT( ENTINDEX(ENT(pKiller)) );	// index number of secondary entity
		WRITE_LONG( 7 | DRC_FLAG_DRAMATIC);   // eventflags (priority and flags)
	MESSAGE_END();

//  Print a standard message
	// TODO: make this go direct to console
	return; // just remove for now
/*
	char	szText[ 128 ];

	if ( pKiller->flags & FL_MONSTER )
	{
		// killed by a monster
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " was killed by a monster.\n" );
		return;
	}

	if ( pKiller == pVictim->pev )
	{
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " commited suicide.\n" );
	}
	else if ( pKiller->flags & FL_CLIENT )
	{
		strcpy ( szText, STRING( pKiller->netname ) );

		strcat( szText, " : " );
		strcat( szText, killer_weapon_name );
		strcat( szText, " : " );

		strcat ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, "\n" );
	}
	else if ( FClassnameIs ( pKiller, "worldspawn" ) )
	{
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " fell or drowned or something.\n" );
	}
	else if ( pKiller->solid == SOLID_BSP )
	{
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " was mooshed.\n" );
	}
	else
	{
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " died mysteriously.\n" );
	}

	UTIL_ClientPrintAll( szText );
*/
}

//=========================================================
// PlayerGotWeapon - player has grabbed a weapon that was
// sitting in the world
//=========================================================
void CHalfLifeMultiplay :: PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
float CHalfLifeMultiplay :: FlWeaponRespawnTime( CBasePlayerItem *pWeapon )
{
	if ( weaponstay.value > 0 )
	{
		// make sure it's only certain weapons
		if ( !(pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD) )
		{
			return gpGlobals->time + 0;		// weapon respawns almost instantly
		}
	}

	return gpGlobals->time + WEAPON_RESPAWN_TIME;
}

// when we are within this close to running out of entities,  items 
// marked with the ITEM_FLAG_LIMITINWORLD will delay their respawn
#define ENTITY_INTOLERANCE	100

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn 
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================
float CHalfLifeMultiplay :: FlWeaponTryRespawn( CBasePlayerItem *pWeapon )
{
	if ( pWeapon && pWeapon->m_iId && (pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD) )
	{
		if ( NUMBER_OF_ENTITIES() < (gpGlobals->maxEntities - ENTITY_INTOLERANCE) )
			return 0;

		// we're past the entity tolerance level,  so delay the respawn
		return FlWeaponRespawnTime( pWeapon );
	}

	return 0;
}

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeMultiplay :: VecWeaponRespawnSpot( CBasePlayerItem *pWeapon )
{
	return pWeapon->pev->origin;
}

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CHalfLifeMultiplay :: WeaponShouldRespawn( CBasePlayerItem *pWeapon )
{
	if ( pWeapon->pev->spawnflags & SF_NORESPAWN )
	{
		return GR_WEAPON_RESPAWN_NO;
	}

	return GR_WEAPON_RESPAWN_YES;
}

//=========================================================
// CanHaveWeapon - returns FALSE if the player is not allowed
// to pick up this weapon
//=========================================================
BOOL CHalfLifeMultiplay::CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pItem )
{
	if ( weaponstay.value > 0 )
	{
		if ( pItem->iFlags() & ITEM_FLAG_LIMITINWORLD )
			return CGameRules::CanHavePlayerItem( pPlayer, pItem );

		// check if the player already has this weapon
		for ( int i = 0 ; i < MAX_ITEM_TYPES ; i++ )
		{
			CBasePlayerItem *it = pPlayer->m_rgpPlayerItems[i];

			while ( it != NULL )
			{
				if ( it->m_iId == pItem->m_iId )
				{
					return FALSE;
				}

				it = it->m_pNext;
			}
		}
	}

	return CGameRules::CanHavePlayerItem( pPlayer, pItem );
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::CanHaveItem( CBasePlayer *pPlayer, CItem *pItem )
{
	return TRUE;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem )
{
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::ItemShouldRespawn( CItem *pItem )
{
	if ( pItem->pev->spawnflags & SF_NORESPAWN )
	{
		return GR_ITEM_RESPAWN_NO;
	}

	return GR_ITEM_RESPAWN_YES;
}


//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CHalfLifeMultiplay::FlItemRespawnTime( CItem *pItem )
{
	return gpGlobals->time + ITEM_RESPAWN_TIME;
}

//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeMultiplay::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->pev->origin;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount )
{
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::IsAllowedToSpawn( CBaseEntity *pEntity )
{
//	if ( pEntity->pev->flags & FL_MONSTER )
//		return FALSE;

	return TRUE;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::AmmoShouldRespawn( CBasePlayerAmmo *pAmmo )
{
	if ( pAmmo->pev->spawnflags & SF_NORESPAWN )
	{
		return GR_AMMO_RESPAWN_NO;
	}

	return GR_AMMO_RESPAWN_YES;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo )
{
	return gpGlobals->time + AMMO_RESPAWN_TIME;
}

//=========================================================
//=========================================================
Vector CHalfLifeMultiplay::VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo )
{
	return pAmmo->pev->origin;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlHealthChargerRechargeTime( void )
{
	return 60;
}


float CHalfLifeMultiplay::FlHEVChargerRechargeTime( void )
{
	return 30;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::DeadPlayerWeapons( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_GUN_ACTIVE;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::DeadPlayerAmmo( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_AMMO_ACTIVE;
}

edict_t *CHalfLifeMultiplay::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	edict_t *pentSpawnSpot = CGameRules::GetPlayerSpawnSpot( pPlayer );	
	if ( IsMultiplayer() && pentSpawnSpot->v.target )
	{
		FireTargets( STRING(pentSpawnSpot->v.target), pPlayer, pPlayer, USE_TOGGLE, 0 );
	}

	return pentSpawnSpot;
}


//=========================================================
//=========================================================
int CHalfLifeMultiplay::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	// half life deathmatch has only enemies
	return GR_NOTTEAMMATE;
}

BOOL CHalfLifeMultiplay :: PlayFootstepSounds( CBasePlayer *pl, float fvol )
{
	return FALSE; // FragMented!
}

BOOL CHalfLifeMultiplay :: FAllowFlashlight( void ) 
{ 
	return flashlight.value != 0; 
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay :: FAllowMonsters( void )
{
	return ( allowmonsters.value != 0 );
}

//=========================================================
//======== CHalfLifeMultiplay private functions ===========
#define INTERMISSION_TIME		6
extern int m_iTotalCheckPoints;
void CHalfLifeMultiplay :: GoToIntermission( void )
{
	if ( g_fGameOver )
		return;  // intermission has already been triggered, so ignore.




	MESSAGE_BEGIN(MSG_ALL, SVC_INTERMISSION);
	MESSAGE_END();


	// FragMented!
	for ( int i = 1; i <= gpGlobals->maxClients; i++ ) 
	{ 
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i ); 
		if (plr) 
		{
			// SaRcaZm - Start
			plr->pev->iuser3 = 0; // Reset laps
			// SaRcaZm - End
			plr->pev->iuser4 = 0; // Reset checkpoints
			// SaRcaZm - V2 - Start
			//UTIL_SayText("* Scores are now being uploaded to http://stats.hlrally.net\n", plr);
			//UTIL_SayText("  Press enter when finished looking at the scoreboard.\n", plr);
			// SaRcaZm - V2 - End
		}
	}
	m_iTotalCheckPoints = 0;


	// bounds check
	int time = (int)CVAR_GET_FLOAT( "mp_chattime" );
	if ( time < 10 )
		CVAR_SET_STRING( "mp_chattime", "10" );
	else if ( time > MAX_INTERMISSION_TIME )
		CVAR_SET_STRING( "mp_chattime", UTIL_dtos1( MAX_INTERMISSION_TIME ) );

	m_flIntermissionEndTime = gpGlobals->time + ( (int)mp_chattime.value );
	g_flIntermissionStartTime = gpGlobals->time;

	g_fGameOver = TRUE;
	m_iEndIntermissionButtonHit = FALSE;


}

#define MAX_RULE_BUFFER 1024

typedef struct mapcycle_item_s
{
	struct mapcycle_item_s *next;

	char mapname[ 32 ];
	int  minplayers, maxplayers;
	char rulebuffer[ MAX_RULE_BUFFER ];
} mapcycle_item_t;

typedef struct mapcycle_s
{
	struct mapcycle_item_s *items;
	struct mapcycle_item_s *next_item;
} mapcycle_t;

/*
==============
DestroyMapCycle

Clean up memory used by mapcycle when switching it
==============
*/
void DestroyMapCycle( mapcycle_t *cycle )
{
	mapcycle_item_t *p, *n, *start;
	p = cycle->items;
	if ( p )
	{
		start = p;
		p = p->next;
		while ( p != start )
		{
			n = p->next;
			delete p;
			p = n;
		}
		
		delete cycle->items;
	}
	cycle->items = NULL;
	cycle->next_item = NULL;
}

static char com_token[ 1500 ];

/*
==============
COM_Parse

Parse a token out of a string
==============
*/
char *COM_Parse (char *data)
{
	int             c;
	int             len;
	
	len = 0;
	com_token[0] = 0;
	
	if (!data)
		return NULL;
		
// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;                    // end of file;
		data++;
	}
	
// skip // comments
	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}
	

// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				return data;
			}
			com_token[len] = c;
			len++;
		}
	}

// parse single characters
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c == ',' )
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return data+1;
	}

// parse a regular word
	do
	{
		com_token[len] = c;
		data++;
		len++;
		c = *data;
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c == ',' )
			break;
	} while (c>32);
	
	com_token[len] = 0;
	return data;
}

/*
==============
COM_TokenWaiting

Returns 1 if additional data is waiting to be processed on this line
==============
*/
int COM_TokenWaiting( char *buffer )
{
	char *p;

	p = buffer;
	while ( *p && *p!='\n')
	{
		if ( !isspace( *p ) || isalnum( *p ) )
			return 1;

		p++;
	}

	return 0;
}



/*
==============
ReloadMapCycleFile


Parses mapcycle.txt file into mapcycle_t structure
==============
*/
int ReloadMapCycleFile( char *filename, mapcycle_t *cycle )
{
	char szBuffer[ MAX_RULE_BUFFER ];
	char szMap[ 32 ];
	int length;
	char *pFileList;
	char *aFileList = pFileList = (char*)LOAD_FILE_FOR_ME( filename, &length );
	int hasbuffer;
	mapcycle_item_s *item, *newlist = NULL, *next;

	if ( pFileList && length )
	{
		// the first map name in the file becomes the default
		while ( 1 )
		{
			hasbuffer = 0;
			memset( szBuffer, 0, MAX_RULE_BUFFER );

			pFileList = COM_Parse( pFileList );
			if ( strlen( com_token ) <= 0 )
				break;

			strcpy( szMap, com_token );

			// Any more tokens on this line?
			if ( COM_TokenWaiting( pFileList ) )
			{
				pFileList = COM_Parse( pFileList );
				if ( strlen( com_token ) > 0 )
				{
					hasbuffer = 1;
					strcpy( szBuffer, com_token );
				}
			}

			// Check map
			if ( IS_MAP_VALID( szMap ) )
			{
				// Create entry
				char *s;

				item = new mapcycle_item_s;

				strcpy( item->mapname, szMap );

				item->minplayers = 0;
				item->maxplayers = 0;

				memset( item->rulebuffer, 0, MAX_RULE_BUFFER );

				if ( hasbuffer )
				{
					s = g_engfuncs.pfnInfoKeyValue( szBuffer, "minplayers" );
					if ( s && s[0] )
					{
						item->minplayers = atoi( s );
						item->minplayers = max( item->minplayers, 0 );
						item->minplayers = min( item->minplayers, gpGlobals->maxClients );
					}
					s = g_engfuncs.pfnInfoKeyValue( szBuffer, "maxplayers" );
					if ( s && s[0] )
					{
						item->maxplayers = atoi( s );
						item->maxplayers = max( item->maxplayers, 0 );
						item->maxplayers = min( item->maxplayers, gpGlobals->maxClients );
					}

					// Remove keys
					//
					g_engfuncs.pfnInfo_RemoveKey( szBuffer, "minplayers" );
					g_engfuncs.pfnInfo_RemoveKey( szBuffer, "maxplayers" );

					strcpy( item->rulebuffer, szBuffer );
				}

				item->next = cycle->items;
				cycle->items = item;
			}
			else
			{
				ALERT( at_console, "Skipping %s from mapcycle, not a valid map\n", szMap );
			}

		}

		FREE_FILE( aFileList );
	}

	// Fixup circular list pointer
	item = cycle->items;

	// Reverse it to get original order
	while ( item )
	{
		next = item->next;
		item->next = newlist;
		newlist = item;
		item = next;
	}
	cycle->items = newlist;
	item = cycle->items;

	// Didn't parse anything
	if ( !item )
	{
		return 0;
	}

	while ( item->next )
	{
		item = item->next;
	}
	item->next = cycle->items;
	
	cycle->next_item = item->next;

	return 1;
}

/*
==============
CountPlayers

Determine the current # of active players on the server for map cycling logic
==============
*/
int CountPlayers( void )
{
	int	num = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pEnt = UTIL_PlayerByIndex( i );

		if ( pEnt )
		{
			num = num + 1;
		}
	}

	return num;
}

/*
==============
ExtractCommandString

Parse commands/key value pairs to issue right after map xxx command is issued on server
 level transition
==============
*/
void ExtractCommandString( char *s, char *szCommand )
{
	// Now make rules happen
	char	pkey[512];
	char	value[512];	// use two buffers so compares
								// work without stomping on each other
	char	*o;
	
	if ( *s == '\\' )
		s++;

	while (1)
	{
		o = pkey;
		while ( *s != '\\' )
		{
			if ( !*s )
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;

		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		strcat( szCommand, pkey );
		if ( strlen( value ) > 0 )
		{
			strcat( szCommand, " " );
			strcat( szCommand, value );
		}
		strcat( szCommand, "\n" );

		if (!*s)
			return;
		s++;
	}
}

/*
==============
ChangeLevel

Server is changing to a new level, check mapcycle.txt for map name and setup info
==============
*/
void CHalfLifeMultiplay :: ChangeLevel( void )
{
	static char szPreviousMapCycleFile[ 256 ];
	static mapcycle_t mapcycle;

	char szNextMap[32];
	char szFirstMapInList[32];
	char szCommands[ 1500 ];
	char szRules[ 1500 ];
	int minplayers = 0, maxplayers = 0;
	strcpy( szFirstMapInList, "hldm1" );  // the absolute default level is hldm1

	int	curplayers;
	BOOL do_cycle = TRUE;

	// find the map to change to
	char *mapcfile = (char*)CVAR_GET_STRING( "mapcyclefile" );
	ASSERT( mapcfile != NULL );

	szCommands[ 0 ] = '\0';
	szRules[ 0 ] = '\0';

	curplayers = CountPlayers();

	// Has the map cycle filename changed?
	if ( stricmp( mapcfile, szPreviousMapCycleFile ) )
	{
		strcpy( szPreviousMapCycleFile, mapcfile );

		DestroyMapCycle( &mapcycle );

		if ( !ReloadMapCycleFile( mapcfile, &mapcycle ) || ( !mapcycle.items ) )
		{
			ALERT( at_console, "Unable to load map cycle file %s\n", mapcfile );
			do_cycle = FALSE;
		}
	}

	if ( do_cycle && mapcycle.items )
	{
		BOOL keeplooking = FALSE;
		BOOL found = FALSE;
		mapcycle_item_s *item;

		// Assume current map
		strcpy( szNextMap, STRING(gpGlobals->mapname) );
		strcpy( szFirstMapInList, STRING(gpGlobals->mapname) );

		// Traverse list
		for ( item = mapcycle.next_item; item->next != mapcycle.next_item; item = item->next )
		{
			keeplooking = FALSE;

			ASSERT( item != NULL );

			if ( item->minplayers != 0 )
			{
				if ( curplayers >= item->minplayers )
				{
					found = TRUE;
					minplayers = item->minplayers;
				}
				else
				{
					keeplooking = TRUE;
				}
			}

			if ( item->maxplayers != 0 )
			{
				if ( curplayers <= item->maxplayers )
				{
					found = TRUE;
					maxplayers = item->maxplayers;
				}
				else
				{
					keeplooking = TRUE;
				}
			}

			if ( keeplooking )
				continue;

			found = TRUE;
			break;
		}

		if ( !found )
		{
			item = mapcycle.next_item;
		}			
		
		// Increment next item pointer
		mapcycle.next_item = item->next;

		// Perform logic on current item
		strcpy( szNextMap, item->mapname );

		ExtractCommandString( item->rulebuffer, szCommands );
		strcpy( szRules, item->rulebuffer );
	}

	if ( !IS_MAP_VALID(szNextMap) )
	{
		strcpy( szNextMap, szFirstMapInList );
	}

	g_fGameOver = TRUE;

	ALERT( at_console, "CHANGE LEVEL: %s\n", szNextMap );
	if ( minplayers || maxplayers )
	{
		ALERT( at_console, "PLAYER COUNT:  min %i max %i current %i\n", minplayers, maxplayers, curplayers );
	}
	if ( strlen( szRules ) > 0 )
	{
		ALERT( at_console, "RULES:  %s\n", szRules );
	}
	
	CHANGE_LEVEL( szNextMap, NULL );
	if ( strlen( szCommands ) > 0 )
	{
		SERVER_COMMAND( szCommands );
	}
}

#define MAX_MOTD_CHUNK	  60
#define MAX_MOTD_LENGTH   1536 // (MAX_MOTD_CHUNK * 4)

void CHalfLifeMultiplay :: SendMOTDToClient( edict_t *client )
{
	// read from the MOTD.txt file
	int length, char_count = 0;
	char *pFileList;
	char *aFileList = pFileList = (char*)LOAD_FILE_FOR_ME( (char *)CVAR_GET_STRING( "motdfile" ), &length );

	// send the server name
	MESSAGE_BEGIN( MSG_ONE, gmsgServerName, NULL, client );
		WRITE_STRING( CVAR_GET_STRING("hostname") );
	MESSAGE_END();

	// Send the message of the day
	// read it chunk-by-chunk,  and send it in parts

	while ( pFileList && *pFileList && char_count < MAX_MOTD_LENGTH )
	{
		char chunk[MAX_MOTD_CHUNK+1];
		
		if ( strlen( pFileList ) < MAX_MOTD_CHUNK )
		{
			strcpy( chunk, pFileList );
		}
		else
		{
			strncpy( chunk, pFileList, MAX_MOTD_CHUNK );
			chunk[MAX_MOTD_CHUNK] = 0;		// strncpy doesn't always append the null terminator
		}

		char_count += strlen( chunk );
		if ( char_count < MAX_MOTD_LENGTH )
			pFileList = aFileList + char_count; 
		else
			*pFileList = 0;

		MESSAGE_BEGIN( MSG_ONE, gmsgMOTD, NULL, client );
			WRITE_BYTE( *pFileList ? FALSE : TRUE );	// FALSE means there is still more message to come
			WRITE_STRING( chunk );
		MESSAGE_END();
	}

	FREE_FILE( aFileList );
}
	
// SaRcaZm - V6 - Start
#define MAX_TEXTFILE_CHUNK	  60
//#define MAX_TEXTFILE_LENGTH   5120

void CHalfLifeMultiplay::SendTextFileToClient (edict_t *client, char *aFileList, int length, int gmsgNum)
{
	// Compress the files using ZLib
	int compressedlength = length + 13;
	char *compressed = (char *) malloc (compressedlength);

// SaRcaZm - V8 - Start
	// FragMented Zlib dll
	if (!ZLibFuncs.ZLib_Compress2 || ZLibFuncs.ZLib_Compress2 ((unsigned char *)compressed, (unsigned long *) &compressedlength, (const unsigned char *)aFileList, length, 9) != Z_OK)
	{
// SaRcaZm - V8 - End
		// Just copy the plain text and try sending that
		UTIL_ClientPrintAll (HUD_PRINTNOTIFY, "Couldn't init ZLIB\n");
		memcpy (compressed, aFileList, length);
		compressedlength = length;
	}

	int char_count = 0;
	char *pFileList = compressed;

	// Send the text file
	// read it chunk-by-chunk,  and send it in parts
	while (char_count < compressedlength)
	{
		char chunk[MAX_TEXTFILE_CHUNK+1];
		int size = 0;

		if ( compressedlength - char_count < MAX_TEXTFILE_CHUNK )
			size = compressedlength - char_count;
		else
			size = MAX_TEXTFILE_CHUNK;

		memcpy( chunk, pFileList, size);
		chunk[MAX_TEXTFILE_CHUNK] = 0;		// strncpy doesn't always append the null terminator

		char_count += size;
		if ( char_count < compressedlength )
			pFileList = compressed + char_count; 
		else
			char_count = compressedlength + 1;

		MESSAGE_BEGIN( MSG_ONE, gmsgNum, NULL, client );
			WRITE_BYTE( size );		// < MAX_TEXTFILE_CHUNK means it is the end of the message

			for (int i = 0; i < size; i++)
				//WRITE_STRING( chunk );
				WRITE_CHAR (*(chunk + i));

		MESSAGE_END();
	}

	free (compressed);
}
// SaRcaZm - V6 - End
