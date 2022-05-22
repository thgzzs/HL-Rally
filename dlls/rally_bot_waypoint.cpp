/*
********************************************************************************
*
*	Copyright (c) HL Rally Team 2001
*	
*   Author: FragMented!
*  
*   Purpose: HL Rally Checkpoint System
*
********************************************************************************
*/ 

// Includes
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "saverestore.h"
#include "gamerules.h"

#include "rally_rounds.h"

int LastWaypoint;

// --------------
// Link to entity
// --------------
LINK_ENTITY_TO_CLASS(info_waypoint, CRallyWayPoint);

// ---------------
// Spawn Function
// ---------------

void CRallyWayPoint::Spawn(void)
{
	SetThink( NULL );
	SetUse( NULL );

	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	SET_MODEL(ENT(pev), STRING(pev->model));    // link into world

	SetBits( pev->effects, EF_NODRAW );			// make it not draw anything

	// SaRcaZm - V8 - Start
	// Drop the waypoint to the ground
	TraceResult tr;
	vec3_t start = pev->origin;
	vec3_t dest = start - Vector (0, 0, 64);

	UTIL_TraceLine ( start, dest, dont_ignore_monsters, ENT(pev), &tr );
	pev->origin = tr.vecEndPos;

	//iNumPaths = iNumReversePaths = 0;
	//speedfactor = 100;	// Default
	// SaRcaZm - V8 - End
}

// ----------------------
// KeyValue Function -  Gets values from entity
// ----------------------
// SaRcaZm - V7 - Start
void CRallyWayPoint::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "number"))
	{
		number = atoi( pkvd->szValue );

		if(number > LastWaypoint)
			LastWaypoint = number;

		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "randomofs"))
	{
		offset = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "path"))
	{
		path = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	// SaRcaZm - V8 - Start
	else if (FStrEq(pkvd->szKeyName, "altpaths"))
	{
		// More damned string handling.
		// How come I always end up doing this?
		char *szString = pkvd->szValue;

		int j = 0;
		int iLen = strlen (szString);
		iNumPaths = 0;

		if (!iLen)
			return;		// No alternate paths available

		while (true)
		{
			iPaths[iNumPaths] = atoi (szString);
			iNumPaths++;
			while ((j < iLen) && (*szString != ','))
			{
				szString++;
				j++;
			}

			if (j + 1 < iLen)
				szString++;
			else
				break;
		}
	}
	else if (FStrEq(pkvd->szKeyName, "altpaths"))
	{
		char *szString = pkvd->szValue;

		int j = 0;
		int iLen = strlen (szString);
		iNumReversePaths = 0;

		if (!iLen)
			return;		// No alternate paths available

		while (true)
		{
			iReversePaths[iNumReversePaths] = atoi (szString);
			iNumReversePaths++;
			while ((j < iLen) && (*szString != ','))
			{
				szString++;
				j++;
			}

			if (j + 1 < iLen)
				szString++;
			else
				break;
		}
	}
	else if (FStrEq(pkvd->szKeyName, "speedfactor"))
	{
		speedfactor = atoi( pkvd->szValue ) / 100;
	}
	// SaRcaZm - V8 - End
	else
		CBaseEntity::KeyValue( pkvd );
}
// SaRcaZm - V7 - End
