#include "extdll.h"
#include "util.h"
#include "client.h"
#include "cbase.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

#include "rally_bot.h"
#include "rally_rounds.h"

#include "carinfo.h"

// From pm_math.c	(Slightly modified)
#define	PITCH	0	// up / down
#define	YAW		1	// left / right
#define	ROLL	2	// fall over

vec3_t ForwardVector (const vec3_t angles, vec3_t forward)
{
	float		angle;
	float		sp, sy, cp, cy;
	
	angle = angles[YAW] * (M_PI*2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = sin(angle);
	cp = cos(angle);

	forward[0] = cp*cy;
	forward[1] = cp*sy;
	forward[2] = -sp;

	return forward;
}

float AngleBetweenVectors( vec3_t v1, vec3_t v2 )
{
	float angle;
//	float l1 = v1.Length();
//	float l2 = v2.Length();

//	if ( !l1 || !l2 )
//		return 0.0f;

//	angle = acos( DotProduct( v1, v2 ) ) / (l1*l2);
	angle = acos (DotProduct (v1, v2));
	angle = ( angle  * 180.0f ) / M_PI;

	return angle;
}

float NormalizeAngle (float angle)
{
	if (angle > 360)
		angle -= 360;
	else if (angle < 0)
		angle += 360;

	return angle;
}

float SubtractWrap (float ang1, float ang2)
{
	if ((ang1 > 270) && (ang2 < 90))
		ang2 += 360;
	else if ((ang1 < 90) && (ang2 > 270))
		ang1 += 360;

	return ang1 - ang2;
}

extern DLL_GLOBAL ULONG g_ulModelIndexPlayer;

// Set in combat.cpp.  Used to pass the damage inflictor for death messages
extern entvars_t *g_pevLastInflictor;

extern int gmsgHealth;
extern int gmsgCurWeapon;
extern int gmsgSetFOV;
extern int LastWaypoint;

#include "pm_defs.h"
extern "C" playermove_t *pmove;

LINK_ENTITY_TO_CLASS( bot, CBot );

bool bShowWaypoints, bShowPaths;
int iBotWaypoint[32];

extern float frametime;

inline edict_t *CREATE_FAKE_CLIENT( const char *netname )
{
    return (*g_engfuncs.pfnCreateFakeClient)( netname );
}

inline char *GET_INFOBUFFER( edict_t *e )
{
    return (*g_engfuncs.pfnGetInfoKeyBuffer)( e );
}

inline void SET_CLIENT_KEY_VALUE( int clientIndex, char *infobuffer,
                                  char *key, char *value )
{
    (*g_engfuncs.pfnSetClientKeyValue)( clientIndex, infobuffer, key, value );
}

static float Fix( float angle )
{
	while ( angle < 0 )
		angle += 360;
	while ( angle > 360 )
		angle -= 360;

	return angle;
}


static void FixupAngles( Vector &v )
{
	v.x = Fix( v.x );
	v.y = Fix( v.y );
	v.z = Fix( v.z );
}

void BotCreate( void )
{
	edict_t *BotEnt;

	bShowWaypoints = bShowPaths = false;

	bool found = false;
	char szSect[30], szName[30];
	CCarInfo *botinfo = new CCarInfo ("bots.txt");

	botinfo->getNextManufacturer (szSect);
	while (szSect[0] && !found)
	{
		// Loop through all the models
		botinfo->getNextModel (szName);

		if (!stricmp (szSect, "General"))
		{
			while (szName[0] && !found)
			{
				if (!stricmp (szName, "Debug"))
				{
					// Read the new stats
					char szAttrib[20], szAttribVal[50];

					// Instead loop through each attribute
					botinfo->getNextAttributeString (szAttrib, szAttribVal);
					while (szAttrib[0] && szAttribVal[0])
					{
						if (!stricmp (szAttrib, "ShowWaypoints"))
							bShowWaypoints = (bool) atoi (szAttribVal);
						else if (!stricmp (szAttrib, "ShowPaths"))
							bShowPaths = (bool) atoi (szAttribVal);

						// Get the next attribute
						botinfo->getNextAttributeString (szAttrib, szAttribVal);
					}
				}

				botinfo->getNextModel (szName);
			}
		}
		// Bot profiles
		else if (!stricmp (szSect, "Profiles"))
		{
			while (szName[0] && !found)
			{
				// TODO: Keep an array of used names so we don't use the same one twice
				if (g_engfuncs.pfnRandomFloat (0, 1000) > 800)
				{
					// Read the new stats
/*					char szAttrib[20], szAttribVal[50];

					// Instead loop through each attribute
					botinfo->getNextAttributeString (szAttrib, szAttribVal);
					while (szAttrib[0] && szAttribVal[0])
					{
						// Copy the attribute value to the correct variable
						if (!stricmp (szAttrib, "?"))
							? = (float) atoi (szAttribVal)*10;
							strcpy (?, szAttribVal);

						// Get the next attribute
						botinfo->getNextAttributeString (szAttrib, szAttribVal);
					}
*/
					found = true;
					break;
				}
				if (!found)
					botinfo->getNextModel (szName);
			}
		}

		// Get the next manufacturer
		if (!found)
			botinfo->getNextManufacturer (szSect);
	}
	delete botinfo;

	if (!found)
		strcpy (szName, "Auto-Bot");

	BotEnt = CREATE_FAKE_CLIENT( szName );

	if (FNullEnt( BotEnt ))
		ALERT ( at_console, "NULL Ent in CREATE_FAKE_CLIENT!\n" );
	else
	{
		char ptr[128];  // allocate space for message from ClientConnect
		CBot *BotClass;
		char *infobuffer;
		int clientIndex;

		BotClass = GetClassPtr( (CBot *) VARS(BotEnt) );
		infobuffer = GET_INFOBUFFER( BotClass->edict( ) );
		clientIndex = BotClass->entindex( );

		ClientConnect( BotClass->edict( ), szName, "127.0.0.1", ptr );
		DispatchSpawn( BotClass->edict( ) );

		BotClass->m_fSkill = bot_skill.value;
	}
}

extern cvar_t joinmidrace;
extern int gmsgTeamInfo;
extern "C" PM_SetStats (int index, int accel, int handling, int topspeed);

int FindClosestWaypoint (vec3_t loc)
{
	// search the world for waypoints
	edict_t *WayPoint = FIND_ENTITY_BY_CLASSNAME( NULL, "info_waypoint");
	float min = 9999;
	int closest = -1;

	while ( !FNullEnt( WayPoint ) )
	{
		CRallyWayPoint *pEnt = (CRallyWayPoint *)CBaseEntity::Instance( WayPoint );
		float dist = (pEnt->pev->origin - loc).Length();
		if (dist < min)
		{
			min = dist;
			closest = pEnt->number;
		}
		WayPoint = FIND_ENTITY_BY_CLASSNAME( WayPoint, "info_waypoint" );
	}

	return closest;
}

void CBot::Spawn (void)
{
	pev->classname		= MAKE_STRING("player");
	pev->health			= 100;
	pev->armorvalue		= 0;
	pev->takedamage		= DAMAGE_AIM;
	pev->solid			= SOLID_SLIDEBOX; // Creme disable normal 2d box

	if ((g_pGameRules->m_RoundState != 1) || !joinmidrace.value)	// Round Not Started or they can't join
		pev->movetype	= MOVETYPE_TOSS;
	else
		pev->movetype	= MOVETYPE_WALK; // Round In Progress

	pev->max_health		= pev->health;
	pev->flags		   &= FL_PROXY;	// keep proxy flag sey by engine
	pev->flags		    = FL_CLIENT | FL_FAKECLIENT;
	pev->air_finished	= gpGlobals->time + 12;
	pev->dmg			= 2;				// initial water damage
	pev->effects		= 0;
	pev->deadflag		= DEAD_NO;
	pev->dmg_take		= 0;
	pev->dmg_save		= 0;
	pev->friction		= 4.5;   // 7.5
	pev->gravity		= 12.0;  // 5
	pev->maxspeed		= 10000;
	m_bitsHUDDamage		= -1;
	m_bitsDamageType	= 0;
	m_afPhysicsFlags	= 0;
	m_fLongJump			= FALSE;// no longjump module. 

	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "slj", "0" );
	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "hl", "1" );

	pev->fov = m_iFOV				= 0;// init field of view.
	m_iClientFOV		= -1; // make sure fov reset is sent

	m_flNextDecalTime	= 0;// let this player decal as soon as he spawns.

	m_flgeigerDelay = gpGlobals->time + 2.0;	// wait a few seconds until user-defined message registrations
												// are recieved by all clients

	m_flTimeStepSound	= 0;
	m_iStepLeft = 0;
	m_flFieldOfView		= 0.5;		// some monsters use this to determine whether or not the player is looking at them.

	m_bloodColor	= DONT_BLEED;	// FragMented... Cars dont bleed :P
	m_flNextAttack	= UTIL_WeaponTimeBase();
	StartSneaking();

	m_iFlashBattery = 99;
	m_flFlashLightTime = 1; // force first message

	// dont let uninitialized value here hurt the player
	m_flFallVelocity = 0;

	g_pGameRules->SetDefaultPlayerTeam( this );
	g_pGameRules->GetPlayerSpawnSpot( this );

	SET_MODEL(ENT(pev), "models/player.mdl");
	g_ulModelIndexPlayer = pev->modelindex;
	pev->sequence		= LookupActivity( ACT_IDLE );
	UTIL_SetSize( pev, Vector(0, 0, 0), Vector(0, 0, 0));

	pev->view_ofs = VEC_VIEW;

	Precache();
	m_HackedGunPos		= Vector( 0, 32, 0 );

	if ( m_iPlayerSound == SOUNDLIST_EMPTY )
	{
		ALERT ( at_console, "Couldn't alloc player sound slot!\n" );
	}

	m_fNoPlayerSound = FALSE;// normal sound behavior.

	m_pLastItem = NULL;
	m_fInitHUD = TRUE;
	m_iClientHideHUD = -1;  // force this to be recalculated
	m_fWeapon = FALSE;
	m_pClientActiveItem = NULL;
	m_iClientBattery = -1;

	// reset all ammo values to 0
	for ( int i = 0; i < MAX_AMMO_SLOTS; i++ )
	{
		m_rgAmmo[i] = 0;
		m_rgAmmoLast[i] = 0;  // client ammo values also have to be reset  (the death hud clear messages does on the client side)
	}

	m_lastx = m_lasty = 0;

	g_pGameRules->PlayerSpawn( this );

	SetThink( BotThink );
	pev->nextthink = gpGlobals->time + 0.1;

	// Read in the new statistics
	CCarInfo *carinfo = new CCarInfo (hlConst.szCarInfo);
	char szMake[20], szModel[20];

	bool found = false;
	carinfo->getNextManufacturer (szMake);
	while (szMake[0] && !found)
	{
		// Loop through all the models
		carinfo->getNextModel (szModel);
		while (szModel[0] && !found)
		{
			// This is now random to create variety
			//if (FStrEq (szModel, "206"))
			if (g_engfuncs.pfnRandomFloat (0, 1000) > 800)
			{
				// Read the new stats
				char szAttrib[20], szAttribVal[50];

				// Instead loop through each attribute
				carinfo->getNextAttributeString (szAttrib, szAttribVal);
				while (szAttrib[0] && szAttribVal[0])
				{
					// Copy the attribute value to the correct variable
					if (!stricmp (szAttrib, "Acceleration"))
						pev->vuser4[0] = (float) atoi (szAttribVal)*10;
					else if (!stricmp (szAttrib, "Handling"))
						pev->vuser4[1] = (float) atoi (szAttribVal);
					else if (!stricmp (szAttrib, "TopSpeed"))
						pev->vuser4[2] = (float) atoi (szAttribVal)*10;

					// Get the next attribute
					carinfo->getNextAttributeString (szAttrib, szAttribVal);
				}

				found = true;
			}
			if (!found)
				carinfo->getNextModel (szModel);
		}

		// Get the next manufacturer
		if (!found)
			carinfo->getNextManufacturer (szMake);
	}
	delete carinfo;

	if (!szMake[0] || !szModel[0])
	{
		// Trusty failsafe
		strcpy (szMake, "Peugeot");
		strcpy (szModel, "206" );
		PM_SetStats (ENTINDEX(edict()), 80, 80*5, 80*15);
	}
	else
		PM_SetStats (ENTINDEX(edict()), pev->vuser4[0], pev->vuser4[1]*5, pev->vuser4[2]*15);

	strcpy (m_szTeamName, szMake);
	g_engfuncs.pfnSetClientKeyValue( entindex(), g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "team", szMake );

	MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
		WRITE_BYTE( entindex() );
		WRITE_STRING( szMake );	// This is supposed to be a string
	MESSAGE_END(); 
//	g_pGameRules->ChangePlayerTeam( this, szMake, false, false);

	g_engfuncs.pfnSetClientKeyValue( entindex(), g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "model", szModel );

	m_bFirstWaypoint = true;
	m_bKicked = false;
	m_iCurrentPath = 0;
	fABSpectateTime = -1;
	m_fPrevDist = m_fPrevNextDist = 8196;
	m_flPreviousCommandTime = gpGlobals->time;

	iBotWaypoint[ENTINDEX(edict())] = FindClosestWaypoint (pev->origin);
	SetupWaypointData ();
	pev->angles = UTIL_VecToAngles((m_vTarg - pev->origin).Normalize());

	// Setup their time extensions
	if (CVAR_GET_FLOAT ("mp_racemode") == 2)
	{
		// Crappy extern references
		// Really should fix that one of these days
		extern float tetimes[32];
		extern int iTimePerLap;
		tetimes[entindex()] = tetimes[i] = iStartTime + (iTimePerLap * numlaps.value);
	}
}

void CBot::Killed( entvars_t *pevAttacker, int iGib )
{
   CSound *pSound;

   g_pGameRules->PlayerKilled( this, pevAttacker, g_pevLastInflictor );

   if (m_pTank != NULL)
   {
      m_pTank->Use( this, this, USE_OFF, 0 );
      m_pTank = NULL;
   }

   // this client isn't going to be thinking for a while, so reset the sound
   // until they respawn
   pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex(edict( )) );
   {
      if (pSound)
      {
         pSound->Reset( );
      }
   }

   SetAnimation( PLAYER_DIE );
   
   pev->modelindex = g_ulModelIndexPlayer;    // don't use eyes

#if !defined(DUCKFIX)
   pev->view_ofs      = Vector( 0, 0, -8 );
#endif
   pev->deadflag      = DEAD_DYING;
   pev->solid         = SOLID_NOT;
   pev->movetype      = MOVETYPE_TOSS;
   ClearBits( pev->flags, FL_ONGROUND );
   if (pev->velocity.z < 10)
      pev->velocity.z += RANDOM_FLOAT( 0, 300 );

   // clear out the suit message cache so we don't keep chattering
   SetSuitUpdate( NULL, FALSE, 0 );

   // send "health" update message to zero
   m_iClientHealth = 0;
   MESSAGE_BEGIN( MSG_ONE, gmsgHealth, NULL, pev );
      WRITE_BYTE( m_iClientHealth );
   MESSAGE_END( );

   // Tell Ammo Hud that the player is dead
   MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
      WRITE_BYTE( 0 );
      WRITE_BYTE( 0xFF );
      WRITE_BYTE( 0xFF );
   MESSAGE_END( );

   // reset FOV
   m_iFOV = m_iClientFOV = 0;

   MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
      WRITE_BYTE( 0 );
   MESSAGE_END( );

   // UNDONE: Put this in, but add FFADE_PERMANENT and make fade time 8.8 instead of 4.12
   // UTIL_ScreenFade( edict( ), Vector( 128, 0, 0 ), 6, 15, 255, FFADE_OUT | FFADE_MODULATE );

   if (( pev->health < -40 && iGib != GIB_NEVER ) || iGib == GIB_ALWAYS)
   {
      GibMonster( );   // This clears pev->model
      pev->effects |= EF_NODRAW;

// TAKE THIS OUT!!! THIS HAPPENS SOMETIMES WHEN HEALTH IS < -40, THEN THE
// THINK FUNCTION DOESN'T EVER GET SET TO PlayerDeathThink
// (should we REALLY be doing this???)
//      return;
   }

   DeathSound( );
   
   SetThink( PlayerDeathThink );
   pev->nextthink = gpGlobals->time + 0.1;
}


void CBot::PlayerDeathThink( void )
{
   float flForward;

   if (FBitSet( pev->flags, FL_ONGROUND ))
   {
      flForward = pev->velocity.Length( ) - 20;
      if (flForward <= 0)
         pev->velocity = g_vecZero;
      else    
         pev->velocity = flForward * pev->velocity.Normalize( );
   }

   if (HasWeapons( ))
   {
      // we drop the guns here because weapons that have an area effect and can kill their user
      // will sometimes crash coming back from CBasePlayer::Killed() if they kill their owner because the
      // player class sometimes is freed. It's safer to manipulate the weapons once we know
      // we aren't calling into any of their code anymore through the player pointer.

      PackDeadPlayerItems( );
   }

   if (pev->modelindex && (!m_fSequenceFinished) && (pev->deadflag == DEAD_DYING))
   {
      StudioFrameAdvance( );

      m_iRespawnFrames++;
      if (m_iRespawnFrames < 60)  // animations should be no longer than this
         return;
   }

   if (pev->deadflag == DEAD_DYING)
      pev->deadflag = DEAD_DEAD;
   
   StopAnimation( );

   pev->effects |= EF_NOINTERP;
   pev->framerate = 0.0;

   if (pev->deadflag == DEAD_DEAD)
   {
      if (g_pGameRules->FPlayerCanRespawn( this ))
      {
         m_fDeadTime = gpGlobals->time;
         pev->deadflag = DEAD_RESPAWNABLE;
      }
      
      return;
   }

   // check if time to respawn...
   if (gpGlobals->time > (m_fDeadTime + 5))
   {
      pev->button = 0;
      m_iRespawnFrames = 0;

      //ALERT( at_console, "Respawn\n" );

      respawn( pev, !(m_afPhysicsFlags & PFLAG_OBSERVER) );
      pev->nextthink = -1;
   }
}


#define SPD_MULT		6.5
#define ACCEL_LIMIT		500
#define MAX_SPEED		240 * SPD_MULT

void CBot::HornThink (void)
{
	// Horn
	TraceResult tr;
	vec3_t dest;
	dest[0] = pev->origin[0] + (cos((pev->angles[1]) * 0.0174532) * 16)*5;
	dest[1] = pev->origin[1] + (sin((pev->angles[1]) * 0.0174532) * 16)*5;
	dest[2] = pev->origin[2] + 15;

	UTIL_TraceLine ( pev->origin, dest, dont_ignore_monsters, ENT(pev), &tr );

	if(FBitSet( VARS( tr.pHit )->flags, FL_CLIENT ))
		pev->fuser2 = 1.0f;
	else
		pev->fuser2 = 0.0f;
}

extern int iLightningTexture;

void WaypointDrawBeam(Vector start, Vector end, int width, int noise, int red, int green, int blue, int brightness, int speed)
{
	MESSAGE_BEGIN(MSG_ALL, SVC_TEMPENTITY);
		WRITE_BYTE( TE_BEAMPOINTS);
		WRITE_COORD(start.x);
		WRITE_COORD(start.y);
		WRITE_COORD(start.z);
		WRITE_COORD(end.x);
		WRITE_COORD(end.y);
		WRITE_COORD(end.z);
		WRITE_SHORT( iLightningTexture );
		WRITE_BYTE( 1 ); // framestart
		WRITE_BYTE( 10 ); // framerate
		WRITE_BYTE( 10 ); // life in 0.1's
		WRITE_BYTE( width ); // width
		WRITE_BYTE( noise );  // noise

		WRITE_BYTE( red );   // r, g, b
		WRITE_BYTE( green );   // r, g, b
		WRITE_BYTE( blue );   // r, g, b

		WRITE_BYTE( brightness );   // brightness
		WRITE_BYTE( speed );    // speed
	MESSAGE_END();
}

int NextWaypoint (int currway)
{
	// Next waypoint
	if (direction.value > -1)
	{
		currway++;
		if(currway > LastWaypoint)
			currway = 0;	// Next Lap
	}
	else
	{
		currway--;
		if (currway < 0)
			currway = LastWaypoint;	// Next Lap
	}

	return currway;
}

void CBot::SetupWaypointData (void)
{
	int iIndex = ENTINDEX(edict());
	float nextspeedfactor;

	// Figure out the waypoint after this one
	int nextway = NextWaypoint (iBotWaypoint[iIndex]);;

	// Setup the values for later
	vec3_t oldtarg;
	if (m_bFirstWaypoint)
		oldtarg = pev->origin;
	else
		oldtarg = m_vTarg;

	vec3_t nexttarg = Vector (0, 0, 0);

	// search the world for waypoints
	edict_t *WayPoint = FIND_ENTITY_BY_CLASSNAME( NULL, "info_waypoint");

	while ( !FNullEnt( WayPoint ) )
	{
		CRallyWayPoint *pEnt = (CRallyWayPoint *)CBaseEntity::Instance( WayPoint );
		if ( pEnt && (pEnt->number == iBotWaypoint[iIndex]) && (pEnt->path == m_iCurrentPath))
		{
			m_vTarg = m_vTargCentre = pEnt->pev->origin;

			// Randomise the target so they don't all follow the same line
			m_iOffset = pEnt->offset;
			for (int i = 0; i < 2; i++)
				m_vTarg[i] += g_engfuncs.pfnRandomFloat (-m_iOffset, m_iOffset);

			// Figure out the new angle / distance
			vec3_t forward = Vector (0, 0, 0);
			if (pev->velocity.Length () > 10)
				forward = pev->velocity.Normalize ();
			else
				forward = ForwardVector (pev->angles.Normalize (), forward);

			float angles_diff = fabs (180 - AngleBetweenVectors (forward, (pev->origin - m_vTarg).Normalize ()));
			float dist = (m_vTarg - oldtarg).Length();

			// Factor in distance to next point if tight angle
			// N.B. HACK!!!
			if ((angles_diff > 10) && (angles_diff < 50) && !m_bFirstWaypoint)
			{
				int iTexFactor = 5;	// Default
				float rgfl1[3], rgfl2[3];
				const char *pTextureName;
				vec3_t vecSrc = m_vTarg;
				vec3_t vecEnd = vecSrc + Vector (0, 0, -8196);

				// copy trace vector into array for trace_texture
				vecSrc.CopyToArray(rgfl1);
				vecEnd.CopyToArray(rgfl2);
				// Get texture name from the world
				pTextureName = TRACE_TEXTURE( ENT(0), rgfl1, rgfl2 );

				// Scale the speed based on the texture
				if (pTextureName)
				{
					char szbuffer[64];
					strcpy(szbuffer, pTextureName);
					szbuffer[CBTEXTURENAMEMAX - 1] = 0;

					// Convert it to lower case
					char *tmp = szbuffer;
					while (*tmp)
					{
						*tmp = tolower (*tmp);
						tmp++;
					}

					// Some kind of dirt-like substance
					if (strstr(szbuffer, "roadgras") || (strstr(szbuffer, "dirt") || strstr(szbuffer, "gras") || strstr(szbuffer, "mud") || strstr(szbuffer, "soil") || strstr(szbuffer, "3dmroad")))
						iTexFactor = 3;
					// Sand
					else if(strstr(szbuffer, "sand") || strstr(szbuffer, "desert"))
						iTexFactor = 3;
					// Snow
					else if(strstr(szbuffer, "snow"))
						iTexFactor = 2;
				}

				// Now, based on the angle / distance we SHOULD be able to figure out the speed
				float temp = dist / angles_diff;
				m_fSpeed = temp * iTexFactor * SPD_MULT;

				// Scale it back based on the skill
				m_fSpeed *= m_fSkill / 100.0f;
			}
			// On a straight or our very first waypoint, go for it!!
			else
				m_fSpeed = MAX_SPEED;

			// Sanity check the m_fSpeed
			if (m_fSpeed > MAX_SPEED)
				m_fSpeed = MAX_SPEED;

			if (!pEnt->speedfactor)
				pEnt->speedfactor = 1;
			m_fSpeed *= pEnt->speedfactor;

			m_bFirstWaypoint = false;

			// Alternate paths code
			// If the current waypoint has different paths, pick a random one
			if ((direction.value > -1) && pEnt->iNumPaths)
				m_iCurrentPath = pEnt->iPaths[(int)RANDOM_FLOAT (0, 100) % pEnt->iNumPaths];
			else if (pEnt->iNumReversePaths)
				m_iCurrentPath = pEnt->iReversePaths[(int)RANDOM_FLOAT (0, 100) % pEnt->iNumReversePaths];
		}
		else if (pEnt && (pEnt->number == nextway) && (pEnt->path == m_iCurrentPath))
		{
			nexttarg = pEnt->pev->origin;

			if (!pEnt->speedfactor)
				pEnt->speedfactor = 1;
			nextspeedfactor = pEnt->speedfactor;
		}

		WayPoint = FIND_ENTITY_BY_CLASSNAME( WayPoint, "info_waypoint" );
	}

	// Calculate the speed for the waypoint after this one
	if (nexttarg != Vector (0, 0, 0))
	{
		m_vNextWay = nexttarg;

		// Angle to next waypoint
		m_fNextWayAngle = UTIL_VecToAngles((nexttarg - m_vTarg).Normalize())[1];

		// Funky speed calcs based on angles
		float angles_diff = AngleBetweenVectors ((m_vTarg - oldtarg).Normalize (), (nexttarg - m_vTarg).Normalize ());
		float dist = (nexttarg - m_vTarg).Length();

		// Now, based on the angle / distance we SHOULD be able to figure out the speed
		float temp = dist / angles_diff;

		if (angles_diff > 10)	// Factor in distance to next point
			m_fNextWaySpeed = temp * 4 * SPD_MULT;
		else
			m_fNextWaySpeed = MAX_SPEED;

		m_fNextWaySpeed *= nextspeedfactor;
	}
	else
	{
		m_fNextWayAngle = 0;
		m_fNextWaySpeed = 0;
	}
}

float TopSpeed = MAX_SPEED;
void CBot::MoveToWaypoint (void)
{
	int iIndex = ENTINDEX(edict());

	// Someone was screwing with iuser4 when it is being used elsewhere
	if(pev->vuser3[1] == 0)
		pev->vuser3[1] = UTIL_VecToAngles((m_vTarg - pev->origin).Normalize())[1];

	// Set the topspeed (Ugly HACK!!)
	if (m_fPrevSpeed > TopSpeed)
		TopSpeed = m_fPrevSpeed;

	// Draw debug lines
	if (bShowWaypoints)
	{
		vec3_t start = m_vTarg - Vector(0, 0, 34);
		vec3_t end = start + Vector(0, 0, 68);
		WaypointDrawBeam(start, end, 30, 0, 0, 0, 255, 250, 5);
	}
	if (bShowPaths)
	{
		WaypointDrawBeam(pev->origin, m_vTarg, 30, 0, 0, 0, 255, 250, 5);
	}

	// Drive to Waypoint
	pev->angles[1] = UTIL_VecToAngles((m_vTarg - pev->origin).Normalize())[1];

	// Slow down their top range acceleration
	float invspeed = -m_fPrevSpeed + TopSpeed + 100;
	float accel_limit = (invspeed / TopSpeed) * 25;

	float currspeed = m_fSpeed;
	if (currspeed > m_fPrevSpeed + accel_limit)
		currspeed = m_fPrevSpeed + accel_limit;

	// Start to think about our next waypoint based on our current speed
	float dist = (m_vTargCentre - pev->origin).Length ();
	float maxdist = (m_fPrevSpeed / TopSpeed) * 1000;

	if ((dist < maxdist) && m_fNextWayAngle && m_fNextWaySpeed)
	{
		// Scale down our speed based on the distance to the next waypoint
		if (currspeed > m_fNextWaySpeed)
			currspeed = -((currspeed - m_fNextWaySpeed) * 100);		// Hit the brakes!
	}

	// Stop them from turning too far at once
	float fMaxTurn = (invspeed / TopSpeed) * 35;
	if (SubtractWrap (pev->angles[1], pev->vuser3[1]) < -fMaxTurn)
		pev->angles[1] = pev->vuser3[1] - fMaxTurn;
	else if (SubtractWrap (pev->angles[1], pev->vuser3[1]) > fMaxTurn)
		pev->angles[1] = pev->vuser3[1] + fMaxTurn;
	pev->angles[1] = NormalizeAngle (pev->angles[1]);

	// Save for later
	pev->vuser3[1] = pev->angles[1];

	// Let's move it
	m_fNewSpeed = currspeed * 10;

	m_fPrevSpeed = pev->velocity.Length ();	// Save it for later

	// Check to see if we should search for a new waypoint
	// If we want to turn around too far, proceed to the next waypoint
	if (m_fPrevSpeed > 10)
	{
		// Do some comparisons of the angle to the waypoint with our current velocity
		float fVel_CurrWay, fVel_NextWay;
		fVel_CurrWay = AngleBetweenVectors (pev->velocity.Normalize (), (m_vTarg - pev->origin).Normalize ());
		fVel_NextWay = AngleBetweenVectors (pev->velocity.Normalize (), (m_vNextWay - pev->origin).Normalize ());

		// Choose the one with the least turn
		if (fVel_NextWay < fVel_CurrWay)
			m_iWrongWay++;
		else
			m_iWrongWay = 0;

		// Assume this isn't a fluke occurence
		if ((m_iWrongWay > 20) && (gpGlobals->time - m_fLastSkip > 3))
		{
			// See if we can actually make it to the waypoint we want to skip to
			TraceResult tr;
			UTIL_TraceLine ( pev->origin, m_vNextWay, ignore_monsters, ENT(pev), &tr );

			if (tr.flFraction == 1.0)
			{
				iBotWaypoint[iIndex] = NextWaypoint (iBotWaypoint[iIndex]);
				SetupWaypointData ();
				m_fLastSkip = gpGlobals->time;
			}
			else
				m_iWrongWay = 0;
		}
	}

	// If we haven't gotten any closer this frame, search for a new waypoint
	float nextdist = (m_vNextWay - pev->origin).Length ();
	if (dist > m_fPrevDist)
	{
		// If we're getting closer to the next one, keep on going
		if (nextdist < m_fPrevNextDist)
			iBotWaypoint[iIndex] = NextWaypoint (iBotWaypoint[iIndex]);
		else
			iBotWaypoint[iIndex] = FindClosestWaypoint (pev->origin);

		SetupWaypointData ();
		m_fPrevDist = m_fPrevNextDist = 8196;
	}
	else
	{
		m_fPrevDist = dist;
		m_fPrevNextDist = nextdist;
	}
}

void CBot::BotThinkMain( void )
{
	pev->fuser1 = 1;	// Bots are always ready to play
	if (IsAlive( ))
	{
		// Should be spectating now
		if (fABSpectateTime > 0)
		{
//			if (fABSpectateTime < gpGlobals->time)
				pev->effects |= EF_NODRAW;

			m_fNewSpeed = 0;
//			if (pev->velocity.Length() > 10)
//				m_fNewSpeed = -10000;
		}
		// If there are no waypoints on the map then it will never get into here
		else if (((m_vTargCentre - pev->origin).Length() > m_iOffset) && !m_bFirstWaypoint)
		{
			MoveToWaypoint ();
			// Other various thought processes
//			HornThink ();
		}
		// We need to find the next waypoint
		else
		{
			int iIndex = ENTINDEX(edict());
			if (!m_bFirstWaypoint)
				iBotWaypoint[iIndex] = NextWaypoint (iBotWaypoint[iIndex]);
			else
			{
				iBotWaypoint[iIndex] = FindClosestWaypoint (pev->origin);
				/*if (direction.value > -1)
					iBotWaypoint[iIndex] = 0;
				else
					iBotWaypoint[iIndex] = LastWaypoint;*/
			}

			SetupWaypointData ();
		}
	}
	// Dead bot
	else
	{
		// Don't take up any space in world (set size to 0)
		UTIL_SetSize (pev, Vector(0, 0, 0), Vector(0, 0, 0));
	}
}

byte CBot::ThrottledMsec (void) //Jehannum (Hacked by Periannath34)
{
	float delta;
	byte iNewMsec;

	delta = (gpGlobals->time - m_flPreviousCommandTime) * 1000;

	iNewMsec = (int)delta;
	if (iNewMsec > 255)
		iNewMsec = 255;

	return iNewMsec;
}

void CBot::BotThink (void)
{
	if (m_bKicked)
		return;

	BotThinkMain ();
	
	// Setup the buttons (dunno if this does anything or not)
	if (m_fNewSpeed < 0)
		pev->button = IN_BACK;
	else
		pev->button = IN_FORWARD;

	// Three different ways to stop framerate dependence
	// 1st
	//m_flPreviousCommandTime = gpGlobals->time;
	//g_engfuncs.pfnRunPlayerMove (edict(), pev->angles, m_fNewSpeed, 0, 0, pev->button, 0, ThrottledMsec ());

	// 2nd
	//g_engfuncs.pfnRunPlayerMove (edict(), pev->angles, m_fNewSpeed, 0, 0, pev->button, 0, gpGlobals->frametime * 1000);

	// 3rd
	// Non framerate dependent hack
	float msec = frametime * 350;
	if (msec > 19)
		msec = 19;
	g_engfuncs.pfnRunPlayerMove (edict(), pev->angles, m_fNewSpeed, 0, 0, pev->button, 0, msec);
}
