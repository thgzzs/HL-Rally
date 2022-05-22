/*
********************************************************************************
*
*	Copyright (c) HL Rally Team 2001
*	
*   Author: FragMented!
*  
*   Purpose: HL Rally Key Round Code
*
********************************************************************************
*/ 


#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "game.h"

#include "rally_rounds.h"
#include "rally_bot.h"

extern DLL_GLOBAL BOOL g_fGameOver;
// SaRcaZm - V8 - Start
//extern int gmsgScoreInfo;
// SaRcaZm - V8 - End

extern void respawn(entvars_t *pev, BOOL fCopyCorpse); // needed to respawn players

bool FirstSpawn;	// First Spawn stops the car from respawning in same spot
bool Exploded;		// HP Mode ;D

// SaRcaZm - V8 - Start
bool Said30;

extern float pers_newlaps, pers_numlaps;
extern float pers_racemode, pers_newmode;
extern float pers_direction, pers_newdirection;

bool ab_finished;
// SaRcaZm - V8 - End

int iRoundPrimed = 0;

// SaRcaZm - V6 - Start
int iStartTime = 0, iTimePerLap = 0;
float fGlobalRaceStartTime = -1;
bool bTimeExtensionEnd[32];

// Struct to save the player time extension times
extern cvar_t hptime;
float tetimes[32];	// MAX_CLIENTS = 32
// SaRcaZm - V6 - End

// SaRcaZm - V8 - Start
void RaceRestart (void)
{
	g_pGameRules->RoundEnd (ALL_FINISHED, NULL);
}
// SaRcaZm - V8 - End

void UpdateRaceInfo (void)
{
	int numplayers = 0;

	// Count the number of players
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *plr = UTIL_PlayerByIndex( i );

		if ( plr )
		{
			numplayers++;
		}
	}

	// Send the total number of laps and checkpoints to the client for the HUD
	MESSAGE_BEGIN (MSG_ALL, gmsgRaceInfo);
		WRITE_BYTE (numlaps.value);
		WRITE_BYTE (m_iTotalCheckPoints);
		WRITE_BYTE (numplayers);

		// SaRcaZm - V7 - Start
		if((STRING(gpGlobals->mapname))[0] == 'h' && (STRING(gpGlobals->mapname))[1] == 'p')
			WRITE_BYTE (hptime.value);
		else if (racemode.value == 2)
			WRITE_BYTE (iStartTime + (numlaps.value * iTimePerLap));
		else
			WRITE_BYTE (0);
		// SaRcaZm - V7 - End
	MESSAGE_END ();
}

//==========================
// CRoundRules Constructor 
//==========================
void CGameRules :: RoundPrime ( void ) 
{ 
	// SaRcaZm - V5 - Start
	m_flRoundTime = gpGlobals->time + prerace.value + COUNTDOWN_TIME;//4;
	// SaRcaZm - V5 - End

	m_flCheckRound = -1; 
	m_RoundState = ROUND_NOTSTARTED;

	FirstSpawn = true;
}

//===============
// Restart Round 
//===============
void CGameRules::RoundRestart ( void )
{
	m_RoundState = ROUND_NOTSTARTED;
	// SaRcaZm - V5 - Start
	m_flRoundTime = gpGlobals->time + prerace.value + COUNTDOWN_TIME;//4;
	// SaRcaZm - V5 - End

	// Freeze Everyone 
	for ( int i = 1; i <= gpGlobals->maxClients; i++ ) 
	{ 
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i ); 
		if (plr) 
		{
			plr->pev->movetype = MOVETYPE_TOSS;
			/* SaRcaZm - V5 - Start
				UTIL_SayText(UTIL_VarArgs ("Race Commencing in %i Seconds...\n",
				(int)prerace.value + 4), plr);
			SaRcaZm - V5 - End */

			CLIENT_COMMAND(plr->edict(), "+showscores\n");

			// SaRcaZm - V8 - Start
//			if (!FirstSpawn)
//			if (plr->pev->flags & FL_FAKECLIENT)
//				respawn(plr->pev, FALSE);
			/////////////////////////////
//			g_pGameRules->GetPlayerSpawnSpot( plr );
			///////////////////////////
			plr->RemoveAllItems(false);
			// SaRcaZm - V8 - End
		}
	}
}

//===================
// Check Round Start 
//===================
void CGameRules::RoundCheckStart ( void )
{
	if (gpGlobals->time > m_flRoundTime)
	{
		int pCount = 0;

		// SaRcaZm - V6 - Start
		fGlobalRaceStartTime = gpGlobals->time;
		// SaRcaZm - V6 - End

		m_RoundState = ROUND_STARTED;	// Start Round

		if((STRING(gpGlobals->mapname))[0] == 'h' && (STRING(gpGlobals->mapname))[1] == 'p')
		{
			m_flRoundTime = gpGlobals->time + ROUNDTIME_LIMIT_HP + RANDOM_LONG(10, 60);
		}
		else
		{
			m_flRoundTime = gpGlobals->time + ROUNDTIME_LIMIT; 	// Set the time of the round 
		}

		m_flCheckRound = gpGlobals->time + ROUNDTIME_CHECK; // check for winner in 10 secs 

		// Respawn Everyone 
		for ( int i = 1; i <= gpGlobals->maxClients; i++ ) 
		{ 
			CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i ); 
			if (plr) 
			{
				pCount++;
				if (plr->pev->fuser1)
					plr->pev->movetype = MOVETYPE_WALK;

				// SaRcaZm - V8 - Start
				plr->pev->iuser3 = 0;
				if (direction.value < 0)
					plr->pev->iuser4 = m_iTotalCheckPoints - 2;
				else
					plr->pev->iuser4 = 0;	// Reset the lap counter and checkpoint counter
				// SaRcaZm - V8 - End

				plr->pev->fuser4 = plr->pev->fuser3 = gpGlobals->time;	// Set race and lap times
				MESSAGE_BEGIN( MSG_ONE, gmsgSetTimer, NULL, plr->pev );
					// SaRcaZm - V5 - Start
					//WRITE_COORD (0);	// This is now the global time the timer was supposed to start
					WRITE_COORD (gpGlobals->time);
					//WRITE_SHORT (1);
					// SaRcaZm - V5 - End
				MESSAGE_END();

				// SaRcaZm - V7 - Start
				// Save this till it gets reset at the 1st checkpoint
				//plr->pev->iuser4 = 0;
				// SaRcaZm - V7 - End

				// SaRcaZm - V7 - Start
				if (racemode.value == 2)
					tetimes[i] = iStartTime + (iTimePerLap * numlaps.value);
				// SaRcaZm - V7 - End
			}
		}

		// Hot Potato Mode!
		if((STRING(gpGlobals->mapname))[0] == 'h' && (STRING(gpGlobals->mapname))[1] == 'p')
		{
			CBasePlayer *hpPlayer;
			Exploded = true;

			int i = 0; // Stop infinite loop
			while(pCount && Exploded && i < 1024)
			{
				hpPlayer = (CBasePlayer *)UTIL_PlayerByIndex(RANDOM_LONG(1, pCount));

				if(hpPlayer->pev && hpPlayer->IsAlive())
				{
					hpPlayer->GiveNamedItem("weapon_rallyhp");
					Exploded = false;

					char sz[256];
					sprintf(sz, "WARNING: %s [%s] has a car bomb on board!\n", STRING(hpPlayer->pev->netname), hpPlayer->m_szTeamName);
					UTIL_SayTextAll(sz, hpPlayer);
				}
				i++;
			}

			CVAR_SET_FLOAT ("mp_racemode", 1);
		}

		Said30 = false;

		// Reset the places of all checkpoints on the course
		edict_t *pentFind = FIND_ENTITY_BY_CLASSNAME( NULL, "info_checkpoint" );
		while ( !FNullEnt( pentFind ) )
		{
			CRallyCheckPoint *pEnt = (CRallyCheckPoint *)CBaseEntity::Instance( pentFind );
			if ( pEnt )
			{
				pEnt->ClearPlaces ();
			}
			pentFind = FIND_ENTITY_BY_CLASSNAME( pentFind, "info_checkpoint" );
		}

		ab_finished = false;
	} 
}

//=================
// Check Round End 
//=================
void CGameRules::RoundCheckEnd ( void ) 
{
	BOOL bAllFinished = TRUE;

	float flTempRoundTime = m_flRoundTime;

	if((STRING(gpGlobals->mapname))[0] == 'h' && (STRING(gpGlobals->mapname))[1] == 'p')
		flTempRoundTime *= 4;

	// SaRcaZm - V5 - Start
	if ( flTempRoundTime <= gpGlobals->time )
	{
		RoundEnd ( TIME_UP, NULL );
		return;
	}

	// Moved this up from the bottom of this function
	// Check to see if all players have crossed the finish line
	// SaRcaZm - V7 - Start
	for ( int i = 1; (i <= gpGlobals->maxClients) /*&& bAllFinished*/; i++ ) 
	{ 
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );
		bool bFinishedRace = true, bTimeOut = true;

		if (!plr)
			continue;

		// SaRcaZm - V8 - Start
		if ((plr->fABSpectateTime != -1) && (plr->fABSpectateTime < gpGlobals->time))
			plr->StartDeathCam ();
		// SaRcaZm - V8 - End

		if ((plr->pev->iuser3 != numlaps.value) && !(plr->pev->iuser1))
			bFinishedRace = false;

		if (racemode.value == 2)
		{
			// Check to see if any players' time extension time has run out
			if (!bTimeExtensionEnd[i] &&
					(tetimes[i] - (gpGlobals->time - fGlobalRaceStartTime) <= 0))	// Time ran out
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgSetTimer, NULL, plr->pev );
					WRITE_COORD (-1);
				MESSAGE_END ();
				MESSAGE_BEGIN (MSG_ONE, gmsgRaceEnd, NULL, plr->pev);
					WRITE_BYTE (ENTINDEX(plr->edict()));
					WRITE_BYTE (plr->pev->iuser4);
					// Haven't finished lap, so send a NULL lap time
					WRITE_COORD (-1);
					WRITE_COORD (gpGlobals->time - fGlobalRaceStartTime);
				MESSAGE_END ();

				// SaRcaZm - V7 - Start
				// Start observer mode
				plr->StartDeathCam ();
				// SaRcaZm - V7 - End

				bTimeExtensionEnd[i] = true;
			}
			else if (plr && !bTimeExtensionEnd[i] && !(plr->pev->iuser1))
				bTimeOut = false;

			bAllFinished = bAllFinished && (bFinishedRace || bTimeOut);
		}
		else
			bAllFinished = bAllFinished && bFinishedRace;
	}
	// SaRcaZm - V7 - End

	if (bAllFinished)
	{
		RoundEnd (ALL_FINISHED, NULL);
		return;
	}
	// SaRcaZm - V5 - End
	else if ( m_flRoundTime <= gpGlobals->time + 30 && !Said30)
		Said30 = true;
	// SaRcaZm - V7 - End
}

//===========
// Round End
//===========
void CGameRules::RoundEnd ( int reason, char *finisher )
{
	// loop through all clients, put them in observer mode 
	for ( int i = 1; i <= gpGlobals->maxClients; i++ ) 
	{
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );

		if (plr)
		{
			char sz[256];
			plr->pev->viewmodel	= NULL;

			switch (reason)
			{
				case HP_END:
					// Hot Potato Mode! BOOM!
					if(plr->HasNamedPlayerItem("weapon_rallyhp") && !Exploded)
					{
						plr->RemoveAllItems(false);

						MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, plr->pev->origin );
							WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
							WRITE_COORD( plr->pev->origin.x );	// Send to PAS because of the sound
							WRITE_COORD( plr->pev->origin.y );
							WRITE_COORD( plr->pev->origin.z );
							WRITE_SHORT( g_sModelIndexFireball );
							WRITE_BYTE( (plr->pev->dmg - 50) * 3  ); // scale * 10
							WRITE_BYTE( 15  ); // framerate
							WRITE_BYTE( TE_EXPLFLAG_NONE );
						MESSAGE_END();

						//RadiusDamage ( plr->pev->origin, plr->pev, NULL, 150, 500, CLASS_NONE, DMG_BLAST );
						Exploded = true;
						plr->AddPoints (-4, true);
						// Oops, Hot potatoes lose money
						plr->RemoveMoney (400);

						plr->pev->deadflag = DEAD_DEAD;

						sprintf(sz, "** Hot potato detonates on %s. **\n", finisher);
						UTIL_SayTextAll(sz, plr);
					}

					plr->StartDeathCam ();

					// Default "I tried my best" money
					plr->AddMoney (600);
					plr->UpdateMoney ();

					break;

				case AB_FINISH:
					// SaRcaZm - V8 - Start
					if (ab_finished)
						break;			// Don't say they finished 1st more than once

					plr->StartDeathCam ();
					ab_finished = true;
					sprintf(sz, "%s finished the race in 1st place.\n", finisher);
					UTIL_SayText(sz, plr);
					// SaRcaZm - V8 - End
					break;
				case ALL_FINISHED:
					// SaRcaZm - V8 - Start
					//UTIL_SayText("Race Restarting\n", plr);	// This is useless
					// SaRcaZm - V8 - End
					break;

				default:
				case TIME_UP:
					UTIL_SayText("Times Up...\n", plr);
					break;
			}

			// SaRcaZm - V5 - Start
			if (reason != AB_FINISH)
			{
			// SaRcaZm - V5 - End
				plr->pev->effects |= EF_NODRAW;

				// SaRcaZm - V7 - Start
				//if(!plr->IsAlive() && plr->pev->iuser3 != numlaps.value)
				//	EMIT_SOUND(plr->edict(), CHAN_VOICE, "voc/nexttime.wav", 1, ATTN_NORM);
				// SaRcaZm - V7 - End

				plr->StartDeathCam ();

				// Stop the timer
				MESSAGE_BEGIN( MSG_ONE, gmsgSetTimer, NULL, plr->pev );
					// SaRcaZm - V5 - Start
					//WRITE_COORD (0);
					//WRITE_SHORT (0);
					WRITE_COORD (-1);
					// SaRcaZm - V5 - End
				MESSAGE_END();

				CLIENT_COMMAND(plr->edict(), "+showscores\n");
			// SaRcaZm - V5 - Start
			}
			else
			{
				// Let them skid to a stop
				// Stop the timer
				MESSAGE_BEGIN( MSG_ONE, gmsgSetTimer, NULL, plr->pev );
					//WRITE_COORD (0);
					//WRITE_SHORT (0);
					WRITE_COORD (-1);
				MESSAGE_END();
			}
			// SaRcaZm - V5 - End
		}
	}

	m_RoundState = ROUND_END; 
	// This was doubling the amount of time between races
	//m_flRoundTime = gpGlobals->time + ROUNDTIME_DELAY; 
	m_flRoundTime = gpGlobals->time + prerace.value;
}

//=========
// Think
//=========
void CGameRules:: RoundThink ( void ) 
{ 
	int i = 1;
	bool bCanStart = false;

	if (!iRoundPrimed)
	{
		RoundPrime ();
		iRoundPrimed = 1;
	}

	///// Check game rules ///// 
	switch (m_RoundState) 
	{ 
	case ROUND_NOTSTARTED: 
		// SaRcaZm - V8 - Start
		// If mp_numlaps has changed, update it now
		if (pers_newlaps != 0)
			CVAR_SET_FLOAT ("mp_numlaps", pers_newlaps);
		if (pers_newmode != 999)
			CVAR_SET_FLOAT ("mp_racemode", pers_newmode);
		if (pers_newdirection != 999)
			CVAR_SET_FLOAT ("mp_direction", pers_newdirection);

		pers_numlaps = CVAR_GET_FLOAT ("mp_numlaps");
		pers_newlaps = 0;

		pers_racemode = CVAR_GET_FLOAT ("mp_racemode");
		pers_newmode = 999;

		pers_direction = CVAR_GET_FLOAT ("mp_direction");
		pers_newdirection = 999;
		// SaRcaZm - V8 - End

		for ( i = 1; i <= gpGlobals->maxClients; i++ ) 
		{ 
			CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i ); 
			if (plr && plr->pev->fuser1)
			{
				bCanStart = true;
				break;
			}
		}

		// Don't start until there are players on the server
		if (bCanStart)
		{
			// SaRcaZm - V5 - Start
			//if (gpGlobals->time > (m_flRoundTime - 4) && !startsaid)
			if (gpGlobals->time > (m_flRoundTime - COUNTDOWN_TIME))// && !startsaid)
			{
				// Instead of this send the countdown message
				for ( i = 1; i <= gpGlobals->maxClients; i++ ) 
				{ 
					CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i ); 
					if (plr)
					{
						//EMIT_SOUND(plr->edict(), CHAN_VOICE, "voc/321.wav", 1, ATTN_NORM);

						// Remove players from observer mode
						// SaRcaZm - V7 - Start

						// SaRcaZm - V8 - Start
						// Respawn the bots
						if (plr->pev->flags & FL_FAKECLIENT)
							respawn(plr->pev, FALSE);
						// SaRcaZm - V8 - End

						CLIENT_COMMAND(plr->edict(), "-showscores\n");
						// SaRcaZm - V7 - End
						if (plr->pev->fuser1)
						{
							plr->StopObserver ();
							plr->EnableControl (true);
							plr->pev->movetype = MOVETYPE_TOSS;

							// SaRcaZm - V7 - Start
							//plr->pev->fixangle = true;
							//MESSAGE_BEGIN( MSG_ONE, gmsgFixAngle, NULL, plr->pev );
							//MESSAGE_END();

							// SaRcaZm - V8 - Start
							if (!(plr->pev->flags & FL_FAKECLIENT))
								DROP_TO_FLOOR (plr->edict ());

							// SaRcaZm - V8 - End

							plr->pev->viewmodel	= MAKE_STRING("models/interior.mdl");
							// SaRcaZm - V7 - End
						}
					}
				}
//				startsaid = true;

				// Start a countdown on the HUD
				//MESSAGE_BEGIN( MSG_ALL, gmsgSetTimer );
				//	WRITE_COORD (4);
				//	WRITE_SHORT (-1);
				//MESSAGE_END();

				MESSAGE_BEGIN (MSG_ALL, gmsgCountdown);
					WRITE_COORD (gpGlobals->time);
				MESSAGE_END ();
				// SaRcaZm - V5 - End

				// SaRcaZm - V6 - Start
				// Update the players out of time
				memset (bTimeExtensionEnd, 0, sizeof (bTimeExtensionEnd));
				// SaRcaZm - V6 - End

				// Update the race info
				UpdateRaceInfo ();

				// SaRcaZm - V8 - Start
				m_RoundState = ROUND_COUNTDOWN;
				// SaRcaZm - V8 - End
			}
		}
	// SaRcaZm - V8 - End
		else if (gpGlobals->time > (m_flRoundTime - COUNTDOWN_TIME))//4))
			m_flRoundTime = gpGlobals->time + COUNTDOWN_TIME;//4;

/*		if ( m_flRoundTime <= gpGlobals->time) 
			RoundCheckStart();
*/		break;

	case ROUND_COUNTDOWN:
		if ( m_flRoundTime <= gpGlobals->time) 
			RoundCheckStart();
		break;
	// SaRcaZm - V8 - End

	case ROUND_STARTED: 
		if ( m_flCheckRound <= gpGlobals->time ) 
			RoundCheckEnd (); 
		break;
	case ROUND_END: 
		if ( m_flRoundTime <= gpGlobals->time ) 
			RoundRestart(); 
		break;
	default: 
		//ALERT(at_console, "Priming Round Code\n");
		RoundPrime();
		break;
	}
}
