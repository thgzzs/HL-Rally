// Triangle rendering, if any

#include "hud.h"
#include "cl_util.h"

// Triangle rendering apis are in gEngfuncs.pTriAPI

#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"

#include "pm_defs.h"
#include "rally_effects.h"
#include "rally_vguifx.h"

#include "pm_defs.h"

extern "C" playermove_t *pmove;

#define DLLEXPORT __declspec( dllexport )

#define D 0.0174532

extern "C"
{
	void DLLEXPORT HUD_DrawNormalTriangles( void );
	void DLLEXPORT HUD_DrawTransparentTriangles( void );
};

extern "C" void Global_DLLEXPORT_Info( float time, const char *func_name );

extern float rpms;
extern int cam_thirdperson;

extern CHudRadar m_HudRadar;





/*
=================
Draw_Triangles

Example routine.  Draws a sprite offset from the player origin.
=================
*/
void Draw_Triangles( void )
{
}


/*
=================
HUD_DrawNormalTriangles

Non-transparent triangles-- add them here
=================
*/

void DLLEXPORT HUD_DrawNormalTriangles( void )
{

	Global_DLLEXPORT_Info(gEngfuncs.GetClientTime(), "HUD_DrawNormalTriangles");
	gHUD.m_Spectator.DrawOverview();
	
	// FragMented!
	CRallyEffects::getSingleton()->render();
}



/*
=================
HUD_DrawTransparentTriangles

Render any triangles with transparent rendermode needs here
=================
*/
void DLLEXPORT HUD_DrawTransparentTriangles( void )
{
	Global_DLLEXPORT_Info(gEngfuncs.GetClientTime(), "HUD_DrawTransparentTriangles");

	if ((CRallyVGUIFX::getSingleton()->GetMode () == MODE_VGUI) || (CRallyVGUIFX::getSingleton()->GetMode () == MODE_SHOWROOM))
		CRallyVGUIFX::getSingleton()->VGUIBackground ();	// Draw VGUI background


	// Add the HLRally Special Effects
	CRallyEffects::getSingleton()->renderTransTriangles();

}


void HUD_DrawOrthoTriangles( void )
{
	float zVal = 0;

	float angle, hyp, OfsX, OfsY, CenterX, CenterY;
	float needle;

	// Crem: Render RPM HUD Needle
	if (!(gHUD.m_iHideHUDDisplay & HIDEHUD_ALL) && !(gHUD.m_iHideHUDDisplay & HIDEHUD_WEAPONS) &&
		(CRallyVGUIFX::getSingleton()->GetMode () != MODE_VGUI) && (CRallyVGUIFX::getSingleton()->GetMode () != MODE_SHOWROOM))
	{
		gEngfuncs.pTriAPI->CullFace( TRI_NONE ); //no culling
		gEngfuncs.pTriAPI->RenderMode(kRenderTransTexture); //holes/masking

		if (cam_thirdperson && !g_iUser1)
		{
			gEngfuncs.pTriAPI->SpriteTexture(CRallyEffects::getSingleton()->m_pSprHudNeedle,	0);
			gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad

			gEngfuncs.pTriAPI->Color4f( 1, 1, 1, 1.0 );


			// Center of the needle
			CenterX = ScreenWidth - 78;
			CenterY = ScreenHeight - 79;

			needle = 75;
			
			hyp = sqrt(needle*needle + needle*needle); 
			angle = rpms;
			
			OfsX = (sin(angle * D) * hyp);
			OfsY = (cos(angle * D) * hyp);

	//		gEngfuncs.Con_DPrintf("OfsX = %f; hyp = %f\n", OfsX, hyp);

			// the third value of vertex3f will always be 0 in ortho mode, 
			// don't change it unless you wan't funny things to happen.

			//bottom left
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);
			gEngfuncs.pTriAPI->Vertex3f(CenterX-OfsX, CenterY+OfsY, zVal); 

			//top left
			gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);
			gEngfuncs.pTriAPI->Vertex3f(CenterX-OfsY, CenterY-OfsX, zVal);

			//top right
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);
			gEngfuncs.pTriAPI->Vertex3f(CenterX+OfsX, CenterY-OfsY, zVal);

			//bottom right
			gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);
			gEngfuncs.pTriAPI->Vertex3f(CenterX+OfsY, CenterY+OfsX, zVal);

			gEngfuncs.pTriAPI->End(); //end our list of vertexes
		}
		gHUD.m_HudRadar.OrthoTriDraw(gEngfuncs.GetClientTime());

		gEngfuncs.pTriAPI->RenderMode(kRenderNormal); //return to normal
		gEngfuncs.pTriAPI->CullFace( TRI_FRONT ); // FragMented!: re-enable backface culling
	}
}
