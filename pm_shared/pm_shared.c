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

#include <assert.h>
#include "mathlib.h"
#include "const.h"
#include "usercmd.h"
#include "pm_defs.h"
#include "pm_shared.h"
#include "pm_movevars.h"
#include "pm_debug.h"
#include "rally.h"

#include "com_model.h"

#include <stdio.h>  // NULL
#include <math.h>   // sqrt
#include <string.h> // strcpy
#include <stdlib.h> // atoi
#include <ctype.h>  // isspace

#define DEG 0.0174532

#ifdef CLIENT_DLL
	// Spectator Mode
	#include "..\common\hltv.h"
	float	vecNewViewAngles[3];
	float	vecNewViewOrigin[3];
	int		iHasNewViewAngles;
	int		iHasNewViewOrigin;
	int		iIsSpectator;
	unsigned int	uiDirectorFlags;

	int		iJumpSpectator;
	float	vJumpOrigin[3];
	float	vJumpAngles[3];
#endif

static int pm_shared_initialized = 0;
#pragma warning( disable : 4305 )

//typedef enum {mod_brush, mod_sprite, mod_alias, mod_studio} modtype_t;

playermove_t *pmove = NULL;

rally_t g_rv;

/*typedef struct
{
	int			planenum;
	short		children[2];	// negative numbers are contents
} dclipnode_t;*/

/*typedef struct mplane_s
{
	vec3_t	normal;			// surface normal
	float	dist;			// closest appoach to origin
	byte	type;			// for texture axis selection and fast side tests
	byte	signbits;		// signx + signy<<1 + signz<<1
	byte	pad[2];
} mplane_t;

typedef struct hull_s
{
	dclipnode_t	*clipnodes;
	mplane_t	*planes;
	int			firstclipnode;
	int			lastclipnode;
	vec3_t		clip_mins;
	vec3_t		clip_maxs;
} hull_t;
*/

#define PM_DEAD_VIEWHEIGHT	-8
#define MAX_CLIMB_SPEED	200
#define STUCK_MOVEUP 1
#define STUCK_MOVEDOWN -1
#define VEC_HULL_MIN		-5  // -32
#define VEC_HULL_MAX		5   //  32
#define VEC_VIEW			10
#define	STOP_EPSILON	0.1


#define CTEXTURESMAX		512			// max number of textures loaded
#define CBTEXTURENAMEMAX	13			// only load first n chars of name

#define CHAR_TEX_CONCRETE	'C'			// texture types
#define CHAR_TEX_METAL		'M'
#define CHAR_TEX_DIRT		'D'
#define CHAR_TEX_VENT		'V'
#define CHAR_TEX_GRATE		'G'
#define CHAR_TEX_TILE		'T'
#define CHAR_TEX_SLOSH		'S'
#define CHAR_TEX_WOOD		'W'
#define CHAR_TEX_COMPUTER	'P'
#define CHAR_TEX_GLASS		'Y'
#define CHAR_TEX_FLESH		'F'

#define STEP_CONCRETE	0		// default step sound
#define STEP_METAL		1		// metal floor
#define STEP_DIRT		2		// dirt, sand, rock
#define STEP_VENT		3		// ventillation duct
#define STEP_GRATE		4		// metal grating
#define STEP_TILE		5		// floor tiles
#define STEP_SLOSH		6		// shallow liquid puddle
#define STEP_WADE		7		// wading in liquid
#define STEP_LADDER		8		// climbing ladder

#define PLAYER_FATAL_FALL_SPEED		1024// approx 60 feet
#define PLAYER_MAX_SAFE_FALL_SPEED	580// approx 20 feet
#define DAMAGE_FOR_FALL_SPEED		(float) 100 / ( PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED )// damage per unit per second.
#define PLAYER_MIN_BOUNCE_SPEED		200
#define PLAYER_FALL_PUNCH_THRESHHOLD (float)350 // won't punch player's screen/make scrape noise unless player falling at least this fast.

// double to float warning
#pragma warning(disable : 4244)

#define max(a, b)  (((a) > (b)) ? (a) : (b))
#define min(a, b)  (((a) < (b)) ? (a) : (b))

#define	PITCH	0
#define	YAW		1
#define	ROLL	2 

#define MAX_CLIENTS 32

#define	CONTENTS_CURRENT_0		-9
#define	CONTENTS_CURRENT_90		-10
#define	CONTENTS_CURRENT_180	-11
#define	CONTENTS_CURRENT_270	-12
#define	CONTENTS_CURRENT_UP		-13
#define	CONTENTS_CURRENT_DOWN	-14

#define CONTENTS_TRANSLUCENT	-15

static vec3_t rgv3tStuckTable[54];
static int rgStuckLast[MAX_CLIENTS][2];

// Texture names
static int gcTextures = 0;
static char grgszTextureName[CTEXTURESMAX][CBTEXTURENAMEMAX];	
static char grgchTextureType[CTEXTURESMAX];

int g_onladder = 0;

float g_global_test = 0;

void Global_DLLEXPORT_Info ( float time, const char *func_name) {

//	pmove->Con_DPrintf("%4.0f| time: %6.4f  %-30s\n", g_global_test, time, func_name );
	g_global_test += 1;

}


vec3_t pAngles;
int DebugDraw;

void PM_SwapTextures( int i, int j )
{
	char chTemp;
	char szTemp[ CBTEXTURENAMEMAX ];
	
	strcpy( szTemp, grgszTextureName[ i ] );
	chTemp = grgchTextureType[ i ];
	
	strcpy( grgszTextureName[ i ], grgszTextureName[ j ] );
	grgchTextureType[ i ] = grgchTextureType[ j ];
	
	strcpy( grgszTextureName[ j ], szTemp );
	grgchTextureType[ j ] = chTemp;
}

void PM_SortTextures( void )
{
	// Bubble sort, yuck, but this only occurs at startup and it's only 512 elements...
	int i, j;

	for ( i = 0 ; i < gcTextures; i++ )
	{
		for ( j = i + 1; j < gcTextures; j++ )
		{
			if ( stricmp( grgszTextureName[ i ], grgszTextureName[ j ] ) > 0 )
			{
				PM_SwapTextures( i, j );
			}
		}
	}
}

void PM_InitTextureTypes()
{
	char buffer[512];
	int i, j;
	byte *pMemFile;
	int fileSize, filePos;
	static qboolean bTextureTypeInit = false;

	if ( bTextureTypeInit )
		return;

	memset(&(grgszTextureName[0][0]), 0, CTEXTURESMAX * CBTEXTURENAMEMAX);
	memset(grgchTextureType, 0, CTEXTURESMAX);

	gcTextures = 0;
	memset(buffer, 0, 512);

	fileSize = pmove->COM_FileSize( "sound/materials.txt" );
	pMemFile = pmove->COM_LoadFile( "sound/materials.txt", 5, NULL );
	if ( !pMemFile )
		return;

	filePos = 0;
	// for each line in the file...
	while ( pmove->memfgets( pMemFile, fileSize, &filePos, buffer, 511 ) != NULL && (gcTextures < CTEXTURESMAX) )
	{
		// skip whitespace
		i = 0;
		while(buffer[i] && isspace(buffer[i]))
			i++;
		
		if (!buffer[i])
			continue;

		// skip comment lines
		if (buffer[i] == '/' || !isalpha(buffer[i]))
			continue;

		// get texture type
		grgchTextureType[gcTextures] = toupper(buffer[i++]);

		// skip whitespace
		while(buffer[i] && isspace(buffer[i]))
			i++;
		
		if (!buffer[i])
			continue;

		// get sentence name
		j = i;
		while (buffer[j] && !isspace(buffer[j]))
			j++;

		if (!buffer[j])
			continue;

		// null-terminate name and save in sentences array
		j = min (j, CBTEXTURENAMEMAX-1+i);
		buffer[j] = 0;
		strcpy(&(grgszTextureName[gcTextures++][0]), &(buffer[i]));
	}

	// Must use engine to free since we are in a .dll
	pmove->COM_FreeFile ( pMemFile );

	PM_SortTextures();

	bTextureTypeInit = true;
}

char PM_FindTextureType( char *name )
{
	int left, right, pivot;
	int val;

	assert( pm_shared_initialized );

	left = 0;
	right = gcTextures - 1;

	while ( left <= right )
	{
		pivot = ( left + right ) / 2;

		val = strnicmp( name, grgszTextureName[ pivot ], CBTEXTURENAMEMAX-1 );
		if ( val == 0 )
		{
			return grgchTextureType[ pivot ];
		}
		else if ( val > 0 )
		{
			left = pivot + 1;
		}
		else if ( val < 0 )
		{
			right = pivot - 1;
		}
	}

	return CHAR_TEX_CONCRETE;
}


int PM_MapTextureTypeStepType(char chTextureType)
{
	switch (chTextureType)
	{
		default:
		case CHAR_TEX_CONCRETE:	return STEP_CONCRETE;	
		case CHAR_TEX_METAL: return STEP_METAL;	
		case CHAR_TEX_DIRT: return STEP_DIRT;	
		case CHAR_TEX_VENT: return STEP_VENT;	
		case CHAR_TEX_GRATE: return STEP_GRATE;	
		case CHAR_TEX_TILE: return STEP_TILE;
		case CHAR_TEX_SLOSH: return STEP_SLOSH;
	}
}

/*
====================
PM_CatagorizeTextureType

Determine texture info for the texture we are standing on.
====================
*/
void PM_CatagorizeTextureType( void )
{
	vec3_t start, end;
	const char *pTextureName;

	VectorCopy( pmove->origin, start );
	VectorCopy( pmove->origin, end );

	// Straight down
	end[2] -= 64;

	// Fill in default values, just in case.
	pmove->sztexturename[0] = '\0';
	pmove->chtexturetype = CHAR_TEX_CONCRETE;

	pTextureName = pmove->PM_TraceTexture( pmove->onground, start, end );
	if ( !pTextureName )
		return;

	// strip leading '-0' or '+0~' or '{' or '!'
	if (*pTextureName == '-' || *pTextureName == '+')
		pTextureName += 2;

	if (*pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ')
		pTextureName++;
	// '}}'
	
	strcpy( pmove->sztexturename, pTextureName);
	pmove->sztexturename[ CBTEXTURENAMEMAX - 1 ] = 0;
		
	// get texture type
	pmove->chtexturetype = PM_FindTextureType( pmove->sztexturename );	
}


/*
================
PM_AddToTouched

Add's the trace result to touch list, if contact is not already in list.
================
*/
qboolean PM_AddToTouched(pmtrace_t tr, vec3_t impactvelocity)
{
	int i;

	for (i = 0; i < pmove->numtouch; i++)
	{
		if (pmove->touchindex[i].ent == tr.ent)
			break;
	}
	if (i != pmove->numtouch)  // Already in list.
		return false;

	VectorCopy( impactvelocity, tr.deltavelocity );

	if (pmove->numtouch >= MAX_PHYSENTS)
		pmove->Con_DPrintf("Too many entities were touched!\n");

	pmove->touchindex[pmove->numtouch++] = tr;
	return true;
}

/*
================
PM_CheckVelocity

See if the player has a bogus velocity value.
================
*/
void PM_CheckVelocity ()
{
	int		i;

	// bound velocity
	for (i=0 ; i<3 ; i++)
	{
		// See if it's bogus.
		if (IS_NAN(pmove->velocity[i]))
		{
			pmove->Con_Printf ("PM  Got a NaN velocity %i\n", i);
			pmove->velocity[i] = 0;
		}
		if (IS_NAN(pmove->origin[i]))
		{
			pmove->Con_Printf ("PM  Got a NaN origin on %i\n", i);
			pmove->origin[i] = 0;
		}

		// Bound it.
		if (pmove->velocity[i] > pmove->movevars->maxvelocity) 
		{
			//pmove->Con_DPrintf ("PM  Got a velocity too high on %i\n", i);
			pmove->velocity[i] = pmove->movevars->maxvelocity;
		}
		else if (pmove->velocity[i] < -pmove->movevars->maxvelocity)
		{
			//pmove->Con_DPrintf ("PM  Got a velocity too low on %i\n", i);
			pmove->velocity[i] = -pmove->movevars->maxvelocity;
		}
	}
}

/*
==================
PM_ClipVelocity

Slide off of the impacting object
returns the blocked flags:
0x01 == floor
0x02 == step / wall
==================
*/
int PM_ClipVelocity (vec3_t in, vec3_t normal, vec3_t out, float overbounce)
{
	float	backoff;
	float	change;
	float angle;
	int		i, blocked;
	
	angle = normal[ 2 ];

	blocked = 0x00;            // Assume unblocked.

/*	Creme: disabled this as a test.. this is another nasty valve 2d hack

	if (angle > 0)      // If the plane that is blocking us has a positive z component, then assume it's a floor.
		blocked |= 0x01;		// 
	if (!angle)         // If the plane has no Z, it is vertical (wall/step)
		blocked |= 0x02;		// 
*/	
	// Determine how far along plane to slide based on incoming direction.
	// Scale by overbounce factor.
	backoff = DotProduct (in, normal) * overbounce;

	for (i=0 ; i<3 ; i++)
	{
		change = normal[i]*backoff;
		out[i] = in[i] - change;
		// If out velocity is too small, zero it out.
//		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
//			out[i] = 0;
	}
	
	// Return blocking flags.
	return blocked;
}

void PM_AddCorrectGravity ()
{
	float	ent_gravity;

	if ( pmove->waterjumptime )
		return;

	if (pmove->gravity)
		ent_gravity = pmove->gravity;
	else
		ent_gravity = 1.0;

	// Add gravity so they'll be in the correct position during movement
	// yes, this 0.5 looks wrong, but it's not.  
	pmove->velocity[2] -= (ent_gravity * pmove->movevars->gravity * 0.5 * pmove->frametime );
	pmove->velocity[2] += pmove->basevelocity[2] * pmove->frametime;
	pmove->basevelocity[2] = 0;

	PM_CheckVelocity();
}


void PM_FixupGravityVelocity ()
{
	float	ent_gravity;

	if ( pmove->waterjumptime )
		return;

	if (pmove->gravity)
		ent_gravity = pmove->gravity;
	else
		ent_gravity = 1.0;

	// Get the correct velocity for the end of the dt 
  	pmove->velocity[2] -= (ent_gravity * pmove->movevars->gravity * pmove->frametime * 0.5 );

	PM_CheckVelocity();
}



void PM_AirAccelerate (vec3_t wishdir, float wishspeed, float accel)
{
	int			i;
	float		addspeed, accelspeed, currentspeed, wishspd = wishspeed;
		
	if (pmove->dead)
		return;
	if (pmove->waterjumptime)
		return;
	
	if (wishspd > 30)
		wishspd = 30;
	// Determine veer amount
	currentspeed = DotProduct (pmove->velocity, wishdir);
	// See how much to add
	addspeed = wishspd - currentspeed;
	// If not adding any, done.
	if (addspeed <= 0)
		return;
	// Determine acceleration speed after acceleration

	accelspeed = accel * wishspeed * pmove->frametime * pmove->friction;
	// Cap it
	if (accelspeed > addspeed)
		accelspeed = addspeed;
	
	// Adjust pmove vel.
	for (i=0 ; i<3 ; i++)
	{
		pmove->velocity[i] += accelspeed*wishdir[i];	
	}
}

/*
===================
PM_WaterMove

===================
*/
void PM_WaterMove (void)
{
	int		i;
	vec3_t	wishvel;
	float	wishspeed;
	vec3_t	wishdir;
	vec3_t	start, dest;
	vec3_t  temp;
	pmtrace_t	trace;

	float speed, newspeed, addspeed, accelspeed;

	// user intentions
	for (i=0 ; i<3 ; i++)
		wishvel[i] = pmove->forward[i]*pmove->cmd.forwardmove + pmove->right[i]*pmove->cmd.sidemove;

	// Sinking after no other movement occurs
	if (!pmove->cmd.forwardmove && !pmove->cmd.sidemove && !pmove->cmd.upmove)
		wishvel[2] -= 60;		// drift towards bottom
	else  // Go straight up by upmove amount.
		wishvel[2] += pmove->cmd.upmove;

	// Copy it over and determine speed
	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	// Cap speed.
	if (wishspeed > pmove->maxspeed)
	{
		VectorScale (wishvel, pmove->maxspeed/wishspeed, wishvel);
		wishspeed = pmove->maxspeed;
	}

	VectorAdd (pmove->velocity, pmove->basevelocity, pmove->velocity);

	// Water friction
	VectorCopy(pmove->velocity, temp);
	speed = VectorNormalize(temp);
	if (speed)
	{
		newspeed = speed - pmove->frametime * speed * pmove->movevars->friction * pmove->friction;

		if (newspeed < 0)
			newspeed = 0;
		VectorScale (pmove->velocity, newspeed/speed, pmove->velocity);
	}
	else
		newspeed = 0;

	// water acceleration
	if ( wishspeed < 0.1f )
	{
		return;
	}

	addspeed = wishspeed - newspeed;
	if (addspeed > 0)
	{

		VectorNormalize(wishvel);
		accelspeed = pmove->movevars->accelerate * wishspeed * pmove->frametime * pmove->friction;
		if (accelspeed > addspeed)
			accelspeed = addspeed;

		for (i = 0; i < 3; i++)
			pmove->velocity[i] += accelspeed * wishvel[i];
	}

	// Now move
	// assume it is a stair or a slope, so press down from stepheight above
	VectorMA (pmove->origin, pmove->frametime, pmove->velocity, dest);
	VectorCopy (dest, start);
	start[2] += pmove->movevars->stepsize + 1;
	trace = pmove->PM_PlayerTrace (start, dest, PM_NORMAL, -1 );
	if (!trace.startsolid && !trace.allsolid)	// FIXME: check steep slope?
	{	// walked up the step, so just keep result and exit
		VectorCopy (trace.endpos, pmove->origin);
		return;
	}
	
	// Try moving straight along out normal path.
	PM_FlyMove ();
}


/*
===================
PM_AirMove

===================
*/
void PM_AirMove (void)
{
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;

	// Copy movement amounts
	fmove = pmove->cmd.forwardmove;
	smove = pmove->cmd.sidemove;
	
	// Zero out z components of movement vectors
	pmove->forward[2] = 0;
	pmove->right[2]   = 0;
	// Renormalize
	VectorNormalize (pmove->forward);
	VectorNormalize (pmove->right);

	// Determine x and y parts of velocity
	for (i=0 ; i<2 ; i++)       
	{
		wishvel[i] = pmove->forward[i]*fmove + pmove->right[i]*smove;
	}
	// Zero out z part of velocity
	wishvel[2] = 0;             

	 // Determine maginitude of speed of move
	VectorCopy (wishvel, wishdir);  
	wishspeed = VectorNormalize(wishdir);

	// Clamp to server defined max speed
	if (wishspeed > pmove->maxspeed)
	{
		VectorScale (wishvel, pmove->maxspeed/wishspeed, wishvel);
		wishspeed = pmove->maxspeed;
	}
	
	PM_AirAccelerate (wishdir, wishspeed, pmove->movevars->airaccelerate);

	// Add in any base velocity to the current velocity.
	VectorAdd (pmove->velocity, pmove->basevelocity, pmove->velocity );

	PM_FlyMove ();
}

qboolean PM_InWater( void )
{
	return ( pmove->waterlevel > 1 );
}

/*
=============
PM_CheckWater

Sets pmove->waterlevel and pmove->watertype values.
=============
*/
qboolean PM_CheckWater ()
{
	vec3_t	point;
	int		cont;
	int		truecont;
	float     height;
	float		heightover2;

	// Pick a spot just above the players feet.
	point[0] = pmove->origin[0] + (pmove->player_mins[pmove->usehull][0] + pmove->player_maxs[pmove->usehull][0]) * 0.5;
	point[1] = pmove->origin[1] + (pmove->player_mins[pmove->usehull][1] + pmove->player_maxs[pmove->usehull][1]) * 0.5;
	point[2] = pmove->origin[2] + pmove->player_mins[pmove->usehull][2] + 1;
	
	// Assume that we are not in water at all.
	pmove->waterlevel = 0;
	pmove->watertype = CONTENTS_EMPTY;

	// Grab point contents.
	cont = pmove->PM_PointContents (point, &truecont );
	// Are we under water? (not solid and not empty?)
	if (cont <= CONTENTS_WATER && cont > CONTENTS_TRANSLUCENT )
	{
		// Set water type
		pmove->watertype = cont;

		// We are at least at level one
		pmove->waterlevel = 1;

		height = (pmove->player_mins[pmove->usehull][2] + pmove->player_maxs[pmove->usehull][2]);
		heightover2 = height * 0.5;

		//pmove->Con_DPrintf("height; %f,   heightover2: %f\n", height, heightover2);

		// Now check a point that is at the player hull midpoint.
		point[2] = pmove->origin[2] + heightover2;
		cont = pmove->PM_PointContents (point, NULL );
		// If that point is also under water...
		if (cont <= CONTENTS_WATER && cont > CONTENTS_TRANSLUCENT )
		{
			// Set a higher water level.
			pmove->waterlevel = 2;

			// Now check the eye position.  (view_ofs is relative to the origin)
			point[2] = pmove->origin[2] + pmove->view_ofs[2];

			cont = pmove->PM_PointContents (point, NULL );
			if (cont <= CONTENTS_WATER && cont > CONTENTS_TRANSLUCENT ) 
				pmove->waterlevel = 3;  // In over our eyes
		}

		// Adjust velocity based on water current, if any.
		if ( ( truecont <= CONTENTS_CURRENT_0 ) &&
			 ( truecont >= CONTENTS_CURRENT_DOWN ) )
		{
			// The deeper we are, the stronger the current.
			static vec3_t current_table[] =
			{
				{1, 0, 0}, {0, 1, 0}, {-1, 0, 0},
				{0, -1, 0}, {0, 0, 1}, {0, 0, -1}
			};

			VectorMA (pmove->basevelocity, 50.0*pmove->waterlevel, current_table[CONTENTS_CURRENT_0 - truecont], pmove->basevelocity);
		}
	}
	return pmove->waterlevel > 1;

return 0;	
}

/*
=============
PM_CatagorizePosition
=============
*/
void PM_CatagorizePosition (void)
{
	vec3_t		point;
	pmtrace_t		tr;
	vec3_t	vforward, vright, vup, vtemp, vorigin;
	int color ;

	//Creme: Fixme, checkwater crashes at the moment :(
	//PM_CheckWater();


	AngleVectors(pmove->vuser1, vforward, vright, vup);
//DebugDraw = 0;
	// DEBUG
if (DebugDraw == 1) {

	if (pmove->server) {
//		pmove->Con_DPrintf("Server");
		color = 77;
	} else {
//		pmove->Con_DPrintf("Client");
		color = 35;
	}

//	pmove->Con_DPrintf(" Angles x: %f; y: %f; z: %f; g: %i\n", pmove->angles[0], pmove->angles[1], pmove->angles[2], pmove->onground);
//	pmove->Con_DPrintf("   vUp? x: %f; y: %f; z: %f\n", vup[0], vup[1], vup[2]);

	VectorCopy (pmove->origin, vorigin);
	vorigin[2] += 20;

	VectorScale(vright, 10, vtemp);
	VectorAdd(vorigin, vtemp, point);
	PM_ParticleLine(vorigin, point, color, pmove->frametime*1.5, 0);

	VectorScale(vforward, 10, vtemp);
	VectorAdd(vorigin, vtemp, point);
	PM_ParticleLine(vorigin, point, color, pmove->frametime*1.5, 0);

	VectorScale(vup, 9, vtemp);
	VectorAdd(vorigin, vtemp, point);
	PM_ParticleLine(vorigin, point, color, pmove->frametime*1.5, 0);

	VectorCopy(point, vorigin);

	VectorScale(vup, 1, vtemp);
	VectorAdd(vorigin, vtemp, point);
	PM_ParticleLine(vorigin, point, 111, pmove->frametime*1.5, 0);
}
	// DEBUG END  */

	if (Length(pmove->velocity) > 5) {

		VectorScale(vup, -2, vtemp);
		VectorAdd(pmove->origin, vtemp, point);

	} else { 

		VectorCopy(pmove->origin, point);
		point[2] += -16;

	}


	if (pmove->velocity[2] > 1800)   // Crem was 180 Shooting up really fast.  Definitely not on ground.
	{
		pmove->onground = -1;
	}
	else
	{
		// Try and move down.
		tr = pmove->PM_PlayerTrace (pmove->origin, point, PM_NORMAL, -1 );
		// If we hit a steep plane, we are not on ground
		if ( tr.plane.normal[2] < -1.4)
			pmove->onground = -1;	// too steep
		else {
			if ((pmove->onground == -1) && (tr.ent != -1)) {

//				if (pmove->velocity[2] < -2) {
					//pmove->Con_DPrintf(" Landed! \n");
					pmove->PM_PlaySound(CHAN_AUTO, "car_lands.wav", 0.2, ATTN_NORM, 0, 100);	

//				}
			}

			pmove->onground = tr.ent;  // Otherwise, point to index of ent under us.

		}


		// If we are on something...
		if (pmove->onground != -1){

			// Then we are not in water jump sequence
			pmove->waterjumptime = 0;

			// Creme: this valve crap is no longer needed, as i use better code now :)

			// If we could make the move, drop us down that 1 pixel
/*			if (pmove->waterlevel < 2 && !tr.startsolid && !tr.allsolid);
				VectorCopy (tr.endpos, pmove->origin);*/

		} else {

			// Creme: Check if stuck, by checking x/y velocity 
			// sometimes z velocity is approx -5, so its not a good indicator

			if (pmove->velocity[0] + pmove->velocity[1] == 0) {
//				pmove->Con_DPrintf	("Stuck! Attempting to unstick\n");
//				pmove->onground = 0;	// NASTY HACK - let em try drive
			}


		}


		// Standing on an entity other than the world
		if (tr.ent > 0)   // So signal that we are touching something.
		{
			PM_AddToTouched(tr, pmove->velocity);
		}
	}
}

/*
=================
PM_GetRandomStuckOffsets

When a player is stuck, it's costly to try and unstick them
Grab a test offset for the player based on a passed in index
=================
*/
int PM_GetRandomStuckOffsets(int nIndex, int server, vec3_t offset)
{
 // Last time we did a full
	int idx;
	idx = rgStuckLast[nIndex][server]++;

	VectorCopy(rgv3tStuckTable[idx % 54], offset);

	return (idx % 54);
}

void PM_ResetStuckOffsets(int nIndex, int server)
{
	rgStuckLast[nIndex][server] = 0;
}

/*
=================
NudgePosition

If pmove->origin is in a solid position, 
try nudging slightly on all axis to
allow for the cut precision of the net coordinates
=================
*/
#define PM_CHECKSTUCK_MINTIME 0.05  // Don't check again too quickly.

int PM_CheckStuck (void)
{
	vec3_t	base;
	vec3_t  offset;
	vec3_t  test;
	int     hitent;
	int		idx;
	float	fTime;
	int i;
	pmtrace_t traceresult;

	static float rgStuckCheckTime[MAX_CLIENTS][2]; // Last time we did a full

	// If position is okay, exit
	hitent = pmove->PM_TestPlayerPosition (pmove->origin, &traceresult );
	if (hitent == -1 )
	{
		PM_ResetStuckOffsets( pmove->player_index, pmove->server );
		return 0;
	}

	VectorCopy (pmove->origin, base);

	// 
	// Deal with precision error in network.
	// 
	if (!pmove->server)
	{
		// World or BSP model
		if ( ( hitent == 0 ) ||
			 ( pmove->physents[hitent].model != NULL ) )
		{
			int nReps = 0;
			PM_ResetStuckOffsets( pmove->player_index, pmove->server );
			do 
			{
				i = PM_GetRandomStuckOffsets(pmove->player_index, pmove->server, offset);

				VectorAdd(base, offset, test);
				if (pmove->PM_TestPlayerPosition (test, &traceresult ) == -1)
				{
					PM_ResetStuckOffsets( pmove->player_index, pmove->server );
		
					VectorCopy ( test, pmove->origin );
					return 0;
				}
				nReps++;
			} while (nReps < 54);
		}
	}

	// Only an issue on the client.

	if (pmove->server)
		idx = 0;
	else
		idx = 1;

	fTime = pmove->Sys_FloatTime();
	// Too soon?
	if (rgStuckCheckTime[pmove->player_index][idx] >= 
		( fTime - PM_CHECKSTUCK_MINTIME ) )
	{
		return 1;
	}
	rgStuckCheckTime[pmove->player_index][idx] = fTime;

	pmove->PM_StuckTouch( hitent, &traceresult );

	i = PM_GetRandomStuckOffsets(pmove->player_index, pmove->server, offset);

	VectorAdd(base, offset, test);
	if ( ( hitent = pmove->PM_TestPlayerPosition ( test, NULL ) ) == -1 )
	{
		//Con_DPrintf("Nudged\n");

		PM_ResetStuckOffsets( pmove->player_index, pmove->server );

		if (i >= 27)
			VectorCopy ( test, pmove->origin );

		return 0;
	}

	// If player is flailing while stuck in another player ( should never happen ), then see
	//  if we can't "unstick" them forceably.
	if ( pmove->cmd.buttons & ( IN_JUMP | IN_DUCK | IN_ATTACK ) && ( pmove->physents[ hitent ].player != 0 ) )
	{
		float x, y, z;
		float xystep = 8.0;
		float zstep = 18.0;
		float xyminmax = xystep;
		float zminmax = 4 * zstep;
		
		for ( z = 0; z <= zminmax; z += zstep )
		{
			for ( x = -xyminmax; x <= xyminmax; x += xystep )
			{
				for ( y = -xyminmax; y <= xyminmax; y += xystep )
				{
					VectorCopy( base, test );
					test[0] += x;
					test[1] += y;
					test[2] += z;

					if ( pmove->PM_TestPlayerPosition ( test, NULL ) == -1 )
					{
						VectorCopy( test, pmove->origin );
						return 0;
					}
				}
			}
		}
	}

	//VectorCopy (base, pmove->origin);

	return 1;
}

#define	CHASE_DISTANCE		112		// Desired distance from target
#define CHASE_PADDING		4		// Minimum allowable distance between the view and a solid face

// Get the origin of the Observer based around the target's position and angles
void GetChaseOrigin( vec3_t targetangles, int iTargetIndex, vec3_t offset, vec3_t *returnvec )
{
	vec3_t forward;
	vec3_t vecEnd;
	vec3_t vecStart;
	struct pmtrace_s *trace;
	physent_t *target;

	target = &(pmove->physents[ iTargetIndex ]);

	// Trace back from the target using the player's view angles
	AngleVectors(targetangles, forward, NULL, NULL);

	// Without view_ofs, just guess at adding 28 (standing player) to the origin to get the eye-height
	VectorCopy( target->origin, vecStart );
	vecStart[2] += 64;
	VectorMA(offset, CHASE_DISTANCE, forward, vecEnd);
	VectorSubtract( vecStart, vecEnd, vecEnd );

	trace = pmove->PM_TraceLine( vecStart, vecEnd, 0, 2, iTargetIndex );

	// Return the position
	VectorMA( trace->endpos, CHASE_PADDING, trace->plane.normal, *returnvec );

}

/*
===============
PM_SpectatorMove
===============
*/
void PM_SpectatorMove (void)
{
	float	speed, drop, friction, control, newspeed;
	//float   accel;
	float	currentspeed, addspeed, accelspeed;
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;

#ifdef CLIENT_DLL
	if ( pmove->runfuncs )
	{
		// Set spectator flag
		iIsSpectator = SPEC_IS_SPECTATOR;
	}
#endif

	// Are we locked onto a target?
	if ( pmove->iuser2 )
	{
		vec3_t vecViewAngle;
		vec3_t vecNewOrg;
		vec3_t vecOffset;
		int i;

		// Find the client this player's targeting
		for (i = 0; i < pmove->numphysent; i++)
		{
			if ( pmove->physents[i].info == pmove->iuser2 )
				break;
		}

		if (i == pmove->numphysent)
			return;

		VectorCopy( vec3_origin, vecOffset );

		// Calculate a camera position based upon the target's origin and angles
		if (pmove->iuser1 == 1)
		{
			// Locked onto the target
			VectorCopy( pmove->physents[i].angles, vecViewAngle );
			vecViewAngle[0] = 0;

#ifdef CLIENT_DLL
			if ( pmove->runfuncs )
			{
				// Force the client to start smoothing both the spectator's origin and angles
				iIsSpectator |= (SPEC_SMOOTH_ANGLES | SPEC_SMOOTH_ORIGIN);
			}
#endif
		}
		else
		{
			// Freelooking around the target
			VectorCopy( pmove->angles, vecViewAngle );
		}

		GetChaseOrigin( vecViewAngle, i, vecOffset, &vecNewOrg);
		VectorCopy( vecNewOrg, pmove->origin );
//		VectorCopy( vecViewAngle, pmove->angles );
		VectorCopy( vec3_origin, pmove->velocity );

#ifdef CLIENT_DLL
		if ( pmove->runfuncs )
		{
			// Copy the desired angles into the client global var so we can force them to the player's view
			VectorCopy( pmove->angles, vecNewViewAngles );
			iHasNewViewAngles = true;
			VectorCopy( pmove->origin, vecNewViewOrigin );
			iHasNewViewOrigin = true;
		}
#endif
	}
	else
	{
		// Move around in normal spectator method
		// friction
		speed = Length (pmove->velocity);
		if (speed < 1)
		{
			VectorCopy (vec3_origin, pmove->velocity)
		}
		else
		{
			drop = 0;

			friction = pmove->movevars->friction*1.5;	// extra friction
			control = speed < pmove->movevars->stopspeed ? pmove->movevars->stopspeed : speed;
			drop += control*friction*pmove->frametime;

			// scale the velocity
			newspeed = speed - drop;
			if (newspeed < 0)
				newspeed = 0;
			newspeed /= speed;

			VectorScale (pmove->velocity, newspeed, pmove->velocity);
		}

		// accelerate
		fmove = pmove->cmd.forwardmove;
		smove = pmove->cmd.sidemove;
		
		VectorNormalize (pmove->forward);
		VectorNormalize (pmove->right);

		for (i=0 ; i<3 ; i++)
		{
			wishvel[i] = pmove->forward[i]*fmove + pmove->right[i]*smove;
		}
		wishvel[2] += pmove->cmd.upmove;

		VectorCopy (wishvel, wishdir);
		wishspeed = VectorNormalize(wishdir);

		// clamp to server defined max speed
		if (wishspeed > pmove->movevars->spectatormaxspeed)
		{
			VectorScale (wishvel, pmove->movevars->spectatormaxspeed/wishspeed, wishvel);
			wishspeed = pmove->movevars->spectatormaxspeed;
		}

		currentspeed = DotProduct(pmove->velocity, wishdir);
		addspeed = wishspeed - currentspeed;
		if (addspeed <= 0)
			return;
		accelspeed = pmove->movevars->accelerate*pmove->frametime*wishspeed;
		if (accelspeed > addspeed)
			accelspeed = addspeed;
		
		for (i=0 ; i<3 ; i++)
			pmove->velocity[i] += accelspeed*wishdir[i];	

		// move
		VectorMA (pmove->origin, pmove->frametime, pmove->velocity, pmove->origin);
	}
}





/*
============
PM_AddGravity

============
*/
void PM_AddGravity ()
{
	float	ent_gravity;

	if (pmove->gravity)
		ent_gravity = pmove->gravity;
	else
		ent_gravity = 1.0;

	// Add gravity incorrectly
	pmove->velocity[2] -= (ent_gravity * pmove->movevars->gravity * pmove->frametime );
	pmove->velocity[2] += pmove->basevelocity[2] * pmove->frametime;
	pmove->basevelocity[2] = 0;
	PM_CheckVelocity();
}
/*
============
PM_PushEntity

Does not change the entities velocity at all
============
*/
pmtrace_t PM_PushEntity (vec3_t push)
{
	pmtrace_t	trace;
	vec3_t	end;
		
	VectorAdd (pmove->origin, push, end);

	trace = pmove->PM_PlayerTrace (pmove->origin, end, PM_NORMAL, -1 );
	
	VectorCopy (trace.endpos, pmove->origin);

	// So we can run impact function afterwards.
	if (trace.fraction < 1.0 &&
		!trace.allsolid)
	{
		PM_AddToTouched(trace, pmove->velocity);
	}

	return trace;
}	

/*
============
PM_Physics_Toss()

Dead player flying through air., e.g.
============
*/
void PM_Physics_Toss()
{
	pmtrace_t trace;
	vec3_t	move;
	float	backoff;

	PM_CheckWater();

	if (pmove->velocity[2] > 0)
		pmove->onground = -1;

	// If on ground and not moving, return.
	if ( pmove->onground != -1 )
	{
		if (VectorCompare(pmove->basevelocity, vec3_origin) &&
		    VectorCompare(pmove->velocity, vec3_origin))
			return;
	}

	PM_CheckVelocity ();

// add gravity
	if ( pmove->movetype != MOVETYPE_FLY &&
		 pmove->movetype != MOVETYPE_BOUNCEMISSILE &&
		 pmove->movetype != MOVETYPE_FLYMISSILE )
		PM_AddGravity ();

// move origin
	// Base velocity is not properly accounted for since this entity will move again after the bounce without
	// taking it into account
	VectorAdd (pmove->velocity, pmove->basevelocity, pmove->velocity);
	
	PM_CheckVelocity();
	VectorScale (pmove->velocity, pmove->frametime, move);
	VectorSubtract (pmove->velocity, pmove->basevelocity, pmove->velocity);

	trace = PM_PushEntity (move);	// Should this clear basevelocity

	PM_CheckVelocity();

	if (trace.allsolid)
	{	
		// entity is trapped in another solid
		pmove->onground = trace.ent;
		VectorCopy (vec3_origin, pmove->velocity);
		return;
	}
	
	if (trace.fraction == 1)
	{
		PM_CheckWater();
		return;
	}


	if (pmove->movetype == MOVETYPE_BOUNCE)
		backoff = 2.0 - pmove->friction;
	else if (pmove->movetype == MOVETYPE_BOUNCEMISSILE)
		backoff = 2.0;
	else
		backoff = 1;

	PM_ClipVelocity (pmove->velocity, trace.plane.normal, pmove->velocity, backoff);

	// stop if on ground
	if (trace.plane.normal[2] > 0.7)
	{		
		float vel;
		vec3_t base;

		VectorClear( base );
		if (pmove->velocity[2] < pmove->movevars->gravity * pmove->frametime)
		{
			// we're rolling on the ground, add static friction.
			pmove->onground = trace.ent;
			//pmove->velocity[2] = 0;
		}

		vel = DotProduct( pmove->velocity, pmove->velocity );

		// Con_DPrintf("%f %f: %.0f %.0f %.0f\n", vel, trace.fraction, ent->velocity[0], ent->velocity[1], ent->velocity[2] );

		if (vel < (30 * 30) || (pmove->movetype != MOVETYPE_BOUNCE && pmove->movetype != MOVETYPE_BOUNCEMISSILE))
		{
			pmove->onground = trace.ent;
			VectorCopy (vec3_origin, pmove->velocity);
		}
		else
		{
			VectorScale (pmove->velocity, (1.0 - trace.fraction) * pmove->frametime * 0.9, move);
			trace = PM_PushEntity (move);
		}
		VectorSubtract( pmove->velocity, base, pmove->velocity )
	}
	
// check for in water
	PM_CheckWater();
}

/*
====================
PM_NoClip

====================
*/
void PM_NoClip()
{
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
//	float		currentspeed, addspeed, accelspeed;

	// Copy movement amounts
	fmove = pmove->cmd.forwardmove;
	smove = pmove->cmd.sidemove;
	
	VectorNormalize ( pmove->forward ); 
	VectorNormalize ( pmove->right );

	for (i=0 ; i<3 ; i++)       // Determine x and y parts of velocity
	{
		wishvel[i] = pmove->forward[i]*fmove + pmove->right[i]*smove;
	}
	wishvel[2] += pmove->cmd.upmove;

	VectorMA (pmove->origin, pmove->frametime, wishvel, pmove->origin);
	
	// Zero out the velocity so that we don't accumulate a huge downward velocity from
	//  gravity, etc.
	VectorClear( pmove->velocity );

}



void PM_CheckFalling( void )
{
	if ( pmove->onground != -1 ) 
	{		
		pmove->flFallVelocity = 0;
	}
}

/*
=================
PM_PlayWaterSounds

=================
*/
void PM_PlayWaterSounds( void )
{
	// Did we enter or leave water?
	if  ( ( pmove->oldwaterlevel == 0 && pmove->waterlevel != 0 ) ||
		  ( pmove->oldwaterlevel != 0 && pmove->waterlevel == 0 ) )
	{
		pmove->PM_PlaySound( CHAN_BODY, "player/pl_wade1.wav", 1, ATTN_NORM, 0, PITCH_NORM );
	}
}


/*
=============
PM_DropPunchAngle

=============
*/
void PM_DropPunchAngle ( vec3_t punchangle )
{
	float	len;
	
	len = VectorNormalize ( punchangle );
	len -= (10.0 + len * 0.5) * pmove->frametime;
	len = max( len, 0.0 );
	VectorScale ( punchangle, len, punchangle);
}

/*
==============
PM_CheckParamters

==============
*/
void PM_CheckParamters( void )
{
	float spd;
	float maxspeed;
	vec3_t	v_angle;

	spd = ( pmove->cmd.forwardmove * pmove->cmd.forwardmove ) +
		  ( pmove->cmd.sidemove * pmove->cmd.sidemove ) +
		  ( pmove->cmd.upmove * pmove->cmd.upmove );
	spd = sqrt( spd );

	maxspeed = pmove->clientmaxspeed; //atof( pmove->PM_Info_ValueForKey( pmove->physinfo, "maxspd" ) );
	if ( maxspeed != 0.0 )
	{
		pmove->maxspeed = min( maxspeed, pmove->maxspeed );
	}

	if ( ( spd != 0.0 ) &&
		 ( spd > pmove->maxspeed ) )
	{
		float fRatio = pmove->maxspeed / spd;
		pmove->cmd.forwardmove *= fRatio;
		pmove->cmd.sidemove    *= fRatio;
		pmove->cmd.upmove      *= fRatio;
	}

	if ( pmove->flags & FL_FROZEN || pmove->flags & FL_ONTRAIN || pmove->dead )
	{
		pmove->cmd.forwardmove = 0;
		pmove->cmd.sidemove    = 0;
		pmove->cmd.upmove      = 0;
	}


	PM_DropPunchAngle( pmove->punchangle );

	// Take angles from command.
	if ( !pmove->dead )
	{
		VectorCopy ( pmove->cmd.viewangles, v_angle );         
		VectorAdd( v_angle, pmove->punchangle, v_angle );

		// Set up view angles.
//		pmove->angles[ROLL]	=	PM_CalcRoll ( v_angle, pmove->velocity, pmove->movevars->rollangle, pmove->movevars->rollspeed )*4;
//		pmove->angles[PITCH] =	v_angle[PITCH];
		pmove->angles[YAW]   =	v_angle[YAW];
	}
	else
	{
		VectorCopy( pmove->oldangles, pmove->angles );
	}

	// Set dead player view_offset
	if ( pmove->dead )
	{
		pmove->view_ofs[2] = PM_DEAD_VIEWHEIGHT;
	}

	// Adjust client view angles to match values used on server.
	if (pmove->angles[YAW] > 180.0f)
	{
		pmove->angles[YAW] -= 360.0f;
	}

}

void PM_ReduceTimers( void )
{
	if ( pmove->flTimeStepSound > 0 )	{
		pmove->flTimeStepSound -= pmove->cmd.msec;
		if ( pmove->flTimeStepSound < 0 )		{
			pmove->flTimeStepSound = 0;
		}
	}
	if ( pmove->flDuckTime > 0 )	{
		pmove->flDuckTime -= pmove->cmd.msec;
		if ( pmove->flDuckTime < 0 )		{
			pmove->flDuckTime = 0;
		}
	}
	if ( pmove->flSwimTime > 0 )	{
		pmove->flSwimTime -= pmove->cmd.msec;
		if ( pmove->flSwimTime < 0 )		{
			pmove->flSwimTime = 0;
		}
	}

}

// SaRcaZm - V2 - Start
void PM_DriveMove ();
// SaRcaZm - V2 - End

// SaRcaZm - V8 - Start
// HACK, HACK, HACK
//#ifndef CLIENT_DLL
vec3_t carstats[32];

void PM_SetStats (int index, int accel, int handling, int topspeed)
{
	carstats[index][0] = accel;
	carstats[index][1] = handling;
	carstats[index][2] = topspeed;
}
//#endif
// SaRcaZm - V8 - End

/*
=============
PlayerMove

Returns with origin, angles, and velocity modified in place.

Numtouch and touchindex[] will be set if any of the physents
were contacted during the move.
=============
*/

void PM_PlayerMove ( qboolean server )
{
	physent_t *pLadder = NULL;
//	int i;

/*	for (i = 0; i < 4; i++) {
		pmove->player_mins[i][0] = -5; // -5;
		pmove->player_mins[i][1] = -5; //-5;
		pmove->player_mins[i][2] = -5; //-5;
		pmove->player_maxs[i][0] = 5; //5;
		pmove->player_maxs[i][1] = 5; //5;
		pmove->player_maxs[i][2] = 5; //5;
	}
*/
/*	if (server)
	Global_DLLEXPORT_Info(pmove->time, "PM_PlayerMove(sv)");
	else
	Global_DLLEXPORT_Info(pmove->time, "PM_PlayerMove(cl)");
*/
//	if (server) { 
//	} else {
//		pmove->Con_DPrintf("Client: PM_PlayerMove\n");
//	}

	// Are we running server code?
	pmove->server = server;                

	// Adjust speeds etc.
	PM_CheckParamters();

	// Assume we don't touch anything
	pmove->numtouch = 0;                    

//	pmove->usehull = 1;                    
	
	// # of msec to apply movement
	pmove->frametime = pmove->cmd.msec * 0.001;    

//	PM_ReduceTimers();

	// Convert view angles to vectors
	AngleVectors (pmove->angles, pmove->forward, pmove->right, pmove->up);

//PM_ShowClipBox();

	DebugDraw = 0;

	// Creme: yet another nasty hack to stop the car from being vertical 
	// like this at the start of some levels.. 
	// (when the startpoint is high off the ground)
	if ((pmove->vuser1[0] == 90) && (pmove->vuser1[2] == -90)) {
		pmove->vuser1[0] = pmove->vuser1[2] = 0;
	}
	if ((pmove->angles[0] == 90) && (pmove->angles[2] == -90)) {
		pmove->angles[0] = pmove->angles[2] = 0;
	}


#ifdef CLIENT_DLL
	if ( pmove->runfuncs )
	{
		iIsSpectator = false;
		iHasNewViewAngles = false;
		iHasNewViewOrigin = false;
	}
#endif

	// Special handling for spectator and observers. (iuser1 is set if the player's in observer mode)
	if ( pmove->spectator || pmove->iuser1 > 0 )
	{
		PM_SpectatorMove();
		PM_CatagorizePosition();
		return;
	}

	// Always try and unstick us unless we are in NOCLIP mode
	if ( pmove->movetype != MOVETYPE_NOCLIP && pmove->movetype != MOVETYPE_NONE )
	{
		if ( PM_CheckStuck() )
		{
			return;  // Can't move, we're stuck
		}
	}

	// Now that we are "unstuck", see where we are ( waterlevel and type, pmove->onground ).
	PM_CatagorizePosition();

/*	if (server) {
		pmove->Con_DPrintf("          S:");
	} else {
		pmove->Con_DPrintf("          C:");
	}

//	pmove->Con_DPrintf(" Angles x: %4.2f; y: %4.2f; z: %4.2f;", pmove->angles[0], pmove->angles[1], pmove->angles[2], pmove->onground);
	pmove->Con_DPrintf(" vuser1 x: %4.2f; y: %4.2f; z: %4.2f; g: %i\n", 
	pmove->vuser1[0], pmove->vuser1[1], pmove->vuser1[2], pmove->onground);*/


	PM_PlayerAngles ();

	// Store off the starting water level
	pmove->oldwaterlevel = pmove->waterlevel;

	// If we are not on ground, store off how fast we are moving down
	if ( pmove->onground == -1 )
	{
		pmove->flFallVelocity = -pmove->velocity[2];
	}

	g_onladder = 0;

	// Slow down, I'm pulling it! (a box maybe) but only when I'm standing on ground
	if ( ( pmove->onground != -1 ) && ( pmove->cmd.buttons & IN_USE) )
	{
	//	VectorScale( pmove->velocity, 0.3, pmove->velocity );
	}

	DebugDraw = 0;  // Crem: debug lines for normal (1 = true)

	// Handle movement
	switch ( pmove->movetype )
	{
	default:
		pmove->Con_DPrintf("Bogus pmove player movetype %i on (%i) 0=cl 1=sv\n", pmove->movetype, pmove->server);
		break;

	case MOVETYPE_NONE:
		break;

	case MOVETYPE_NOCLIP:
		PM_NoClip();
		break;

	case MOVETYPE_TOSS:
	case MOVETYPE_BOUNCE:

		PM_Physics_Toss();
		PM_AddCorrectGravity();

		if ( pmove->onground != -1 )
		{
			PM_CatagorizeTextureType();
			PM_Friction();
		}

		// Make sure velocity is valid.
		PM_CheckVelocity();

		// Are we on ground now
		if ( pmove->onground == -1 )
		{
			PM_AirMove();  // Take into account movement when in air.
			PM_Crash();
		}


		// Set final flags.
		PM_CatagorizePosition();

		// Now pull the base velocity back out.
		// Base velocity is set if you are on a moving object, like
		//  a conveyor (or maybe another monster?)
		VectorSubtract (pmove->velocity, pmove->basevelocity, pmove->velocity );

		// Add any remaining gravitational component.
		if ( !PM_InWater() )
		{
			PM_FixupGravityVelocity();
		}
		

		// Make sure velocity is valid.
		PM_CheckVelocity();
		PM_CheckFalling();

		break;

	case MOVETYPE_FLY:
	
		PM_CheckWater();

		// Perform the move accounting for any base velocity.
		VectorAdd (pmove->velocity, pmove->basevelocity, pmove->velocity);
		PM_FlyMove ();
		VectorSubtract (pmove->velocity, pmove->basevelocity, pmove->velocity);
		break;

	case MOVETYPE_WALK:

		PM_AddCorrectGravity();

		// SaRcaZm - V8 - Start
		// Customise these values based on the car statistics
		pmove->maxspeed = pmove->clientmaxspeed = pmove->vuser4[2] * 15;
//		pmove->Con_DPrintf ("Top speed: %f, %f, %f = %f\n", pmove->vuser4[0], pmove->vuser4[1], pmove->vuser4[2], pmove->maxspeed);
		// SaRcaZm - V8 - End

		if ( pmove->onground != -1 )
		{
			PM_CatagorizeTextureType();
			PM_Friction();
		}

		// Make sure velocity is valid.
		PM_CheckVelocity();

		// Are we on ground now
		if ( pmove->onground != -1 )
		{
			// Check collisions.
			PM_Crash();
			// We aren't walking, we're driving
			PM_DriveMove ();

		}
		else
		{
			PM_AirMove();  // Take into account movement when in air.
			PM_Crash();
		}



		// Set final flags.
		PM_CatagorizePosition();

		// Now pull the base velocity back out.
		// Base velocity is set if you are on a moving object, like
		//  a conveyor (or maybe another monster?)
		VectorSubtract (pmove->velocity, pmove->basevelocity, pmove->velocity );

		// Add any remaining gravitational component.
		if ( !PM_InWater() )
		{
			PM_FixupGravityVelocity();
		}
		

		// Make sure velocity is valid.
		PM_CheckVelocity();


		PM_CheckFalling();
		break;
	}

/*	if (pmove->server) {
		pmove->Con_DPrintf("S:");
	} else {
		pmove->Con_DPrintf("C:");
	}

//	pmove->Con_DPrintf(" Angles x: %4.2f; y: %4.2f; z: %4.2f;", 
//		pmove->angles[0], pmove->angles[1], pmove->angles[2], pmove->onground);
	pmove->Con_DPrintf(" vuser1 x: %4.2f; y: %4.2f; z: %4.2f; g: %i (END)\n", 
		pmove->vuser1[0], pmove->vuser1[1], pmove->vuser1[2], pmove->onground);
*/

//	pmove->Con_Printf("         actual velocity: %f\n ", Length(pmove->velocity));


}

void PM_CreateStuckTable( void )
{
	float x, y, z;
	int idx;
	int i;
	float zi[3];

	memset(rgv3tStuckTable, 0, 54 * sizeof(vec3_t));

	idx = 0;
	// Little Moves.
	x = y = 0;
	// Z moves
	for (z = -0.125 ; z <= 0.125 ; z += 0.125)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}
	x = z = 0;
	// Y moves
	for (y = -0.125 ; y <= 0.125 ; y += 0.125)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}
	y = z = 0;
	// X moves
	for (x = -0.125 ; x <= 0.125 ; x += 0.125)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}

	// Remaining multi axis nudges.
	for ( x = - 0.125; x <= 0.125; x += 0.250 )
	{
		for ( y = - 0.125; y <= 0.125; y += 0.250 )
		{
			for ( z = - 0.125; z <= 0.125; z += 0.250 )
			{
				rgv3tStuckTable[idx][0] = x;
				rgv3tStuckTable[idx][1] = y;
				rgv3tStuckTable[idx][2] = z;
				idx++;
			}
		}
	}

	// Big Moves.
	x = y = 0;
	zi[0] = 0.0f;
	zi[1] = 1.0f;
	zi[2] = 6.0f;

	for (i = 0; i < 3; i++)
	{
		// Z moves
		z = zi[i];
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}

	x = z = 0;

	// Y moves
	for (y = -2.0f ; y <= 2.0f ; y += 2.0)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}
	y = z = 0;
	// X moves
	for (x = -2.0f ; x <= 2.0f ; x += 2.0f)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}

	// Remaining multi axis nudges.
	for (i = 0 ; i < 3; i++)
	{
		z = zi[i];
		
		for (x = -2.0f ; x <= 2.0f ; x += 2.0f)
		{
			for (y = -2.0f ; y <= 2.0f ; y += 2.0)
			{
				rgv3tStuckTable[idx][0] = x;
				rgv3tStuckTable[idx][1] = y;
				rgv3tStuckTable[idx][2] = z;
				idx++;
			}
		}
	}
}



/*
This modume implements the shared player physics code between any particular game and 
the engine.  The same PM_Move routine is built into the game .dll and the client .dll and is
invoked by each side as appropriate.  There should be no distinction, internally, between server
and client.  This will ensure that prediction behaves appropriately.
*/

void PM_Move ( struct playermove_s *ppmove, int server )
{
	assert( pm_shared_initialized );

	pmove = ppmove;

		// SaRcaZm - V8 - Start
//#ifndef CLIENT_DLL
	pmove->vuser4[0] = carstats[pmove->player_index + 1][0];
	pmove->vuser4[1] = carstats[pmove->player_index + 1][1];
	pmove->vuser4[2] = carstats[pmove->player_index + 1][2];
//#endif
//	pmove->Con_DPrintf ("Stats: %f, %f, %f\n", pmove->vuser4[0], pmove->vuser4[1], pmove->vuser4[2]);
	// SaRcaZm - V8 - End

	PM_PlayerMove( ( server != 0 ) ? true : false );

	if ( pmove->onground != -1 )
	{
		pmove->flags |= FL_ONGROUND;
	}
	else
	{
		pmove->flags &= ~FL_ONGROUND;
	}

	// In single player, reset friction after each movement to FrictionModifier Triggers work still.
	if ( !pmove->multiplayer && ( pmove->movetype == MOVETYPE_WALK  ) )
	{
		pmove->friction = 1.0f;
	}
}

int PM_GetInfo( int ent )
{
	if ( ent >= 0 && ent <= pmove->numvisent )
	{
		return pmove->visents[ ent ].info;
	}
	return -1;
}

void PM_Init( struct playermove_s *ppmove )
{
	assert( !pm_shared_initialized );

	pmove = ppmove;

	PM_CreateStuckTable();
	PM_InitTextureTypes();

	pm_shared_initialized = 1;

	pmove->vuser1[0] = 0;	pmove->vuser1[1] = 0;	pmove->vuser1[2] = 0;
}
