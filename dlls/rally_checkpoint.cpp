/*
********************************************************************************
*
*	Copyright (c) HL Rally Team 2001
*	
*   Author: FragMented!
*	Heavily Modified by: SaRcaZm
*  
*   Purpose: HL Rally Checkpoint System
*
********************************************************************************
*/ 

// SaRcaZm - Start
// Notes:
//		pev->iuser1	:	Indicates what spectator mode we are in
//		pev->iuser2	:	Used to store the entindex of the player we are following
//		pev->iuser3	:	Number of laps completed
//		pev->iuser4	:	Number of checkpoints on current lap
//						If finished race, final position
//		pev->fuser1	:	Whether they can start or not (1 = Can start, 0 = Can't start)
//		pev->fuser2	:	State of horn (1 = on, 0 = off)
//		pev->fuser3	:	Beginning time of the current lap
//		pev->fuser4	:	Beginning time of the current race
//		pev->vuser3	:	Stores miscellaneous information
//					:	vuser3[0]	=	Amount of money
//					:	vuser3[1]	=	Bot angles
//					:	vuser3[2]	=	Bot waypoint number
//		pev->vuser4	:	Stores the carinfo.txt values for the player
//					:	vuser4[0]	=	Acceleration
//					:	vuser4[1]	=	Handling
//					:	vuser4[2]	=	Top speed
// SaRcaZm - End

// Includes
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "saverestore.h"
#include "gamerules.h"

#include "rally_rounds.h"

// SaRcaZm - V8 - Start
extern int gmsgRallyScoreInfo;
extern float tetimes[32];
// SaRcaZm - V8 - End

void CRallyCheckPoint::ClearPlaces (void)
{
	memset (m_pPlaces, 0, sizeof (m_pPlaces));
}

// --------------
// Link to entity
// --------------
LINK_ENTITY_TO_CLASS(info_checkpoint, CRallyCheckPoint);

// ------------------------------------------------------------------------
// Function Definition
// ------------------------------------------------------------------------

// ---------------
// Spawn Function
// ---------------

void CRallyCheckPoint::Spawn(void)
{
	SetThink( NULL );
	SetUse( NULL );


	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	SET_MODEL(ENT(pev), STRING(pev->model));    // link into world


	SetBits( pev->effects, EF_NODRAW );			// make it not draw anything


	m_iTotalCheckPoints++;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ ) 
	{ 
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i ); 
		if (plr) 
		{
			// Reset number of laps completed
			plr->pev->iuser3 = 0;
			plr->pev->iuser4 = 0;
		}
	}

	// Clear the array of places
	ClearPlaces ();
}

// ----------------------
// KeyValue Function -  Gets values from entity
// ----------------------
void CRallyCheckPoint::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "number"))
	{
		m_fCurrentCheckPoint = atof( pkvd->szValue );
	}
	// SaRcaZm - V6 - Start
	else if (FStrEq(pkvd->szKeyName, "timeextension"))
	{
		m_iTimeExtension = atoi (pkvd->szValue);
	}
	// SaRcaZm - V6 - End
	// SaRcaZm - V8 - Start
	else if (FStrEq(pkvd->szKeyName, "timeextensionreverse"))
	{
		m_iTimeExtensionReverse = atoi (pkvd->szValue);
	}
	// SaRcaZm - V8 - End
}

void CRallyCheckPoint :: Touch (CBaseEntity *pBase)
{
	//if(!pBase->IsNetClient() && !(pBase->pev->flags & FL_FAKECLIENT))
	//	return;

	entvars_t *pevOther = pBase->pev;
	CBasePlayer *pPlayer = (CBasePlayer *)pBase;

	// SaRcaZm - V7 - Start
	// Checkpoint lag bug fix
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (pPlayer == m_pPlaces[pPlayer->pev->iuser3][i])
			return;		// They have touched this one already
	}
	// SaRcaZm - V7 - End

	// Lap Complete
	// SaRcaZm - V8 - Start
	// Allow for reverse (special case for ab_ races)
	//if (m_iTotalCheckPoints - pPlayer->pev->iuser4 == 0)
	//pPlayer->pev->iuser4 = m_iTotalCheckPoints - 1;
	bool ab_race = false;
	if( ((STRING(gpGlobals->mapname))[0] == 'a' || (STRING(gpGlobals->mapname))[0] == 'A')
			&& ((STRING(gpGlobals->mapname))[1] == 'b' || (STRING(gpGlobals->mapname))[1] == 'B'))
		ab_race = true;

	if ((m_fCurrentCheckPoint == pPlayer->pev->iuser4)
		&& (
			(ab_race && (direction.value < 0) && (pPlayer->pev->iuser4 == 0))
			||
			(pPlayer->pev->iuser4 == m_iTotalCheckPoints - 1)
		   )
	   )
	// SaRcaZm - V8 - End
	{
		m_iScoreIncrement = 5;

		// Increase the number of people that have touched this checkpoint on this lap
		// Loop through and find the next vacant spot
		i = 0;
		while (m_pPlaces[pPlayer->pev->iuser3][i] != NULL)
			i++;

		// Save the position for later
		m_pPlaces[pPlayer->pev->iuser3][i] = pPlayer;

		// Save it as the players' current position
		pPlayer->pev->iuser4 = i + 1;

		// SaRcaZm - V8 - Start
		// Oops, this was being done too early
		pPlayer->pev->iuser3++;		// Increment number of laps completed

		// AB_ Mode Game Code
		if (ab_race)
		{
			// Array of scores
			int scores[4] = { 8, 6, 4, 2 };

			MESSAGE_BEGIN (MSG_ALL, gmsgRaceEnd);
				WRITE_BYTE (ENTINDEX(pPlayer->edict()));
				WRITE_BYTE (pPlayer->pev->iuser4);
				WRITE_COORD (gpGlobals->time - fGlobalRaceStartTime);
				WRITE_COORD (gpGlobals->time - fGlobalRaceStartTime);
			MESSAGE_END ();

//			g_pGameRules->RoundEnd(AB_FINISH, (char *)STRING(pPlayer->pev->netname));

			if (pPlayer->pev->iuser4 == 1)
				g_pGameRules->m_flRoundTime = gpGlobals->time + 30;				// Finish in 30 seconds

			MESSAGE_BEGIN( MSG_ONE, gmsgSetTimer, NULL, pPlayer->pev );
				WRITE_COORD (-1);
			MESSAGE_END();

			// Update their score
			if (pPlayer->pev->iuser4 <= 5)	// but only if they finished in the top 4
			{
				m_iScoreIncrement += scores[pPlayer->pev->iuser4 - 1];
				pPlayer->AddMoney (scores[pPlayer->pev->iuser4 - 1] * 100);
			}

			// Even if they didn't finish in the top 4, give them some more money
			pPlayer->AddMoney (600);
			pPlayer->UpdateMoney ();	// Send the new amount to the client

			pPlayer->fABSpectateTime = gpGlobals->time + 5;
			return;
		}
		else if (pPlayer->pev->iuser3 == numlaps.value)		// Finished Race
		// SaRcaZm - V8 - End
		{
			// Array of scores
			int scores[4] = { 8, 6, 4, 2 };

			// First person over line gives everyone else 30 seconds to finish
			// SaRcaZm - V8 - Start
			if (m_pPlaces[pPlayer->pev->iuser3-1][0] == pPlayer)
			// SaRcaZm - V8 - End
				g_pGameRules->m_flRoundTime = gpGlobals->time + 30;

			// Send the race end event to the client
			MESSAGE_BEGIN (MSG_ALL, gmsgRaceEnd);
				WRITE_BYTE (ENTINDEX(pPlayer->edict()));
				WRITE_BYTE (pPlayer->pev->iuser4);
				WRITE_COORD (gpGlobals->time - pPlayer->pev->fuser3);
				// SaRcaZm - V6 - Start
				//WRITE_COORD (gpGlobals->time - pPlayer->pev->fuser4);
				WRITE_COORD (gpGlobals->time - fGlobalRaceStartTime);
				// SaRcaZm - V6 - End
			MESSAGE_END ();
			// Send them to observer mode
			pPlayer->StartDeathCam ();

			// SaRcaZm - V6 - Start
			// Update their score
			if (pPlayer->pev->iuser4 <= 5)	// but only if they finished in the top 4
			{
				m_iScoreIncrement += scores[pPlayer->pev->iuser4 - 1];
				pPlayer->AddMoney (scores[pPlayer->pev->iuser4 - 1] * 100);
			}

			// Even if they didn't finish in the top 4, give them some more money
			pPlayer->AddMoney (600);
			pPlayer->UpdateMoney ();	// Send the new amount to the client
			// SaRcaZm - V6 - End

			// SaRcaZm - V7 - Start
			// See if this player was being watched by anyone in observer mode and update accordingly
			for ( int i = 1; i <= gpGlobals->maxClients; i++ ) 
			{
				CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );
				// SaRcaZm - V7 - Start
				if (plr && plr->pev->iuser1 && plr->pev->iuser2 == ENTINDEX(pPlayer->edict()))
					plr->Observer_FindNextPlayer (false);
				// SaRcaZm - V7 - End
			}
			// SaRcaZm - V7 - End
		}
		else		// Just finished lap
		{
			// Send the lap finished event to the client
			MESSAGE_BEGIN (MSG_ALL, gmsgLapEnd);
				WRITE_BYTE (ENTINDEX (pPlayer->edict()));
				WRITE_BYTE (pPlayer->pev->iuser4);
				WRITE_COORD (gpGlobals->time - pPlayer->pev->fuser3);

				// SaRcaZm - V8 - Start
				if (racemode.value == 2)
				{
					if (direction.value < 0)
						WRITE_BYTE (m_iTimeExtensionReverse);
					else
						WRITE_BYTE (m_iTimeExtension);
				}
				else
					WRITE_BYTE (0);
				// SaRcaZm - V8 - End
			MESSAGE_END ();

			// Reset the lap time
			pPlayer->pev->fuser3 = gpGlobals->time;

			// SaRcaZm - V8 - Start
			// Reset the checkpoint counter
			pPlayer->pev->iuser4 = (direction.value < 0) ? m_iTotalCheckPoints - 2 : 0;

			// Add the time extension time
			if (racemode.value == 2)
			{
				if (direction.value < 0)
					tetimes[ENTINDEX(pPlayer->edict())] += m_iTimeExtensionReverse;
				else
					tetimes[ENTINDEX(pPlayer->edict())] += m_iTimeExtension;
			}

			// If they were first, give them some bonus money
			if (pPlayer->pev->iuser4 == 1)
				pPlayer->AddMoney (100);
			// SaRcaZm - V8 - End
		}
	}
	// CheckPoint
	else if ((pPlayer->pev->iuser3 != numlaps.value) && (m_fCurrentCheckPoint == pPlayer->pev->iuser4))
	{
		// Loop through and find the next vacant spot
		i = 0;
		while (m_pPlaces[pPlayer->pev->iuser3][i] != NULL)
			i++;

		// Save the position for later
		m_pPlaces[pPlayer->pev->iuser3][i] = pPlayer;

		// Send the checkpoint event to the client
		MESSAGE_BEGIN (MSG_ALL, gmsgCheckpoint);
			WRITE_BYTE (ENTINDEX(pPlayer->edict()));
			WRITE_BYTE (i + 1);

			// SaRcaZm - V8 - Start
			if (racemode.value == 2)
			{
				if (direction.value < 0)
					WRITE_BYTE (m_iTimeExtensionReverse);
				else
					WRITE_BYTE (m_iTimeExtension);
			}
			else
				WRITE_BYTE (0);
			// SaRcaZm - V8 - End
		MESSAGE_END ();

		m_iScoreIncrement = 1;
		// SaRcaZm - V6 - Start
		// Allow reverse
		//pPlayer->pev->iuser4 = m_fCurrentCheckPoint + 1;
		if ((direction.value < 0) && (pPlayer->pev->iuser4 < 1))
			pPlayer->pev->iuser4 = m_iTotalCheckPoints - 1;
		else
			pPlayer->pev->iuser4 = (direction.value < 0) ? m_fCurrentCheckPoint - 1 : m_fCurrentCheckPoint + 1;

		// SaRcaZm - V8 - Start
		// Add the time extension time
		if (racemode.value == 2)
		{
			if (direction.value < 0)
				tetimes[ENTINDEX(pPlayer->edict())] += m_iTimeExtensionReverse;
			else
				tetimes[ENTINDEX(pPlayer->edict())] += m_iTimeExtension;
		}
		// SaRcaZm - V8 - End

		// Give them some bonus money for making it to a checkpoint
		pPlayer->AddMoney (20);
		// SaRcaZm - V6 - End
	}

	// SaRcaZm - V8 - Start
	if (m_iScoreIncrement)
	{
		pPlayer->AddPoints (m_iScoreIncrement, true);
		m_iScoreIncrement = 0;
	}
	// SaRcaZm - V8 - End
}
