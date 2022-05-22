//
// rally_vguifx.cpp
//
// Model effects on the VGUI
//
// By SaRcaZm
//
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

#include "rally_vguifx.h"

#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "in_defs.h"

#include "cvardef.h"

#include "pm_defs.h"
#include "pm_debug.h"
#include "pmtrace.h"

#include "vgui_teamfortressviewport.h"

#include <windows.h>


// Global engine <-> studio model rendering code interface
extern engine_studio_api_t IEngineStudio;

// Creme (non laggy yaw hack, extended from frags stuff ;)
extern float RallyYaw;

// Creme: more stuff from the tri.cpp (hudneedle), not sure if all of this is used now.
extern float currentspeed;
extern float cam_originx, cam_originy, cam_originz;
extern float cam_anglesx, cam_anglesy, cam_anglesz;

extern float g_accelerating;
extern float g_braking;
extern float g_reversing;
extern float g_handbraking;

extern float JoyMoved;

extern int gear;

extern vec3_t realvieworg;
extern vec3_t cam_origin;
extern cvar_t *cam_viewmode;

extern float oldrpms;
extern float rpm_delta;
extern float rpms;


#include "carinfo.h"
anim_paths_t animpaths[MAX_ANIMATIONS];
extern vec3_t vgui_origin, vgui_angles;

modellife_t *vehicles = NULL;
static float curyaw   = -180.0f;

// Why hasn't this already been defined?
#define DEG2RAD( a ) ( a * M_PI ) / 180.0F
// SaRcaZm - V7 - Start
#define CAR_PITCH	0
// SaRcaZm - V7 - End

void EV_ModelCallback ( struct tempent_s *ent, float frametime, float currenttime )
{
	// Rotate the model around
	// This was soooo much easier when the model was parallel to the ground:
	static int rotdir = 1;

	// SaRcaZm - V7 - Start
	if (realvieworg != ent->entity.baseline.vuser3)
	{
		vec3_t vAngles, vForward, vRight, vUp;
		// Realculate the position
		vAngles[0] = cam_anglesx;
		vAngles[1] = cam_anglesy;
		vAngles[2] = 0;
		AngleVectors(vAngles, vForward, vRight, vUp);
		ent->entity.baseline.vuser1 = realvieworg + (vForward * CRallyVGUIFX::getSingleton()->vg_x->value) + (vRight * CRallyVGUIFX::getSingleton()->vg_y->value) + (vUp * CRallyVGUIFX::getSingleton()->vg_z->value);
	}
	// SaRcaZm - V7 - End

	// Move the model back into place
	ent->entity.origin = ent->entity.baseline.vuser1;

	modellife_t *tmp = vehicles;
	modellife_t *prev = NULL;
	while (tmp != NULL)
	{
		// Reset the model lifetime
		//if (tmp->ent == ent)
		if (tmp->reference == ent->entity.baseline.iuser1)
		{
			// SaRcaZm - V7 - Start
			// Update life or it will die
			if (tmp->life == 0.0f)
				ent->die = tmp->life;
			else
				ent->die = tmp->life = currenttime + 0.3;
			// SaRcaZm - V7 - End

			// Check to see if we are rotating with the mouse
			if (tmp->mousedown)
			{
				if (ent->entity.angles[YAW] < tmp->angle[YAW])
					rotdir = 1;
				else if (ent->entity.angles[YAW] > tmp->angle[YAW])
					rotdir = -1;

				ent->entity.angles[YAW] = tmp->angle[YAW];	// Let the mouse rotate the model
				ent->entity.baseline.vuser2[YAW] = curyaw = tmp->angle[YAW];
			}
			else
			{
				ent->entity.baseline.vuser2[YAW] += rotdir;
				ent->entity.angles = ent->entity.baseline.vuser2;	// Just spin around
			}

			if (tmp->life == 0.0f)	// If it equals 0, remove the vehicle pointer
			{
				modellife_t *next = tmp->next;
				free (tmp);
				tmp = next;

				if (prev == NULL)
					vehicles = NULL;
				else
					prev->next = tmp;
				continue;
			}

			ent->entity.angles[PITCH] = cos(DEG2RAD(ent->entity.angles[YAW])) * CAR_PITCH;
			ent->entity.angles[ROLL] = sin(DEG2RAD(ent->entity.angles[YAW])) * (CAR_PITCH/2);
			tmp->angle = ent->entity.angles;
			curyaw = ent->entity.angles[YAW];

			// Create a dynamic light to light up the car
			dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocElight (1);
			VectorCopy (ent->entity.origin, dl->origin);
			dl->origin[2] += 50;
			dl->origin[0] -= 20;
			dl->origin[1] += 20;
			dl->radius = 70;
			dl->color.r = 120;
			dl->color.g = 120;
			dl->color.b = 130;
			dl->die = currenttime + 0.1;

			break;
		}

		prev = tmp;
		tmp = tmp->next;
	}
}

CRallyVGUIFX* CRallyVGUIFX::ms_pSingleton = NULL;

void MP3Playlist (void);

// FragMented! v2
CRallyVGUIFX::CRallyVGUIFX()
{
	vg_x = gEngfuncs.pfnRegisterVariable( "vg_x", "65", FCVAR_ARCHIVE );
	vg_y = gEngfuncs.pfnRegisterVariable( "vg_y", "9", FCVAR_ARCHIVE );
	vg_z = gEngfuncs.pfnRegisterVariable( "vg_z", "-23", FCVAR_ARCHIVE );

	cl_2dcountdown = gEngfuncs.pfnRegisterVariable("cl_2dcountdown", "0", FCVAR_ARCHIVE);

	extern char rally_playlist[80];
	cvar_t *rally_mp3skin = gEngfuncs.pfnRegisterVariable( "rally_mp3skin", "default", FCVAR_ARCHIVE );
	strcpy (rally_playlist, "playlist.pls");	// Set the default
	gEngfuncs.pfnAddCommand ("rally_mp3playlist", MP3Playlist);

}

// FragMented! v2
CRallyVGUIFX::~CRallyVGUIFX()
{
	while (vehicles)
	{
		modellife_t *tmp = vehicles;
		vehicles = vehicles->next;
		free (tmp);
	}
}

void CRallyVGUIFX::ClearVGUIModels (void)
{
	// SaRcaZm - V7 - Start
	// Clean up the list a bit
	modellife_t *current = vehicles;
	modellife_t *prev = NULL;
	while (current != NULL)
	{
		// For some strange reason we are getting a corrupted vehicles list
		// See if we can't clean it up a bit
		// Dunno if this is necessary, but it's done now
		if ((current->name[0] < 'A') || (current->name[0] > 'z'))
		{
			// This one is corrupted, so delete it
			modellife_t *tmp = current;
			current = current->next;
			free (tmp);

			// Update vehicles if we just deleted the head
			if (prev == NULL)
				vehicles = current;
		}

		// Sometimes this doesn't get cleared
		if (current && (current->life == 0.0f || current->life < gEngfuncs.GetClientTime()))
		{
			modellife_t *next = current->next;
			free (current);
			current = next;

			if (prev == NULL)
				vehicles = NULL;
			else
				prev->next = current;
			continue;
		}

		prev = current;
		if (current != NULL)
			current = current->next;
	}
	// SaRcaZm - V7 - End

	current = vehicles;
	while (current != NULL)
	{
		current->life = 0.0f;
		prev = current;
		current = current->next;
	}
}

void CRallyVGUIFX::VGUI (char *model)
{
	char sz[80];
	sprintf (sz, "models/player/%s/%s.mdl", model, model);
	int iModelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex( sz );
	vec3_t vAngles, vForward, vRight, vUp;

	// SaRcaZm - V7 - Start
	// Calculate the position
	//vAngles[0] = cam_anglesx;
	//vAngles[1] = cam_anglesy;
	//vAngles[2] = 0;
	vAngles = vgui_angles;
	realvieworg = vgui_origin;
	AngleVectors(vAngles, vForward, vRight, vUp);
	// SaRcaZm - V7 - End

	vec3_t m_vOrigin = realvieworg + (vForward * vg_x->value) + (vRight * vg_y->value) + (vUp * vg_z->value);
	vec3_t m_vAngles = Vector ((float)(cos(DEG2RAD(curyaw)) * CAR_PITCH), curyaw, (float)(sin(DEG2RAD(curyaw)) * (CAR_PITCH)));

	// Delete all other temp models
	ClearVGUIModels ();

	// Create the temp model
	TEMPENTITY *vehicle = gEngfuncs.pEfxAPI->R_TempModel( m_vOrigin, Vector (0, 0, 0), m_vAngles, 999, iModelIndex, TE_BOUNCE_NULL );
	if ( vehicle )
	{
		// Set the vehicle parameters
		vehicle->flags |= ( FTENT_CLIENTCUSTOM );		// So it calls the callback function.
		vehicle->callback = EV_ModelCallback;			// So we can set the angles and origin back
		vehicle->entity.baseline.vuser1 = m_vOrigin;	// Save the origin and angles for later use
		vehicle->entity.baseline.vuser2 = m_vAngles;

		// SaRcaZm - V7 - Start
		vehicle->entity.baseline.vuser3 = realvieworg;
		// SaRcaZm - V7 - End

		// SaRcaZm - V5 - Start
		modellife_t *tmp = (modellife_t *) malloc (sizeof (modellife_t));
		//tmp->ent = vehicle;
		// SaRcaZm - V7 - Start
		tmp->life = gEngfuncs.GetClientTime () + 0.5;	// Need to constantly update this or it will automagically die
		// SaRcaZm - V7 - End
		tmp->angle = m_vAngles;
		tmp->origin = m_vOrigin;
		strcpy (tmp->name, model);
		tmp->next = NULL;
		tmp->mousedown = 0;
		vehicle->entity.baseline.iuser1 = tmp->reference = gEngfuncs.pfnRandomLong (0, 1000);
		// SaRcaZm - V5 - End

		// Update the pointers
		if (vehicles != NULL)
		{
			modellife_t *next = vehicles;
			vehicles = tmp;
			vehicles->next = next;
		}
		else
			vehicles = tmp;
	}
}


#define VGUI_BG_R	0.0f
#define VGUI_BG_G	0.0f
#define VGUI_BG_B	0.0f
#define VGUI_BG_A	1.0f

void CRallyVGUIFX::VGUIBackground (void)
{
	// Draw a black box around the screen and model to give it a black background
	vec3_t vAngles, vForward, vRight, vUp;
	vec3_t corners[4], farcorners[4], showroomfloor[4], showroomroof[4];
	int iScreenX = 5, iScreenY = 4, iScreenZ = 5;
	// Thank god I made this easily modifiable since bloody Creme
	// changed his mind and made me move everything around (mumble...mumble...mumble)
	//int iFarScreenX = 100, iFarScreenY = 50, iFarScreenZ = 110;
	int iFarScreenX = 100, iFarScreenY = 80, iFarScreenZ = 110;

	// SaRcaZm - V7 - Start
	// This seems to be screwed up
	/*vAngles[0] = cam_anglesx;
	vAngles[1] = cam_anglesy;
	vAngles[2] = 0;*/

	realvieworg = vgui_origin;
	vAngles = vgui_angles;
	// SaRcaZm - V7 - End

	AngleVectors(vAngles, vForward, vRight, vUp);

	corners[0] = realvieworg + (vRight *  iScreenX) + (vUp *  iScreenY) + (vForward * iScreenZ);	// Top right
	corners[1] = realvieworg + (vRight * -iScreenX) + (vUp *  iScreenY) + (vForward * iScreenZ);	// Top left
	corners[2] = realvieworg + (vRight * -iScreenX) + (vUp * -iScreenY) + (vForward * iScreenZ);	// Bottom left
	corners[3] = realvieworg + (vRight *  iScreenX) + (vUp * -iScreenY) + (vForward * iScreenZ);	// Bottom right

	farcorners[0] = realvieworg + (vRight *  iFarScreenX) + (vUp *  iFarScreenY) + (vForward * iFarScreenZ);		// Top right
	farcorners[1] = realvieworg + (vRight * -(iFarScreenX/2)) + (vUp *  iFarScreenY) + (vForward * iFarScreenZ);	// Top left
	farcorners[2] = realvieworg + (vRight * -(iFarScreenX/2)) + (vUp * -iFarScreenY) + (vForward * iFarScreenZ);	// Bottom left
	farcorners[3] = realvieworg + (vRight *  iFarScreenX) + (vUp * -iFarScreenY) + (vForward * iFarScreenZ);		// Bottom right

	static const int tex_coords[4][2] = { { 0, 0 }, { 0, 1 }, { 1, 1 }, { 1, 0 } };
	// Do this with a texture to avoid the disappearing background bug
	HSPRITE hsprWallTexture = LoadSprite( "sprites/vgui_back.spr" ); // Load sprite, as normal;
	const model_s *pWallTexture = gEngfuncs.GetSpritePointer( hsprWallTexture ); // Get pointer to texture;
	gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)pWallTexture, 0 ); // Set texture for polys, second parameter is the frame number for animated sprites

	// Initialisation
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
	gEngfuncs.pTriAPI->Begin( TRI_QUADS );

	// Draw these with a texture now
	// Back of window
	for (int i = 0; i < 4; i++)
	{
		gEngfuncs.pTriAPI->Brightness( 1 ); // set relative brightness of vertex
		gEngfuncs.pTriAPI->TexCoord2f( tex_coords[i][0], tex_coords[i][1] ); // set 2d texture coordinate for vertex (from 0.0 to 1.0 across whole texture)
		gEngfuncs.pTriAPI->Color4f (VGUI_BG_R, VGUI_BG_G, VGUI_BG_B, VGUI_BG_A);
		gEngfuncs.pTriAPI->Vertex3fv (farcorners[i]);
	}

	// Right side
	gEngfuncs.pTriAPI->Brightness( 1 ); // set relative brightness of vertex
	gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
	gEngfuncs.pTriAPI->Color4f (VGUI_BG_R, VGUI_BG_G, VGUI_BG_B, VGUI_BG_A);
	gEngfuncs.pTriAPI->Vertex3fv (farcorners[0]);
	gEngfuncs.pTriAPI->Brightness( 1 ); // set relative brightness of vertex
	gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
	gEngfuncs.pTriAPI->Color4f (VGUI_BG_R, VGUI_BG_G, VGUI_BG_B, VGUI_BG_A);
	gEngfuncs.pTriAPI->Vertex3fv (corners[0]);
	gEngfuncs.pTriAPI->Brightness( 1 ); // set relative brightness of vertex
	gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
	gEngfuncs.pTriAPI->Color4f (VGUI_BG_R, VGUI_BG_G, VGUI_BG_B, VGUI_BG_A);
	gEngfuncs.pTriAPI->Vertex3fv (corners[3]);
	gEngfuncs.pTriAPI->Brightness( 1 ); // set relative brightness of vertex
	gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
	gEngfuncs.pTriAPI->Color4f (VGUI_BG_R, VGUI_BG_G, VGUI_BG_B, VGUI_BG_A);
	gEngfuncs.pTriAPI->Vertex3fv (farcorners[3]);
	// Left side
	gEngfuncs.pTriAPI->Brightness( 1 ); // set relative brightness of vertex
	gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
	gEngfuncs.pTriAPI->Color4f (VGUI_BG_R, VGUI_BG_G, VGUI_BG_B, VGUI_BG_A);
	gEngfuncs.pTriAPI->Vertex3fv (farcorners[1]);
	gEngfuncs.pTriAPI->Brightness( 1 ); // set relative brightness of vertex
	gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
	gEngfuncs.pTriAPI->Color4f (VGUI_BG_R, VGUI_BG_G, VGUI_BG_B, VGUI_BG_A);
	gEngfuncs.pTriAPI->Vertex3fv (corners[1]);
	gEngfuncs.pTriAPI->Brightness( 1 ); // set relative brightness of vertex
	gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
	gEngfuncs.pTriAPI->Color4f (VGUI_BG_R, VGUI_BG_G, VGUI_BG_B, VGUI_BG_A);
	gEngfuncs.pTriAPI->Vertex3fv (corners[2]);
	gEngfuncs.pTriAPI->Brightness( 1 ); // set relative brightness of vertex
	gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
	gEngfuncs.pTriAPI->Color4f (VGUI_BG_R, VGUI_BG_G, VGUI_BG_B, VGUI_BG_A);
	gEngfuncs.pTriAPI->Vertex3fv (farcorners[2]);
	// Top side
	gEngfuncs.pTriAPI->Brightness( 1 ); // set relative brightness of vertex
	gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
	gEngfuncs.pTriAPI->Color4f (VGUI_BG_R, VGUI_BG_G, VGUI_BG_B, VGUI_BG_A);
	gEngfuncs.pTriAPI->Vertex3fv (farcorners[0]);
	gEngfuncs.pTriAPI->Brightness( 1 ); // set relative brightness of vertex
	gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
	gEngfuncs.pTriAPI->Color4f (VGUI_BG_R, VGUI_BG_G, VGUI_BG_B, VGUI_BG_A);
	gEngfuncs.pTriAPI->Vertex3fv (corners[0]);
	gEngfuncs.pTriAPI->Brightness( 1 ); // set relative brightness of vertex
	gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
	gEngfuncs.pTriAPI->Color4f (VGUI_BG_R, VGUI_BG_G, VGUI_BG_B, VGUI_BG_A);
	gEngfuncs.pTriAPI->Vertex3fv (corners[1]);
	gEngfuncs.pTriAPI->Brightness( 1 ); // set relative brightness of vertex
	gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
	gEngfuncs.pTriAPI->Color4f (VGUI_BG_R, VGUI_BG_G, VGUI_BG_B, VGUI_BG_A);
	gEngfuncs.pTriAPI->Vertex3fv (farcorners[1]);
	// Bottom side
	gEngfuncs.pTriAPI->Brightness( 1 ); // set relative brightness of vertex
	gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
	gEngfuncs.pTriAPI->Color4f (VGUI_BG_R, VGUI_BG_G, VGUI_BG_B, VGUI_BG_A);
	gEngfuncs.pTriAPI->Vertex3fv (farcorners[2]);
	gEngfuncs.pTriAPI->Brightness( 1 ); // set relative brightness of vertex
	gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
	gEngfuncs.pTriAPI->Color4f (VGUI_BG_R, VGUI_BG_G, VGUI_BG_B, VGUI_BG_A);
	gEngfuncs.pTriAPI->Vertex3fv (corners[2]);
	gEngfuncs.pTriAPI->Brightness( 1 ); // set relative brightness of vertex
	gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
	gEngfuncs.pTriAPI->Color4f (VGUI_BG_R, VGUI_BG_G, VGUI_BG_B, VGUI_BG_A);
	gEngfuncs.pTriAPI->Vertex3fv (corners[3]);
	gEngfuncs.pTriAPI->Brightness( 1 ); // set relative brightness of vertex
	gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
	gEngfuncs.pTriAPI->Color4f (VGUI_BG_R, VGUI_BG_G, VGUI_BG_B, VGUI_BG_A);
	gEngfuncs.pTriAPI->Vertex3fv (farcorners[3]);

	gEngfuncs.pTriAPI->End ();

	// SaRcaZm - V5 - Start
	HSPRITE hsprTexture;
	const model_s *pTexture;
	if (m_iMode == MODE_SHOWROOM)
	{
		// This has moved too of course...bloody Creme, the next time I see him...
		showroomfloor[0] = realvieworg + (vForward * 40)  + (vRight * -30) + (vUp * -32);	// Front Left
		showroomfloor[1] = realvieworg + (vForward * 40)  + (vRight *  50) + (vUp * -32);	// Front Right
		showroomfloor[2] = realvieworg + (vForward * 110) + (vRight *  50) + (vUp * -16);	// Back Right
		showroomfloor[3] = realvieworg + (vForward * 110) + (vRight * -30) + (vUp * -16);	// Back Left

		// Now draw the showroom floor under the model
		// Setup rendering
		hsprTexture = LoadSprite( "sprites/showroom_f.spr" ); // Load sprite, as normal;
		pTexture = gEngfuncs.GetSpritePointer( hsprTexture ); // Get pointer to texture;

		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)pTexture, 0 ); // Set texture for polys, second parameter is the frame number for animated sprites
		gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
		gEngfuncs.pTriAPI->Begin( TRI_QUADS ); // This defines what type of poly is being drawn. In this case it will be 4sided quads.

		// Show room floor
		for (i = 0; i < 4; i++)
		{
			gEngfuncs.pTriAPI->Brightness( 1 ); // set relative brightness of vertex
			gEngfuncs.pTriAPI->TexCoord2f( tex_coords[i][0], tex_coords[i][1] ); // set 2d texture coordinate for vertex (from 0.0 to 1.0 across whole texture)
			gEngfuncs.pTriAPI->Color4f (1, 1, 1, 1);
			gEngfuncs.pTriAPI->Vertex3fv( showroomfloor[i] );
		}
		// End rendering
		gEngfuncs.pTriAPI->End();
	}
	// SaRcaZm - V5 - End

	// Draw the shadow of the car
	// Check to see if there is a model being displayed
	if (vehicles && vehicles->life > gEngfuncs.GetClientTime ())
	{
		int length = 26, width = 12, height = 0;
		vec3_t forward, right, up;
		vec3_t corners[4];
		//AngleVectors (vehicles[iModelIndex].angle, forward, right, up);
		AngleVectors (vehicles->angle, forward, right, up);

		// Set the corners of the car
		corners[FRONT_LEFT]  = vehicles->origin + (forward *  length) + (right * -width);
		corners[BACK_LEFT]   = vehicles->origin + (forward * -length) + (right * -width);
		corners[BACK_RIGHT]  = vehicles->origin + (forward * -length) + (right *  width);
		corners[FRONT_RIGHT] = vehicles->origin + (forward *  length) + (right *  width);

		// Setup rendering
		hsprTexture = LoadSprite( "sprites/r_shadow.spr" ); // Load sprite, as normal;
		pTexture = gEngfuncs.GetSpritePointer( hsprTexture ); // Get pointer to texture;
		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)pTexture, 0 ); // Set texture for polys, second parameter is the frame number for animated sprites
		gEngfuncs.pTriAPI->RenderMode( kRenderTransTexture );
		gEngfuncs.pTriAPI->Begin( TRI_QUADS ); // This defines what type of poly is being drawn. In this case it will be 4sided quads.

		for (i = 0; i < 4; i++)
		{
			gEngfuncs.pTriAPI->Brightness( 1 ); // set relative brightness of vertex
			gEngfuncs.pTriAPI->TexCoord2f( tex_coords[i][0], tex_coords[i][1] ); // set 2d texture coordinate for vertex (from 0.0 to 1.0 across whole texture)
			gEngfuncs.pTriAPI->Color4f (1, 1, 1, 1);
			gEngfuncs.pTriAPI->Vertex3fv( corners[i] );
		}

		// End rendering
		gEngfuncs.pTriAPI->End();
	}

	gEngfuncs.pTriAPI->CullFace( TRI_FRONT ); // FragMented!: re-enable backface culling
}


void CRallyVGUIFX::SetMode (int iMode)
{
	m_iMode = iMode; 
//	m_iMode = MODE_NORM; //TDK
	// SaRcaZm - V7 - Start
	if (m_iMode == MODE_NORM)
		gViewPort->UpdateSpectatorMenu ();
	// SaRcaZm - V7 - End
}

int CRallyVGUIFX::GetMode (void)
{
	return m_iMode;
}

// SaRcaZm - V5 - Start
// From PM_Math.c
void NormalizeAngles( float *angles )
{
	int i;
	// Normalize angles
	for ( i = 0; i < 3; i++ )
	{
		if ( angles[i] > 180.0 )
		{
			angles[i] -= 360.0;
		}
		else if ( angles[i] < -180.0 )
		{
			angles[i] += 360.0;
		}
	}
}

// From PM_Math.c
void InterpolateAngles( float *start, float *end, float *output, float frac )
{
	int i;
	float ang1, ang2;
	float d;
	
	NormalizeAngles( start );
	NormalizeAngles( end );

	for ( i = 0 ; i < 3 ; i++ )
	{
		ang1 = start[i];
		ang2 = end[i];

		d = ang2 - ang1;
		if ( d > 180 )
		{
			d -= 360;
		}
		else if ( d < -180 )
		{	
			d += 360;
		}

		output[i] = ang1 + d * frac;
	}

	NormalizeAngles( output );
}

void InterpolateVec( float *start, float *end, float *output, float frac )
{
	int i;
	float ang1, ang2;
	float d;
	
	for ( i = 0 ; i < 3 ; i++ )
	{
		ang1 = start[i];
		ang2 = end[i];

		d = ang2 - ang1;

		output[i] = ang1 + d * frac;
	}
}

void CRallyVGUIFX::DoCountdown (void)
{
	// SaRcaZm - V8 - Start
	// Check to see if we need to do anything
	if (((m_fCountdownStart + 4) < gEngfuncs.GetClientTime()) || (m_iCountdownDone == 4))
	{
		if (m_iMode == MODE_COUNTDOWN)
			m_iMode = MODE_NORM;
		return;
	}

	// Check to see if we have completed this phase
	if ((gEngfuncs.GetClientTime() - m_fCountdownStart) < m_iCountdownDone)
		return;

	m_iCountdownDone++;

	// Don't show the countdown when in the VGUI
	if ((m_iMode == MODE_VGUI) || (m_iMode == MODE_SHOWROOM))
		return;

	m_iMode = MODE_COUNTDOWN;
	// SaRcaZm - V8 - End

	// Do a center print instead of the fancy 3D countdown
	if (cl_2dcountdown->value)
	{
		int iNum = 4 - m_iCountdownDone;
		char sz[5];

		if (iNum)
			sprintf (sz, "%i\n", iNum);
		else
			sprintf (sz, "Go!\n");

		CenterPrint (sz);
		return;
	}

	vec3_t origin = Vector (80, 5, 0);
	vec3_t angles = Vector (0, 0, 0);

	if (m_iCountdownDone == 4)
		CustomAnim ("go.mdl", NUM_ANIM);
	else if (m_iCountdownDone == 3)
		CustomAnim ("one.mdl", NUM_ANIM);
	else if (m_iCountdownDone == 2)
		CustomAnim ("two.mdl", NUM_ANIM);
	else if (m_iCountdownDone == 1)
		CustomAnim ("three.mdl", NUM_ANIM);
}

void CRallyVGUIFX::StartCountdown (float fStartTime)
{
	m_fCountdownStart = fStartTime;
	m_iCountdownDone = 0;
}

void EV_CustomAnimCallback ( struct tempent_s *ent, float frametime, float currenttime )
{
	anim_paths_t *path;// = &animpaths[ent->entity.baseline.iuser1];
	vec3_t start_origin = ent->entity.baseline.vuser1;
	vec3_t start_angles = ent->entity.baseline.vuser2;
	float start_time;// = ent->entity.baseline.fuser1;

	// Update the life of the model
	modellife_t *tmp = vehicles;
	modellife_t *prev = NULL;
	while (tmp != NULL)
	{
		// Reset the model lifetime
		//if (tmp->ent == ent)
		if (ent->entity.baseline.iuser1 == tmp->reference)
		{
			// If it's life is over, remove the vehicle pointer
			if (tmp->life == 0.0f)
			{
				ent->die = tmp->life;

				// Delete the pointer
				modellife_t *next = tmp->next;
				free (tmp);
				tmp = next;

				if (prev == NULL)
					vehicles = NULL;
				else
					prev->next = tmp;
				return;
			}

			start_time = tmp->animstart;
			path = &animpaths[tmp->anim];
			break;
		}

		prev = tmp;
		tmp = tmp->next;
	}

	// SaRcaZm - V7 - Start
	if (!tmp)	// Couldn't find our animation so fail
	{
		ent->die = 0.0f;
		return;
	}
	// SaRcaZm - V7 - End

	// Figure out which phase of the animation we are in
	for (int i = 0; i < path->num_nodes; i++)
	{
		if ((currenttime - start_time) < path->nodes[i].time)
			break;
	}

	// Now i contains the path phase
	// Calculate the current position of the camera
	vec3_t vAngles, vForward, vRight, vUp;
	vAngles[0] = cam_anglesx;
	vAngles[1] = cam_anglesy;
	vAngles[2] = 0;
	AngleVectors(vAngles, vForward, vRight, vUp);

	if (i == path->num_nodes)	// We have reached the end of the animation
	{
		ent->entity.origin = start_origin + (vRight * (path->nodes[path->num_nodes - 1].position[0] * path->scale)) +
											(vUp * (path->nodes[path->num_nodes - 1].position[1] * path->scale)) +
											(vForward * (path->nodes[path->num_nodes - 1].position[2] * path->scale));
		ent->entity.angles = start_angles + path->nodes[path->num_nodes - 1].angles;

		// If this is a return animation, start the next animation
		if (tmp->ret > -1)
		{
			tmp->anim = tmp->ret;
			tmp->animstart = currenttime;
			tmp->ret = -1;
		}
	}
	else
	{
		// Interpolate to create a smooth transition
		vec3_t from_origin, from_angles;
		if (i > 0)	// Avoid crash
		{
			from_origin = start_origin + (vRight * (path->nodes[i-1].position[0] * path->scale)) +
												(vUp * (path->nodes[i-1].position[1] * path->scale)) +
												(vForward * (path->nodes[i-1].position[2] * path->scale));
			from_angles = start_angles + path->nodes[i-1].angles;
		}
		vec3_t to_origin = start_origin + (vRight * (path->nodes[i].position[0] * path->scale)) +
											(vUp * (path->nodes[i].position[1] * path->scale)) +
											(vForward * (path->nodes[i].position[2] * path->scale));
		vec3_t to_angles = start_angles + path->nodes[i].angles;

		if (i > 0)	// If we have something to iterpolate
		{
			// Calc how long this portion of the animation has been going
			float animtime = (currenttime - start_time) - path->nodes[i-1].time;
			animtime /= (path->nodes[i].time - path->nodes[i-1].time);

			InterpolateVec (from_origin, to_origin, ent->entity.origin, animtime);
			InterpolateAngles (from_angles, to_angles, ent->entity.angles, animtime);
		}
		else		// Nothing to interpolate from, so just copy the to_ values
		{
			ent->entity.origin = to_origin;
			ent->entity.angles = to_angles;
		}
	}

	// Create a dynamic light to give the model some light
	dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocDlight (1);
	VectorCopy (ent->entity.origin, dl->origin);
	dl->radius = 40;
	dl->color.r = 255;
	dl->color.g = 255;
	dl->color.b = 255;
	dl->die = currenttime + 0.1;
}

// N.B. float* origin is relative to the current view origin
int CRallyVGUIFX::CustomAnim (char *szModel, int iAnimIndex)
{
	// Expand the model name
	char szModelName[80];
	sprintf (szModelName, "models/%s", szModel);
	int iModelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex( szModelName );

	// SaRcaZm - V6 - Start
	static bool initialised = false;
	if (!initialised)
	{
		// Read the Anim.txt file
		// This CCarInfo class is really getting a workout now
	//	CCarInfo *animinfo = new CCarInfo ("Anim.txt");
		CCarInfo *animinfo = new CCarInfo (gViewPort->m_szAnimTxt, strlen (gViewPort->m_szAnimTxt));
		char szMake[20];
		int i = 0;

		// So, you think I could create a few more indents??
		animinfo->getNextManufacturer (szMake);
		
		while (szMake[0])
		{
			char szModel[20];
			int j = 0;
			anim_paths_t *path;

			// Get the index to the animation we are loading
			if (!stricmp (szMake, "Numbers"))
				path = &animpaths[NUM_ANIM];
			else if (!stricmp (szMake, "CustomiseNoAnim"))
				path = &animpaths[CUST_NO_ANIM];
			else if (!strncmp (szMake, "CustomiseButton", 15))
				path = &animpaths[CUST_TOPBUTTON_ANIM + (szMake[15] - '0')];
			else
				break;		// Oops, we weren't expecting that

			path->num_nodes = 0;

			// Loop through all the models
			animinfo->getNextModel (szModel);
			while (szModel[0])
			{
				// Read each of the headings in
				char szAttrib[20], szAttribVal[256];

				if (!stricmp (szModel, "General"))
				{
					animinfo->getNextAttributeString (szAttrib, szAttribVal);
					while (szAttrib[0] && szAttribVal[0])
					{
						if (!stricmp (szAttrib, "Origin"))
						{
							int pos = 0;

							path->origin[0] = atof (szAttribVal);
							// Skip till the first ','
							while (szAttribVal[pos] && (szAttribVal[pos] != ','))
								pos++;
							pos++;
							path->origin[1] = atof (szAttribVal + pos);
							// Skip till the next ','
							while (szAttribVal[pos] && (szAttribVal[pos] != ','))
								pos++;
							pos++;
							path->origin[2] = atof (szAttribVal + pos);
						}
						else if (!stricmp (szAttrib, "Angle"))
						{
							int pos = 0;

							path->angles[0] = atof (szAttribVal);
							// Skip till the first ','
							while (szAttribVal[pos] && (szAttribVal[pos] != ','))
								pos++;
							pos++;
							path->angles[1] = atof (szAttribVal + pos);
							// Skip till the next ','
							while (szAttribVal[pos] && (szAttribVal[pos] != ','))
								pos++;
							pos++;
							path->angles[2] = atof (szAttribVal + pos);
						}
						else if (!stricmp (szAttrib, "Length"))
							path->length = atof (szAttribVal);
						else if (!stricmp (szAttrib, "Scale"))
							path->scale = atof (szAttribVal);

						// Get the next attribute
						animinfo->getNextAttributeString (szAttrib, szAttribVal);
					}
				}
				else if (!strncmp (szModel, "Node", 4))
				{
					numpath_t *node = &path->nodes[szModel[4] - '0'];
					path->num_nodes++;	// Add this node to the pile

					animinfo->getNextAttributeString (szAttrib, szAttribVal);
					while (szAttrib[0] && szAttribVal[0])
					{
						if (!stricmp (szAttrib, "Position"))
						{
							int pos = 0;

							node->position[0] = atof (szAttribVal);
							// Skip till the first ','
							while (szAttribVal[pos] && (szAttribVal[pos] != ','))
								pos++;
							pos++;
							node->position[1] = atof (szAttribVal + pos);
							// Skip till the next ','
							while (szAttribVal[pos] && (szAttribVal[pos] != ','))
								pos++;
							pos++;
							node->position[2] = atof (szAttribVal + pos);
						}
						else if (!stricmp (szAttrib, "Angle"))
						{
							int pos = 0;

							node->angles[0] = atof (szAttribVal);
							// Skip till the first ','
							while (szAttribVal[pos] && (szAttribVal[pos] != ','))
								pos++;
							pos++;
							node->angles[1] = atof (szAttribVal + pos);
							// Skip till the next ','
							while (szAttribVal[pos] && (szAttribVal[pos] != ','))
								pos++;
							pos++;
							node->angles[2] = atof (szAttribVal + pos);
						}
						else if (!stricmp (szAttrib, "Time"))
							node->time = atof (szAttribVal);

						// Get the next attribute
						animinfo->getNextAttributeString (szAttrib, szAttribVal);
					}
				}
				else
					break;		// Oops, we weren't expecting this attribute

				// Get the next model
				j++;
				animinfo->getNextModel (szModel);
			}

			// Get the next manufacturer
			animinfo->getNextManufacturer (szMake);
			i++;
		}

		delete animinfo;
		initialised = true;
	}
	// SaRcaZm - V6 - Start

	// SaRcaZm - V7 - Start
	// Calculate the current position
	vec3_t vAngles, vForward, vRight, vUp;
	vAngles[0] = cam_anglesx;
	vAngles[1] = cam_anglesy;
	vAngles[2] = 0;
	AngleVectors(vAngles, vForward, vRight, vUp);

	// Calculate the start position and angle for the model
	vec3_t vOrigin = realvieworg + (vForward * animpaths[iAnimIndex].origin[0]) + (vUp * animpaths[iAnimIndex].origin[1]) + (vRight * animpaths[iAnimIndex].origin[2]);
	vAngles = vAngles + animpaths[iAnimIndex].angles;

	// Create the temp model
	modellife_t *tmp;
	TEMPENTITY *model = gEngfuncs.pEfxAPI->R_TempModel( vOrigin, Vector (0, 0, 0), vAngles, animpaths[iAnimIndex].length, iModelIndex, TE_BOUNCE_NULL );

	if (model)
	{
		// Set the model parameters
		model->flags |= ( FTENT_CLIENTCUSTOM );		// So it calls the callback function.
		model->entity.baseline.vuser1 = vOrigin;		// Save the origin and angles for later use
		model->entity.baseline.vuser2 = vAngles;
		// SaRcaZm - V7 - End
//		model->entity.baseline.fuser1 = m_fCurrentTime;
//		model->entity.baseline.iuser1 = iAnimIndex;
		model->callback = EV_CustomAnimCallback;		// So we can set the angles and origin back

		// Add this model to the vehicle life list
		tmp = (modellife_t *) malloc (sizeof (modellife_t));
		//tmp->ent = model;
		tmp->life = animpaths[iAnimIndex].length;
		tmp->angle = animpaths[iAnimIndex].angles;
		tmp->anim = iAnimIndex;
		tmp->ret = -1;
		tmp->animstart = gEngfuncs.GetClientTime();//gEngfuncs.GetClientTime (); // FragMented! v2
		model->entity.baseline.iuser1 = tmp->reference = gEngfuncs.pfnRandomLong (0, 1000);
		tmp->origin = gEngfuncs.GetLocalPlayer()->origin; // FragMented! v2
		//strcpy (tmp->name, szModel);
		tmp->next = NULL;
		tmp->mousedown = 0;

		// Update the pointers
		if (vehicles != NULL)
		{
			modellife_t *next = vehicles;
			vehicles = tmp;
			vehicles->next = next;
		}
		else
			vehicles = tmp;

		// SaRcaZm - V7 - Start
		return tmp->reference;
	}

	return -1;
	// SaRcaZm - V7 - End
}

void CRallyVGUIFX::SetCustomAnim (int iAnimNum, int iAnimIndex)
{
	modellife_t *tmp = vehicles;
	while (tmp)
	{
		if (tmp->reference == iAnimNum)
		{
			//tmp->anim = iAnimIndex;
			tmp->animstart = gEngfuncs.GetClientTime () - tmp->life;
			tmp->ret = iAnimIndex;
			break;
		}

		tmp = tmp->next;
	}
}

void CRallyVGUIFX::StopCustomAnim (int iAnimNum)
{
	modellife_t *tmp = vehicles;
	while (tmp)
	{
		if (tmp->reference == iAnimNum)
		{
			tmp->life = 0.0f;
			break;
		}

		tmp = tmp->next;
	}
}
// SaRcaZm - V5 - End
