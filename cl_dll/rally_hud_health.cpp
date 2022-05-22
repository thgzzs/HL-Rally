/***********************************************************************
*
*   File: - RallyHud.cpp
*    
*   Purpose: - Draws all sprites to the HUD
*
*   Author: - FragMented!
*
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*
*   Copyright 2001 FragMented and HL Rally Team
*
*
**************************************************************************
*/



#include "STDIO.H"
#include "STDLIB.H"
#include "MATH.H"

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>

#include "cvardef.h"
#include "usercmd.h"

#include "const.h"
#include "pm_defs.h"

#include "cl_entity.h"
#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "in_defs.h"

#include "ref_params.h"

extern cl_enginefunc_t gEngfuncs;

//struct ref_params_s *pparams;

int extern_dmg;
int extern_dmg_pos;

//
//-----------------------------------------------------
//

	int m_iHealth;
	int m_HUD_dmg_prev_front_l;
	int m_HUD_dmg_prev_front_m;
	int m_HUD_dmg_prev_front_r;
	int m_HUD_dmg_prev_mid_l;
	int m_HUD_dmg_prev_mid_m;
	int m_HUD_dmg_prev_mid_r;
	int m_HUD_dmg_prev_back_l;
	int m_HUD_dmg_prev_back_m;
	int m_HUD_dmg_prev_back_r;

	short int front_l;
	short int front_r;

	short int mid_l;
	short int mid_r;

	short int back_l;
	short int back_r;

	int dam_x, dam_y;
	int hud_carx, hud_cary;

	#define dam_r 255
	int dam_g, dam_b;

//
//-----------------------------------------------------
//

int CHudRallyHealth::Init(void)
{
	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem(this);

	front_l = front_r = mid_l = mid_r = back_l = back_r = 0;
	return 1;
}


//-----------------------------------------------------


void CHudRallyHealth::Think(void)
{
	if(!extern_dmg)
		return;

	switch(extern_dmg_pos)
	{
		case 0:
			front_r += extern_dmg *2;
			break;
		case 1:
			front_l += extern_dmg *2;
			break;
		case 2:
			back_r += extern_dmg *2;
			break;
		case 3:
			back_l += extern_dmg *2;
			break;
	}



	if(front_l > m_iHealth *1.92)
		front_l = m_iHealth *1.92;

	if(front_r > m_iHealth *1.92)
		front_r = m_iHealth *1.92;

	if(back_l > m_iHealth *1.92)
		back_l = m_iHealth *1.92;

	if(back_r > m_iHealth *1.92)
		back_r = m_iHealth *1.92;

	extern_dmg = 0;

}

//-----------------------------------------------------

int CHudRallyHealth::Draw(float flTime)
{

	Think();

	m_HUD_dmg_prev_front_l = gHUD.GetSpriteIndex( "front_l" );
	m_HUD_dmg_prev_front_m = gHUD.GetSpriteIndex( "front_m" );
	m_HUD_dmg_prev_front_r = gHUD.GetSpriteIndex( "front_r" );
	m_HUD_dmg_prev_mid_l = gHUD.GetSpriteIndex( "mid_l" );
	m_HUD_dmg_prev_mid_m = gHUD.GetSpriteIndex( "mid_m" );
	m_HUD_dmg_prev_mid_r = gHUD.GetSpriteIndex( "mid_r" );
	m_HUD_dmg_prev_back_l = gHUD.GetSpriteIndex( "back_l" );
	m_HUD_dmg_prev_back_m = gHUD.GetSpriteIndex( "back_m" );
	m_HUD_dmg_prev_back_r = gHUD.GetSpriteIndex( "back_r" );

	m_iHealth = gHUD.m_Health.m_iHealth;


	return 1;

//--------------------------

	// Variate colours
	int health_g = m_iHealth * 1.92;
	int health_b = m_iHealth * .255;


		
//--------------------------


	hud_carx = 80;
	hud_cary = ScreenHeight - gHUD.m_iFontHeight;

	dam_y = hud_cary;
	dam_x = hud_carx - 150 - gHUD.m_iFontHeight;

//--------------------------
		
	hud_cary = hud_cary - 150;

	// Draws car
	dam_x = hud_carx - 54;
	dam_y = hud_cary + 30;

	dam_g = health_g - front_l;
	dam_b = health_b;

	SPR_Set(gHUD.GetSprite(m_HUD_dmg_prev_front_l), dam_r, dam_g, dam_b);
	SPR_DrawAdditive(0, dam_x, dam_y, &gHUD.GetSpriteRect(m_HUD_dmg_prev_front_l));

	//--------------------------

	dam_x = hud_carx - 34;

	dam_g = health_g - (front_l + front_r) /2;
	dam_b = health_b;


	SPR_Set(gHUD.GetSprite(m_HUD_dmg_prev_front_m), dam_r, dam_g, dam_b);
	SPR_DrawAdditive(0, dam_x, dam_y, &gHUD.GetSpriteRect(m_HUD_dmg_prev_front_m));

	//--------------------------

	dam_x = hud_carx - 10;

	dam_g = health_g - front_r;
	dam_b = health_b;

	SPR_Set(gHUD.GetSprite(m_HUD_dmg_prev_front_r), dam_r, dam_g, dam_b);
	SPR_DrawAdditive(0, dam_x, dam_y, &gHUD.GetSpriteRect(m_HUD_dmg_prev_front_r));

	//--------------------------

	dam_x = hud_carx - 54;
	dam_y = hud_cary + 59;

	dam_g = health_g - (front_l + back_l) /2;
	dam_b = health_b;

	SPR_Set(gHUD.GetSprite(m_HUD_dmg_prev_mid_l), dam_r, dam_g, dam_b);
	SPR_DrawAdditive(0, dam_x, dam_y, &gHUD.GetSpriteRect(m_HUD_dmg_prev_mid_l));

	//--------------------------

	dam_x = hud_carx - 34;

	dam_g = health_g - (front_r + back_r + front_l + back_l) /4;
	dam_b = health_b;

	SPR_Set(gHUD.GetSprite(m_HUD_dmg_prev_mid_m), dam_r, dam_g, dam_b);
	SPR_DrawAdditive(0, dam_x, dam_y, &gHUD.GetSpriteRect(m_HUD_dmg_prev_mid_m));

	//--------------------------

	dam_x = hud_carx - 10;

	dam_g = health_g - (front_r + back_r) /2;
	dam_b = health_b;

	SPR_Set(gHUD.GetSprite(m_HUD_dmg_prev_mid_r), dam_r, dam_g, dam_b);
	SPR_DrawAdditive(0, dam_x, dam_y, &gHUD.GetSpriteRect(m_HUD_dmg_prev_mid_r));

	//--------------------------

	dam_x = hud_carx - 54;
	dam_y = hud_cary + 119;

	dam_g = health_g - back_l;
	dam_b = health_b;

	SPR_Set(gHUD.GetSprite(m_HUD_dmg_prev_back_l), dam_r, dam_g, dam_b);
	SPR_DrawAdditive(0, dam_x, dam_y, &gHUD.GetSpriteRect(m_HUD_dmg_prev_back_l));

	//--------------------------

	dam_x = hud_carx - 34;

	dam_g = health_g - (back_r + back_l)/2;
	dam_b = health_b ;

	SPR_Set(gHUD.GetSprite(m_HUD_dmg_prev_mid_m), dam_r, dam_g, dam_b);
	SPR_DrawAdditive(0, dam_x, dam_y, &gHUD.GetSpriteRect(m_HUD_dmg_prev_back_m));

	//--------------------------

	dam_x = hud_carx - 10;

	dam_g = health_g - back_r;
	dam_b = health_b;

	SPR_Set(gHUD.GetSprite(m_HUD_dmg_prev_mid_r), dam_r, dam_g, dam_b);
	SPR_DrawAdditive(0, dam_x, dam_y, &gHUD.GetSpriteRect(m_HUD_dmg_prev_back_r));

		


	return 1;

}

//-----------------------------------------------------