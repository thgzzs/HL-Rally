// rally_spectate.cpp
// Contains all the spectator stuff
// By SaRcaZm - based on a tutorial by Robin Walker

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"

extern int gmsgCurWeapon;
extern int gmsgSetFOV;
extern int gmsgTeamInfo;
extern int gmsgSpectator;

// Player has become a spectator. Set it up.
// This was moved from player.cpp.
void CBasePlayer::StartObserver( Vector vecPosition, Vector vecViewAngle )
{
	// SaRcaZm - V7 - Start
	if (!GetClassPtr((CBasePlayer *)pev)->IsNetClient() && !(pev->flags & FL_FAKECLIENT))
		return;

	// No point in sending network traffic if they are already a spectator
	bool bNoNetwork = IsObserver ();

	// Holster weapon immediately, to allow it to cleanup
	if (m_pActiveItem)
		m_pActiveItem->Holster( );

	if ( m_pTank != NULL )
	{
		m_pTank->Use( this, this, USE_OFF, 0 );
		m_pTank = NULL;
	}

	// clear out the suit message cache so we don't keep chattering
	SetSuitUpdate(NULL, FALSE, 0);

	// reset FOV
	m_iFOV = m_iClientFOV = 0;
	pev->fov = m_iFOV;

	// Setup flags
	// SaRcaZm - V8 - Start
	m_iHideHUD = (HIDEHUD_HEALTH | HIDEHUD_WEAPONS);
	//m_iHideHUD = HIDEHUD_ALL;
	// SaRcaZm - V8 - End
	// SaRcaZm - V5 - Start
	m_afPhysicsFlags &= ~PFLAG_OBSERVER;
	m_afPhysicsFlags |= PFLAG_OBSERVER;
	// SaRcaZm - V5 - End
	pev->effects = EF_NODRAW;
	pev->view_ofs = g_vecZero;
	pev->angles = pev->v_angle = vecViewAngle;
	pev->fixangle = TRUE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NONE;
	ClearBits( m_afPhysicsFlags, PFLAG_DUCKING );
	ClearBits( pev->flags, FL_DUCKING );
	pev->deadflag = DEAD_RESPAWNABLE;
	pev->health = 1;

	// Tried to put this, made little difference..
	//pev->flags |= FL_SPECTATOR;
	
	// Clear out the status bar
	m_fInitHUD = TRUE;

	// Update Team Status
	pev->team = 0;

	// Remove all the player's stuff
	RemoveAllItems( FALSE );

	// Move them to the new position
	UTIL_SetOrigin( pev, vecPosition );

	// Find a player to watch
	m_flNextObserverInput = 0;
	// SaRcaZm - V7 - Start
	Observer_SetMode( OBS_CHASE_FREE );
	// SaRcaZm - V7 - End

	if (bNoNetwork)
		return;
	// SaRcaZm - V7 - End

	// clear any clientside entities attached to this player
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_KILLPLAYERATTACHMENTS );
		WRITE_BYTE( (BYTE)entindex() );
	MESSAGE_END();

	// Tell Ammo Hud that the player is dead
	MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
		WRITE_BYTE(0);
		WRITE_BYTE(0XFF);
		WRITE_BYTE(0xFF);
	MESSAGE_END();

	// Tell all clients this player is now a spectator
	MESSAGE_BEGIN( MSG_ALL, gmsgSpectator );  
		WRITE_BYTE( ENTINDEX( edict() ) );
		WRITE_BYTE( 1 );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
		WRITE_BYTE(0);
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
		WRITE_BYTE( ENTINDEX(edict()) );
		WRITE_STRING( "" );
	MESSAGE_END();

	Observer_FindNextPlayer(false);
}

// Leave observer mode
void CBasePlayer::StopObserver( void )
{
	// Turn off spectator
	if ( IsObserver() )
	{
		// Tell all clients this player is not a spectator anymore
		MESSAGE_BEGIN( MSG_ALL, gmsgSpectator );  
			WRITE_BYTE( ENTINDEX( edict() ) );
			WRITE_BYTE( 0 );
		MESSAGE_END();

		// SaRcaZm - V7 - Start
		MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
			WRITE_BYTE( ENTINDEX(edict()) );
			WRITE_STRING( m_szTeamName );
		MESSAGE_END();
		// SaRcaZm - V7 - End

		pev->iuser1 = pev->iuser2 = 0; 
		m_hObserverTarget = NULL;
	}

	// SaRcaZm - V4 - Start
	// Draw the hud and player model again
	m_iHideHUD = 0;
	pev->effects = 0;
	ClearBits (m_afPhysicsFlags, PFLAG_OBSERVER);
	pev->maxspeed = CVAR_GET_FLOAT("sv_maxspeed");
	// SaRcaZm - V7 - Start
	// Do this above
	// Join their team again
//	MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
//		WRITE_BYTE( ENTINDEX(edict()) );
//		WRITE_STRING( m_szTeamName );
//	MESSAGE_END();
	// SaRcaZm - V7 - End
	// SaRcaZm - V4 - End

	// SaRcaZm - V7 - Start
	pev->fuser1 = 1;	// They can start the next race
	// SaRcaZm - V7 - End
}

// Find the next client in the game for this player to spectate
void CBasePlayer::Observer_FindNextPlayer( bool bReverse )
{
	// MOD AUTHORS: Modify the logic of this function if you want to restrict the observer to watching
	//				only a subset of the players. e.g. Make it check the target's team.

	int		iStart;
	if ( m_hObserverTarget )
		iStart = ENTINDEX( m_hObserverTarget->edict() );
	else
		iStart = ENTINDEX( edict() );
	int	    iCurrent = iStart;
	m_hObserverTarget = NULL;
	int iDir = bReverse ? -1 : 1; 

	do
	{
		iCurrent += iDir;

		// Loop through the clients
		if (iCurrent > gpGlobals->maxClients)
			iCurrent = 1;
		if (iCurrent < 1)
			iCurrent = gpGlobals->maxClients;

		CBaseEntity *pEnt = UTIL_PlayerByIndex( iCurrent );
		if ( !pEnt )
			continue;
		if ( pEnt == this )
			continue;
		// Don't spec observers or invisible players
		if ( ((CBasePlayer*)pEnt)->IsObserver() || (pEnt->pev->effects & EF_NODRAW) )
			continue;

		// MOD AUTHORS: Add checks on target here.

		m_hObserverTarget = pEnt;
		break;

	} while ( iCurrent != iStart );

	// Did we find a target?
	if ( m_hObserverTarget )
	{
		// Store the target in pev so the physics DLL can get to it
		pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );
		// Move to the target
		UTIL_SetOrigin( pev, m_hObserverTarget->pev->origin );

	// SaRcaZm - V7 - Start		
/*		ALERT( at_console, "Now Tracking %s\n", STRING( m_hObserverTarget->pev->netname ) );
	}
	else
	{
		ALERT( at_console, "No observer targets.\n" );
*/	}
	// SaRcaZm - V7 - End
}

// Handle buttons in observer mode
void CBasePlayer::Observer_HandleButtons()
{
	// Slow down mouse clicks
	if ( m_flNextObserverInput > gpGlobals->time )
		return;

	// SaRcaZm - V7 - Start
	/* No choice for HLRally
	// Jump changes from modes: Chase to Roaming
	if ( m_afButtonPressed & IN_JUMP )
	{
		if ( pev->iuser1 == OBS_ROAMING )
			Observer_SetMode( OBS_CHASE_LOCKED );
		else if ( pev->iuser1 == OBS_CHASE_LOCKED )
			Observer_SetMode( OBS_CHASE_FREE );
		else
			Observer_SetMode( OBS_ROAMING );

		m_flNextObserverInput = gpGlobals->time + 0.2;
	}
	*/
	// SaRcaZm - V7 - End

	// Attack moves to the next player
	if ( m_afButtonPressed & IN_ATTACK && pev->iuser1 != OBS_ROAMING )
	{
		Observer_FindNextPlayer( false );

		m_flNextObserverInput = gpGlobals->time + 0.2;
	}

	// Attack2 moves to the prev player
	if ( m_afButtonPressed & IN_ATTACK2 && pev->iuser1 != OBS_ROAMING )
	{
		Observer_FindNextPlayer( true );

		m_flNextObserverInput = gpGlobals->time + 0.2;
	}
}

// Attempt to change the observer mode
void CBasePlayer::Observer_SetMode( int iMode )
{
	// Just abort if we're changing to the mode we're already in
	if ( iMode == pev->iuser1 )
		return;

	// Changing to Roaming?
	if ( iMode == OBS_ROAMING )
	{
		// MOD AUTHORS: If you don't want to allow roaming observers at all in your mod, just abort here.
		pev->iuser1 = OBS_ROAMING;
		pev->iuser2 = 0;

		ClientPrint( pev, HUD_PRINTCENTER, "#Spec_Mode3" );
		pev->maxspeed = 320;
		return;
	}

	// Changing to Chase Lock?
	if ( iMode == OBS_CHASE_LOCKED )
	{
		// If changing from Roaming, or starting observing, make sure there is a target
		if ( m_hObserverTarget == NULL )
			Observer_FindNextPlayer( false );

		if (m_hObserverTarget)
		{
			pev->iuser1 = OBS_CHASE_LOCKED;
			pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );
			ClientPrint( pev, HUD_PRINTCENTER, "#Spec_Mode1" );
			pev->maxspeed = 0;
		}
		else
		{
			ClientPrint( pev, HUD_PRINTCENTER, "#Spec_NoTarget"  );
			Observer_SetMode(OBS_ROAMING);
		}

		return;
	}

	// Changing to Chase Freelook?
	if ( iMode == OBS_CHASE_FREE )
	{
		// If changing from Roaming, or starting observing, make sure there is a target
		if ( m_hObserverTarget == NULL )
			Observer_FindNextPlayer( false );

		if (m_hObserverTarget)
		{
			pev->iuser1 = OBS_CHASE_FREE;
			pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );
			ClientPrint( pev, HUD_PRINTCENTER, "#Spec_Mode2" );
			pev->maxspeed = 0;
		}
		else
		{
			ClientPrint( pev, HUD_PRINTCENTER, "#Spec_NoTarget"  );
			Observer_SetMode(OBS_ROAMING);
		}
		
		return;
	}
}
