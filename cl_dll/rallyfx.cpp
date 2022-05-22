// rallyfx.cpp
// Renders HLRally Special effects
// By SaRcaZm, Creme, FragMented
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

#include "rallyfx.h"

#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "in_defs.h"

#include "cvardef.h"

// Crem pm
#include "pm_defs.h"
#include "pm_debug.h"
#include "pmtrace.h"

// SaRcaZm - V6 - Start
#include "vgui_teamfortressviewport.h"

char *skidtex_names[SKIDTEX_COUNT] = {
	"sprites/r_skid_dirt.spr",
	"sprites/r_skid_sand.spr",
	"sprites/r_skid_snow.spr",
	"sprites/r_skid.spr",
};
// SaRcaZm - V6 - End

#pragma warning( disable : 4244 )

#define SKID_LENGTH_OFS		7
#define SKID_WIDTH_OFS		3

#define TRACE_HEIGHT		3
#define TRACE_LENGTH		56

extern "C" playermove_t *pmove;

// Global engine <-> studio model rendering code interface
extern engine_studio_api_t IEngineStudio;
extern float rallyangle;
// Creme (non laggy yaw hack, extended from frags stuff ;)
extern float RallyYaw;

// Creme: more stuff from the tri.cpp (hudneedle), not sure if all of this is used now.
extern float currentspeed;
extern float cam_originx, cam_originy, cam_originz;
extern float cam_anglesx, cam_anglesy, cam_anglesz;

extern vec3_t realvieworg;
extern vec3_t cam_origin;
extern cvar_t *cam_viewmode;

extern float oldrpms;
extern float rpm_delta;
extern float rpms;

// SaRcaZm - V6 - Start
extern int gear;
// SaRcaZm - V6 - End

extern float g_accelerating;
extern float g_braking;
extern float g_reversing;
extern float g_handbraking;
extern float g_tractionloss;
extern int g_backfire;

// Creme HACK HACK HACK!
extern float JoyMoved;

// SaRcaZm - V5 - Start
#include "carinfo.h"

// SaRcaZm - V5 - End
// SaRcaZm - V7 - Start
extern vec3_t vgui_origin, vgui_angles, vLocalAngles;
// SaRcaZm - V7 - End

#include "rally_vguifx.h"
extern rally_teaminfo_t rallyinfo[MAX_TEAMS];

CRallyFX::CRallyFX (void)
{
	// FragMented
	m_pCurrentEntity	= NULL;
	m_pSkidlist			= NULL;
	m_pCurrentPers		= NULL;
	m_iPersMax			= 0;
	m_pCvarSkidfx = m_pCvarShadowfx = m_pCvarSkidTime = m_pCvarDustfx 
		= m_pCvarFogfx = NULL; // FragMented - RC7
	m_pCvarHPfx = m_pCvarHPfx_r = m_pCvarHPfx_g = m_pCvarHPfx_b = NULL;


	// SaRcaZm - V6 - Start
	m_pCvarManual		= NULL;
	// SaRcaZm - V6 - End

	// SaRcaZm - V5 - Start
	cl_2dcountdown = NULL;
	m_iCountdownDone = 0;
}

CRallyFX::~CRallyFX (void)
{
	Free_Skidlist ();

	// SaRcaZm - V5 - Start
	// Free the vehicles array

	// SaRcaZm - V5 - End

	m_pCurrentEntity = NULL;	// FragMented! Public Beta 1.1
}

void MP3Playlist (void);
extern char rally_playlist[80];
void CRallyFX::Init (void)
{
	cvar_t *rally_mp3skin = gEngfuncs.pfnRegisterVariable( "rally_mp3skin", "default", FCVAR_ARCHIVE );
	strcpy (rally_playlist, "playlist.pls");	// Set the default
	gEngfuncs.pfnAddCommand ("rally_mp3playlist", MP3Playlist);

	m_pCvarSkidfx = gEngfuncs.pfnRegisterVariable( "rally_skid", "1", FCVAR_ARCHIVE );
	m_pCvarShadowfx = gEngfuncs.pfnRegisterVariable( "rally_shadows", "4", FCVAR_ARCHIVE );
	m_pCvarShadowOpponentsfx = gEngfuncs.pfnRegisterVariable( "rally_opponentshadows", "1", FCVAR_ARCHIVE );
	m_pCvarSkidTime = gEngfuncs.pfnRegisterVariable( "rally_skidmarktime", "30", FCVAR_ARCHIVE );
	m_pCvarDustfx = gEngfuncs.pfnRegisterVariable( "rally_dust", "1", FCVAR_ARCHIVE );

	// FragMented!
	m_pCvarHPfx = gEngfuncs.pfnRegisterVariable( "rally_hpdisplay", "1", FCVAR_ARCHIVE );
	m_pCvarHPfx_r = gEngfuncs.pfnRegisterVariable( "rally_hp_glowshell_r", "1", FCVAR_ARCHIVE );
	m_pCvarHPfx_g = gEngfuncs.pfnRegisterVariable( "rally_hp_glowshell_g", "1", FCVAR_ARCHIVE );
	m_pCvarHPfx_b = gEngfuncs.pfnRegisterVariable( "rally_hp_glowshell_b", "1", FCVAR_ARCHIVE );

	// FragMented - RC7
	m_pCvarFogfx = gEngfuncs.pfnRegisterVariable( "rally_fog", "0", FCVAR_ARCHIVE );

	// SaRcaZm - V6 - Start
	m_pCvarManual = gEngfuncs.pfnRegisterVariable ("rally_manual", "0", FCVAR_ARCHIVE);
	// SaRcaZm - V6 - End
	// SaRcaZm - V7 - Start
	m_pCvarSparkfx = gEngfuncs.pfnRegisterVariable ("rally_sparks", "0", FCVAR_ARCHIVE);
	// SaRcaZm - V7 - End


//	vg_x->value = 60; vg_y->value = 5; vg_z->value = -15;

	// SaRcaZm - V5 - Start
	cl_2dcountdown = gEngfuncs.pfnRegisterVariable("cl_2dcountdown", "0", FCVAR_ARCHIVE);
	// SaRcaZm - V5 - End
	oldrpms = 10;

	// SaRcaZm - V8 - Start
	// Clear out the light list
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		m_pLightList[i].hBrightness = 0;
		m_pLightList[i].headlightOn = 0;
	}
	// SaRcaZm - V8 - End
}

void CRallyFX::Dust (void)
{
	// SaRcaZm - V6 - Start
	//========================================
	// MATERIAL SPITTING (FragMented, Crem)
	//========================================
//	m_pCvarDustfx = IEngineStudio.GetCvar( "rally_dust" );

	if (m_pCvarDustfx->value)
	{
		// Creme: Sprite Customisation vars
		int spr_rendermode;
		float spr_scale_mul = 1;
		float spr_alpha = 1;
		float spr_life = 1;

		vec3_t start, end;
		VectorCopy( m_vOrigin, start );
		VectorCopy( m_vOrigin, end );
		end[2] -= 64;

		char *Sprite = NULL;

		char *TextureName = (char *)gEngfuncs.pEventAPI->EV_TraceTexture( 0, start, end);

		if(TextureName)
		{
	//		gEngfuncs.Con_DPrintf(TextureName);
	//	gEngfuncs.Con_DPrintf("\n");


			// SaRcaZm, Frag, Creme
			// Oops, this needs to be case insensitive
			char *tmp = TextureName;
			while (*tmp)
			{
				*tmp = tolower (*tmp);
				tmp++;
			}

			if (strstr(TextureName, "dirt") || strstr(TextureName, "mud") || strstr(TextureName, "cliff")) {
				
				Sprite = "sprites/r_dirt.spr";
				spr_rendermode = kRenderTransAlpha;
				spr_alpha = 0.6;

			} else if (strstr(TextureName, "gras")) {

				Sprite = "sprites/r_grass.spr";
				spr_rendermode = kRenderTransAlpha;
				spr_scale_mul = 0.25;
				spr_alpha = 1;

			} else if(strstr(TextureName, "sand") || strstr(TextureName, "desert")) {

				Sprite = "sprites/r_sand.spr";
				spr_rendermode = kRenderTransAlpha;

			} else if(strstr(TextureName, "snow")) {

				if (pmove->RandomLong(0, 10) > 4) {
				
					if (pmove->RandomLong(0, 10) > 4) {
						Sprite = "sprites/r_dirt.spr";
					} else {
						Sprite = "sprites/r_snow.spr";
					}

					spr_rendermode = kRenderTransAlpha;
					spr_scale_mul = 0.0125;
					spr_life = 0.999;

				} else {
					Sprite = "sprites/r_smoke.spr";
					spr_rendermode = kRenderTransAlpha;
					spr_scale_mul = 1;
					spr_alpha = 0.25;
					spr_life = 0.999;
				}

			} else if (m_bIsSkidding) {
				Sprite = "sprites/r_smoke.spr";
				spr_rendermode = kRenderTransAlpha;
				spr_scale_mul = 0.65;
				spr_alpha = 0.4;
				if (g_tractionloss){ 
					spr_alpha = spr_alpha * g_tractionloss + 0.1; 
					if (spr_alpha > 0.4) spr_alpha = 0.4;
				}
				spr_life = 0.999;
			}

			if (Sprite)
			{
				vec3_t tempvec, origin_vec, vel_vec;
				float smoke_size;

				int m_iSmoke = gEngfuncs.pEventAPI->EV_FindModelIndex( Sprite );
				int m_iDust =  gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/r_rocks.spr" );

				if ((m_fSpeed > 30) && ((pmove->onground != -1) || !m_bLocalPlayer)) 
				{ 
					vec3_t dustpoint[4];
					int num_wheels;

					//VectorCopy (m_vOrigin, origin_vec);


					// Creme: Back
					dustpoint[0] = m_vOrigin + (up * 1.5) + (right * 9) - (forward * 19); 
					dustpoint[1] = m_vOrigin + (up * 1.5) - (right * 9) - (forward * 19); 

					// Creme: Front
					dustpoint[2] = m_vOrigin + (up * 1.5) + (right * 9) + (forward * 10); 
					dustpoint[3] = m_vOrigin + (up * 1.5) - (right * 9) + (forward * 10); 

					// Creme: Simple optimisations on other player cars.
					// only do 2 dusty wheels, and cut the spr_life.

					if ((!m_bLocalPlayer) || (m_bInteriorView)) { 
						num_wheels = 2;
						spr_life      *= 0.6;
						spr_scale_mul /= 0.6; // Scale it back up a bit. (same amount)

					} else {
						num_wheels = 4;
					}

					for (int i = 0; i < num_wheels; i++)
					{
						// Creme: Throw spr_life in a temp variable so it doesnt get wasted down.
						float fspr_life;

						// work out the start position of the sprite
						origin_vec = dustpoint[i];

						VectorCopy (m_vVelocity, vel_vec);
						// work out its random trajectory for the smoke sprite
						if (m_bLocalPlayer) {
							vel_vec[0] = (vel_vec[0] * 0.97) + pmove->RandomLong(m_fSpeed / -30, m_fSpeed / 30);
							vel_vec[1] = (vel_vec[1] * 0.97) + pmove->RandomLong(m_fSpeed / -30, m_fSpeed / 30);
							vel_vec[2] = (vel_vec[2] * 0.97) + pmove->RandomLong(0, m_fSpeed / 10);
						} else {
							vel_vec[0] = (vel_vec[0] * 0.85) + pmove->RandomLong(m_fSpeed / -20, m_fSpeed / 20);
							vel_vec[1] = (vel_vec[1] * 0.85) + pmove->RandomLong(m_fSpeed / -20, m_fSpeed / 20);
							vel_vec[2] = (vel_vec[2] * 0.85) + pmove->RandomLong(0, m_fSpeed / 15);
						}

						smoke_size = m_fSpeed * 0.002 * pmove->RandomFloat(0.9, 1.2);

						// Limit Smoke Size
						if(smoke_size < 0.05)
							smoke_size = 0.05;
						else if(smoke_size > 1.5)
							smoke_size = 1.5;


						if (i >= 2) {  // Creme: Front wheels dust lasts shorter
							fspr_life = spr_life * 0.75;
						} else {
							fspr_life = spr_life;
						}

						gEngfuncs.pEfxAPI->R_TempSprite( origin_vec, vel_vec, smoke_size * spr_scale_mul, 
							m_iSmoke, spr_rendermode, kRenderFxNone, spr_alpha, fspr_life, 
							FTENT_DUSTSPRITE | FTENT_SPRCYCLE);

					}
				}
			}
		}
	}
	// SaRcaZm - V6 - End
}




void CRallyFX::RenderSkidMarks (void)
{
	// Check to see if we have any skid marks to draw
	if (m_pSkidlist)
	{
		cl_entity_t *pThisPlayer = gEngfuncs.GetLocalPlayer();

		// SaRcaZm - V5 - Start
		// Check to see if the first skid mark has run out of time
		// These things are being created soo fast now we have to delete more / frame
		while (m_pSkidlist && m_pSkidlist->time && ((m_fCurrentTime > m_pSkidlist->time) || Length(m_pSkidlist->vertices[0] - pThisPlayer->origin) > 3000))
		//if (m_pSkidlist->time && (m_fCurrentTime > m_pSkidlist->time))
		{
			skidlist_t *tmp;
			tmp = m_pSkidlist->next;
			free (m_pSkidlist);
			m_pSkidlist = tmp;
		}
		// SaRcaZm - V5 - End

		//gEngfuncs.Con_DPrintf("%.2f\n", Length(m_pSkidlist->vertices[0] - m_pThisPlayer->origin));

		// SaRcaZm - V6 - Start
		// Now that skidmarks aren't all one texture, have to set up rendering for each individual one

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
			HSPRITE hsprTexture = LoadSprite( skidtex_names[skid->texture] ); // Load sprite, as normal;
			const model_s *pTexture = gEngfuncs.GetSpritePointer( hsprTexture ); // Get pointer to texture;

			gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)pTexture, 0 ); // Set texture for polys, second parameter is the frame number for animated sprites
			gEngfuncs.pTriAPI->Begin( TRI_QUADS ); // This defines what type of poly is being drawn. In this case it will be 4sided quads.

			for (int j = 0; j < 4; j++)
			{
				// Render the skid mark using the Triangle API (OpenGL anyone?)
				gEngfuncs.pTriAPI->TexCoord2f( tex_coords[j][0], tex_coords[j][1] ); // set 2d texture coordinate for vertex (from 0.0 to 1.0 across whole texture)
				gEngfuncs.pTriAPI->Vertex3f( skid->vertices[j].x, skid->vertices[j].y,  skid->vertices[j].z ); // set 3d position of vertex
			}

			// End rendering
			gEngfuncs.pTriAPI->End();
		}

		gEngfuncs.pTriAPI->CullFace( TRI_FRONT ); // FragMented!: re-enable backface culling

		// SaRcaZm - V6 - End
	}
}

void CRallyFX::RenderFog ( void ) {
/*	float g_fFogColor[4];
	g_fFogColor[0] = 128;
	g_fFogColor[1] = 96;
	g_fFogColor[2] = 64;*/

	//Not in water and we want fog.
//	bool bFog = g_iWaterLevel < 2 && g_fStartDist > 0 && g_fEndDist > 0;
	m_pCvarFogfx = IEngineStudio.GetCvar( "rally_fog" );

	if(m_pCvarFogfx->value)
		gEngfuncs.pTriAPI->Fog ( Vector(128, 112, 96), 256, 6000, 1);

}

#define X 0
#define Y 1
#define Z 2

// Creme: This routine simply updates the gear/rpm values. 
// The rendering is moved back to tri.cpp in DrawOrthoTris
void CRallyFX::RenderHUDNeedles (void)
{

	cl_entity_t *player;
	vec3_t vertice[4], vTemp, vAngles;
	vec3_t vForward, vRight, vUp;


	currentspeed = Length(pmove->velocity) * 3.25;

	player = gEngfuncs.GetLocalPlayer();
	if ( !player )
		return;

	int old_gear = gear;
	if(!m_pCvarManual->value)
	{
		// Creme: new calculated gear change setup
		float auto_speed;

		auto_speed = currentspeed / (gear * 2);

		//gEngfuncs.Con_DPrintf("    auto_speed:%f; currentspeed:%f; gear:%i\n", 
		//	auto_speed, currentspeed, gear);

		if (auto_speed > 190) {
			gear ++;
		} else if (auto_speed < 120) {
			
			// if still in 1st / 2nd do this check so it doesnt toggle
			if (currentspeed > 550) {
				gear--;
			} else {
				if (auto_speed < 80) {
					gear--;
				}
			}
		}

		if (gear > 6) {
			gear = 6;
		} else if (gear < 1) {
			if (currentspeed > 5) {
				gear = 1;
			} else {
				gear = 0;
			}
		} 
	}
	// Creme: throw in a backfire!
	if (old_gear > gear) {
		g_backfire = 2;
	}


	// Rotate the needle
	if ((currentspeed < 20) || (pmove->onground == -1) || !gear || g_tractionloss) {
		if (g_accelerating) {
			if (g_tractionloss) {
				//rpms += ((15 + (g_tractionloss * 300)) * pmove->frametime);
				rpms += ((15 + ((g_tractionloss/gear) * 300)) * pmove->frametime);
			} else {
				rpms += (1100 * pmove->frametime);
			}
		} else if (JoyMoved > 0.05) {
			rpms += ((1100*(JoyMoved)) * pmove->frametime);
		// SaRcaZm - V8 - Start
		} else if (g_reversing == 2) {	// 2 indicates the accelerator is pressed
			rpms += (1100 * pmove->frametime);
		// SaRcaZm - V8 - End
		} else {
			rpms -= (400 * pmove->frametime);
			rpms += (400 * JoyMoved) * pmove->frametime;
		}

	} else {
		
		//int ispd = currentspeed;
		//ispd = ispd % 125;
		//rpms = (ispd * 0.8);// + (currentspeed / 6);
		if (gear == -1)
			rpms = currentspeed;
		else {
			rpms = currentspeed / (gear * 2);
		}
	}	
	// SaRcaZm - V6 - End


	if (rpms < 10) {
		oldrpms = 10;
		rpms = 15;
	} else if (rpms > 250) {
		oldrpms = 250;
		rpms = 201;
	} else {

		// Average out the difference and work it out over frame time.

		float delta_max = 550;
		
		rpm_delta = (rpms - oldrpms) / pmove->frametime;

		if (rpm_delta > delta_max) {
			rpm_delta = delta_max;
		} else if (rpm_delta < -delta_max) {
			rpm_delta = -delta_max;
		}

		rpm_delta *= pmove->frametime;

		rpms = oldrpms + rpm_delta;

		rpms = (oldrpms + rpms*3) / 4;

//		gEngfuncs.Con_DPrintf("         rpm_delta: %f \n", 
//			rpm_delta);


	}

	if (rpms < 10) {
		rpms = 15;
	} else if (rpms > 250) {
		rpms = 250;
	}

	if (oldrpms > (rpms + 2))
		g_accelerating = 2;
	else
		g_accelerating = 0;

	oldrpms = rpms;

}


static float SkidMarkSoundTime = 0;

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

// CalcSkidMarks by: Sarcazm
void CRallyFX::CalcSkidMarks (void)
{
	if (m_fSpeed < 20)
		return;

	//	m_pCvarSkidfx = IEngineStudio.GetCvar( "rally_skid" );
	if (m_pCvarSkidfx->value)
	{
		vec3_t dist_vec;
		static skidlist_t *lastskid = NULL;
		vec3_t last_pos, last_right;

		// Find the distance between the current point and the previous point
		dist_vec = m_pCurrentPers->skid.last_pos[0] - m_vCorners[BACK_RIGHT];

		// Only draw if we have moved far enough, we have skidmarks left to use
		// SaRcaZm - V6 - Start
		// Increase the distance to 20 so we don't get too many drawn at low speed
		if ((dist_vec.Length () > 15) || (m_pCurrentPers->skid.bFirstTime))
		// SaRcaZm - V6 - End
		{
			pmtrace_t tr;
			vec3_t rightside_vec, leftside_vec, fr_rightside_vec, fr_leftside_vec, temp_vec, temp_right;

			// Check to see what angle the car is on in relation to the view
			//temp_vec = m_vNormal - forward;
			vec3_t vel_vec = m_vVelocity;
			VectorNormalize (vel_vec);
			temp_vec = vel_vec - forward;

			// Rear skid points

			// Trace down from above the right side of the car to find the height of the ground there
			rightside_vec = m_vCorners[BACK_RIGHT] + (forward * SKID_LENGTH_OFS) + (right * -SKID_WIDTH_OFS) + Vector (0, 0, TRACE_HEIGHT);
			gEngfuncs.pEventAPI->EV_PlayerTrace( rightside_vec, rightside_vec + Vector (0, 0, -TRACE_LENGTH), PM_STUDIO_BOX, -1, &tr );
			rightside_vec = tr.endpos;

			// Trace down from above the left side of the car to find the height of the ground there
			leftside_vec = m_vCorners[BACK_LEFT] + (forward * SKID_LENGTH_OFS) + (right * SKID_WIDTH_OFS) + Vector (0, 0, TRACE_HEIGHT);
			gEngfuncs.pEventAPI->EV_PlayerTrace( leftside_vec, leftside_vec + Vector (0, 0, -TRACE_LENGTH), PM_STUDIO_BOX, -1, &tr );
			leftside_vec = tr.endpos;

			// Front skid points

			// Trace down from above the front right side of the car to find the height of the ground there
			fr_rightside_vec = m_vCorners[FRONT_RIGHT] + (forward * -SKID_LENGTH_OFS * 1.2) + (right * -SKID_WIDTH_OFS) + Vector (0, 0, TRACE_HEIGHT);
			gEngfuncs.pEventAPI->EV_PlayerTrace( fr_rightside_vec, fr_rightside_vec + Vector (0, 0, -TRACE_LENGTH), PM_STUDIO_BOX, -1, &tr );
			fr_rightside_vec = tr.endpos;

			// Trace down from above the front left side of the car to find the height of the ground there
			fr_leftside_vec = m_vCorners[FRONT_LEFT] + (forward * -SKID_LENGTH_OFS * 1.2) + (right * SKID_WIDTH_OFS) + Vector (0, 0, TRACE_HEIGHT);
			gEngfuncs.pEventAPI->EV_PlayerTrace( fr_leftside_vec, fr_leftside_vec + Vector (0, 0, -TRACE_LENGTH), PM_STUDIO_BOX, -1, &tr );
			fr_leftside_vec = tr.endpos;

			// Save the angle of the car for analysis later in input.cpp
			if (m_bLocalPlayer)
				rallyangle = temp_vec[YAW];

			// SaRcaZm - V8 - Start
			// Fix the skidmark widths
			vec3_t r;
			/*float angle = acos (DotProduct (forward, m_vNormal));
			angle = ( angle  * 180.0f ) / M_PI;
			if (angle > 23 && angle < 35)
				AngleVectors (vel_vec, NULL, r, NULL);
			else
				VectorCopy (right, r);

			// Change the right vector to allow for the slope of the ground
			if ((temp_vec[YAW] > 0.4) || (temp_vec[YAW] < -0.4))
			{
				CrossProduct (r, tr.plane.normal, temp_right);
				VectorNormalize (temp_right);
			}
			else
				VectorCopy (r, temp_right);*/
			vectoangles (pmove->velocity, r);
			CrossProduct (r, tr.plane.normal, temp_right);
			VectorNormalize (temp_right);
			// SaRcaZm - V8 - End

			// If it was part of the map, draw a skid mark on it
			// BUT only if: 1) We are on the ground;
			//				2) This is not the first time through this function; AND
			//				3) We are going slow enough; AND
			//					a) We are accelerating; OR
			//					b) We are braking; OR
			//				4) We are on a big enough angle to the camera i.e sliding enough
			//				5) We arent a local reversing player
			// creme        6) Or handbraking and speed > 25
			// Enough brackets for you?

			// SaRcaZm - V6 - Start
			// Both the current and the previous positions must be on the ground
			// SaRcaZm - V5 - Start
			m_bIsSkidding = false;
			if 
			(
				m_bLocalPlayer && 
				m_bIsOnGround && 
				!m_pCurrentPers->skid.bFirstTime && 
				m_pCurrentPers->skid.bOnGround &&
				(gear != 0) &&

				(
					(
						(m_fSpeed > 25) && (m_fSpeed < 200) && 
						(
							g_accelerating || g_braking 
						) || (m_fSpeed > 25) && (g_handbraking || (g_tractionloss))
					) ||
					(
						(temp_vec[YAW] > 0.45) || (temp_vec[YAW] < -0.45)
					) &&
					( !g_reversing ) 
				)
			)

			{
				// SaRcaZm - V6 - Start
				// Figure out which skidmark to draw
				int skidtex_index = SKIDTEX_NONE;
				vec3_t start, end;

				m_bIsSkidding = true;
				VectorCopy( m_vOrigin, start );
				VectorCopy( m_vOrigin, end );
				end[2] -= 64;
				char *TextureName = (char *)gEngfuncs.pEventAPI->EV_TraceTexture( 0, start, end);

				if(TextureName)
				{
					// Convert it to lower case
					char *tmp = TextureName;
					while (*tmp)
					{
						*tmp = tolower (*tmp);
						tmp++;
					}

				//	gEngfuncs.Con_DPrintf(TextureName);

					// FragMented -- RC7

					// SaRcaZm - V7 - Start
					// noskid texture prevents skidmarks from being displayed....
					if (strstr (TextureName, "noskid"))
						return;
					// I dunno whether these were typos or not...
					else if (/*!*/strstr(TextureName, "roadgras") /*&&*/ || (strstr(TextureName, "dirt") || strstr(TextureName, "gras") || strstr(TextureName, "mud") || strstr(TextureName, "soil") || strstr(TextureName, "3dmroad")))
					// SaRcaZm - V7 - End
						skidtex_index = SKIDTEX_DIRT;
					else if(strstr(TextureName, "sand") || strstr(TextureName, "desert"))
						skidtex_index = SKIDTEX_SAND;
					else if(strstr(TextureName, "snow"))
						skidtex_index = SKIDTEX_SNOW;
					else
					{
						if (SkidMarkSoundTime < m_fCurrentTime)
						{
							if ((m_fSpeed > 60) || (temp_vec[YAW] > 0.15) || (temp_vec[YAW] < -0.15))
							{

								RallySound->fmodPlay3D ("skid_tarmac.wav", false, 0, m_vOrigin, m_vVelocity);
								SkidMarkSoundTime = m_fCurrentTime + 0.5;

							}
						}
						// SaRcaZm - V8 - Start
						skidtex_index = SKIDTEX_NONE;
						// Dunno what this does, but it's stopping skidmarks on tarmac
						//if (g_tractionloss < 1)
						//	goto endif;
						// SaRcaZm - V8 - End
					}

				}
				// SaRcaZm - V6 - End

				skidlist_t *skid;
//				m_pCvarSkidTime = IEngineStudio.GetCvar( "rally_skidmarktime" );

				// First skid mark
				skid = (skidlist_t *) malloc (sizeof (skidlist_t));
				memset (skid, 0, sizeof (*skid));

				// First vertex
				skid->vertices[0] = rightside_vec; // + (temp_right * 2);
				// Second vertex
				skid->vertices[1] = m_pCurrentPers->skid.last_pos[0]; // + (m_pCurrentPers->skid.last_right * 2);
				// Third vertex
				skid->vertices[2] = m_pCurrentPers->skid.last_pos[0] + (m_pCurrentPers->skid.last_right * -4);
				// Fourth vertex
				skid->vertices[3] = rightside_vec + (temp_right * -4);

				// Make it disappear after rally_skidmarktime seconds
				if (m_pCvarSkidTime->value)
					skid->time = m_fCurrentTime + m_pCvarSkidTime->value;

				// SaRcaZm - V6 - Start
				skid->texture = skidtex_index;
				// SaRcaZm - V6 - End

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
				skid->vertices[1] = m_pCurrentPers->skid.last_pos[1]; // + (m_pCurrentPers->skid.last_right * -2);
				// Third vertex
				skid->vertices[2] = m_pCurrentPers->skid.last_pos[1] + (m_pCurrentPers->skid.last_right * 4);
				// Fourth vertex
				skid->vertices[3] = leftside_vec + (temp_right * 4);
				// Make it disappear after rally_skidmarktime seconds
				if (m_pCvarSkidTime->value)
					skid->time = m_fCurrentTime + m_pCvarSkidTime->value;

				// SaRcaZm - V6 - Start
				skid->texture = skidtex_index;
				// SaRcaZm - V6 - End

				// Add it to the linked list
				lastskid->next = skid;
				lastskid = lastskid->next;

				// Third skid mark
				skid = (skidlist_t *) malloc (sizeof (skidlist_t));
				memset (skid, 0, sizeof (*skid));

				// First vertex
				skid->vertices[0] = fr_rightside_vec; // + (temp_right * 2);
				// Second vertex
				skid->vertices[1] = m_pCurrentPers->skid.last_pos[2]; // + (m_pCurrentPers->skid.last_right * 2);
				// Third vertex
				skid->vertices[2] = m_pCurrentPers->skid.last_pos[2] + (m_pCurrentPers->skid.last_right * -4);
				// Fourth vertex
				skid->vertices[3] = fr_rightside_vec + (temp_right * -4);

				// Make it disappear after rally_skidmarktime seconds
				if (m_pCvarSkidTime->value)
					skid->time = m_fCurrentTime + m_pCvarSkidTime->value;

				// SaRcaZm - V6 - Start
				skid->texture = skidtex_index;
				// SaRcaZm - V6 - End

				// Add it to the linked list
				lastskid->next = skid;
				lastskid = lastskid->next;

				// Fourth skid mark
				skid = (skidlist_t *) malloc (sizeof (skidlist_t));
				memset (skid, 0, sizeof (*skid));

				// First vertex
				skid->vertices[0] = fr_leftside_vec; // + (temp_right * -2);
				// Second vertex
				skid->vertices[1] = m_pCurrentPers->skid.last_pos[3]; // + (m_pCurrentPers->skid.last_right * -2);
				// Third vertex
				skid->vertices[2] = m_pCurrentPers->skid.last_pos[3] + (m_pCurrentPers->skid.last_right * 4);
				// Fourth vertex
				skid->vertices[3] = fr_leftside_vec + (temp_right * 4);

				// SaRcaZm - V6 - Start
				skid->texture = skidtex_index;
				// SaRcaZm - V6 - End

				// Make it disappear after rally_skidmarktime seconds
				if (m_pCvarSkidTime->value)
					skid->time = m_fCurrentTime + m_pCvarSkidTime->value;

				// Add it to the linked list
				lastskid->next = skid;
				lastskid = lastskid->next;
			}
			else {
				m_pCurrentPers->skid.bFirstTime = false;
//				if (g_tractionloss) {
//					RallySound->fmodPlay("skid_tarmac.wav", false, 0);
//				}


			}

//			endif:

			// Save the current position as the last position
			m_pCurrentPers->skid.last_pos[0] = rightside_vec;
			m_pCurrentPers->skid.last_pos[1] = leftside_vec;
			m_pCurrentPers->skid.last_pos[2] = fr_rightside_vec;
			m_pCurrentPers->skid.last_pos[3] = fr_leftside_vec;
			m_pCurrentPers->skid.last_right = temp_right;

			// SaRcaZm - V6 - Start
			m_pCurrentPers->skid.bOnGround = m_bIsOnGround;
			// SaRcaZm - V6 - End
		}

		// Render this as part of HUD_DrawTransparentTriangles
		//RenderSkidMarks ();
	}
}

void CRallyFX::Shadows (void)
{
	pmtrace_t tr;
	// Trace up to see if we are inside
/*	gEngfuncs.pEventAPI->EV_PlayerTrace( m_vOrigin, m_vOrigin + Vector (0, 0, 2056), PM_STUDIO_BOX, -1, &tr );
	// Move the endpos up and check the contents of that point
	tr.endpos[2]++;
	int contents = gEngfuncs.PM_PointContents( tr.endpos, NULL );*/

	cl_entity_t *pThisPlayer = gEngfuncs.GetLocalPlayer();

	// SaRcaZm - V8 - Start
	if(!pThisPlayer || g_iUser1) /// FragMented! HL Rally Public Beta 1.1
		return;

	// Level Change Crash Fix
		if (!pThisPlayer->model )// || ((const int) m_pThisPlayer->model == (const int) m_pThisPlayer->model->name))
			return;

		// Don't draw if we can't see the player
		if ((pThisPlayer->curstate.effects & EF_NODRAW) || (!strcmpi("models/interior.mdl", pThisPlayer->model->name)))
			return;

	// SaRcaZm - V8 - End

	gEngfuncs.pEventAPI->EV_PushPMStates();
//	gEngfuncs.pEventAPI->EV_SetSolidPlayers ( -1 );		// This doesn't seem to do anything
	gEngfuncs.pEventAPI->EV_SetTraceHull( 0 );
	// SaRcaZm - V7 - End

	AngleVectors (vLocalAngles, forward, right, up);
	// Creme: set the corners to this player.
/*	m_vCorners[FRONT_LEFT]  = m_pThisPlayer->origin + (forward * 25) + (right * -11);
	m_vCorners[BACK_LEFT]   = m_pThisPlayer->origin + (forward * -25) + (right * -11);
	m_vCorners[BACK_RIGHT]  = m_pThisPlayer->origin + (forward * -22) + (right * 11);
	m_vCorners[FRONT_RIGHT] = m_pThisPlayer->origin + (forward * 22) + (right * 11);
*/
	// No shadows when inside, so check to see what our trace hit
	if (m_pCvarShadowfx->value) // && ((contents == CONTENTS_EMPTY) || (contents == CONTENTS_SKY)))
	{
		HSPRITE hsprTexture = LoadSprite( "sprites/r_shadow.spr" ); // Load sprite, as normal;
		const model_s *pTexture = gEngfuncs.GetSpritePointer( hsprTexture ); // Get pointer to texture;
		vec3_t corners[4];

		// Determine the corners of the shadow
		corners[0] = m_vCorners[0];
		corners[1] = m_vCorners[1];
		corners[2] = m_vCorners[2];
		corners[3] = m_vCorners[3];

		// If we are off the ground, shrink the shadow according to the height of the ground
		if (pmove->onground == -1)
		{
			vec3_t origin;
			float height;

			// We must shrink the size of the shadow based on the height above the ground
			// Trace down from the origin to get the distance to the ground
			origin = pThisPlayer->origin + Vector (0, 0, TRACE_HEIGHT);
			gEngfuncs.pEventAPI->EV_PlayerTrace( origin, origin + Vector (0, 0, -96), PM_STUDIO_BOX | PM_WORLD_ONLY, -1, &tr );
			height = (pThisPlayer->origin - tr.endpos).Length();

			// Move all of the points towards the centre of the car
			// Move forward / back by (height / 5.0f) and left / right by (height / 10.0f)
			corners[0] = corners[0] + (forward * -(height / 5.0f)) + (right *  (height / 10.0f));
			corners[1] = corners[1] + (forward *  (height / 5.0f)) + (right *  (height / 10.0f));
			corners[2] = corners[2] + (forward *  (height / 5.0f)) + (right * -(height / 10.0f));
			corners[3] = corners[3] + (forward * -(height / 5.0f)) + (right * -(height / 10.0f));
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
		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)pTexture, 0 );
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
			vertex_top = corners[0] + (forward * -(inc_length * i)) + Vector (0, 0, TRACE_HEIGHT);
			gEngfuncs.pEventAPI->EV_PlayerTrace( vertex_top, vertex_top + Vector (0, 0, -2056), PM_STUDIO_BOX | PM_WORLD_ONLY, -1, &tr );
			vertex_top = tr.endpos;
		//	gEngfuncs.pTriAPI->Brightness( 1 );
			gEngfuncs.pTriAPI->TexCoord2f( 0, tex_top );
			gEngfuncs.pTriAPI->Vertex3f( vertex_top.x, vertex_top.y, vertex_top.z );

			// Bottom left vertex in the line
			vertex_bottom = corners[0] + (forward * -(inc_length * (i + 1))) + Vector (0, 0, TRACE_HEIGHT);
			gEngfuncs.pEventAPI->EV_PlayerTrace( vertex_bottom, vertex_bottom + Vector (0, 0, -2056), PM_STUDIO_BOX | PM_WORLD_ONLY, -1, &tr );
			vertex_bottom = tr.endpos;
			//gEngfuncs.pTriAPI->Brightness( 1 );
			gEngfuncs.pTriAPI->TexCoord2f( 0, tex_bottom );
			gEngfuncs.pTriAPI->Vertex3f( vertex_bottom.x, vertex_bottom.y, vertex_bottom.z );

			// Now, loop through and draw the rest of the vertices in the line
			for (int j = 1; j <= m_pCvarShadowfx->value; j++)
			{
				float tex_width = (1.0f / m_pCvarShadowfx->value) * j;
				vec3_t vertex;

				// Next top point
				vertex = vertex_top + (right * (inc_width * j)) + Vector (0, 0, TRACE_HEIGHT);
				gEngfuncs.pEventAPI->EV_PlayerTrace( vertex, vertex + Vector (0, 0, -2056), PM_STUDIO_BOX | PM_WORLD_ONLY, -1, &tr );
				//gEngfuncs.pTriAPI->Brightness( 1 );
				gEngfuncs.pTriAPI->TexCoord2f( tex_width, tex_top );
				gEngfuncs.pTriAPI->Vertex3f( tr.endpos.x, tr.endpos.y, tr.endpos.z );

				// Next bottom point
				vertex = vertex_bottom + (right * (inc_width * j)) + Vector (0, 0, TRACE_HEIGHT);
				gEngfuncs.pEventAPI->EV_PlayerTrace( vertex, vertex + Vector (0, 0, -2056), PM_STUDIO_BOX | PM_WORLD_ONLY, -1, &tr );
				//gEngfuncs.pTriAPI->Brightness( 1 );
				gEngfuncs.pTriAPI->TexCoord2f( tex_width, tex_bottom );
				gEngfuncs.pTriAPI->Vertex3f( tr.endpos.x, tr.endpos.y, tr.endpos.z );
			}

			// End rendering
			gEngfuncs.pTriAPI->End();
		}

		gEngfuncs.pTriAPI->CullFace( TRI_FRONT ); // FragMented!: re-enable backface culling
	}

		// SaRcaZm - V7 - Start
	gEngfuncs.pEventAPI->EV_PopPMStates();
	// SaRcaZm - V7 - End
}



void FlamePoly (vec3_t vertice0, vec3_t vertice1, vec3_t vertice2, vec3_t vertice3)
{

	gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
	gEngfuncs.pTriAPI->Vertex3f( vertice0[X], vertice0[Y], vertice0[Z]);

	gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
	gEngfuncs.pTriAPI->Vertex3f( vertice1[X], vertice1[Y], vertice1[Z]);

	gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
	gEngfuncs.pTriAPI->Vertex3f( vertice2[X], vertice2[Y], vertice2[Z]);

	gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
	gEngfuncs.pTriAPI->Vertex3f( vertice3[X], vertice3[Y], vertice3[Z]);
}


//========================================
// Exhaust Flame
//========================================
static float m_fBackFireTime = 0;
#define BACKFIRETIME 0.15

void CRallyFX::Exhaust (void)
{
	vec3_t vertice[4];

	if(g_backfire && m_bLocalPlayer)
	{
		// First time enters
		if (g_backfire != -2) {
//			gEngfuncs.Con_DPrintf ("*** Fired! %i  ***", g_backfire);
			// Check to make sure we didnt just play this thing
			if (m_fBackFireTime < gEngfuncs.GetClientTime ()) {
				RallySound->fmodPlay3D ("backfire.wav", false, 0, m_vOrigin, m_vVelocity);
			}
			m_fBackFireTime = gEngfuncs.GetClientTime () + BACKFIRETIME;
			g_backfire = -2;
		}

		if (m_fBackFireTime > gEngfuncs.GetClientTime () ) {
		
			//gEngfuncs.pEfxAPI->R_MuzzleFlash( m_pCurrentEntity->attachment[0], 8 );

			vec3_t Ofs;

			// add some random to swap the verts around
			int i;;

			float fi;
			i = fi = (m_fBackFireTime - gEngfuncs.GetClientTime()) / BACKFIRETIME * 4; 

//			gEngfuncs.Con_DPrintf ("gEngfuncs.GetClientTime (): %f       fi: %f  i: %i; \n", gEngfuncs.GetClientTime (), fi, i);

			if (i > 3) i = 3; else if (i < 0) i = 0;

			// the resulting random vert
			int j = i;
			int k;

/*			Ofs = (up * -2) + (right * -2);

			j = i;
			vertice[j] = m_vBackFireOrigin + Ofs;
			if ((i + 3) > 3) { j = (i + 3) - 4; } else { j = i + 3; }
			vertice[j] = m_vBackFireOrigin + -Ofs;

			Ofs = (up * -2) + (right * 2);
			if ((i + 1) > 3) { j = (i + 1) - 4; } else { j = i + 1; }
			vertice[j] = m_vBackFireOrigin + Ofs;
			if ((i + 2) > 3) { j = (i + 2) - 4; } else { j = i + 2; }
			vertice[j] = m_vBackFireOrigin + -Ofs;
*/

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
			vertice[j] = m_vBackFireOrigin + (right * -2) + (up * -2);
			k = 1;
			if ((i + k) > 3) { j = (i + k) - 4; } else { j = i + k; }
			vertice[j] = m_vBackFireOrigin + (right * 2) + (up * -2);
			k = 2;
			if ((i + k) > 3) { j = (i + k) - 4; } else { j = i + k; }
			vertice[j] = m_vBackFireOrigin + (right * 2) + (up * 2);
			k = 3;
			if ((i + k) > 3) { j = (i + k) - 4; } else { j = i + k; }
			vertice[j] = m_vBackFireOrigin + (right * -2) + (up * 2);

			FlamePoly (vertice[0], vertice[1], vertice[2], vertice[3]);

//			i = 0;
			k = 0;
			if ((i + k) > 3) { j = (i + k) - 4; } else { j = i + k; }
			vertice[j] = m_vBackFireOrigin + (right * -2) + (up * -2) + (forward * 3);
			k = 1;
			if ((i + k) > 3) { j = (i + k) - 4; } else { j = i + k; }
			vertice[j] = m_vBackFireOrigin + (right * -2) + (up * -2) + (forward * -5);
			k = 2;
			if ((i + k) > 3) { j = (i + k) - 4; } else { j = i + k; }
			vertice[j] = m_vBackFireOrigin + (right * 2) + (up * 2) + (forward * -5);
			k = 3;
			if ((i + k) > 3) { j = (i + k) - 4; } else { j = i + k; }
			vertice[j] = m_vBackFireOrigin + (right * 2) + (up * 2) + (forward * 3);

			FlamePoly (vertice[0], vertice[1], vertice[2], vertice[3]);

			k = 0;
			if ((i + k) > 3) { j = (i + k) - 4; } else { j = i + k; }
			vertice[j] = m_vBackFireOrigin + (right * -2) + (up * 2) + (forward * 3);
			k = 1;
			if ((i + k) > 3) { j = (i + k) - 4; } else { j = i + k; }
			vertice[j] = m_vBackFireOrigin + (right * -2) + (up * 2) + (forward * -5);
			k = 2;
			if ((i + k) > 3) { j = (i + k) - 4; } else { j = i + k; }
			vertice[j] = m_vBackFireOrigin + (right * 2) + (up * -2) + (forward * -5);
			k = 3;
			if ((i + k) > 3) { j = (i + k) - 4; } else { j = i + k; }
			vertice[j] = m_vBackFireOrigin + (right * 2) + (up * -2) + (forward * 3);

			FlamePoly (vertice[0], vertice[1], vertice[2], vertice[3]);

			gEngfuncs.pTriAPI->End();
			gEngfuncs.pTriAPI->RenderMode( kRenderNormal ); // kRenderTransTexture





		} else {
			// We are done, clear the variable
			g_backfire = 0;
		}
	}
}

void CRallyFX::HP (void)
{
	// HP_ weapon model
	if(m_pCurrentEntity->curstate.weaponmodel == gEngfuncs.pEventAPI->EV_FindModelIndex( "models/hp_bomb.mdl"))
	{
		m_pCvarHPfx = IEngineStudio.GetCvar( "rally_hpdisplay" );

		// GlowShell For these modes
		if(m_pCvarHPfx->value == 0 || m_pCvarHPfx->value == 1 || m_pCvarHPfx->value == 2)
		{
			m_pCvarHPfx_r = IEngineStudio.GetCvar( "rally_hp_glowshell_r" );
			m_pCvarHPfx_g = IEngineStudio.GetCvar( "rally_hp_glowshell_g" );
			m_pCvarHPfx_b = IEngineStudio.GetCvar( "rally_hp_glowshell_b" );

			m_pCurrentEntity->curstate.renderfx = kRenderFxGlowShell;
			m_pCurrentEntity->curstate.rendercolor.r = m_pCvarHPfx_r->value;
			m_pCurrentEntity->curstate.rendercolor.g = m_pCvarHPfx_g->value;
			m_pCurrentEntity->curstate.rendercolor.b = m_pCvarHPfx_b->value;
			m_pCurrentEntity->curstate.renderamt = 0;
		}

		// SaRcaZm - V7 - Start
		// This should be in the if
		// Draw a sprite for these modes
		if(m_pCvarHPfx->value == 1 || m_pCvarHPfx->value == 3)
		{
//			int SpriteIndex =  gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/hp_bomb.spr" );
//			gEngfuncs.pEfxAPI->R_DefaultSprite ( Vector(m_pCurrentEntity->curstate.origin[0], m_pCurrentEntity->curstate.origin[2], m_pCurrentEntity->curstate.origin[3] -15) , SpriteIndex, 0 );
//			gEngfuncs.pEfxAPI->R_DefaultSprite ( Vector(m_vOrigin[0], m_vOrigin[2], m_vOrigin[3] - 15) , SpriteIndex, 0 );
		}
		// SaRcaZm - V7 - End
	}
}

// SaRcaZm - V5 - Start
void CRallyFX::Lights(vec3_t angles)
{
	VectorCopy(m_vOrigin, m_pLightList[m_pCurrentEntity->index].origin);
	VectorCopy(angles, m_pLightList[m_pCurrentEntity->index].angles);

	// For testing purposes only
	//m_pLightList[m_pCurrentEntity->index].headlightOn = true;

	if(m_bLocalPlayer)
	{
		m_pLightList[m_pCurrentEntity->index].isBrake = g_braking;
		m_pLightList[m_pCurrentEntity->index].isReverse = g_reversing;

		if(m_fSpeed < 10)
		{
			m_pLightList[m_pCurrentEntity->index].isBrake = true;
			m_pLightList[m_pCurrentEntity->index].isReverse = false;
		}
	}
	else
	{
		if (m_fSpeed < m_pLightList[m_pCurrentEntity->index].prevSpeed * 0.98)
			m_pLightList[m_pCurrentEntity->index].isBrake = true;
		else
			m_pLightList[m_pCurrentEntity->index].isBrake = false;

		if (m_fSpeed < 10)
		{
			m_pLightList[m_pCurrentEntity->index].isBrake = true;
		}

		m_pLightList[m_pCurrentEntity->index].isReverse = false; // Temporary

		m_pLightList[m_pCurrentEntity->index].prevSpeed = m_fSpeed;
	}

	for(int j = 0; j < 13; j++)
	{
		if(!strcmpi(g_PlayerExtraInfo[m_pCurrentEntity->index].teamname, rallyinfo[j].szTeamName))
		{
			m_pLightList[m_pCurrentEntity->index].teamidx = j;
		}
	}

	if(g_PlayerInfoList[m_pCurrentEntity->index].model)
	{
		for(j = 0; j < 3; j++)
		{
			if(!strcmpi(g_PlayerInfoList[m_pCurrentEntity->index].model, rallyinfo[m_pLightList[m_pCurrentEntity->index].teamidx].szModels[j].szModelName))
			{
				m_pLightList[m_pCurrentEntity->index].modelidx = j;
			}
		}
	}

	// SaRcaZm - V6 - Start
	// Update from Frag
	m_pLightList[m_pCurrentEntity->index].lastupdate = gEngfuncs.GetClientTime();
	// SaRcaZm - V6 - End
}

void CRallyFX::RenderLights(void)
{
	vec3_t		vForward, vRight, vUp, vTemp, vOfs, org;

	for (int i = 0; i <= MAX_PLAYERS; i++)
	{
		// SaRcaZm - V8 - Start
		// Error check the headlightOn variable
		if ((m_pLightList[i].headlightOn < 0) || (m_pLightList[i].headlightOn > 1))
			m_pLightList[i].headlightOn = 0;
		// SaRcaZm - V8 - End

		// SaRcaZm - V6 - Start
		// Update from Frag
		if((gEngfuncs.GetClientTime() < m_pLightList[i].lastupdate + pmove->frametime*2)
			&& (m_pLightList[i].isBrake || m_pLightList[i].isReverse || m_pLightList[i].bBrightness))
		// SaRcaZm - V6 - End
		{
			cl_entity_t *local = gEngfuncs.GetLocalPlayer();

			if(i == local->index)
			{
				m_pLightList[i].isBrake = g_braking;
				m_pLightList[i].isReverse = g_reversing;
			}

			if(!(m_pLightList[i].isBrake || m_pLightList[i].isReverse)) {
				m_pLightList[i].bBrightness = m_pLightList[i].bBrightness - 0.08;
			}
			else {
				m_pLightList[i].bBrightness = m_pLightList[i].bBrightness + 0.06;
			}

			if(m_pLightList[i].bBrightness > .6)
				m_pLightList[i].bBrightness = .6;
			else if (m_pLightList[i].bBrightness < 0)
				m_pLightList[i].bBrightness = 0;

			if(m_pLightList[i].headlightOn)
			{
				if(m_pLightList[i].bBrightness < .2)
				{
					m_pLightList[i].bBrightness = .2;
					m_pLightList[i].wasBrake = true;
				}
			}

			if(m_pLightList[i].isBrake)
				m_pLightList[i].wasBrake = true;
			else if (m_pLightList[i].isReverse)
				m_pLightList[i].wasBrake = false;



			AngleVectors(m_pLightList[i].angles, vForward, vRight, vUp);

			if (m_pLightList[i].wasBrake) {
					VectorScale(vRight, rallyinfo[m_pLightList[i].teamidx].szModels[m_pLightList[i].modelidx].fBrakeLightWidth, vRight);
			}
			else {
				VectorScale(vRight, rallyinfo[m_pLightList[i].teamidx].szModels[m_pLightList[i].modelidx].fBrakeLightWidth*.92, vRight);
			}

		//	gEngfuncs.Con_DPrintf("%i %i\n", teamidx, modelidx);

			VectorScale(vForward, -rallyinfo[m_pLightList[i].teamidx].szModels[m_pLightList[i].modelidx].fBrakeLightDst, vForward);
			VectorScale(vUp, rallyinfo[m_pLightList[i].teamidx].szModels[m_pLightList[i].modelidx].fBrakeLightHeight, vUp);

			VectorCopy(m_pLightList[i].origin, org);
			VectorAdd(org, vRight, org);
			VectorAdd(org, vForward, org);
			VectorAdd(org, vUp, org);


			if ( !gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer( SPR_Load("sprites/flare3.spr") ), 0 ))
				return;

			gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd ); // kRenderTransTexture
			gEngfuncs.pTriAPI->CullFace( TRI_NONE );
			gEngfuncs.pTriAPI->Begin( TRI_QUADS );

			if(m_pLightList[i].wasBrake) {
				gEngfuncs.pTriAPI->Color4f( 1, .1, .1, m_pLightList[i].bBrightness );
			}
			else {
				gEngfuncs.pTriAPI->Color4f( .9, .9, 1, m_pLightList[i].bBrightness );
			}

			gEngfuncs.pTriAPI->Brightness( 1 );
			gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
			gEngfuncs.pTriAPI->Vertex3f( org[0], org[1], org[2]);

			VectorSubtract(org, vUp, org);
			VectorScale(vUp, .8, vTemp);
			VectorAdd(org, vTemp, org);

			gEngfuncs.pTriAPI->Brightness( 1 );
			gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
			gEngfuncs.pTriAPI->Vertex3f( org[0], org[1], org[2]);

			VectorScale(vRight, 1.4, vTemp);

			VectorCopy(m_pLightList[i].origin, org);
			VectorAdd(org, vTemp, org);
			VectorAdd(org, vForward, org);
			VectorScale(vUp, .8, vTemp);
			VectorAdd(org, vTemp, org);

			gEngfuncs.pTriAPI->Brightness( 1 );
			gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
			gEngfuncs.pTriAPI->Vertex3f( org[0], org[1], org[2]);

			VectorCopy(m_pLightList[i].origin, org);
			VectorScale(vRight, 1.4, vTemp);
			VectorAdd(org, vTemp, org);
			VectorAdd(org, vForward, org);
			VectorAdd(org, vUp, org);

			gEngfuncs.pTriAPI->Brightness( 1 );
			gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
			gEngfuncs.pTriAPI->Vertex3f( org[0], org[1], org[2]);

			gEngfuncs.pTriAPI->End();

			// Create a entity light to light up the Car
			dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocElight (i*2);
			VectorCopy (org, dl->origin);
			dl->radius = 2;
			if(m_pLightList[i].wasBrake)
			{
				dl->color.r = m_pLightList[i].bBrightness * 240;
				dl->color.g = 2;
				dl->color.b = 2;
			}
			else
			{
				dl->color.r = m_pLightList[i].bBrightness * 200;
				dl->color.g = m_pLightList[i].bBrightness * 200;
				dl->color.b = m_pLightList[i].bBrightness * 200;
			}
			dl->die = gEngfuncs.GetClientTime () + 0.1;


			VectorCopy(m_pLightList[i].origin, org);
			VectorAdd(org, -vRight, org);
			VectorAdd(org, vForward, org);
			VectorAdd(org, vUp, org);

			gEngfuncs.pTriAPI->Begin( TRI_QUADS );

			if(m_pLightList[i].wasBrake)
				gEngfuncs.pTriAPI->Color4f( 1, .1, .1, m_pLightList[i].bBrightness );
			else
				gEngfuncs.pTriAPI->Color4f( .9, .9, 1, m_pLightList[i].bBrightness );

			gEngfuncs.pTriAPI->Brightness( 1 );
			gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
			gEngfuncs.pTriAPI->Vertex3f( org[0], org[1], org[2]);

			VectorSubtract(org, vUp, org);
			VectorScale(vUp, .8, vTemp);
			VectorAdd(org, vTemp, org);

			gEngfuncs.pTriAPI->Brightness( 1 );
			gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
			gEngfuncs.pTriAPI->Vertex3f( org[0], org[1], org[2]);

			VectorScale(-vRight, 1.4, vTemp);

			VectorCopy(m_pLightList[i].origin, org);
			VectorAdd(org, vTemp, org);
			VectorAdd(org, vForward, org);
			VectorScale(vUp, .8, vTemp);
			VectorAdd(org, vTemp, org);

			gEngfuncs.pTriAPI->Brightness( 1 );
			gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
			gEngfuncs.pTriAPI->Vertex3f( org[0], org[1], org[2]);

			VectorCopy(m_pLightList[i].origin, org);
			VectorScale(vRight, 1.4, vRight);
			VectorAdd(org, -vRight, org);
			VectorAdd(org, vForward, org);
			VectorAdd(org, vUp, org);

			gEngfuncs.pTriAPI->Brightness( 1 );
			gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
			gEngfuncs.pTriAPI->Vertex3f( org[0], org[1], org[2]);

			gEngfuncs.pTriAPI->End();
			gEngfuncs.pTriAPI->RenderMode( kRenderTransTexture );

			gEngfuncs.pTriAPI->CullFace( TRI_FRONT ); // FragMented!: re-enable backface culling

			// Create a dynamic light to light up the Ground
			dlight_t *dl2 = gEngfuncs.pEfxAPI->CL_AllocElight (i*2+1);
			VectorCopy (org, dl2->origin);
			dl2->radius = 2;
			dl2->color.r = dl->color.r;
			dl2->color.g = dl->color.g;
			dl2->color.b = dl->color.b;
			dl2->die = gEngfuncs.GetClientTime () + 0.1;

		}
		else
			m_pLightList[i].bBrightness = 0;

		if(m_pLightList[i].hBrightness || m_pLightList[i].headlightOn)
		{
			if(!m_pLightList[i].headlightOn)
				m_pLightList[i].hBrightness -= .1;
			else
				m_pLightList[i].hBrightness += .1;

			if(m_pLightList[i].hBrightness > 1)
				m_pLightList[i].hBrightness = 1;
			else if (m_pLightList[i].hBrightness < 0)
				m_pLightList[i].hBrightness = 0;


			AngleVectors(m_pLightList[i].angles, vForward, vRight, vUp);
			VectorScale(vForward, rallyinfo[m_pLightList[i].teamidx].szModels[m_pLightList[i].modelidx].fHeadLightDst*10, vForward);
			VectorCopy(m_pLightList[i].origin, org);
			VectorAdd(org, vRight, org);
			VectorAdd(org, vForward, org);
			VectorAdd(org, vUp, org);

			// Create a dynamic light to light up the Ground
			dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocDlight (i);
			VectorCopy (org, dl->origin);
			dl->radius = 230;
			dl->color.r = 160 * m_pLightList[i].hBrightness;
			dl->color.g = 160 * m_pLightList[i].hBrightness;
			dl->color.b = 150 * m_pLightList[i].hBrightness;
			dl->die = gEngfuncs.GetClientTime () + 0.1;

		/*	AngleVectors(m_pLightList[i].angles, vForward, vRight, vUp);
			VectorCopy(m_pLightList[i].origin, org);
			VectorScale(vRight, rallyinfo[m_pLightList[i].teamidx].szModels[m_pLightList[i].modelidx].fHeadLightWidth, vRight);
			VectorScale(vForward, rallyinfo[m_pLightList[i].teamidx].szModels[m_pLightList[i].modelidx].fHeadLightDst, vForward);
			VectorScale(vUp, rallyinfo[m_pLightList[i].teamidx].szModels[m_pLightList[i].modelidx].fHeadLightHeight, vUp);
			VectorAdd(org, vRight, org);
			VectorAdd(org, vForward, org);
			VectorAdd(org, vUp, org);

			vec3_t out;
			VectorCopy(m_pLightList[i].origin, out);
			VectorScale(vForward, 1.15, vForward);
			VectorAdd(out, vRight, out);
			VectorAdd(out, vForward, out);
			VectorAdd(out, vUp, out);


			BEAM *lightBeam = gEngfuncs.pEfxAPI->R_BeamCirclePoints( 0,
				org,						// start
				out,						// end
				gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/smoke.spr" ),	// Model
				0.00001,					// Life
				1,							// Width
				0,							// Amp
				0.01,						// Brightness
				0,							// Speed
				0,							// Start Frame
				0,							// Frame Rate
				150,						// R
				150,						// G
				100							// B
			);

			lightBeam->flags |= FBEAM_SHADEOUT;

			VectorAdd(org, -(vRight*2), org);
			VectorAdd(out, -(vRight*2), out);

			BEAM *lightBeam2 = gEngfuncs.pEfxAPI->R_BeamCirclePoints( 0,
				org,						// start
				out,						// end
				gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/smoke.spr" ),	// Model
				0.00001,					// Life
				1,							// Width
				0,							// Amp
				0.01,						// Brightness
				0,							// Speed
				0,							// Start Frame
				0,							// Frame Rate
				150,						// R
				150,						// G
				100							// B
			);

			lightBeam2->flags |= FBEAM_SHADEOUT;*/
		}
	}
}

void CRallyFX::SetHeadlightStatus (int iIndex, int iState)
{
	m_pLightList[iIndex].headlightOn = iState;
	m_pLightList[iIndex].bBrightness = .3;
	m_pLightList[iIndex].wasBrake = true;
}

void CRallyFX::CalcVariables (void)
{
	vec3_t n;
	int i;

	// Get the current time
	m_fCurrentTime = gEngfuncs.GetClientTime ();

	// Are we rendering effects for the current player?
	if(gEngfuncs.GetLocalPlayer()->index == m_pCurrentEntity->index)
		m_bLocalPlayer = true;
	else
		m_bLocalPlayer = false;

	// Find the current player persistent structure
	// First, find the last point
	for (i = 0; i < m_iPersMax; i++)
	{
		// SaRcaZm - V5 - Start
		//if (m_pPers[i].pEnt == m_pCurrentEntity)
		if (m_pPers[i].pEnt == m_pCurrentEntity->index)
		// SaRcaZm - V5 - End
			break;
	}

	// If we didn't find anything that matched, create a new one
	// Shouldn't need to check for over running array as it is based on the maximum number of players
	if (i >= m_iPersMax)
	{
		m_iPersMax++;

		// SaRcaZm - V5 - Start
		//m_pPers[m_iPersMax].pEnt = m_pCurrentEntity;
		m_pPers[m_iPersMax].pEnt = m_pCurrentEntity->index;
		// SaRcaZm - V5 - End

		m_pPers[m_iPersMax].skid.bFirstTime = true;
		i = m_iPersMax;	// May not need this line
	}
	m_pCurrentPers = &m_pPers[i];

	// Velocity, origin and angles
	VectorCopy (m_pCurrentEntity->origin, m_vOrigin);

	if (m_bLocalPlayer) {
		VectorCopy (pmove->velocity, m_vVelocity);
	} else {

	
/* Creme: new velocity haxor!
notes: from cl_entity.h
struct cl_entity_s {
	int						current_position;  // Last received history update index
	position_history_t		ph[ HISTORY_MAX ];   // History of position and angle updates for this player
*/
		int cp = m_pCurrentEntity->current_position; // shorter ;)
		int pp; // Previous index position
		float animtime;

		if (cp == 0) {
			pp = 63;
		} else {
			pp = cp - 1;
		}

		VectorSubtract (m_pCurrentEntity->ph[cp].origin, 
						m_pCurrentEntity->ph[pp].origin,
						m_vVelocity);

		animtime = m_pCurrentEntity->ph[cp].animtime - m_pCurrentEntity->ph[pp].animtime;

		VectorScale(m_vVelocity, 1/animtime, m_vVelocity);

//		pmove->Con_DPrintf ("       cp: %i,  animtime: %f, speed: %f\n", cp, animtime, Length(m_vVelocity));

	}

	VectorCopy (m_vVelocity, m_pCurrentEntity->baseline.basevelocity);

	// Creme: baseline.origin[1] stores a float Length of velocity
	m_pCurrentEntity->baseline.origin[1] = Length(m_vVelocity);


//	VectorCopy (gHUD.m_vecAngles, m_vAngles);
	VectorCopy (m_pCurrentEntity->angles, m_vAngles);

	// FragMented!: OnGround Fix
	vec3_t traceend;
	VectorCopy(m_vOrigin, traceend);

	// SaRcaZm - V5 - Start
	// In car fix
	if (!stricmp (m_pCurrentEntity->model->name, "models/interior.mdl"))
		traceend[2] -= 15;
	else
		traceend[2] -= 5;
	// SaRcaZm - V5 - End

	pmtrace_t trace;

	gEngfuncs.pEventAPI->EV_PlayerTrace( m_vOrigin, traceend, PM_STUDIO_BOX, -1, &trace );

	if(trace.plane.dist)
		m_bIsOnGround = true;
	else
		m_bIsOnGround = false;

	// gEngfuncs.Con_DPrintf("%.2f\n", trace.plane.dist);

	// Creme: laggy shadow fix.
	if (m_bLocalPlayer) {
		m_vAngles[YAW] = RallyYaw;
	}

	// Calculate some angles
	AngleVectors (m_pCurrentEntity->angles, forward, right, up);
	IEngineStudio.GetViewInfo( m_vRenderOrigin, m_vUp, m_vRight, m_vNormal );

	// Calculate forward speed!
	VectorCopy (m_vVelocity, n);
	n[2] = 0;
	m_fSpeed = Length(n);


	if (m_bLocalPlayer) {
		// Set the corners of the car
		m_vCorners[FRONT_LEFT]  = m_vOrigin + (forward * 22) + (right * -12);
		m_vCorners[BACK_LEFT]   = m_vOrigin + (forward * -22) + (right * -12);
		m_vCorners[BACK_RIGHT]  = m_vOrigin + (forward * -22) + (right * 12);
		m_vCorners[FRONT_RIGHT] = m_vOrigin + (forward * 22) + (right * 12);
		vLocalAngles = m_vAngles;
		// Special hack because attachments are lagged a frame.. sigh valve
		m_vBackFireOrigin = m_pCurrentEntity->attachment[0] + (m_vVelocity*pmove->frametime);
	}
}

void CRallyFX::WheelAnimation (void) 
{
}

// SaRcaZm - V8 - Start
void CRallyFX::AddHorn (int index)
{
	m_bHorn[index] = true;
}
// SaRcaZm - V8 - End

void CRallyFX::Render (cl_entity_s *pplayer)
{
	// Initialise the variables for this frame
	m_pCurrentEntity = pplayer;

	// Only players can have rally fx
	if (m_pCurrentEntity->player)
	{
		m_bInteriorView = false;
		CalcVariables ();

		if(m_bLocalPlayer)
		{
			// SaRcaZm - V8 - Start
			RenderHUDNeedles ();
			// SaRcaZm - V8 - End
			CalcSkidMarks ();
//			Exhaust ();
			ParticleEffects ();
		}
		Dust ();
		HP ();

		// SaRcaZm - V8 - Start
		if (m_bHorn[m_pCurrentEntity->index])
		{
			RallySound->fmodPlay3D ("horn.wav", false, 0, m_vOrigin, m_vVelocity);
			m_bHorn[m_pCurrentEntity->index] = false;
		}
		// SaRcaZm - V8 - End

		// SaRcaZm - V7 - Start
		// Sound stuff
		if (m_bLocalPlayer)
		{
			// Play the sound
			RallySound->fmodEngineChangePitch(rpms, m_pCurrentEntity->index, g_accelerating, m_vOrigin, m_vVelocity, true);
		}
		else// if (1 == 0)
		{
			// Creme 4-Aug-2003: new calculated gear change setup
			float revs;
			float o_currentspeed;

			o_currentspeed = m_fSpeed * 1.25;
			m_gear = int(o_currentspeed / 250) + 1;
			revs = o_currentspeed / (m_gear);

			if (m_fSpeed > 2)
			{
				//gEngfuncs.Con_DPrintf("    auto_speed:%f; currentspeed:%f; gear:%i\n", auto_speed, currentspeed, gear);
				/*
				if (revs > 190) {
					m_gear ++;
				} else if (revs < 120) {
					
					// if still in 1st / 2nd do this check so it doesnt toggle
					if (o_currentspeed > 550) {
						m_gear--;
					} else {
						if (revs < 80) {
							m_gear--;
						}
					}
				}
				*/

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
			// Creme 4-Aug-2003: Store our value in baseline.origin as hl never uses it
			// SaRcaZm - V8 - Start
			else if (m_iMode == MODE_COUNTDOWN)
				revs = m_pCurrentEntity->baseline.origin[0] + ((pmove->RandomFloat(400,1100)) * pmove->frametime);
			else
				revs = 15;	// Constant idling noise
			// SaRcaZm - V8 - End
			// Creme 4-Aug-2003: end

			if (revs < 10)
				revs = 15;
			else if (revs > 250)
			{
				m_pCurrentEntity->baseline.origin[0] = 250;
				revs = 100 + pmove->RandomFloat(0,150);
			}

			revs = ((m_pCurrentEntity->baseline.origin[0] * 3) + (revs)) / 4;
			m_pCurrentEntity->baseline.origin[0] = revs;
			//pmove->Con_DPrintf ("       gear: %f, revs: %f\n", m_gear, revs);

			// If we got this far, play the sound
			RallySound->fmodEngineChangePitch(revs, m_pCurrentEntity->index, 1, m_vOrigin, m_vVelocity, false);
		}
		// SaRcaZm - V7 - End

		Lights(m_pCurrentEntity->angles);

		// Save the current position as the new old origin
		VectorCopy (m_vOrigin, m_pCurrentPers->lastorigin);
	}
	// If it isn't a player, could be the interior view
	else if (!stricmp (m_pCurrentEntity->model->name, "models/interior.mdl"))
	{
		m_bInteriorView = true;
		// Only run this once per frame / time
		if (m_fCurrentTime != gEngfuncs.GetClientTime ())
		{
			CalcVariables ();
			Dust ();
			CalcSkidMarks ();

			// SaRcaZm - V8 - Start
			// Sound stuff
			RenderHUDNeedles ();
			RallySound->fmodEngineChangePitch(rpms, m_pCurrentEntity->index, g_accelerating, m_vOrigin, m_vVelocity, true);

			if (m_bHorn[m_pCurrentEntity->index])
			{
				RallySound->fmodPlay3D ("horn.wav", false, 0, m_vOrigin, m_vVelocity);
				m_bHorn[m_pCurrentEntity->index] = false;
			}
			// SaRcaZm - V8 - End
		}
	}

	// SaRcaZm - V5 - Start
	// See if the countdown needs updating
//	DoCountdown ();
	// SaRcaZm - V5 - End

	m_pCurrentEntity = NULL;
}
// SaRcaZm - V5 - End

void CRallyFX::RenderTriangles (void)
{
	RenderSkidMarks ();
	Shadows ();
	// SaRcaZm - V5 - Start
	// Changes by FragMented!
	RenderLights ();
	// SaRcaZm - V5 - End
	Exhaust ();

}

void ParticleCallback( struct particle_s *particle, float frametime )
{
	int i;

	for ( i = 0; i < 3; i++ )
	{
		particle->org[ i ] += particle->vel[ i ] * frametime;
	}
	particle->org[ 2 ] -= 10.0f * frametime;	// Add gravity
}

// Make particles fly from the wall / other car when the car scrapes along it
// SaRcaZm - V8 - Start
#define NUM_SPARK_POINTS	6
void CRallyFX::ParticleEffects (void)
{
	// Scrape along walls
	int width = 12, length = 22, height = 8;
	pmtrace_t tr;

	if (m_pCvarSparkfx->value <= 0)
		return;
	else if (m_pCvarSparkfx->value > 6)
		m_pCvarSparkfx->value = 6;

	// Only scrape along walls if going fast enough
	if (m_fSpeed > 100)
	{
		vec3_t forward, right, up;
		AngleVectors (m_pCurrentEntity->angles, forward, right, up);
		vec3_t points[NUM_SPARK_POINTS];
		vec3_t midpoint = m_vOrigin + (up * 10);	// This will save time later
		vec3_t height = up * 10;

		points[0] = midpoint + (right *	 (width + 2));		// Mid Right Side
		points[1] = midpoint + (right *  -(width + 2));		// Mid Left Side
		points[2] = height + points[0] + (forward *  (length + 2));	// Front Right Side
		points[3] = height + points[1] + (forward *  (length + 2));	// Front Left Side
		points[4] = height + points[0] + (forward * -(length + 2));	// Back Right Side
		points[5] = height + points[1] + (forward * -(length + 2));	// Back Left Side

		/*points[2] = height + m_vCorners[FRONT_RIGHT];	//points[0] + (forward *  (length + 2));	// Front Right Side
		points[3] = height + m_vCorners[FRONT_LEFT];		//points[1] + (forward *  (length + 2));	// Front Left Side
		points[4] = height + m_vCorners[BACK_RIGHT];		//points[0] + (forward * -(length + 2));	// Back Right Side
		points[5] = height + m_vCorners[BACK_LEFT];		//points[1] + (forward * -(length + 2));	// Back Left Side*/

		for (int i = 0; i < m_pCvarSparkfx->value; i++)
		{
			gEngfuncs.pEventAPI->EV_PlayerTrace( m_vOrigin, points[i], PM_STUDIO_BOX, -1, &tr );
			if (tr.fraction != 1.0)
				break;
		}

		if (tr.fraction == 1.0)
			return;

		// Only do particles if the wall is verticle
		float angle = DotProduct (tr.plane.normal, m_vNormal);
		angle = fabs (angle);

		if ((angle > 0.1) && (angle < 0.9))
			return;

		// Should maybe play a scraping sound here?

		// Don't spew too many particles. That would be obscene
		if (gEngfuncs.pfnRandomFloat (0.0f, 10.0f) > 2.5f)
			return;

		gEngfuncs.pEfxAPI->R_SparkShower( tr.endpos );
	}
}
// SaRcaZm - V8 - End


void CRallyFX::Free_Skidlist (void)
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
	for (int i = 0; i < MAX_PLAYERS; i++)
		m_pPers[i].skid.bFirstTime = true;

	// Reset the maximum pointer into the pers array
	m_iPersMax = 0;
}

