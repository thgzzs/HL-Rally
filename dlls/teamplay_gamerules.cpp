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
#include	"teamplay_gamerules.h"
#include	"game.h"
#include	"rally_rounds.h"

static char team_names[MAX_TEAMS][MAX_TEAMNAME_LENGTH];
static int team_scores[MAX_TEAMS];
static int num_teams = 0;

extern DLL_GLOBAL BOOL		g_fGameOver;

// SaRcaZm - Start
extern int iRoundPrimed;
// SaRcaZm - End
// SaRcaZm - V3 - Start
#include "carinfo.h"
// SaRcaZm - V3 - End

CHalfLifeTeamplay :: CHalfLifeTeamplay()
{
	m_DisableDeathMessages = FALSE;
	m_DisableDeathPenalty = FALSE;

	memset( team_names, 0, sizeof(team_names) );
	memset( team_scores, 0, sizeof(team_scores) );
	num_teams = 0;

	// Copy over the team from the server config
	m_szTeamList[0] = 0;
	// FragMented hack hack

	// Cache this because the team code doesn't want to deal with changing this in the middle of a game
	// SaRcaZm - V3 - Start
	//strncpy( m_szTeamList, "Citroen;Ford;Hyundai;Lancia;Mitsubishi;Peugeot;Renault;Skoda;Subaru;Toyota;Volkswagon", TEAMPLAY_TEAMLISTLENGTH );

	// Read this in from a file now
	// SaRcaZm - V5 - Start
	//CCarInfo *carinfo = new CCarInfo ("carinfo.txt");
	CCarInfo *carinfo = new CCarInfo (hlConst.szCarInfo);
	// SaRcaZm - V5 - End
	char szMake[30];

	carinfo->getNextManufacturer (szMake);

	while (szMake[0])
	{
		// Copy it to the list
		strcat (m_szTeamList, szMake);
		strcat (m_szTeamList, ";");
		// Get the next manufacturer
		memset(szMake, 0, 30);
		carinfo->getNextManufacturer (szMake);
	}

	// Add spectator onto the list of acceptable teams
	// SaRcaZm - V5 - Start
	// Temporary fix for buggy spectator mode
	//strcat (m_szTeamList, "Spectator");
	// SaRcaZm - V5 - End
	delete carinfo;
	// SaRcaZm - V3 - End

	edict_t *pWorld = INDEXENT(0);
	if ( pWorld && pWorld->v.team )
	{
		if ( teamoverride.value )
		{
			const char *pTeamList = STRING(pWorld->v.team);
			if ( pTeamList && strlen(pTeamList) )
			{
				strncpy( m_szTeamList, pTeamList, TEAMPLAY_TEAMLISTLENGTH );
			}
		}
	}
	// Has the server set teams
	if ( strlen( m_szTeamList ) )
		m_teamLimit = TRUE;
	else
		m_teamLimit = FALSE;

	RecountTeams();

	// SaRcaZm - Start
	// This func is called at map change, so make sure we prime the round again
	iRoundPrimed = 0;
	// SaRcaZm - End
}

extern cvar_t timeleft, fragsleft;

#include "voice_gamemgr.h"
extern CVoiceGameMgr	g_VoiceGameMgr;

void CHalfLifeTeamplay :: Think ( void )
{  
	///// Check game rules /////
	static int last_frags;
	static int last_time;

	int frags_remaining = 0;
	int time_remaining = 0;

	g_VoiceGameMgr.Update(gpGlobals->frametime);

	if ( g_fGameOver )   // someone else quit the game already
	{
		CHalfLifeMultiplay::Think();
		return;
	}

	//float flTimeLimit = 9010; // FragMented
	float flTimeLimit = timelimit.value * 60; // WTF were you doing Frag?

	time_remaining = (int)(flTimeLimit ? ( flTimeLimit - gpGlobals->time ) : 0);

	if ( flTimeLimit != 0 && gpGlobals->time >= flTimeLimit &&
		((g_pGameRules->m_RoundState == ROUND_NOTSTARTED) || (g_pGameRules->m_RoundState == ROUND_COUNTDOWN)))
	{
		GoToIntermission();
		return;
	}

	float flFragLimit = fraglimit.value;
	if ( flFragLimit )
	{
		int bestfrags = 9999;
		int remain;

		// check if any team is over the frag limit
		for ( int i = 0; i < num_teams; i++ )
		{
			if ( team_scores[i] >= flFragLimit &&
				((g_pGameRules->m_RoundState == ROUND_NOTSTARTED) || (g_pGameRules->m_RoundState == ROUND_COUNTDOWN)))
			{
				GoToIntermission();
				return;
			}

			remain = flFragLimit - team_scores[i];
			if ( remain < bestfrags )
			{
				bestfrags = remain;
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

	RoundThink();
	HornThink();
}

//=========================================================
// ClientCommand
// the user has typed a command which is unrecognized by everything else;
// this check to see if the gamerules knows anything about the command
//=========================================================
BOOL CHalfLifeTeamplay :: ClientCommand( CBasePlayer *pPlayer, const char *pcmd )
{
	if(g_VoiceGameMgr.ClientCommand(pPlayer, pcmd))
		return TRUE;

	if ( FStrEq( pcmd, "menuselect" ) )
	{
		if ( CMD_ARGC() < 2 )
			return TRUE;

		int slot = atoi( CMD_ARGV(1) );

		// select the item from the current menu

		return TRUE;
	}

	return FALSE;
}

extern int gmsgGameMode;
extern int gmsgSayText;
extern int gmsgTeamInfo;
extern int gmsgTeamNames;
// SaRcaZm - V8 - Start
//extern int gmsgScoreInfo;
// SaRcaZm - V8 - End

void CHalfLifeTeamplay :: UpdateGameMode( CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgGameMode, NULL, pPlayer->edict() );
		WRITE_BYTE( 1 );  // game mode teamplay
	MESSAGE_END();
}


const char *CHalfLifeTeamplay::SetDefaultPlayerTeam( CBasePlayer *pPlayer )
{
	// copy out the team name from the model
	// FragMented Comment as no model = team :(
//	char *mdls = g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model" );
//	strncpy( pPlayer->m_szTeamName, mdls, TEAM_NAME_LENGTH );

	RecountTeams();

	// update the current player of the team he is joining
	if ( pPlayer->m_szTeamName[0] == '\0' || !IsValidTeam( pPlayer->m_szTeamName ) || defaultteam.value )
	{
		const char *pTeamName = NULL;
		
		if ( defaultteam.value )
		{
			pTeamName = team_names[0];
		}
		else
		{
			pTeamName = TeamWithFewestPlayers();
		}
		strncpy( pPlayer->m_szTeamName, pTeamName, TEAM_NAME_LENGTH );
	}

	return pPlayer->m_szTeamName;
}


//=========================================================
// InitHUD
//=========================================================
void CHalfLifeTeamplay::InitHUD( CBasePlayer *pPlayer )
{
	int i;

	SetDefaultPlayerTeam( pPlayer );
	CHalfLifeMultiplay::InitHUD( pPlayer );

	// Send down the team names
	MESSAGE_BEGIN( MSG_ONE, gmsgTeamNames, NULL, pPlayer->edict() );  
		WRITE_BYTE( num_teams );
		for ( i = 0; i < num_teams; i++ )
		{
			WRITE_STRING( team_names[ i ] );
		}
	MESSAGE_END();

	RecountTeams();

/*	char *mdls = g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model" );
	// update the current player of the team he is joining
	char text[1024];
	if ( !strcmp( mdls, pPlayer->m_szTeamName ) )
	{
		sprintf( text, "* you are on team \'%s\'\n", pPlayer->m_szTeamName );
	}
	else
	{
		sprintf( text, "* assigned to team %s\n", pPlayer->m_szTeamName );
	}

	ChangePlayerTeam( pPlayer, pPlayer->m_szTeamName, FALSE, FALSE );
	UTIL_SayText( text, pPlayer );*/
	int clientIndex = pPlayer->entindex();
	RecountTeams(); // SZ_Getspace
	// update this player with all the other players team info
	// loop through all active players and send their team info to the new client
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *plr = UTIL_PlayerByIndex( i );
		if ( plr && IsValidTeam( plr->TeamID() ) )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgTeamInfo, NULL, pPlayer->edict() );
				WRITE_BYTE( plr->entindex() );
				WRITE_STRING( plr->TeamID() );
			MESSAGE_END();
		}
	}

	

}


void CHalfLifeTeamplay::ChangePlayerTeam( CBasePlayer *pPlayer, const char *pTeamName, BOOL bKill, BOOL bGib )
{
	int damageFlags = DMG_GENERIC;
	int clientIndex = pPlayer->entindex();

	// SaRcaZm - V7 - Start
	// All this should only be done when the race has started
	// They can now start playing
	pPlayer->pev->fuser1 = 1.0f;

	// Now that they have chosen a team, they can enter the game
	if (m_RoundState == 1)
	{
		pPlayer->pev->movetype	= MOVETYPE_WALK;
		pPlayer->m_iHideHUD = 0;
		pPlayer->pev->effects = 0;
		// Drop them to the floor when they have choosen a team
		DROP_TO_FLOOR (pPlayer->edict ());
		pPlayer->EnableControl (true);
	}
	// SaRcaZm - V7 - End

	if ( !bGib )
	{
		damageFlags |= DMG_NEVERGIB;
	}
	else
	{
		damageFlags |= DMG_ALWAYSGIB;
	}

	if ( bKill )
	{
		// kill the player,  remove a death,  and let them start on the new team
		m_DisableDeathMessages = TRUE;
		m_DisableDeathPenalty = TRUE;

		entvars_t *pevWorld = VARS( INDEXENT(0) );
		pPlayer->TakeDamage( pevWorld, pevWorld, 900, damageFlags );

		m_DisableDeathMessages = FALSE;
		m_DisableDeathPenalty = FALSE;
	}

	// copy out the team name from the model
	strncpy( pPlayer->m_szTeamName, pTeamName, TEAM_NAME_LENGTH );

//  FragMented! comment as no model = team
//	g_engfuncs.pfnSetClientKeyValue( clientIndex, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model", pPlayer->m_szTeamName );
	g_engfuncs.pfnSetClientKeyValue( clientIndex, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "team", pPlayer->m_szTeamName );

	// notify everyone's HUD of the team change
	//FRAGMENTED SZ_GETSPACE ERROR
	// SaRcaZm - V4 - Start
	// We need to do this
	MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
		WRITE_BYTE( clientIndex );
		//WRITE_SHORT( g_pGameRules->GetTeamIndex( pPlayer->m_szTeamName ) + 1 );
		WRITE_STRING( pPlayer->m_szTeamName );	// This is supposed to be a string
	MESSAGE_END(); 

/*	// SaRcaZm - V4 - End
	 FILE *fp; 
	 fp = fopen("c:\\testing_monkeys.txt", "a"); 
		 fprintf(fp, "Sending Message \"TeamInfo\" with Byte \"%i\" and String \"%s\"\n", clientIndex, pPlayer->m_szTeamName); 
	 fclose(fp);*/

	// SaRcaZm - V8 - Start
	/*MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( clientIndex );
		WRITE_SHORT( pPlayer->pev->frags );
		WRITE_SHORT( pPlayer->m_iDeaths );
		WRITE_SHORT( 0 );
		WRITE_SHORT( g_pGameRules->GetTeamIndex( pPlayer->m_szTeamName ) + 1 );
	MESSAGE_END();*/
	pPlayer->AddPoints (0, true);
	// SaRcaZm - V8 - End
}


//=========================================================
// ClientUserInfoChanged
//=========================================================
void CHalfLifeTeamplay::ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer )
{
	// Check if its the model that was changed 
	if(!FStrEq(g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model"), g_engfuncs.pfnInfoKeyValue( infobuffer, "model" )))  	// ReDucTor
	{	
		// We Are changing model
		char *mdls = g_engfuncs.pfnInfoKeyValue( infobuffer, "model" ); // FragMented {
		char sz[1024];
		sprintf( sz, "* %s is now driving a %s %s\n", STRING(pPlayer->pev->netname), pPlayer->m_szTeamName, mdls );
		UTIL_SayTextAll( sz, pPlayer ); // } FragMented

		pPlayer->pev->classname = MAKE_STRING(mdls);
	}
}

extern int gmsgDeathMsg;

//=========================================================
// Deathnotice. 
//=========================================================
void CHalfLifeTeamplay::DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pevInflictor )
{
	if ( m_DisableDeathMessages )
		return;
	
	if ( pVictim && pKiller && pKiller->flags & FL_CLIENT )
	{
		CBasePlayer *pk = (CBasePlayer*) CBaseEntity::Instance( pKiller );

		if ( pk )
		{
			if ( (pk != pVictim) && (PlayerRelationship( pVictim, pk ) == GR_TEAMMATE) )
			{
				MESSAGE_BEGIN( MSG_ALL, gmsgDeathMsg );
					WRITE_BYTE( ENTINDEX(ENT(pKiller)) );		// the killer
					WRITE_BYTE( ENTINDEX(pVictim->edict()) );	// the victim
					WRITE_STRING( "teammate" );		// flag this as a teammate kill
				MESSAGE_END();
				return;
			}
		}
	}

	CHalfLifeMultiplay::DeathNotice( pVictim, pKiller, pevInflictor );
}

//=========================================================
//=========================================================
void CHalfLifeTeamplay :: PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	if ( !m_DisableDeathPenalty )
	{
		CHalfLifeMultiplay::PlayerKilled( pVictim, pKiller, pInflictor );
		RecountTeams();
	}
}


//=========================================================
// IsTeamplay
//=========================================================
BOOL CHalfLifeTeamplay::IsTeamplay( void )
{
	return TRUE;
}

BOOL CHalfLifeTeamplay::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker )
{
	if ( pAttacker && PlayerRelationship( pPlayer, pAttacker ) == GR_TEAMMATE )
	{
		// my teammate hit me.
		if ( (friendlyfire.value == 0) && (pAttacker != pPlayer) )
		{
			// friendly fire is off, and this hit came from someone other than myself,  then don't get hurt
			return FALSE;
		}
	}

	return CHalfLifeMultiplay::FPlayerCanTakeDamage( pPlayer, pAttacker );
}

//=========================================================
//=========================================================
int CHalfLifeTeamplay::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	// half life multiplay has a simple concept of Player Relationships.
	// you are either on another player's team, or you are not.
	if ( !pPlayer || !pTarget || !pTarget->IsPlayer() )
		return GR_NOTTEAMMATE;

	if ( (*GetTeamID(pPlayer) != '\0') && (*GetTeamID(pTarget) != '\0') && !stricmp( GetTeamID(pPlayer), GetTeamID(pTarget) ) )
	{
		return GR_TEAMMATE;
	}

	return GR_NOTTEAMMATE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeTeamplay::ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target )
{
	// always autoaim, unless target is a teammate
	CBaseEntity *pTgt = CBaseEntity::Instance( target );
	if ( pTgt && pTgt->IsPlayer() )
	{
		if ( PlayerRelationship( pPlayer, pTgt ) == GR_TEAMMATE )
			return FALSE; // don't autoaim at teammates
	}

	return CHalfLifeMultiplay::ShouldAutoAim( pPlayer, target );
}

//=========================================================
//=========================================================
int CHalfLifeTeamplay::IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	if ( !pKilled )
		return 0;

	if ( !pAttacker )
		return 1;

	if ( pAttacker != pKilled && PlayerRelationship( pAttacker, pKilled ) == GR_TEAMMATE )
		return -1;

	return 1;
}

//=========================================================
//=========================================================
const char *CHalfLifeTeamplay::GetTeamID( CBaseEntity *pEntity )
{
	if ( pEntity == NULL || pEntity->pev == NULL )
		return "";

	// return their team name
	return pEntity->TeamID();
}


int CHalfLifeTeamplay::GetTeamIndex( const char *pTeamName )
{
	if ( pTeamName && *pTeamName != 0 )
	{
		// try to find existing team
		for ( int tm = 0; tm < num_teams; tm++ )
		{
			if ( !stricmp( team_names[tm], pTeamName ) )
				return tm;
		}
	}
	
	return -1;	// No match
}


const char *CHalfLifeTeamplay::GetIndexedTeamName( int teamIndex )
{
	if ( teamIndex < 0 || teamIndex >= num_teams )
		return "";

	return team_names[ teamIndex ];
}


BOOL CHalfLifeTeamplay::IsValidTeam( const char *pTeamName ) 
{
	if ( !m_teamLimit )	// Any team is valid if the teamlist isn't set
		return TRUE;

	return ( GetTeamIndex( pTeamName ) != -1 ) ? TRUE : FALSE;
}

const char *CHalfLifeTeamplay::TeamWithFewestPlayers( void )
{
	int i;
	int minPlayers = MAX_TEAMS;
	int teamCount[ MAX_TEAMS ];
	char *pTeamName = NULL;

	memset( teamCount, 0, MAX_TEAMS * sizeof(int) );
	
	// loop through all clients, count number of players on each team
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *plr = UTIL_PlayerByIndex( i );

		if ( plr )
		{
			int team = GetTeamIndex( plr->TeamID() );
			if ( team >= 0 )
				teamCount[team] ++;
		}
	}

	// Find team with least players
	for ( i = 0; i < num_teams; i++ )
	{
		if ( teamCount[i] < minPlayers )
		{
			minPlayers = teamCount[i];
			pTeamName = team_names[i];
		}
	}

	return pTeamName;
}


//=========================================================
//=========================================================
void CHalfLifeTeamplay::RecountTeams( void )
{
	char	*pName;
	char	teamlist[TEAMPLAY_TEAMLISTLENGTH];

	// loop through all teams, recounting everything
	num_teams = 0;

	// Copy all of the teams from the teamlist
	// make a copy because strtok is destructive
	strcpy( teamlist, m_szTeamList );
	pName = teamlist;
	pName = strtok( pName, ";" );
	while ( pName != NULL && *pName )
	{
		if ( GetTeamIndex( pName ) < 0 )
		{
			strcpy( team_names[num_teams], pName );
			num_teams++;
		}
		pName = strtok( NULL, ";" );
	}

	if ( num_teams < 2 )
	{
		num_teams = 0;
		m_teamLimit = FALSE;
	}

	// Sanity check
	memset( team_scores, 0, sizeof(team_scores) );

	// loop through all clients
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *plr = UTIL_PlayerByIndex( i );

		if ( plr )
		{
			const char *pTeamName = plr->TeamID();
			// try add to existing team
			int tm = GetTeamIndex( pTeamName );
			
			if ( tm < 0 ) // no team match found
			{ 
				if ( !m_teamLimit )
				{
					// add to new team
					tm = num_teams;
					num_teams++;
					team_scores[tm] = 0;
					strncpy( team_names[tm], pTeamName, MAX_TEAMNAME_LENGTH );
				}
			}

			if ( tm >= 0 )
			{
				team_scores[tm] += plr->pev->frags;
			}
		}
	}
}
