/***********************************************************************
*
*   File: Rally_Effects.cpp
*    
*   Purpose: Renders and controls Half-Life Rally Special Effects
*
*   Author(s): FragMented!, Creme, SaRcaZm
*
*   Copyright: 2001-2004 HL Rally Team
*
**************************************************************************
*/


//	Includes
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "com_model.h"
#include "studio.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "dlight.h"

#include "triangleapi.h"
#include "camera.h"

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>

#include "studio_util.h"
#include "r_studioint.h"

#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "in_defs.h"

#include "cvardef.h"

#include "pm_defs.h"
#include "pm_debug.h"
#include "pmtrace.h"

#include "vgui_teamfortressviewport.h"


#include "Rally_Effects.h"
#include "rally_vguifx.h"
#include "rally_sound.h"

#define TRACE_HEIGHT		3


CRallyEffects* CRallyEffects::ms_pSingleton = NULL;

//	External References
extern "C" playermove_t *pmove;

extern float g_tractionloss;
extern float RallyYaw;

//	More stuff from the tri.cpp (hudneedle), not sure if all of this is used now.
extern float currentspeed;
extern float cam_originx, cam_originy, cam_originz;
extern float cam_anglesx, cam_anglesy, cam_anglesz;

extern vec3_t realvieworg;
extern vec3_t cam_origin;
extern cvar_t *cam_viewmode;

float oldrpms = 0;
float rpm_delta = 0;
float rpms;

extern int gear;

extern float g_accelerating;
extern float g_braking;
extern float g_reversing;
extern float g_handbraking;
extern float g_tractionloss;
extern int g_backfire;
extern float JoyMoved;

extern rally_teaminfo_t rallyinfo[MAX_TEAMS];


//	Textures Types
typedef enum
{
	TEX_DIRT,
	TEX_GRASS,
	TEX_SAND,
	TEX_SNOW,
	TEX_NONE,
	TEX_COUNT
};

//	Dust Textures
const char *pszDustTextureNames[TEX_COUNT] =
{
	"sprites/r_dirt.spr",
	"sprites/r_grass.spr",
	"sprites/r_sand.spr",
	"sprites/r_snow.spr",
	"",
};

//	Skid Textures
const char *pszSkidTextureNames[TEX_COUNT] =
{
	"sprites/r_skid_dirt.spr",
	"sprites/r_skid_dirt.spr",
	"sprites/r_skid_sand.spr",
	"sprites/r_skid_snow.spr",
	"sprites/r_skid.spr",
};

model_t *pSprSkidTextures[TEX_COUNT];




#define SKID_LENGTH_OFS		7
#define SKID_WIDTH_OFS		3

#define TRACE_HEIGHT		3
#define TRACE_LENGTH		56


/*
=================
CRallyEffects

Called on initialization (CRallyEffects::createSingleton())
Class constructor.
=================
*/
CRallyEffects::CRallyEffects()
{
	m_pCvarSkidfx				= gEngfuncs.pfnRegisterVariable( "rally_skid", "1", FCVAR_ARCHIVE );
	m_pCvarShadowfx				= gEngfuncs.pfnRegisterVariable( "rally_shadows", "4", FCVAR_ARCHIVE );
	m_pCvarShadowOpponentsfx	= gEngfuncs.pfnRegisterVariable( "rally_opponentshadows", "1", FCVAR_ARCHIVE );
	m_pCvarSkidTime				= gEngfuncs.pfnRegisterVariable( "rally_skidmarktime", "30", FCVAR_ARCHIVE );
	m_pCvarDustfx				= gEngfuncs.pfnRegisterVariable( "rally_dust", "1", FCVAR_ARCHIVE );
	m_pCvarFogfx				= gEngfuncs.pfnRegisterVariable( "rally_fog", "0", FCVAR_ARCHIVE );
	m_pCvarManual				= gEngfuncs.pfnRegisterVariable( "rally_manual", "0", FCVAR_ARCHIVE );

	m_pCvarSparkfx				= gEngfuncs.pfnRegisterVariable( "rally_sparks", "0", FCVAR_ARCHIVE );

	m_pCvarHeadlightsOpponents	= gEngfuncs.pfnRegisterVariable( "rally_opponentlights", "0", FCVAR_ARCHIVE );

	m_pCvarPartslist			= gEngfuncs.pfnRegisterVariable( "rally_vgui_partslist", "1", FCVAR_ARCHIVE );

	// Hot Potato
	m_pCvarHPfx_r				= gEngfuncs.pfnRegisterVariable( "rally_hp_glowshell_r", "255", FCVAR_ARCHIVE );
	m_pCvarHPfx_g				= gEngfuncs.pfnRegisterVariable( "rally_hp_glowshell_g", "0", FCVAR_ARCHIVE );
	m_pCvarHPfx_b				= gEngfuncs.pfnRegisterVariable( "rally_hp_glowshell_b", "0", FCVAR_ARCHIVE );

	m_pSkidlist					= NULL;

	memset(m_PlayerList, 0, sizeof(m_PlayerList[0]) * MAX_PLAYERS);

	freeSkidmarks(); //	Set firsttime
}


/*
=================
~CRallyEffects

Called on cl_dll shutdown / exit.
Class destructor.
=================
*/
CRallyEffects::~CRallyEffects()
{
	freeSkidmarks();
}


/*
=================
VidInit

Precaches sprites
=================
*/
void CRallyEffects::VidInit()
{
	m_pSprShadow = (struct model_s *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/r_shadow.spr"));
	m_pSprHudNeedle = (struct model_s *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/hud_needle.spr"));
	m_pSprBrakeLight = (struct model_s *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/flare3.spr"));

	for(unsigned short i = 0; i < TEX_COUNT; i++)
	{
		pSprSkidTextures[i] = (struct model_s *)gEngfuncs.GetSpritePointer(SPR_Load(pszSkidTextureNames[i]));
	}
}



//------------------ render ------------------------



/*
=================
render

Called once per frame
Rally Effects public entry point
=================
*/
void CRallyEffects::render()
{
}

/*
=================
renderTransTriangles

Called once per frame
Rally Effects public entry point
=================
*/
void CRallyEffects::renderTransTriangles()
{
	for(unsigned short i = 0; i < 32; i++)
	{
		playerEffectsInfo_t *pPlayer = &m_PlayerList[i];

		if(pPlayer->bIsInPVS)
			renderPlayerTransTriangles(pPlayer);

		pPlayer->bIsInPVS = false;
	}

	// Render
	renderFog();
	renderSkidmarks();
}

/*
=================
renderCalculateRPMs

Called once per frame
Calculates rpms, sets global vars
=================
*/
void CRallyEffects::renderCalculateRPMs()
{
	//	Cant be a spectator
	if(!gEngfuncs.GetLocalPlayer())
		return;

	vec3_t vertice[4], vTemp, vAngles;

	VectorCopy(pmove->velocity, vTemp);
	vTemp[2] = 0.0f;

	currentspeed = Length(vTemp) * 3.25;

	int old_gear = gear;
	if(!m_pCvarManual->value)
	{
		float auto_speed;

		auto_speed = currentspeed / (gear * 2);

		if (auto_speed > 190)
			gear ++;
		else if (auto_speed < 120)
		{
			// if still in 1st / 2nd do this check so it doesnt toggle
			if (currentspeed > 550)
				gear--;
			else
			{
				if (auto_speed < 80)
					gear--;
			}
		}

		if (gear > 6)
			gear = 6;
		else if (gear < 1)
		{
			if (currentspeed > 5)
				gear = 1;
			else
				gear = 0;
		}

		if (CRallyVGUIFX::getSingleton()->GetMode () == MODE_COUNTDOWN)
			gear = 0;
	}

	// Throw in a backfire!
	if (old_gear > gear)
		g_backfire = 2;

	// Rotate the needle
	if ((pmove->onground == -1) || !gear || g_tractionloss || currentspeed < 5)
	{
		if (g_accelerating)
		{
			if (g_tractionloss)
				rpms += ((15 + ((g_tractionloss/gear) * 300)) * pmove->frametime);
			else
				rpms += (1100 * pmove->frametime);
		}

		else if (JoyMoved > 0.05)
			rpms += ((1100*(JoyMoved)) * pmove->frametime);
	
		else if (g_reversing == 2)	// 2 indicates the accelerator is pressed
			rpms += (1100 * pmove->frametime);

		else
		{
			rpms -= (400 * pmove->frametime);
			rpms += (400 * JoyMoved) * pmove->frametime;
		}

	}
	else
	{
		if (gear == -1)
			rpms = currentspeed;
		else
			rpms = currentspeed / (gear * 2);
	}

	if (rpms < 10)
	{
		oldrpms = 10;
		rpms = 15;
	}
	else if (rpms > 250)
	{
		oldrpms = 250;
		rpms = 201;
	}
	else
	{
		// Average out the difference and work it out over frame time.
		float delta_max = 550;
		
		rpm_delta = (rpms - oldrpms) / pmove->frametime;

		if (rpm_delta > delta_max)
			rpm_delta = delta_max;
		else if (rpm_delta < -delta_max)
			rpm_delta = -delta_max;

		rpm_delta *= pmove->frametime;

		rpms = oldrpms + rpm_delta;
		rpms = (oldrpms + rpms*3) / 4;
	}

	if (rpms < 10)
		rpms = 15;
	else if (rpms > 250)
		rpms = 250;

	if (oldrpms > (rpms + 2))
		g_accelerating = 2;
	else
		g_accelerating = 0;

	oldrpms = rpms;
}

/*
=================
renderFog

Called once per frame
Draws Fog
=================
*/
void CRallyEffects::renderFog()
{
	if(m_pCvarFogfx->value)
		gEngfuncs.pTriAPI->Fog ( Vector(128, 112, 96), 256, 6000, 1);
}

/*
=================
renderSkidmarks

Called once per frame
Draws Skidmarks from list
=================
*/
void CRallyEffects::renderSkidmarks()
{
	// Check to see if we have any skid marks to draw
	if (m_pSkidlist)
	{
		cl_entity_t *pThisPlayer = gEngfuncs.GetLocalPlayer();

		// Check to see if the first skid mark has run out of time
		// These things are being created soo fast now we have to delete more / frame
		while (m_pSkidlist && m_pSkidlist->time && ((gEngfuncs.GetClientTime() > m_pSkidlist->time) || Length(m_pSkidlist->vertices[0] - pThisPlayer->origin) > 3000))
		{
			skidlist_t *tmp;
			tmp = m_pSkidlist->next;
			free (m_pSkidlist);
			m_pSkidlist = tmp;
		}


		// Now that we have a list of all the skid marks, draw them all
		// Loop through and render each skid mark
		skidlist_t *skid;
		static const int tex_coords[4][2] = { { 0, 0 }, { 0, 1 }, { 1, 1 }, { 1, 0 } };

		// The optimisation of this part is essential!!
		gEngfuncs.pTriAPI->RenderMode( kRenderTransTexture );
		gEngfuncs.pTriAPI->CullFace( TRI_NONE ); // Set Face culling off
		gEngfuncs.pTriAPI->Brightness( 1 ); // set relative brightness of vertex

		for (skid = m_pSkidlist; skid != NULL; skid = skid->next)
		{
			// Setup rendering
			gEngfuncs.pTriAPI->SpriteTexture( skid->texture, 0 ); // Set texture for polys, second parameter is the frame number for animated sprites
			gEngfuncs.pTriAPI->Begin( TRI_QUADS ); // This defines what type of poly is being drawn. In this case it will be 4sided quads.

			for (int j = 0; j < 4; j++)
			{
				// Render the skid mark using the Triangle API (OpenGL anyone?)
				gEngfuncs.pTriAPI->TexCoord2f( tex_coords[j][0], tex_coords[j][1] ); // Set 2d texture coordinate for vertex (from 0.0 to 1.0 across whole texture)
				gEngfuncs.pTriAPI->Vertex3f( skid->vertices[j].x, skid->vertices[j].y,  skid->vertices[j].z + 1.0f ); // Set 3d position of vertex
			}

			// End rendering
			gEngfuncs.pTriAPI->End();
		}

		gEngfuncs.pTriAPI->CullFace( TRI_FRONT ); // Re-enable backface culling
	}
}



//------------------ renderPlayer ------------------------



/*
=================
renderPlayer

Called once per visable player, per frame.
Every visable player is passed into this method.
=================
*/
void CRallyEffects::renderPlayer(cl_entity_t *pPlayerEnt, int iPlayerIndex, bool bIsInterior)
{
	playerEffectsInfo_t *pPlayer = &m_PlayerList[iPlayerIndex];

	pPlayer->bIsLocal = (pPlayerEnt->index == gEngfuncs.GetLocalPlayer()->index) || bIsInterior;
	pPlayer->bIsInterior = bIsInterior;
	pPlayer->bIsInPVS = true;
	pPlayer->iPlayerIndex = iPlayerIndex;
	pPlayer->pPlayerEnt = pPlayerEnt;

	renderPlayerCalcVariables(pPlayer);
	renderPlayerDust(pPlayer);
	renderPlayerHP(pPlayer);
	renderPlayerCheckSkid(pPlayer);
	renderPlayerWheels(pPlayer);
	renderPlayerSounds(pPlayer);
}

/*
=================
renderPlayerTransTriangles

Called once per player per frame through tri.cpp.
Every visable player is passed into this method.
=================
*/
void CRallyEffects::renderPlayerTransTriangles(CRallyEffects::playerEffectsInfo_t *pPlayer)
{
	renderPlayerShadows(pPlayer);
	renderPlayerExhaust(pPlayer);
	renderPlayerBrakeLights(pPlayer);
	renderPlayerHeadLights(pPlayer);
}



/*
=================
renderPlayerCalcVariables

Called once per visable player, per frame
Sets up the playerEffectsInfo struct for pPlayer
=================
*/
void CRallyEffects::renderPlayerCalcVariables(CRallyEffects::playerEffectsInfo_t *pPlayer)
{
	if (pPlayer->bIsLocal)
	{
		VectorCopy (pmove->velocity, pPlayer->vVelocity);
	}
	else
	{
		float animtime;
		int cp = pPlayer->pPlayerEnt->current_position;		// shorter ;)
		int pp;												// Previous index position

		if (cp == 0)
			pp = 63;
		else
			pp = cp - 1;

		VectorSubtract (pPlayer->pPlayerEnt->ph[cp].origin, 
						pPlayer->pPlayerEnt->ph[pp].origin,
						pPlayer->vVelocity);

		animtime = pPlayer->pPlayerEnt->ph[cp].animtime - pPlayer->pPlayerEnt->ph[pp].animtime;

		VectorScale(pPlayer->vVelocity, 1/animtime, pPlayer->vVelocity);
	}

	VectorCopy (pPlayer->vVelocity, pPlayer->pPlayerEnt->baseline.basevelocity);

	// Baseline.origin[1] stores a float Length of velocity
	pPlayer->pPlayerEnt->baseline.origin[1] = Length(pPlayer->vVelocity);

	// OnGround Fix
	vec3_t traceend;
	VectorCopy(pPlayer->pPlayerEnt->origin, traceend);

	if (pPlayer->bIsInterior)
		traceend[2] -= 15;
	else
		traceend[2] -= 5;

	pmtrace_t trace;
	gEngfuncs.pEventAPI->EV_PlayerTrace(pPlayer->pPlayerEnt->origin, traceend, PM_STUDIO_BOX, -1, &trace);

	if(trace.plane.dist)
		pPlayer->bIsOnGround = true;
	else
		pPlayer->bIsOnGround = false;

	// Laggy shadow fix.
	if (pPlayer->bIsLocal)
		pPlayer->pPlayerEnt->angles[YAW] = RallyYaw;

	// Calculate some angles
	AngleVectors (pPlayer->pPlayerEnt->angles, pPlayer->vForward, pPlayer->vRight, pPlayer->vUp);

	// Calculate forward speed!
	vec3_t tmpVel;
	VectorCopy (pPlayer->vVelocity, tmpVel);
	tmpVel[2] = 0;
	pPlayer->fVelocity2D = Length(tmpVel);

	// Set the corners of the car
	pPlayer->vCorners[FRONT_LEFT]  = pPlayer->pPlayerEnt->origin + (pPlayer->vForward * 22) + (pPlayer->vRight * -12);
	pPlayer->vCorners[BACK_LEFT]   = pPlayer->pPlayerEnt->origin + (pPlayer->vForward * -22) + (pPlayer->vRight * -12);
	pPlayer->vCorners[BACK_RIGHT]  = pPlayer->pPlayerEnt->origin + (pPlayer->vForward * -22) + (pPlayer->vRight * 12);
	pPlayer->vCorners[FRONT_RIGHT] = pPlayer->pPlayerEnt->origin + (pPlayer->vForward * 22) + (pPlayer->vRight * 12);


	if (pPlayer->bIsLocal)
	{
		extern vec3_t vLocalAngles;
		vLocalAngles = pPlayer->pPlayerEnt->angles;

		// Special hack because attachments are lagged a frame.. sigh valve
		m_vBackFireOrigin = pPlayer->pPlayerEnt->attachment[0] + (pPlayer->vVelocity * pmove->frametime);

		// Special light values
		pPlayer->bIsBraking = (g_braking != 0.0f);
		pPlayer->bIsReversing = (g_reversing != 0.0f);
		renderCalculateRPMs();
	}
}

/*
=================
renderPlayerWheels

Called once per visable player, per frame.
Turns the wheels - spin code in SMR (fps of animation, so cant be executed here)
=================
*/
void CRallyEffects::renderPlayerWheels(CRallyEffects::playerEffectsInfo_t *pPlayer)
{
	if(pPlayer->bIsInterior)
		return;

	unsigned short i;
	cl_entity_t *pPlayerEnt = pPlayer->pPlayerEnt;

	for(i = 0; i < 4; i++)
		pPlayerEnt->curstate.controller[i] = pPlayerEnt->latched.prevcontroller[i];

	if(pPlayer->bIsLocal)
	{
		float tempflYaw = pPlayerEnt->latched.prevcontroller[0] - 127;
		float flYaw;
		int iDiv = 30;
		extern float steer_wheel_angle;

		flYaw = (steer_wheel_angle*-20) + 127;

		if (flYaw > 255)
			flYaw = 255;
		else if (flYaw < 0)
			flYaw = 0;

		pPlayerEnt->curstate.controller[0] = flYaw;
	
		if (pmove->onground != -1) 
		{
			if(pmove->velocity[1] > 100)
			{
				pPlayerEnt->curstate.controller[1] = ((pPlayerEnt->curstate.controller[1] * 5) + 100 + pmove->RandomLong(0, 40)) / 6;
				pPlayerEnt->curstate.controller[2] = ((pPlayerEnt->curstate.controller[2] * 6) + 100 + pmove->RandomLong(0, 40)) / 7;
			}
			else 
			{
				pPlayerEnt->curstate.controller[1] = ((pPlayerEnt->curstate.controller[1] * 5) + 100) / 6;
				pPlayerEnt->curstate.controller[2] = ((pPlayerEnt->curstate.controller[2] * 6) + 100) / 7;
			}
		}
		else
		{
			pPlayerEnt->curstate.controller[1] = ((pPlayerEnt->curstate.controller[1] * 20) + 255) / 21;
			pPlayerEnt->curstate.controller[2] = ((pPlayerEnt->curstate.controller[2] * 30) + 255) / 31;
		}
	}
	else
	{
		for(i = 0; i < 4; i++)
			pPlayerEnt->curstate.controller[i] = 127;
	}

	for(i = 0; i < 4; i++)
		pPlayerEnt->latched.prevcontroller[i] = pPlayerEnt->curstate.controller[i];

}

/*
=================
renderPlayerDust

Called once per visable player, per frame.
Particle Effects / Dust coming from player's tyres
=================
*/
void CRallyEffects::renderPlayerDust(CRallyEffects::playerEffectsInfo_t *pPlayer)
{
	if (m_pCvarDustfx->value)
	{
		float spr_scale_mul = 0.9;
		float spr_alpha = 1;
		float spr_life = 1;

		// End point of traceline
		vec3_t end;
		VectorCopy(pPlayer->pPlayerEnt->origin, end);
		end[2] -= 64;

		char *tmp = (char *)gEngfuncs.pEventAPI->EV_TraceTexture( 0, pPlayer->pPlayerEnt->origin, end);

		if(tmp)
		{
			const char *pszTextureSprite = NULL;
			char szTextureName[64]; 

			strcpy(szTextureName, tmp);

			// Case insensitive
			tmp = szTextureName;
			while (*tmp)
			{
				*tmp = tolower (*tmp);
				tmp++;
			}

			if (strstr(szTextureName, "dirt") || strstr(szTextureName, "mud") || strstr(szTextureName, "cliff"))
			{
				pszTextureSprite = pszDustTextureNames[TEX_DIRT];
				spr_alpha = 0.6;
			}
			else if (strstr(szTextureName, "gras"))
			{
				pszTextureSprite = pszDustTextureNames[TEX_GRASS];
				spr_scale_mul = 0.25;
				spr_alpha = 1;
			}
			else if(strstr(szTextureName, "sand") || strstr(szTextureName, "desert"))
			{
				pszTextureSprite = pszDustTextureNames[TEX_SAND];
			}
			else if(strstr(szTextureName, "snow"))
			{

				if (gEngfuncs.pfnRandomLong(0, 10) > 4)
				{
					if (gEngfuncs.pfnRandomLong(0, 10) > 4)
						pszTextureSprite = pszDustTextureNames[TEX_DIRT];
					else
						pszTextureSprite = pszDustTextureNames[TEX_SNOW];

					spr_scale_mul = 0.0125;
				}
				else
				{
					pszTextureSprite = "sprites/r_smoke.spr";
					spr_scale_mul = 1;
					spr_alpha = 0.25;
				}

			}
			else if (pPlayer->bIsSkidding)
			{
				pszTextureSprite = "sprites/r_smoke.spr";
				spr_scale_mul = 0.65;
				spr_alpha = 0.4;

				if (g_tractionloss)
				{ 
					spr_alpha = spr_alpha * g_tractionloss + 0.1; 

					if (spr_alpha > 0.4)
						spr_alpha = 0.4;
				}
			}

			if (pszTextureSprite && (pPlayer->fVelocity2D > 30) && pPlayer->bIsOnGround)
			{
				vec3_t tempvec, vel_vec;
				float smoke_size;

				vec3_t dustpoint[4];
				int num_wheels;

				// Back
				dustpoint[0] = pPlayer->pPlayerEnt->origin + (pPlayer->vUp * 1.5) + (pPlayer->vRight * 9) - (pPlayer->vForward * 19); 
				dustpoint[1] = pPlayer->pPlayerEnt->origin + (pPlayer->vUp * 1.5) - (pPlayer->vRight * 9) - (pPlayer->vForward * 19); 

				// Front
				dustpoint[2] = pPlayer->pPlayerEnt->origin + (pPlayer->vUp * 1.5) + (pPlayer->vRight * 9) + (pPlayer->vForward * 10); 
				dustpoint[3] = pPlayer->pPlayerEnt->origin + (pPlayer->vUp * 1.5) - (pPlayer->vRight * 9) + (pPlayer->vForward * 10); 

				// Simple optimisations on other player cars.
				// only do 2 dusty wheels, and cut the spr_life.
				if (!pPlayer->bIsLocal || pPlayer->bIsInterior)
				{ 
					num_wheels = 2;
					spr_life      *= 0.6;
					spr_scale_mul /= 0.6; // Scale it back up a bit. (same amount)
				}
				else
					num_wheels = 4;

				for (int i = 0; i < num_wheels; i++)
				{
					VectorCopy(pPlayer->vVelocity, vel_vec);

					// work out its random trajectory for the smoke sprite
					if (pPlayer->bIsLocal)
					{
						vel_vec[0] = (vel_vec[0] * 0.97) + gEngfuncs.pfnRandomLong(pPlayer->fVelocity2D / -30, pPlayer->fVelocity2D / 30);
						vel_vec[1] = (vel_vec[1] * 0.97) + gEngfuncs.pfnRandomLong(pPlayer->fVelocity2D / -30, pPlayer->fVelocity2D / 30);
						vel_vec[2] = (vel_vec[2] * 0.97) + gEngfuncs.pfnRandomLong(0, pPlayer->fVelocity2D / 10);
					}
					else
					{
						vel_vec[0] = (vel_vec[0] * 0.85) + gEngfuncs.pfnRandomLong(pPlayer->fVelocity2D / -20, pPlayer->fVelocity2D / 20);
						vel_vec[1] = (vel_vec[1] * 0.85) + gEngfuncs.pfnRandomLong(pPlayer->fVelocity2D / -20, pPlayer->fVelocity2D / 20);
						vel_vec[2] = (vel_vec[2] * 0.85) + gEngfuncs.pfnRandomLong(0, pPlayer->fVelocity2D / 15);
					}

					smoke_size = pPlayer->fVelocity2D * 0.002 * gEngfuncs.pfnRandomFloat(0.9, 1.2);

					// Limit Smoke Size
					if(smoke_size < 0.05)
						smoke_size = 0.05;
					else if(smoke_size > 1.5)
						smoke_size = 1.5;


					if (i >= 2)  // Front wheels dust lasts shorter
					{
						spr_life = spr_life * 0.75;
						smoke_size = smoke_size * 0.75;
					}


					gEngfuncs.pEfxAPI->R_TempSprite(
														dustpoint[i],
														vel_vec,
														smoke_size * spr_scale_mul, 
														gEngfuncs.pEventAPI->EV_FindModelIndex(pszTextureSprite),
														kRenderTransAlpha,
														kRenderFxNone,
														spr_alpha,
														spr_life, 
														FTENT_DUSTSPRITE | FTENT_SPRCYCLE
													);

				}
			}
		}
	}
}


/*
=================
renderPlayerShadows

Called once per visable player, per frame
Draws shadows
=================
*/
void CRallyEffects::renderPlayerShadows(CRallyEffects::playerEffectsInfo_t *pPlayer)
{
	pmtrace_t tr;

	//if(g_iUser1)
	//	return;

	// Don't draw if we can't see the player
	if ((pPlayer->pPlayerEnt->curstate.effects & EF_NODRAW) || pPlayer->bIsInterior)
		return;

	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetTraceHull( 0 );


	// No shadows when inside, so check to see what our trace hit
	if (m_pCvarShadowfx->value)
	{
		vec3_t corners[4];

		// Determine the corners of the shadow
		corners[0] = pPlayer->vCorners[0];
		corners[1] = pPlayer->vCorners[1];
		corners[2] = pPlayer->vCorners[2];
		corners[3] = pPlayer->vCorners[3];

		// If we are off the ground, shrink the shadow according to the height of the ground
		if (!pPlayer->bIsOnGround)
		{
			vec3_t origin;
			float height;

			// We must shrink the size of the shadow based on the height above the ground
			// Trace down from the origin to get the distance to the ground
			origin = pPlayer->pPlayerEnt->origin + Vector (0, 0, TRACE_HEIGHT);
			gEngfuncs.pEventAPI->EV_PlayerTrace( origin, origin + Vector (0, 0, -96), PM_STUDIO_BOX | PM_WORLD_ONLY, -1, &tr );
			height = (pPlayer->pPlayerEnt->origin - tr.endpos).Length();

			// Move all of the points towards the centre of the car
			// Move forward / back by (height / 5.0f) and left / right by (height / 10.0f)
			corners[0] = corners[0] + (pPlayer->vForward * -(height / 5.0f)) + (pPlayer->vRight *  (height / 10.0f));
			corners[1] = corners[1] + (pPlayer->vForward *  (height / 5.0f)) + (pPlayer->vRight *  (height / 10.0f));
			corners[2] = corners[2] + (pPlayer->vForward *  (height / 5.0f)) + (pPlayer->vRight * -(height / 10.0f));
			corners[3] = corners[3] + (pPlayer->vForward * -(height / 5.0f)) + (pPlayer->vRight * -(height / 10.0f));
		}

		// Determine length and width of the car
		float width = (corners[2] - corners[1]).Length();
		float length = (corners[1] - corners[0]).Length();
		// Determine how much to increment the subdivision by each time
		float inc_width = width / m_pCvarShadowfx->value;
		float inc_length = length / m_pCvarShadowfx->value;

		// rally_shadows now determines how much detail goes into the shadows
		// i.e. The number of subdivisions of the shadow

		// Setup rendering
		gEngfuncs.pTriAPI->SpriteTexture(m_pSprShadow, 0 );
		gEngfuncs.pTriAPI->RenderMode( kRenderTransTexture );
		gEngfuncs.pTriAPI->CullFace( TRI_NONE );
		gEngfuncs.pTriAPI->Brightness( 1 );

		for (int i = 0; i < m_pCvarShadowfx->value; i++)
		{
			vec3_t vertex_top, vertex_bottom;
			float tex_top = (1.0f / m_pCvarShadowfx->value) * i;
			float tex_bottom = (1.0f / m_pCvarShadowfx->value) * (i + 1);

			gEngfuncs.pTriAPI->Begin( TRI_QUAD_STRIP );

			// Top left vertex in the line
			vertex_top = corners[0] + (pPlayer->vForward * -(inc_length * i)) + Vector (0, 0, TRACE_HEIGHT);
			gEngfuncs.pEventAPI->EV_PlayerTrace( vertex_top, vertex_top + Vector (0, 0, -2056), PM_STUDIO_BOX | PM_WORLD_ONLY, -1, &tr );
			vertex_top = tr.endpos;
			gEngfuncs.pTriAPI->TexCoord2f( 0, tex_top );
			gEngfuncs.pTriAPI->Vertex3f( vertex_top.x, vertex_top.y, vertex_top.z );

			// Bottom left vertex in the line
			vertex_bottom = corners[0] + (pPlayer->vForward * -(inc_length * (i + 1))) + Vector (0, 0, TRACE_HEIGHT);
			gEngfuncs.pEventAPI->EV_PlayerTrace( vertex_bottom, vertex_bottom + Vector (0, 0, -2056), PM_STUDIO_BOX | PM_WORLD_ONLY, -1, &tr );
			vertex_bottom = tr.endpos;
			gEngfuncs.pTriAPI->TexCoord2f( 0, tex_bottom );
			gEngfuncs.pTriAPI->Vertex3f( vertex_bottom.x, vertex_bottom.y, vertex_bottom.z );

			// Now, loop through and draw the rest of the vertices in the line
			for (int j = 1; j <= m_pCvarShadowfx->value; j++)
			{
				float tex_width = (1.0f / m_pCvarShadowfx->value) * j;
				vec3_t vertex;

				// Next top point
				vertex = vertex_top + (pPlayer->vRight * (inc_width * j)) + Vector (0, 0, TRACE_HEIGHT);
				gEngfuncs.pEventAPI->EV_PlayerTrace( vertex, vertex + Vector (0, 0, -2056), PM_STUDIO_BOX | PM_WORLD_ONLY, -1, &tr );
				gEngfuncs.pTriAPI->TexCoord2f( tex_width, tex_top );
				gEngfuncs.pTriAPI->Vertex3f( tr.endpos.x, tr.endpos.y, tr.endpos.z + .1f);

				// Next bottom point
				vertex = vertex_bottom + (pPlayer->vRight * (inc_width * j)) + Vector (0, 0, TRACE_HEIGHT);
				gEngfuncs.pEventAPI->EV_PlayerTrace( vertex, vertex + Vector (0, 0, -2056), PM_STUDIO_BOX | PM_WORLD_ONLY, -1, &tr );
				gEngfuncs.pTriAPI->TexCoord2f( tex_width, tex_bottom );
				gEngfuncs.pTriAPI->Vertex3f( tr.endpos.x, tr.endpos.y, tr.endpos.z + .1f);
			}

			// End rendering
			gEngfuncs.pTriAPI->End();
		}

		gEngfuncs.pTriAPI->CullFace( TRI_FRONT ); // Re-enable backface culling
	}

	gEngfuncs.pEventAPI->EV_PopPMStates();
}

/*
=================
renderPlayerHP

Called once per visable player, per frame
HP Glowshell
=================
*/
void CRallyEffects::renderPlayerHP(CRallyEffects::playerEffectsInfo_t *pPlayer)
{
	// HP_ weapon model
	if(pPlayer->pPlayerEnt->curstate.weaponmodel == gEngfuncs.pEventAPI->EV_FindModelIndex( "models/hp_bomb.mdl"))
	{
		pPlayer->pPlayerEnt->curstate.renderfx = kRenderFxGlowShell;
		pPlayer->pPlayerEnt->curstate.rendercolor.r = m_pCvarHPfx_r->value;
		pPlayer->pPlayerEnt->curstate.rendercolor.g = m_pCvarHPfx_g->value;
		pPlayer->pPlayerEnt->curstate.rendercolor.b = m_pCvarHPfx_b->value;
		pPlayer->pPlayerEnt->curstate.renderamt = 0;
	}
}

static void vectoangles (vec3_t vec, vec3_t ang)
{
	float forward, pitch, yaw;

	if (vec[1] == 0 && vec[0] == 0)
	{
		yaw = 0;
		if (vec[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		yaw = (int) (atan2 (vec[1], vec[0]) * (180.0 / M_PI));
		if (yaw < 0)
			yaw += 360;

		forward = sqrt (vec[0] * vec[0] + vec[1] * vec[1]);
		pitch = (int) (atan2 (vec[2], forward) * (180.0 / M_PI));
		if (pitch < 0)
			pitch += 360;
	}

	ang[0] = pitch;
	ang[1] = yaw;
	ang[2] = 0;
}


/*
=================
renderPlayerCheckSkid

Called once per visable player, per frame
Adds skidmarks to the linked list
=================
*/
void CRallyEffects::renderPlayerCheckSkid(CRallyEffects::playerEffectsInfo_t *pPlayer)
{
	static float SkidMarkSoundTime = 0;

	if (pPlayer->fVelocity2D < 20 || pPlayer->bIsInterior || !pPlayer->bIsLocal)
	{
		pPlayer->bIsSkidding = false;
		return;
	}

	if (m_pCvarSkidfx->value)
	{
		vec3_t dist_vec;
		static skidlist_t *lastskid = NULL;
		vec3_t last_pos, last_right;

		// Find the distance between the current point and the previous point
		dist_vec = pPlayer->vSkid_last_pos[0] - pPlayer->vCorners[BACK_RIGHT];

		// Only draw if we have moved far enough, we have skidmarks left to use
		// Increase the distance to 20 so we don't get too many drawn at low speed
		if ((dist_vec.Length () > 15) || (pPlayer->bSkid_FirstTime))
		{
			pmtrace_t tr;
			vec3_t rightside_vec, leftside_vec, fr_rightside_vec, fr_leftside_vec, temp_vec, temp_right;

			// Check to see what angle the car is on in relation to the view
			// temp_vec = m_vNormal - pPlayer->vForward;
			vec3_t vel_vec = pPlayer->vVelocity;
			VectorNormalize (vel_vec);
			temp_vec = vel_vec - pPlayer->vForward;

			// Rear skid points
			// Trace down from above the right side of the car to find the height of the ground there
			rightside_vec = pPlayer->vCorners[BACK_RIGHT] + (pPlayer->vForward * SKID_LENGTH_OFS) + (pPlayer->vRight * -SKID_WIDTH_OFS) + Vector (0, 0, TRACE_HEIGHT);
			gEngfuncs.pEventAPI->EV_PlayerTrace( rightside_vec, rightside_vec + Vector (0, 0, -TRACE_LENGTH), PM_STUDIO_BOX, -1, &tr );
			rightside_vec = tr.endpos;

			// Trace down from above the left side of the car to find the height of the ground there
			leftside_vec = pPlayer->vCorners[BACK_LEFT] + (pPlayer->vForward * SKID_LENGTH_OFS) + (pPlayer->vRight * SKID_WIDTH_OFS) + Vector (0, 0, TRACE_HEIGHT);
			gEngfuncs.pEventAPI->EV_PlayerTrace( leftside_vec, leftside_vec + Vector (0, 0, -TRACE_LENGTH), PM_STUDIO_BOX, -1, &tr );
			leftside_vec = tr.endpos;

			// Front skid points
			// Trace down from above the front right side of the car to find the height of the ground there
			fr_rightside_vec = pPlayer->vCorners[FRONT_RIGHT] + (pPlayer->vForward * -SKID_LENGTH_OFS * 1.2) + (pPlayer->vRight * -SKID_WIDTH_OFS) + Vector (0, 0, TRACE_HEIGHT);
			gEngfuncs.pEventAPI->EV_PlayerTrace( fr_rightside_vec, fr_rightside_vec + Vector (0, 0, -TRACE_LENGTH), PM_STUDIO_BOX, -1, &tr );
			fr_rightside_vec = tr.endpos;

			// Trace down from above the front left side of the car to find the height of the ground there
			fr_leftside_vec = pPlayer->vCorners[FRONT_LEFT] + (pPlayer->vForward * -SKID_LENGTH_OFS * 1.2) + (pPlayer->vRight * SKID_WIDTH_OFS) + Vector (0, 0, TRACE_HEIGHT);
			gEngfuncs.pEventAPI->EV_PlayerTrace( fr_leftside_vec, fr_leftside_vec + Vector (0, 0, -TRACE_LENGTH), PM_STUDIO_BOX, -1, &tr );
			fr_leftside_vec = tr.endpos;

			// Fix the skidmark widths
			vec3_t r;
			vectoangles (pmove->velocity, r);
			CrossProduct (r, tr.plane.normal, temp_right);
			VectorNormalize (temp_right);

			// If it was part of the map, draw a skid mark on it
			// BUT only if: 1) We are on the ground;
			//				2) This is not the first time through this function; AND
			//				3) We are going slow enough; AND
			//					a) We are accelerating; OR
			//					b) We are braking; OR
			//				4) We are on a big enough angle to the camera i.e sliding enough
			//				5) We arent a local reversing player
			//		        6) Or handbraking and speed > 25
			// Enough brackets for you?

			// Both the current and the previous positions must be on the ground
			pPlayer->bIsSkidding = false;

			if 
			(
				pPlayer->bIsLocal && 
				pPlayer->bIsOnGround && 
				!pPlayer->bSkid_FirstTime && 
				(gear != 0) &&

				(
					(
						(pPlayer->fVelocity2D > 25) && (pPlayer->fVelocity2D < 200) && 
						(
							g_accelerating || g_braking 
						) || (pPlayer->fVelocity2D > 25) && (g_handbraking || (g_tractionloss))
					) ||
					(
						(temp_vec[YAW] > 0.45) || (temp_vec[YAW] < -0.45)
					) &&
					( !g_reversing ) 
				)
			)

			{
				// Figure out which skidmark to draw
				model_t *pSprSkid = NULL;
				vec3_t start, end;

				pPlayer->bIsSkidding = true;
				VectorCopy( pPlayer->pPlayerEnt->origin, start );
				VectorCopy( pPlayer->pPlayerEnt->origin, end );
				end[2] -= 64;
				char *TextureName = (char *)gEngfuncs.pEventAPI->EV_TraceTexture(0, start, end);

				if(TextureName)
				{
					// Convert it to lower case
					char *tmp = TextureName;
					while (*tmp)
					{
						*tmp = tolower (*tmp);
						tmp++;
					}

					// noskid texture prevents skidmarks from being displayed....
					if (strstr (TextureName, "noskid"))
						return;
					else if (strstr(TextureName, "roadgras") || (strstr(TextureName, "dirt") || strstr(TextureName, "gras") || strstr(TextureName, "mud") || strstr(TextureName, "soil") || strstr(TextureName, "3dmroad")))
						pSprSkid = pSprSkidTextures[TEX_DIRT];
					else if(strstr(TextureName, "sand") || strstr(TextureName, "desert"))
						pSprSkid = pSprSkidTextures[TEX_SAND];
					else if(strstr(TextureName, "snow"))
						pSprSkid = pSprSkidTextures[TEX_SNOW];
					else
					{
						if (SkidMarkSoundTime < gEngfuncs.GetClientTime())
						{
							if ((pPlayer->fVelocity2D > 60) || (temp_vec[YAW] > 0.15) || (temp_vec[YAW] < -0.15))
							{
								RallySound->fmodPlay3D ("skid_tarmac.wav", false, 0, pPlayer->pPlayerEnt->origin, pPlayer->vVelocity);
								SkidMarkSoundTime = gEngfuncs.GetClientTime() + 0.5;
							}
						}

						pSprSkid = pSprSkidTextures[TEX_NONE];
					}
				}

				skidlist_t *skid;

				// First skid mark
				skid = (skidlist_t *) malloc (sizeof (skidlist_t));
				memset (skid, 0, sizeof (*skid));

				// First vertex
				skid->vertices[0] = rightside_vec; // + (temp_right * 2);
				// Second vertex
				skid->vertices[1] = pPlayer->vSkid_last_pos[0]; // + (pPlayer->vSkid_last_right * 2);
				// Third vertex
				skid->vertices[2] = pPlayer->vSkid_last_pos[0] + (pPlayer->vSkid_last_right * -4);
				// Fourth vertex
				skid->vertices[3] = rightside_vec + (temp_right * -4);

				// Make it disappear after rally_skidmarktime seconds
				if (m_pCvarSkidTime->value)
					skid->time = gEngfuncs.GetClientTime() + m_pCvarSkidTime->value;

				skid->texture = pSprSkid;

				// Add it to the linked list
				if (!m_pSkidlist)
				{
					m_pSkidlist = skid;
					lastskid = skid;
				}
				else
				{
					lastskid->next = skid;
					lastskid = lastskid->next;
				}

				// Second skid mark
				skid = (skidlist_t *) malloc (sizeof (skidlist_t));
				memset (skid, 0, sizeof (*skid));

				// First vertex
				skid->vertices[0] = leftside_vec; // + (temp_right * -2);
				// Second vertex
				skid->vertices[1] = pPlayer->vSkid_last_pos[1]; // + (pPlayer->vSkid_last_right * -2);
				// Third vertex
				skid->vertices[2] = pPlayer->vSkid_last_pos[1] + (pPlayer->vSkid_last_right * 4);
				// Fourth vertex
				skid->vertices[3] = leftside_vec + (temp_right * 4);
				// Make it disappear after rally_skidmarktime seconds
				if (m_pCvarSkidTime->value)
					skid->time = gEngfuncs.GetClientTime() + m_pCvarSkidTime->value;

				skid->texture = pSprSkid;

				// Add it to the linked list
				lastskid->next = skid;
				lastskid = lastskid->next;

				// Third skid mark
				skid = (skidlist_t *) malloc (sizeof (skidlist_t));
				memset (skid, 0, sizeof (*skid));

				// First vertex
				skid->vertices[0] = fr_rightside_vec; // + (temp_right * 2);
				// Second vertex
				skid->vertices[1] = pPlayer->vSkid_last_pos[2]; // + (m_pCurrentPers->skid.last_right * 2);
				// Third vertex
				skid->vertices[2] = pPlayer->vSkid_last_pos[2] + (pPlayer->vSkid_last_right * -4);
				// Fourth vertex
				skid->vertices[3] = fr_rightside_vec + (temp_right * -4);

				// Make it disappear after rally_skidmarktime seconds
				if (m_pCvarSkidTime->value)
					skid->time = gEngfuncs.GetClientTime() + m_pCvarSkidTime->value;

				skid->texture = pSprSkid;

				// Add it to the linked list
				lastskid->next = skid;
				lastskid = lastskid->next;

				// Fourth skid mark
				skid = (skidlist_t *) malloc (sizeof (skidlist_t));
				memset (skid, 0, sizeof (*skid));

				// First vertex
				skid->vertices[0] = fr_leftside_vec; // + (temp_right * -2);
				// Second vertex
				skid->vertices[1] = pPlayer->vSkid_last_pos[3]; // + (m_pCurrentPers->skid.last_right * -2);
				// Third vertex
				skid->vertices[2] = pPlayer->vSkid_last_pos[3] + (pPlayer->vSkid_last_right * 4);
				// Fourth vertex
				skid->vertices[3] = fr_leftside_vec + (temp_right * 4);

				skid->texture = pSprSkid;

				// Make it disappear after rally_skidmarktime seconds
				if (m_pCvarSkidTime->value)
					skid->time = gEngfuncs.GetClientTime() + m_pCvarSkidTime->value;

				// Add it to the linked list
				lastskid->next = skid;
				lastskid = lastskid->next;
			}
			else 
			{
				pPlayer->bSkid_FirstTime = false;
			}

			// Save the current position as the last position
			pPlayer->vSkid_last_pos[0] = rightside_vec;
			pPlayer->vSkid_last_pos[1] = leftside_vec;
			pPlayer->vSkid_last_pos[2] = fr_rightside_vec;
			pPlayer->vSkid_last_pos[3] = fr_leftside_vec;
			pPlayer->vSkid_last_right = temp_right;
		}
	}
}

/*
=================
renderPlayerBrakeLights

Called once per visable player, per frame
Draws brake lights and reverse lights
=================
*/
void CRallyEffects::renderPlayerBrakeLights(CRallyEffects::playerEffectsInfo_t *pPlayer)
{
	if(pPlayer->bIsInterior)
		return;

	int teamidx = 0;
	int modelidx = 0;

	vec3_t		vTemp, vOfs, org;

	if(pPlayer->bIsBraking || pPlayer->bIsReversing || pPlayer->fBrightness || pPlayer->bIsHeadlightOn || pPlayer->fBrightnessHeadlight)
	{
		for(int j = 0; j < 13; j++)
		{
			if(!strcmpi(g_PlayerExtraInfo[pPlayer->pPlayerEnt->index].teamname, rallyinfo[j].szTeamName))
			{
				teamidx = j;
				break;
			}
		}
		
		if(g_PlayerInfoList[pPlayer->pPlayerEnt->index].model)
		{
			for(j = 0; j < 3; j++)
			{
				if(!strcmpi(g_PlayerInfoList[pPlayer->pPlayerEnt->index].model, rallyinfo[teamidx].szModels[j].szModelName))
				{
					modelidx = j;
					break;
				}
			}
		}
	}
	else
		return;

	if(pPlayer->bIsBraking)
		pPlayer->bWasBraking = true;

	if(pPlayer->bIsReversing)
		pPlayer->bWasBraking = false;


	if(pPlayer->bIsBraking || pPlayer->bIsReversing || pPlayer->fBrightness)
	{
		if(!(pPlayer->bIsBraking || pPlayer->bIsReversing))
			pPlayer->fBrightness -= 0.08;
		else
			pPlayer->fBrightness += 0.06;

		if(pPlayer->fBrightness > .6)
			pPlayer->fBrightness = .6;
		else if (pPlayer->fBrightness < 0)
			pPlayer->fBrightness = 0;

		if (pPlayer->bWasBraking)
			VectorScale(pPlayer->vRight, rallyinfo[teamidx].szModels[modelidx].fBrakeLightWidth, pPlayer->vRight);
		else
			VectorScale(pPlayer->vRight, rallyinfo[teamidx].szModels[modelidx].fBrakeLightWidth*.92, pPlayer->vRight);


		VectorScale(pPlayer->vForward, -rallyinfo[teamidx].szModels[modelidx].fBrakeLightDst, pPlayer->vForward);
		VectorScale(pPlayer->vUp, rallyinfo[teamidx].szModels[modelidx].fBrakeLightHeight, pPlayer->vUp);

		VectorCopy(pPlayer->pPlayerEnt->origin, org);
		VectorAdd(org, pPlayer->vRight, org);
		VectorAdd(org, pPlayer->vForward, org);
		VectorAdd(org, pPlayer->vUp, org);


		gEngfuncs.pTriAPI->SpriteTexture(m_pSprBrakeLight, 0 );

		gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
		gEngfuncs.pTriAPI->CullFace( TRI_NONE );
		gEngfuncs.pTriAPI->Begin( TRI_QUADS );

			if(pPlayer->bWasBraking)
				gEngfuncs.pTriAPI->Color4f( 1, .1, .1, pPlayer->fBrightness );
			else
				gEngfuncs.pTriAPI->Color4f( .9, .9, 1, pPlayer->fBrightness );

			gEngfuncs.pTriAPI->Brightness( 1 );
			gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
			gEngfuncs.pTriAPI->Vertex3f( org[0], org[1], org[2]);

			VectorSubtract(org, pPlayer->vUp, org);
			VectorScale(pPlayer->vUp, .8, vTemp);
			VectorAdd(org, vTemp, org);

			gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
			gEngfuncs.pTriAPI->Vertex3fv(org);

			VectorScale(pPlayer->vRight, 1.4, vTemp);

			VectorCopy(pPlayer->pPlayerEnt->origin, org);
			VectorAdd(org, vTemp, org);
			VectorAdd(org, pPlayer->vForward, org);
			VectorScale(pPlayer->vUp, .8, vTemp);
			VectorAdd(org, vTemp, org);

			gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
			gEngfuncs.pTriAPI->Vertex3fv(org);

			VectorCopy(pPlayer->pPlayerEnt->origin, org);
			VectorScale(pPlayer->vRight, 1.4, vTemp);
			VectorAdd(org, vTemp, org);
			VectorAdd(org, pPlayer->vForward, org);
			VectorAdd(org, pPlayer->vUp, org);

			gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
			gEngfuncs.pTriAPI->Vertex3fv(org);

		gEngfuncs.pTriAPI->End();

		// Create a entity light to light up the Car
		dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocElight (pPlayer->pPlayerEnt->index*2);
		VectorCopy (org, dl->origin);
		dl->radius = 2;
		if(pPlayer->bWasBraking)
		{
			dl->color.r = pPlayer->fBrightness * 240;
			dl->color.g = 2;
			dl->color.b = 2;
		}
		else
		{
			dl->color.r = pPlayer->fBrightness * 200;
			dl->color.g = pPlayer->fBrightness * 200;
			dl->color.b = pPlayer->fBrightness * 200;
		}
		dl->die = gEngfuncs.GetClientTime () + 0.1;


		VectorCopy(pPlayer->pPlayerEnt->origin, org);
		VectorAdd(org, -pPlayer->vRight, org);
		VectorAdd(org, pPlayer->vForward, org);
		VectorAdd(org, pPlayer->vUp, org);

		gEngfuncs.pTriAPI->Begin( TRI_QUADS );

			if(pPlayer->bWasBraking)
				gEngfuncs.pTriAPI->Color4f( 1, .1, .1, pPlayer->fBrightness );
			else
				gEngfuncs.pTriAPI->Color4f( .9, .9, 1, pPlayer->fBrightness );

			gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
			gEngfuncs.pTriAPI->Vertex3f( org[0], org[1], org[2]);

			VectorSubtract(org, pPlayer->vUp, org);
			VectorScale(pPlayer->vUp, .8, vTemp);
			VectorAdd(org, vTemp, org);

			gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
			gEngfuncs.pTriAPI->Vertex3fv(org);

			VectorScale(-pPlayer->vRight, 1.4, vTemp);

			VectorCopy(pPlayer->pPlayerEnt->origin, org);
			VectorAdd(org, vTemp, org);
			VectorAdd(org, pPlayer->vForward, org);
			VectorScale(pPlayer->vUp, .8, vTemp);
			VectorAdd(org, vTemp, org);

			gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
			gEngfuncs.pTriAPI->Vertex3fv(org);

			VectorCopy(pPlayer->pPlayerEnt->origin, org);
			VectorScale(pPlayer->vRight, 1.4, pPlayer->vRight);
			VectorAdd(org, -pPlayer->vRight, org);
			VectorAdd(org, pPlayer->vForward, org);
			VectorAdd(org, pPlayer->vUp, org);

			gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
			gEngfuncs.pTriAPI->Vertex3fv(org);

		gEngfuncs.pTriAPI->End();

		gEngfuncs.pTriAPI->RenderMode( kRenderNormal );

		gEngfuncs.pTriAPI->CullFace( TRI_FRONT ); // Re-enable backface culling

		// Create a entity light for models
		dlight_t *dl2 = gEngfuncs.pEfxAPI->CL_AllocElight (pPlayer->pPlayerEnt->index*2+1);
		VectorCopy (org, dl2->origin);
		dl2->radius = 2;
		dl2->color.r = dl->color.r;
		dl2->color.g = dl->color.g;
		dl2->color.b = dl->color.b;
		dl2->die = gEngfuncs.GetClientTime () + 0.1;

	}
	else
		pPlayer->fBrightness = 0;
}

/*
=================
renderPlayerHeadLights

Called once per visable player, per frame
Draws brake lights and reverse lights
=================
*/
void CRallyEffects::renderPlayerHeadLights(CRallyEffects::playerEffectsInfo_t *pPlayer)
{
	//	CVar
	if(!pPlayer->bIsLocal && !m_pCvarHeadlightsOpponents->value)
		return;

	if(pPlayer->fBrightnessHeadlight || pPlayer->bIsHeadlightOn)
	{
		vec3_t		vTemp, vOfs, org;

		if(!pPlayer->bIsHeadlightOn)
			pPlayer->fBrightnessHeadlight -= .1;
		else
			pPlayer->fBrightnessHeadlight += .1;

		if(pPlayer->fBrightnessHeadlight > 1)
			pPlayer->fBrightnessHeadlight = 1;
		else if (pPlayer->fBrightnessHeadlight < 0)
			pPlayer->fBrightnessHeadlight = 0;


		AngleVectors(pPlayer->pPlayerEnt->angles, pPlayer->vForward, pPlayer->vRight, pPlayer->vUp);
		VectorScale(pPlayer->vForward, 195.0f, pPlayer->vForward);
		VectorCopy(pPlayer->pPlayerEnt->origin, org);
		VectorAdd(org, pPlayer->vRight, org);
		VectorAdd(org, pPlayer->vForward, org);
		VectorAdd(org, pPlayer->vUp, org);

		// Create a dynamic light to light up the Ground
		dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocDlight (pPlayer->pPlayerEnt->index);
		VectorCopy (org, dl->origin);
		dl->radius = 220;
		dl->color.r = 160 * pPlayer->fBrightnessHeadlight;
		dl->color.g = 160 * pPlayer->fBrightnessHeadlight;
		dl->color.b = 150 * pPlayer->fBrightnessHeadlight;
		dl->die = gEngfuncs.GetClientTime () + 0.1;
	}
}

#define BACKFIRETIME 0.15

/*
=================
renderPlayerExhaust

Called once per visable player, per frame
Draws flame poly and pop sound
=================
*/
void FlamePoly (vec3_t vertice0, vec3_t vertice1, vec3_t vertice2, vec3_t vertice3)
{

	gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
	gEngfuncs.pTriAPI->Vertex3fv( vertice0 );

	gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
	gEngfuncs.pTriAPI->Vertex3fv( vertice1 );

	gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
	gEngfuncs.pTriAPI->Vertex3fv( vertice2 );

	gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
	gEngfuncs.pTriAPI->Vertex3fv( vertice3 );
}

void CRallyEffects::renderPlayerExhaust (CRallyEffects::playerEffectsInfo_t *pPlayer)
{
	if(!pPlayer->bIsLocal || pPlayer->bIsInterior)
		return;

	static float m_fBackFireTime = 0;

	vec3_t vertice[4];

	if(g_backfire)
	{
		// First time enters
		if (g_backfire != -2)
		{
			// Check to make sure we didnt just play this thing
			if (m_fBackFireTime < gEngfuncs.GetClientTime ())
				RallySound->fmodPlay3D ("backfire.wav", false, 0, pPlayer->pPlayerEnt->origin, pPlayer->vVelocity);

			m_fBackFireTime = gEngfuncs.GetClientTime () + BACKFIRETIME;
			g_backfire = -2;
		}

		if (m_fBackFireTime > gEngfuncs.GetClientTime () )
		{
			vec3_t Ofs;

			// add some random to swap the verts around
			int i;

			float fi;
			i = fi = (m_fBackFireTime - gEngfuncs.GetClientTime()) / BACKFIRETIME * 4; 

			if (i > 3) i = 3; else if (i < 0) i = 0;

			// the resulting random vert
			int j = i;
			int k;

			if ( !gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer( SPR_Load("sprites/muzzleflash3.spr") ), 0 ))
				return;
			
			gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd ); // kRenderTransAdd kRenderTransTexture
			gEngfuncs.pTriAPI->CullFace( TRI_NONE );
			gEngfuncs.pTriAPI->Begin( TRI_QUADS );

			float flamefade = (m_fBackFireTime - gEngfuncs.GetClientTime()) / BACKFIRETIME;

			gEngfuncs.pTriAPI->Color4f( flamefade, flamefade, flamefade, 1.0 );
			gEngfuncs.pTriAPI->Brightness( 1 );


			k = 0;
			if ((i + k) > 3) { j = (i + k) - 4; } else { j = i + k; }
			vertice[j] = m_vBackFireOrigin + (pPlayer->vRight * -2) + (pPlayer->vUp * -2);
			k = 1;
			if ((i + k) > 3) { j = (i + k) - 4; } else { j = i + k; }
			vertice[j] = m_vBackFireOrigin + (pPlayer->vRight * 2) + (pPlayer->vUp * -2);
			k = 2;
			if ((i + k) > 3) { j = (i + k) - 4; } else { j = i + k; }
			vertice[j] = m_vBackFireOrigin + (pPlayer->vRight * 2) + (pPlayer->vUp * 2);
			k = 3;
			if ((i + k) > 3) { j = (i + k) - 4; } else { j = i + k; }
			vertice[j] = m_vBackFireOrigin + (pPlayer->vRight * -2) + (pPlayer->vUp * 2);

			FlamePoly (vertice[0], vertice[1], vertice[2], vertice[3]);

			k = 0;
			if ((i + k) > 3) { j = (i + k) - 4; } else { j = i + k; }
			vertice[j] = m_vBackFireOrigin + (pPlayer->vRight * -2) + (pPlayer->vUp * -2) + (pPlayer->vRight * 2);
			k = 1;
			if ((i + k) > 3) { j = (i + k) - 4; } else { j = i + k; }
			vertice[j] = m_vBackFireOrigin + (pPlayer->vRight * -2) + (pPlayer->vUp * -2) + (pPlayer->vRight * -2);
			k = 2;
			if ((i + k) > 3) { j = (i + k) - 4; } else { j = i + k; }
			vertice[j] = m_vBackFireOrigin + (pPlayer->vRight * 2) + (pPlayer->vUp * 2) + (pPlayer->vRight * -2);
			k = 3;
			if ((i + k) > 3) { j = (i + k) - 4; } else { j = i + k; }
			vertice[j] = m_vBackFireOrigin + (pPlayer->vRight * 2) + (pPlayer->vUp * 2) + (pPlayer->vRight * 2);

			FlamePoly (vertice[0], vertice[1], vertice[2], vertice[3]);

			k = 0;
			if ((i + k) > 3) { j = (i + k) - 4; } else { j = i + k; }
			vertice[j] = m_vBackFireOrigin + (pPlayer->vRight * -2) + (pPlayer->vUp * 2) + (pPlayer->vRight * 2);
			k = 1;
			if ((i + k) > 3) { j = (i + k) - 4; } else { j = i + k; }
			vertice[j] = m_vBackFireOrigin + (pPlayer->vRight * -2) + (pPlayer->vUp * 2) + (pPlayer->vRight * -2);
			k = 2;
			if ((i + k) > 3) { j = (i + k) - 4; } else { j = i + k; }
			vertice[j] = m_vBackFireOrigin + (pPlayer->vRight * 2) + (pPlayer->vUp * -2) + (pPlayer->vRight * -2);
			k = 3;
			if ((i + k) > 3) { j = (i + k) - 4; } else { j = i + k; }
			vertice[j] = m_vBackFireOrigin + (pPlayer->vRight * 2) + (pPlayer->vUp * -2) + (pPlayer->vRight * 2);

			FlamePoly (vertice[0], vertice[1], vertice[2], vertice[3]);

			gEngfuncs.pTriAPI->End();
			gEngfuncs.pTriAPI->RenderMode( kRenderNormal ); // kRenderTransTexture

		}
		else 
		{
			// We are done, clear the variable
			g_backfire = 0;
		}
	}
}

/*
=================
renderPlayerSounds

Called once per visable player, per frame
Sound effects
=================
*/
void CRallyEffects::renderPlayerSounds(CRallyEffects::playerEffectsInfo_t *pPlayer)
{
	if (pPlayer->bIsHorn)
	{
		RallySound->fmodPlay3D ("horn.wav", false, 0, pPlayer->pPlayerEnt->origin, pPlayer->vVelocity);
		pPlayer->bIsHorn = false;
	}

	// Sound stuff
	if (pPlayer->bIsLocal || pPlayer->bIsInterior)
	{
		// Play the sound
		RallySound->fmodEngineChangePitch(rpms, pPlayer->iPlayerIndex, g_accelerating, pPlayer->pPlayerEnt->origin, pPlayer->vVelocity, true);
	}
	else
	{
		float revs;
		float o_currentspeed;

		o_currentspeed = pPlayer->fVelocity2D * 1.25;
		int m_gear = int(o_currentspeed / 250) + 1;
		revs = o_currentspeed / (m_gear);

		if (pPlayer->fVelocity2D > 2)
		{
			if (m_gear > 6)
				m_gear = 6;

			else if (m_gear < 1)
			{
				if (o_currentspeed > 5)
					m_gear = 1;
				else
					m_gear = 0;
			} 
		}
		
		// Store our value in baseline.origin as hl never uses it
		else if (CRallyVGUIFX::getSingleton()->GetMode() == MODE_COUNTDOWN)
			revs = pPlayer->pPlayerEnt->baseline.origin[0] + ((pmove->RandomFloat(400,1100)) * pmove->frametime);
		else
			revs = 15;	// Constant idling noise
		
		if (revs < 10)
			revs = 15;
		else if (revs > 250)
		{
			pPlayer->pPlayerEnt->baseline.origin[0] = 250;
			revs = 100 + pmove->RandomFloat(0,150);
		}

		revs = ((pPlayer->pPlayerEnt->baseline.origin[0] * 3) + (revs)) / 4;
		pPlayer->pPlayerEnt->baseline.origin[0] = revs;
		//pmove->Con_DPrintf ("       gear: %f, revs: %f\n", m_gear, revs);

		// If we got this far, play the sound
		RallySound->fmodEngineChangePitch(revs, pPlayer->iPlayerIndex, 1, pPlayer->pPlayerEnt->origin, pPlayer->vVelocity, false);
	}
}

/*
=================
freeSkidmarks

Rarely called
Frees all skidmarks
=================
*/
void CRallyEffects::freeSkidmarks()
{
	// Free all of the skid marks
	while (m_pSkidlist)
	{
		skidlist_t *tmp;
		tmp = m_pSkidlist;
		m_pSkidlist = m_pSkidlist->next;
		free (tmp);
	}

	// Set the firsttime variable to 1 to reinitialise the skid marks
	// Loop through and reset each one
	for (unsigned short i = 0; i < MAX_PLAYERS; i++)
		m_PlayerList[i].bSkid_FirstTime = true;
}