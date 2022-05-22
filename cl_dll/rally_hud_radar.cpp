//-----------------------------------------------------
// RADAR HUD CODE 
//
// Originally sourced from 
// http://www.planethalflife.com/hlsdk2/sdk/creating_a_radar_on_the_hud.htm
/*

 *** Notes from site:

	Client (client side) code
	All the client side code is handled by a CHudRadar class. 
	This is a standard HUD class, so inherit from CHudBase, 
	add a member variable to CHud, call the Init() and VidInit() 
	methods etc. See the GameMenuText tutorial for more info.

*/
//
//-----------------------------------------------------


#include "STDIO.H"
#include "STDLIB.H"
#include "MATH.H"

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>


#include "triangleapi.h"


#include "pm_defs.h"

extern "C" playermove_t *pmove;


float g_fPosx[256]; // Array of entity positions (x coord)
float g_fPosy[256]; // Array of entity positions (y coord)
int g_iNumTargets; // Number of targets to display



//-----------------------------------------------------


DECLARE_MESSAGE( m_HudRadar, HudRadar);


int CHudRadar::Init ( void )
{
	// add our new menu to the list of all the other HUD elements, so we can 
	// receive our new message, and get updated, etc.
	gHUD.AddHudElem ( this );

	HOOK_MESSAGE ( HudRadar ); //lets HL know that our class wants the HudRadar message.

	// Creme 12-April-2004
	m_pCvarHudRadar = gEngfuncs.pfnRegisterVariable ("rally_radar", "1", FCVAR_ARCHIVE);
	m_pSprMap = NULL;

	g_iNumTargets = 0;

	for(int i=0; i < 256; i++)
	{
		g_fPosx[i] = 0;
		g_fPosy[i] = 0;
	}

	return 1;
}

#include<windows.h>
//-----------------------------------------------------

void CHudRadar::InitHUDData ( void )
{
	// Here, we are just initializing all the member variables. 
	// Set HUD_ACTIVE to display the radar at startup 
	// (if you wanted it switchable, you could change that)
	UnpackRGB ( Red, Green, Blue, RGB_YELLOWISH );
	m_iNumTargets = 0;
	m_iFlags |= HUD_ACTIVE;

	m_pSprHUDNumbers = (struct model_s *)gEngfuncs.GetSpritePointer(SPR_Load("sprites/hud_numbers.spr"));

	

	// 11-April-2004: Map overview sprite
	// Creme: ok I dont really mess with str much in C/C++
	// this is a bit of a mess, but it works for now.

	// FIX-ME this process should only be done once per map-change

	char szFile[80] = "";
	char szFilesE[80] = "";  // File sans Extension
	char szPathFile[80] = ""; // File with /sprites/mapname.spr
	char szhlrallyPathFile[80] = ""; // File with hlrally/sprites/mapname.spr

	FILE *pfile = NULL;

	strcpy (szFile, gEngfuncs.pfnGetLevelName());

	// trim "maps/" and extension ".bsp"
	// lose another 5 off strlen, coz we lost start + 5
	strncpy ( szFilesE, szFile + 5, strlen(szFile) - 4 - 5); 

	strcat (szFilesE, ".spr");
	strcpy (szPathFile, "sprites/maps/");

	// Cat the path and filename
	strcat (szPathFile, szFilesE); 

	// add hlrally/ for the fopen check 
	strcpy (szhlrallyPathFile, "hlrally/");
	strcat (szhlrallyPathFile, szPathFile); 

	//		pmove->Con_DPrintf ("Levelname: %s\n",  szPathFile );

	// File exists?
	if ((pfile = fopen(szhlrallyPathFile, "rb")) == NULL)
	{ 
		gEngfuncs.Con_DPrintf("Warning: Unable to open map sprite file: %s\n", szPathFile); 
	}
	else
	{
		fclose(pfile);
		m_pSprMap = (struct model_s *)gEngfuncs.GetSpritePointer(SPR_Load(szPathFile));
	}
}

//-----------------------------------------------------
void CHudRadar::Reset ( void )
{
	// Do nothing for now
}

//-----------------------------------------------------
int CHudRadar::VidInit ( void )
{
	m_hRadarSpr = 0;

	return 1;
}



//-----------------------------------------------------

int CHudRadar::MsgFunc_HudRadar ( const char *pszName, int iSize, void *pbuf )
{
	// Read message
	BEGIN_READ ( pbuf, iSize );
	// Set number of targets
	m_iNumTargets = READ_BYTE();
	// Read+store each x, y coordinate for each target
	for (int i=0;i < m_iNumTargets; i++ )
	{
		m_iPosx[i] = READ_BYTE();
		m_iPosy[i] = READ_BYTE();
	}

	for( ; i < m_iNumTargets + 2; i++)
	{
		m_iPosx[i] = 0;
		m_iPosy[i] = 0;
	}

	gEngfuncs.Con_DPrintf("%i\n", m_iNumTargets);
	return 1;
}


//-----------------------------------------------------
int CHudRadar::Draw ( float flTime )
{


	// Convert member variables to global space, FIXME
	g_iNumTargets = m_iNumTargets;

	for( int i = 0;i < m_iNumTargets;i++)
	{
		
		// scale screen by 1/2 for display

		if(m_iPosx[i] == 0 && m_iPosy[i] == 0)
		{
			g_fPosx[i] = 0;
			g_fPosy[i] = 0;
		}
		else
		{
			g_fPosx[i] = ((g_fPosx[i] * 6) + m_iPosx[i]) / 7;
			g_fPosy[i] = ((g_fPosy[i] * 6) + m_iPosy[i]) / 7;
		}

//		gHUD.DrawHudString ( x, y, 300, "x", Red, Green, Blue );

	}
	
	// do nothing
	return 1;
}


int CHudRadar::OrthoTriDraw ( float flTime )
{
	if(!m_pSprMap || !m_pCvarHudRadar->value)
		return 0;

	int CenterX = 20;
	int CenterY = ScreenHeight - 173;
	float zVal = 0.0f;

	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
	gEngfuncs.pTriAPI->SpriteTexture(m_pSprMap, 0);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
	gEngfuncs.pTriAPI->Color4f( 1, 1, 1, 1.0 );


	// the third value of vertex3f will always be 0 in ortho mode, 
	// don't change it unless you wan't funny things to happen.

	//bottom left
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);
	gEngfuncs.pTriAPI->Vertex3f(CenterX, CenterY+128, zVal); 

	//top left
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);
	gEngfuncs.pTriAPI->Vertex3f(CenterX, CenterY, zVal);

	//top right
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);
	gEngfuncs.pTriAPI->Vertex3f(CenterX+128, CenterY, zVal);

	//bottom right
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);
	gEngfuncs.pTriAPI->Vertex3f(CenterX+128, CenterY+128, zVal);

	gEngfuncs.pTriAPI->End(); //end our list of vertexes




//	pmove->Con_DPrintf ("Ortho! targets: %i\n", g_iNumTargets);
	
	// Load radar background sprite
//	if (!m_hRadarSpr)
//	{
		// quick and nasty way of displaying a sprite - you should precache instead
//		m_hRadarSpr = LoadSprite("sprites/11.spr"); 
//	}

	gEngfuncs.pTriAPI->RenderMode(kRenderTransTexture); //holes/masking
	gEngfuncs.pTriAPI->SpriteTexture(m_pSprHUDNumbers, 0);

	int x,y;

	// Draw an sprite at the position on screen for each entity. 
	for( int i = 0;i < g_iNumTargets;i++)
	{
		
		if(m_iPosx[i] == 0 && m_iPosy[i] == 0)
			continue;

		// scale screen by 1/2 for display

//		x = m_iPosx[i]/2 + 20;
//		y = (ScreenHeight - 50) + (m_iPosy[i]/-2);

		x = g_fPosx[i]/2 + 22;
		y = (ScreenHeight - 45) + (g_fPosy[i]/-2);


//		gHUD.DrawHudString ( x, y, 300, "x", Red, Green, Blue );

		gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
		gEngfuncs.pTriAPI->Color4f( 1, 0, 0, 1.0 );

		// the third value of vertex3f will always be 0 in ortho mode, 
		// don't change it unless you wan't funny things to happen.

		// Creme: Text co-ords are funky to pick up the exact object off the numbers set sprite
		//bottom left
		gEngfuncs.pTriAPI->TexCoord2f(0.6484375f, 0.03125f);
		gEngfuncs.pTriAPI->Vertex3f(x-4, y+4, zVal); 

		//top left
		gEngfuncs.pTriAPI->TexCoord2f(0.6484375f, 0.01171875f);
		gEngfuncs.pTriAPI->Vertex3f(x-4, y-4, zVal);

		//top right
		gEngfuncs.pTriAPI->TexCoord2f(0.671875f, 0.01171875f);
		gEngfuncs.pTriAPI->Vertex3f(x+4, y-4, zVal);

		//bottom right
		gEngfuncs.pTriAPI->TexCoord2f(0.671875f, 0.03125f);
		gEngfuncs.pTriAPI->Vertex3f(x+4, y+4, zVal);

		gEngfuncs.pTriAPI->End(); //end our list of vertexes

//		pmove->Con_DPrintf ("Radar x: %i,  y: %i\n",  x,y );
		
	}


	int thisplayerx = (pmove->origin[0] / 32) + 128;
	int thisplayery = (pmove->origin[1] / 32) + 128;

	x = thisplayerx/2 + 22;
	y = (ScreenHeight - 45) + (thisplayery/-2);


//		gHUD.DrawHudString ( x, y, 300, "x", Red, Green, Blue );

	gEngfuncs.pTriAPI->Begin(TRI_QUADS); //start our quad
	gEngfuncs.pTriAPI->Color4f( 0, 0.9, 0, 1.0 );

	// the third value of vertex3f will always be 0 in ortho mode, 
	// don't change it unless you wan't funny things to happen.

	// Creme: Text co-ords are funky to pick up the exact object off the numbers set sprite
	//bottom left
	gEngfuncs.pTriAPI->TexCoord2f(0.6484375f, 0.03125f);
	gEngfuncs.pTriAPI->Vertex3f(x-4, y+4, zVal); 

	//top left
	gEngfuncs.pTriAPI->TexCoord2f(0.6484375f, 0.01171875f);
	gEngfuncs.pTriAPI->Vertex3f(x-4, y-4, zVal);

	//top right
	gEngfuncs.pTriAPI->TexCoord2f(0.671875f, 0.01171875f);
	gEngfuncs.pTriAPI->Vertex3f(x+4, y-4, zVal);

	//bottom right
	gEngfuncs.pTriAPI->TexCoord2f(0.671875f, 0.03125f);
	gEngfuncs.pTriAPI->Vertex3f(x+4, y+4, zVal);

	gEngfuncs.pTriAPI->End(); //end our list of vertexes
	gEngfuncs.pTriAPI->CullFace( TRI_FRONT ); // FragMented!: re-enable backface culling

	return 1;
}
