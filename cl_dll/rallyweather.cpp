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


/*
//
//-----------------------------------------------------
//

int CHudRally::Init(void)
{
	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem(this);
	return 1;
}


//-----------------------------------------------------

int CHudRally::Draw(float flTime)
{

	return 1;
}

//-----------------------------------------------------
*/