/*
********************************************************************************
*
*	Copyright (c) HL Rally Team 2001
*	
*   Author: FragMented!
*  
*   Purpose: HL Rally Hot Potato Weapon Code
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

float NextThink;

// SaRcaZm - V7 - Start
extern int gmsgHorn;
// SaRcaZm - V7 - End

//=================
// Check Round End 
//=================
void CGameRules::HornThink ( void )
{
	// SaRcaZm - V4 - Start
	// SaRcaZm - V4 - End

	if( gpGlobals->time < NextThink)
		return;

	// Think
	for ( int i = 1; i <= gpGlobals->maxClients; i++ ) 
	{ 
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i ); 

		if (plr) 
		{
			// SaRcaZm - V7 - Start
			if(plr->pev->fuser2 && plr->fHornTime < gpGlobals->time) // Has Horn on
			{
				//EMIT_SOUND(plr->edict(), CHAN_VOICE, "horn.wav", 1, ATTN_NORM);
				// Cool 3D horn with doppler effect now
				MESSAGE_BEGIN( MSG_ALL, gmsgHorn );
					WRITE_BYTE(ENTINDEX(plr->edict()));
				MESSAGE_END();

				plr->fHornTime = gpGlobals->time + 1;	// Stop abuse
			}
			// SaRcaZm - V7 - End
		}
	}

	NextThink = gpGlobals->time + 0.09;

}