// studio_model.cpp
// routines for setting up to draw 3DStudio models

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "com_model.h"
#include "studio.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "dlight.h"
#include "triangleapi.h"

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>

#include "studio_util.h"
#include "r_studioint.h"

#include "StudioModelRenderer.h"
#include "GameStudioModelRenderer.h"

#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "in_defs.h"


// Crem pm
#include "pm_defs.h"
#include "pm_debug.h"
#include "pmtrace.h"

#include "rally.h"

// FragMented! v2 Start
#include "Rally_Effects.h"
#include "rally_vguifx.h"

// FragMented! v2 End
extern float fPlayerAngles;

#pragma warning( disable : 4244 )

extern "C" playermove_t *pmove;
extern "C" rally_t *g_rv;

// Extern CL_VIEWANGLE reference (input.cpp) FragMented
extern float RallyYaw;
extern float steer_wheel_angle;
extern float rpms;

// FragMented NEW suspension
// TEMPORARY VARIABLES UNTIL I FIND SOMEWHERE FOR EM TO GO
	float frontRight;
	float frontLeft;
	float backRight;
	float backLeft;
// end new

float view_pitch, view_roll;

float g_TraceDist;

float currentspeed;

static float g_last_pitch_angle = 0; // This is used to store last frame angles
static float g_last_roll_angle = 0;  // This is used to store last frame angles

static float g_roll_accel = 0 ;  // The amount to update the delta.
static float g_roll_delta = 0;  // The amount to update the roll position.

static float g_pitch_accel = 0 ;  // The amount to update the delta.
static float g_pitch_delta = 0;  // The amount to update the pitch position.
static float LastflYaw;

extern float g_accelerating;
extern float g_braking;
extern float g_reversing;
extern float g_handbraking;
extern float g_tractionloss;

// Global engine <-> studio model rendering code interface
engine_studio_api_t IEngineStudio;

#define TRACE_HEIGHT		3

//====================
// Init
//====================

void CStudioModelRenderer::Init( void )
{
	// Set up some variables shared with engine
	m_pCvarHiModels			= IEngineStudio.GetCvar( "cl_himodels" );
	m_pCvarDeveloper		= IEngineStudio.GetCvar( "developer" );
	m_pCvarDrawEntities		= IEngineStudio.GetCvar( "r_drawentities" );

	m_pChromeSprite			= IEngineStudio.GetChromeSprite();

	IEngineStudio.GetModelCounters( &m_pStudioModelCount, &m_pModelsDrawn );

	// Get pointers to engine data structures
	m_pbonetransform		= (float (*)[MAXSTUDIOBONES][3][4])IEngineStudio.StudioGetBoneTransform();
	m_plighttransform		= (float (*)[MAXSTUDIOBONES][3][4])IEngineStudio.StudioGetLightTransform();
	m_paliastransform		= (float (*)[3][4])IEngineStudio.StudioGetAliasTransform();
	m_protationmatrix		= (float (*)[3][4])IEngineStudio.StudioGetRotationMatrix();

	// FragMented!
	m_pCvarRotatefx = gEngfuncs.pfnRegisterVariable("rally_physrotate", "1", FCVAR_ARCHIVE);
	// FragMented - RC7
	m_pCvarChromefx =gEngfuncs.pfnRegisterVariable("rally_chrome", "1", FCVAR_ARCHIVE);

}




//======================
// CStudioModelRenderer
//======================

CStudioModelRenderer::CStudioModelRenderer( void )
{
	m_fDoInterp			= 1;
	m_fGaitEstimation	= 1;
	m_pCurrentEntity	= NULL;
	m_pCvarHiModels		= NULL;
	m_pCvarDeveloper	= NULL;
	m_pCvarDrawEntities	= NULL;
	m_pChromeSprite		= NULL;
	m_pStudioModelCount	= NULL;
	m_pModelsDrawn		= NULL;
	m_protationmatrix	= NULL;
	m_paliastransform	= NULL;
	m_pbonetransform	= NULL;
	m_plighttransform	= NULL;
	m_pStudioHeader		= NULL;
	m_pBodyPart			= NULL;
	m_pSubModel			= NULL;
	m_pPlayerInfo		= NULL;
	m_pRenderModel		= NULL;
}



//======================
// ~CStudioModelRenderer
//======================

CStudioModelRenderer::~CStudioModelRenderer( void )
{

}



//====================
// StudioCalcBoneAdj
//====================

void CStudioModelRenderer::StudioCalcBoneAdj( float dadt, float *adj, const byte *pcontroller1, const byte *pcontroller2, byte mouthopen )
{
	int					i, j;
	float				value;
	mstudiobonecontroller_t *pbonecontroller;
	
	pbonecontroller = (mstudiobonecontroller_t *)((byte *)m_pStudioHeader + m_pStudioHeader->bonecontrollerindex);

	for (j = 0; j < m_pStudioHeader->numbonecontrollers; j++)
	{
		i = pbonecontroller[j].index;

// Creme: fix to make bone controller 4 available
/*		if (i <= 3)
		{*/

			// check for 360% wrapping
			if (pbonecontroller[j].type & STUDIO_RLOOP)
			{
				if (abs(pcontroller1[i] - pcontroller2[i]) > 128)
				{
					int a, b;
					a = (pcontroller1[j] + 128) % 256;
					b = (pcontroller2[j] + 128) % 256;
					value = ((a * dadt) + (b * (1 - dadt)) - 128) * (360.0/256.0) + pbonecontroller[j].start;
				}
				else 
				{
					value = ((pcontroller1[i] * dadt + (pcontroller2[i]) * (1.0 - dadt))) * (360.0/256.0) + pbonecontroller[j].start;
				}
			}
			else 
			{
				value = (pcontroller1[i] * dadt + pcontroller2[i] * (1.0 - dadt)) / 255.0;
				if (value < 0) value = 0;
				if (value > 1.0) value = 1.0;
				value = (1.0 - value) * pbonecontroller[j].start + value * pbonecontroller[j].end;
			}
			// Con_DPrintf( "%d %d %f : %f\n", m_pCurrentEntity->curstate.controller[j], m_pCurrentEntity->latched.prevcontroller[j], value, dadt );
// Creme: fix to make bone controller 4 available
		/*}
		else
		{
			value = mouthopen / 64.0;
			if (value > 1.0) value = 1.0;				
			value = (1.0 - value) * pbonecontroller[j].start + value * pbonecontroller[j].end;
			// Con_DPrintf("%d %f\n", mouthopen, value );
		}*/

		switch(pbonecontroller[j].type & STUDIO_TYPES)
		{
		case STUDIO_XR:
		case STUDIO_YR:
		case STUDIO_ZR:
			adj[j] = value * (M_PI / 180.0);
			break;
		case STUDIO_X:
		case STUDIO_Y:
		case STUDIO_Z:
			adj[j] = value;
			break;
		}
	}
}



//========================
// StudioCalcBoneQuaterion
//========================

void CStudioModelRenderer::StudioCalcBoneQuaterion( int frame, float s, mstudiobone_t *pbone, mstudioanim_t *panim, float *adj, float *q )
{
	int					j, k;
	vec4_t				q1, q2;
	vec3_t				angle1, angle2;
	mstudioanimvalue_t	*panimvalue;

	for (j = 0; j < 3; j++)
	{
		if (panim->offset[j+3] == 0)
		{
			angle2[j] = angle1[j] = pbone->value[j+3]; // default;
		}
		else
		{
			panimvalue = (mstudioanimvalue_t *)((byte *)panim + panim->offset[j+3]);
			k = frame;
			// DEBUG
			if (panimvalue->num.total < panimvalue->num.valid)
				k = 0;
			while (panimvalue->num.total <= k)
			{
				k -= panimvalue->num.total;
				panimvalue += panimvalue->num.valid + 1;
				// DEBUG
				if (panimvalue->num.total < panimvalue->num.valid)
					k = 0;
			}
			// Bah, missing blend!
			if (panimvalue->num.valid > k)
			{
				angle1[j] = panimvalue[k+1].value;

				if (panimvalue->num.valid > k + 1)
				{
					angle2[j] = panimvalue[k+2].value;
				}
				else
				{
					if (panimvalue->num.total > k + 1)
						angle2[j] = angle1[j];
					else
						angle2[j] = panimvalue[panimvalue->num.valid+2].value;
				}
			}
			else
			{
				angle1[j] = panimvalue[panimvalue->num.valid].value;
				if (panimvalue->num.total > k + 1)
				{
					angle2[j] = angle1[j];
				}
				else
				{
					angle2[j] = panimvalue[panimvalue->num.valid + 2].value;
				}
			}
			angle1[j] = pbone->value[j+3] + angle1[j] * pbone->scale[j+3];
			angle2[j] = pbone->value[j+3] + angle2[j] * pbone->scale[j+3];
		}

		if (pbone->bonecontroller[j+3] != -1)
		{
			angle1[j] += adj[pbone->bonecontroller[j+3]];
			angle2[j] += adj[pbone->bonecontroller[j+3]];
		}
	}

	if (!VectorCompare( angle1, angle2 ))
	{
		AngleQuaternion( angle1, q1 );
		AngleQuaternion( angle2, q2 );
		QuaternionSlerp( q1, q2, s, q );
	}
	else
	{
		AngleQuaternion( angle1, q );
	}
}



//=======================
// StudioCalcBonePosition
//=======================
// Damage in this function?
void CStudioModelRenderer::StudioCalcBonePosition( int frame, float s, mstudiobone_t *pbone, mstudioanim_t *panim, float *adj, float *pos )
{
	int					j, k;
	mstudioanimvalue_t	*panimvalue;

	for (j = 0; j < 3; j++)
	{
		pos[j] = pbone->value[j]; // default;
		if (panim->offset[j] != 0)
		{
			panimvalue = (mstudioanimvalue_t *)((byte *)panim + panim->offset[j]);
			/*
			if (i == 0 && j == 0)
				Con_DPrintf("%d  %d:%d  %f\n", frame, panimvalue->num.valid, panimvalue->num.total, s );
			*/
			
			k = frame;
			// DEBUG
			if (panimvalue->num.total < panimvalue->num.valid)
				k = 0;
			// find span of values that includes the frame we want
			while (panimvalue->num.total <= k)
			{
				k -= panimvalue->num.total;
				panimvalue += panimvalue->num.valid + 1;
  				// DEBUG
				if (panimvalue->num.total < panimvalue->num.valid)
					k = 0;
			}
			// if we're inside the span
			if (panimvalue->num.valid > k)
			{
				// and there's more data in the span
				if (panimvalue->num.valid > k + 1)
				{
					pos[j] += (panimvalue[k+1].value * (1.0 - s) + s * panimvalue[k+2].value) * pbone->scale[j];
				}
				else
				{
					pos[j] += panimvalue[k+1].value * pbone->scale[j];
				}
			}
			else
			{
				// are we at the end of the repeating values section and there's another section with data?
				if (panimvalue->num.total <= k + 1)
				{
					pos[j] += (panimvalue[panimvalue->num.valid].value * (1.0 - s) + s * panimvalue[panimvalue->num.valid + 2].value) * pbone->scale[j];
				}
				else
				{
					pos[j] += panimvalue[panimvalue->num.valid].value * pbone->scale[j];
				}
			}
		}
		if ( pbone->bonecontroller[j] != -1 && adj )
		{
			pos[j] += adj[pbone->bonecontroller[j]];
		}
	}
}



//====================
// StudioSlerpBones
//====================

void CStudioModelRenderer::StudioSlerpBones( vec4_t q1[], float pos1[][3], vec4_t q2[], float pos2[][3], float s )
{
	int			i;
	vec4_t		q3;
	float		s1;

	if (s < 0) s = 0;
	else if (s > 1.0) s = 1.0;

	s1 = 1.0 - s;

	for (i = 0; i < m_pStudioHeader->numbones; i++)
	{
		QuaternionSlerp( q1[i], q2[i], s, q3 );
		q1[i][0] = q3[0];
		q1[i][1] = q3[1];
		q1[i][2] = q3[2];
		q1[i][3] = q3[3];
		pos1[i][0] = pos1[i][0] * s1 + pos2[i][0] * s;
		pos1[i][1] = pos1[i][1] * s1 + pos2[i][1] * s;
		pos1[i][2] = pos1[i][2] * s1 + pos2[i][2] * s;
	}
}



//====================
// StudioSetupBones
//====================

void CStudioModelRenderer::StudioSetupBones ( void )
{
	int					i;
	double				f;

	mstudiobone_t		*pbones;
	mstudioseqdesc_t	*pseqdesc;
	mstudioanim_t		*panim;

	static float		pos[MAXSTUDIOBONES][3];
	static vec4_t		q[MAXSTUDIOBONES];
	float				bonematrix[3][4];

	static float		pos2[MAXSTUDIOBONES][3];
	static vec4_t		q2[MAXSTUDIOBONES];
	static float		pos3[MAXSTUDIOBONES][3];
	static vec4_t		q3[MAXSTUDIOBONES];
	static float		pos4[MAXSTUDIOBONES][3];
	static vec4_t		q4[MAXSTUDIOBONES];

	if (m_pCurrentEntity->curstate.sequence >=  m_pStudioHeader->numseq) 
	{
		m_pCurrentEntity->curstate.sequence = 0;
	}

	pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + m_pCurrentEntity->curstate.sequence;

	f = StudioEstimateFrame( pseqdesc );

	if (m_pCurrentEntity->latched.prevframe > f)
	{
		//Con_DPrintf("%f %f\n", m_pCurrentEntity->prevframe, f );
	}

	panim = StudioGetAnim( m_pRenderModel, pseqdesc );
	StudioCalcRotations( pos, q, pseqdesc, panim, f );

	if (pseqdesc->numblends > 1)
	{
		float				s;
		float				dadt;

		panim += m_pStudioHeader->numbones;
		StudioCalcRotations( pos2, q2, pseqdesc, panim, f );

		dadt = StudioEstimateInterpolant();
		s = (m_pCurrentEntity->curstate.blending[0] * dadt + m_pCurrentEntity->latched.prevblending[0] * (1.0 - dadt)) / 255.0;

		StudioSlerpBones( q, pos, q2, pos2, s );

		if (pseqdesc->numblends == 4)
		{
			panim += m_pStudioHeader->numbones;
			StudioCalcRotations( pos3, q3, pseqdesc, panim, f );

			panim += m_pStudioHeader->numbones;
			StudioCalcRotations( pos4, q4, pseqdesc, panim, f );

			s = (m_pCurrentEntity->curstate.blending[0] * dadt + m_pCurrentEntity->latched.prevblending[0] * (1.0 - dadt)) / 255.0;
			StudioSlerpBones( q3, pos3, q4, pos4, s );

			s = (m_pCurrentEntity->curstate.blending[1] * dadt + m_pCurrentEntity->latched.prevblending[1] * (1.0 - dadt)) / 255.0;
			StudioSlerpBones( q, pos, q3, pos3, s );
		}
	}
	
	if (m_fDoInterp &&
		m_pCurrentEntity->latched.sequencetime &&
		( m_pCurrentEntity->latched.sequencetime + 0.2 > m_clTime ) && 
		( m_pCurrentEntity->latched.prevsequence < m_pStudioHeader->numseq ))
	{
		// blend from last sequence
		static float		pos1b[MAXSTUDIOBONES][3];
		static vec4_t		q1b[MAXSTUDIOBONES];
		float				s;

		pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + m_pCurrentEntity->latched.prevsequence;
		panim = StudioGetAnim( m_pRenderModel, pseqdesc );
		// clip prevframe
		StudioCalcRotations( pos1b, q1b, pseqdesc, panim, m_pCurrentEntity->latched.prevframe );

		if (pseqdesc->numblends > 1)
		{
			panim += m_pStudioHeader->numbones;
			StudioCalcRotations( pos2, q2, pseqdesc, panim, m_pCurrentEntity->latched.prevframe );

			s = (m_pCurrentEntity->latched.prevseqblending[0]) / 255.0;
			StudioSlerpBones( q1b, pos1b, q2, pos2, s );

			if (pseqdesc->numblends == 4)
			{
				panim += m_pStudioHeader->numbones;
				StudioCalcRotations( pos3, q3, pseqdesc, panim, m_pCurrentEntity->latched.prevframe );

				panim += m_pStudioHeader->numbones;
				StudioCalcRotations( pos4, q4, pseqdesc, panim, m_pCurrentEntity->latched.prevframe );

				s = (m_pCurrentEntity->latched.prevseqblending[0]) / 255.0;
				StudioSlerpBones( q3, pos3, q4, pos4, s );

				s = (m_pCurrentEntity->latched.prevseqblending[1]) / 255.0;
				StudioSlerpBones( q1b, pos1b, q3, pos3, s );
			}
		}

		s = 1.0 - (m_clTime - m_pCurrentEntity->latched.sequencetime) / 0.2;
		StudioSlerpBones( q, pos, q1b, pos1b, s );
	}
	else
	{
		//Con_DPrintf("prevframe = %4.2f\n", f);
		m_pCurrentEntity->latched.prevframe = f;
	}

	pbones = (mstudiobone_t *)((byte *)m_pStudioHeader + m_pStudioHeader->boneindex);

	// Hack by SaRcaZm (yes another one...)
	// This one fixes the wheel animations, gaitsequence must be getting reset somewhere
	if (m_pPlayerInfo)
		m_pPlayerInfo->gaitsequence = 3;

	// calc gait animation
	if (m_pPlayerInfo && m_pPlayerInfo->gaitsequence != 0)
	{
		if (m_pPlayerInfo->gaitsequence >= m_pStudioHeader->numseq) 
		{
			m_pPlayerInfo->gaitsequence = 0;
		}

		pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + m_pPlayerInfo->gaitsequence;

		panim = StudioGetAnim( m_pRenderModel, pseqdesc );
		StudioCalcRotations( pos2, q2, pseqdesc, panim, m_pPlayerInfo->gaitframe );

		for (i = 0; i < m_pStudioHeader->numbones; i++)
		{
			if (strcmp( pbones[i].name, "Bip01 Spine") == 0)
				break;
			memcpy( pos[i], pos2[i], sizeof( pos[i] ));
			memcpy( q[i], q2[i], sizeof( q[i] ));
		}
	}



	for (i = 0; i < m_pStudioHeader->numbones; i++) 
	{
		QuaternionMatrix( q[i], bonematrix );

		bonematrix[0][3] = pos[i][0];
		bonematrix[1][3] = pos[i][1];
		bonematrix[2][3] = pos[i][2];

		if (pbones[i].parent == -1) 
		{
			if ( IEngineStudio.IsHardware() )
			{
				ConcatTransforms ((*m_protationmatrix), bonematrix, (*m_pbonetransform)[i]);
				ConcatTransforms ((*m_protationmatrix), bonematrix, (*m_plighttransform)[i]);
			}
			else
			{
				ConcatTransforms ((*m_paliastransform), bonematrix, (*m_pbonetransform)[i]);
				ConcatTransforms ((*m_protationmatrix), bonematrix, (*m_plighttransform)[i]);
			}

			// Apply client-side effects to the transformation matrix
			StudioFxTransform( m_pCurrentEntity, (*m_pbonetransform)[i] );
		} 
		else 
		{
			ConcatTransforms ((*m_pbonetransform)[pbones[i].parent], bonematrix, (*m_pbonetransform)[i]);
			ConcatTransforms ((*m_plighttransform)[pbones[i].parent], bonematrix, (*m_plighttransform)[i]);
		}
	}
}


/*
====================
StudioSaveBones

====================
*/
void CStudioModelRenderer::StudioSaveBones( void )
{
	int		i;

	mstudiobone_t		*pbones;
	pbones = (mstudiobone_t *)((byte *)m_pStudioHeader + m_pStudioHeader->boneindex);

	m_nCachedBones = m_pStudioHeader->numbones;

	for (i = 0; i < m_pStudioHeader->numbones; i++) 
	{
		for(int j = 0; j < 3; j++)
		{
		/*	vec3_t bonepos;
			for(int k = 0; k < 3; k++)
			{
				bonepos[k] = (*m_pbonetransform)[i][j][k];
			//	gEngfuncs.Con_DPrintf("%.2f\n", Length((*m_pbonetransform)[i][j] - boneoffs));
			}*/
			//	(*m_pbonetransform)[i][j][k] -= boneoffs[j]/1000;

		//	gEngfuncs.Con_DPrintf("%.2f\n", Length(bonepos - pmove->vdamage));

			/*if(Length(bonepos - pmove->vdamage) > 1.1)/*< .9 && Length(bonepos - pmove->vdamage) > .6)
			{
				for(k = 0; k < 3; k++)
				{
					boneoffs[i][k] += .05;
			//		pmove->vdamage[k] = 0;
				}
			}*/
			

/*
			if(m_pCurrentEntity->player)
			for(k = 0; k < 3; k++)
			{
				(*m_pbonetransform)[i][j][k] -= pmove->vdamage[k]/10;
			}*/
		}

		strcpy( m_nCachedBoneNames[i], pbones[i].name );
		//gEngfuncs.Con_DPrintf("%s\n", pbones[i].name);
		MatrixCopy( (*m_pbonetransform)[i], m_rgCachedBoneTransform[i] );
		MatrixCopy( (*m_plighttransform)[i], m_rgCachedLightTransform[i] );
	}

/*	for(int k = 0; k < 3; k++)
		{
			pmove->vdamage[k] = 0.00;
		}*/
}



//====================
// StudioMergeBones
//====================

void CStudioModelRenderer::StudioMergeBones ( model_t *m_pSubModel )
{
	int					i, j;
	double				f;
	int					do_hunt = true;

	mstudiobone_t		*pbones;
	mstudioseqdesc_t	*pseqdesc;
	mstudioanim_t		*panim;

	static float		pos[MAXSTUDIOBONES][3];
	float				bonematrix[3][4];
	static vec4_t		q[MAXSTUDIOBONES];

	if (m_pCurrentEntity->curstate.sequence >=  m_pStudioHeader->numseq) 
	{
		m_pCurrentEntity->curstate.sequence = 0;
	}

	pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + m_pCurrentEntity->curstate.sequence;

	f = StudioEstimateFrame( pseqdesc );

/*	if (m_pCurrentEntity->latched.prevframe > f)
	{
		//Con_DPrintf("%f %f\n", m_pCurrentEntity->prevframe, f );
	}*/

	panim = StudioGetAnim( m_pSubModel, pseqdesc );
	StudioCalcRotations( pos, q, pseqdesc, panim, f );

	pbones = (mstudiobone_t *)((byte *)m_pStudioHeader + m_pStudioHeader->boneindex);


	for (i = 0; i < m_pStudioHeader->numbones; i++) 
	{
		for (j = 0; j < m_nCachedBones; j++)
		{
			if (stricmp(pbones[i].name, m_nCachedBoneNames[j]) == 0)
			{
				MatrixCopy( m_rgCachedBoneTransform[j], (*m_pbonetransform)[i] );
				MatrixCopy( m_rgCachedLightTransform[j], (*m_plighttransform)[i] );
				break;
			}
		}
		if (j >= m_nCachedBones)
		{
			QuaternionMatrix( q[i], bonematrix );

			bonematrix[0][3] = pos[i][0];
			bonematrix[1][3] = pos[i][1];
			bonematrix[2][3] = pos[i][2];

			if (pbones[i].parent == -1) 
			{
				if ( IEngineStudio.IsHardware() )
				{
					ConcatTransforms ((*m_protationmatrix), bonematrix, (*m_pbonetransform)[i]);
					ConcatTransforms ((*m_protationmatrix), bonematrix, (*m_plighttransform)[i]);
				}
				else
				{
					ConcatTransforms ((*m_paliastransform), bonematrix, (*m_pbonetransform)[i]);
					ConcatTransforms ((*m_protationmatrix), bonematrix, (*m_plighttransform)[i]);
				}

				// Apply client-side effects to the transformation matrix
				StudioFxTransform( m_pCurrentEntity, (*m_pbonetransform)[i] );
			} 
			else 
			{
				ConcatTransforms ((*m_pbonetransform)[pbones[i].parent], bonematrix, (*m_pbonetransform)[i]);
				ConcatTransforms ((*m_plighttransform)[pbones[i].parent], bonematrix, (*m_plighttransform)[i]);
			}
		}
	}
}



//====================
// StudioGetAnim
//====================

mstudioanim_t *CStudioModelRenderer::StudioGetAnim( model_t *m_pSubModel, mstudioseqdesc_t *pseqdesc )
{
	mstudioseqgroup_t	*pseqgroup;
	cache_user_t *paSequences;

	pseqgroup = (mstudioseqgroup_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqgroupindex) + pseqdesc->seqgroup;

	if (pseqdesc->seqgroup == 0)
	{
		return (mstudioanim_t *)((byte *)m_pStudioHeader + pseqgroup->data + pseqdesc->animindex);
	}

	paSequences = (cache_user_t *)m_pSubModel->submodels;

	if (paSequences == NULL)
	{
		paSequences = (cache_user_t *)IEngineStudio.Mem_Calloc( 16, sizeof( cache_user_t ) ); // UNDONE: leak!
		m_pSubModel->submodels = (dmodel_t *)paSequences;
	}

	if (!IEngineStudio.Cache_Check( (struct cache_user_s *)&(paSequences[pseqdesc->seqgroup])))
	{
		gEngfuncs.Con_DPrintf("loading %s\n", pseqgroup->name );
		IEngineStudio.LoadCacheFile( pseqgroup->name, (struct cache_user_s *)&paSequences[pseqdesc->seqgroup] );
	}
	return (mstudioanim_t *)((byte *)paSequences[pseqdesc->seqgroup].data + pseqdesc->animindex);
}



//====================
// Studio_FxTransform
// Renders Models in Kmode other than NORM
//====================

void CStudioModelRenderer::StudioFxTransform( cl_entity_t *ent, float transform[3][4] )
{
	switch( ent->curstate.renderfx )
	{
	case kRenderFxDistort:
	case kRenderFxHologram:
		if ( gEngfuncs.pfnRandomLong(0,49) == 0 )
		{
			int axis = gEngfuncs.pfnRandomLong(0,1);
			if ( axis == 1 ) // Choose between x & z
				axis = 2;
			VectorScale( transform[axis], gEngfuncs.pfnRandomFloat(1,1.484), transform[axis] );
		}
		else if ( gEngfuncs.pfnRandomLong(0,49) == 0 )
		{
			float offset;
			int axis = gEngfuncs.pfnRandomLong(0,1);
			if ( axis == 1 ) // Choose between x & z
				axis = 2;
			offset = gEngfuncs.pfnRandomFloat(-10,10);
			transform[gEngfuncs.pfnRandomLong(0,2)][3] += offset;
		}
	break;
	case kRenderFxExplode:
		{
			float scale;

			scale = 1.0 + ( m_clTime - ent->curstate.animtime) * 10.0;
			if ( scale > 2 )	// Don't blow up more than 200%
				scale = 2;
			transform[0][1] *= scale;
			transform[1][1] *= scale;
			transform[2][1] *= scale;
		}
	break;

	}
}




//====================
// StudioCalcRotations
//====================


void CStudioModelRenderer::StudioCalcRotations ( float pos[][3], vec4_t *q, mstudioseqdesc_t *pseqdesc, mstudioanim_t *panim, float f )
{
	int					i;
	int					frame;
	mstudiobone_t		*pbone;

	float				s;
	float				adj[MAXSTUDIOCONTROLLERS];
	float				dadt;

	if (f > pseqdesc->numframes - 1)
	{
		f = 0;	// bah, fix this bug with changing sequences too fast
	}

	// Dont allow frames to go negative
	else if ( f < -0.01 )
	{
		f = -0.01;
	}

	frame = (int)f;

	// Con_DPrintf("%d %.4f %.4f %.4f %.4f %d\n", m_pCurrentEntity->curstate.sequence, m_clTime, m_pCurrentEntity->animtime, m_pCurrentEntity->frame, f, frame );
	// gEngfuncs.Con_DPrintf( "ROLL %f PITCH %f YAW %f\n", m_pCurrentEntity->angles[ROLL], m_pCurrentEntity->angles[PITCH], m_pCurrentEntity->angles[YAW] );
	// Con_DPrintf("frame %d %d\n", frame1, frame2 );


	dadt = StudioEstimateInterpolant( );
	s = (f - frame);

	// add in programtic controllers
	pbone		= (mstudiobone_t *)((byte *)m_pStudioHeader + m_pStudioHeader->boneindex);

	StudioCalcBoneAdj( dadt, adj, m_pCurrentEntity->curstate.controller, m_pCurrentEntity->latched.prevcontroller, m_pCurrentEntity->mouth.mouthopen );

	for (i = 0; i < m_pStudioHeader->numbones; i++, pbone++, panim++) 
	{
		StudioCalcBoneQuaterion( frame, s, pbone, panim, adj, q[i] );

		StudioCalcBonePosition( frame, s, pbone, panim, adj, pos[i] );

		// Bones
		// 1 = Front Bonnet Whole Front Piece actually including wheels
		// 2 = Front Bonnet Without Wheels
		// 3 = Front Left Headlight Area, Corner of Bonnet (Use [2] for damage)
		// 4 = Front Center Between Headlights
		// 5 = Front Right Headlight Area, Corner of Bonnet
		// 6 = Front Left Wheel (Use [1] for suspension)
		// 7 = Front Right Wheel (Use [1] for suspension)
		// 8 = Body of car, Wise not to modify... Excludes Front Bumper and Rear Bumper.. A holder / splitter bone
		// 9 = Right Hand Door (Use [0] for Damage)
		// 10 = Left Hand Door (Use [0] for Damage)
		// 11 = Roof Use [1] for upwards push damage if implemented =/
		// 12 = Arse Bone Splitter (Dont use, will look crappy)
		// 13 = Left Back Bone Splitter (No use)
		// 14 = Right Back Bone Splitter (No use)
		// 15 = Front Left splitter (no use)
		// 16 = Front Right splitter (no use)
		// 17 = Back Bumper Including Wheels
		// 18 = Rear Wheel(s) Some cars right only =/
		// 19 = Rear Bumper no wheels
		// 20 = Left Back Damage Point (Use [2] for damage)
		// 21 = Center Back Damage Point (Use [2] for damage)
		// 22 = Right Back Damage Point (Use [2] for damage)
		// 23 = Fucked =/ either wheels both or back right panel
	//	gEngfuncs.Con_DPrintf("%s\n", pbone->name);



		if (m_pCurrentEntity->index == gEngfuncs.GetLocalPlayer()->index)
		{
			if (!strcmpi(pbone->name, "SHOCK-RghFron"))
			{
				pos[i][2] += frontRight;
			}
			else if (!strcmpi(pbone->name, "SHOCK-LefFron"))
			{
				pos[i][2] += frontLeft;
			}
			else if (!strcmpi(pbone->name, "SHOCK-LefBack"))
			{
				pos[i][2] += backLeft;
			}
			else if (!strcmpi(pbone->name, "SHOCK-RghBack"))
			{
				pos[i][2] += backRight;
			}
		}

		// if (0 && i == 0)
		//	Con_DPrintf("%d %d %d %d\n", m_pCurrentEntity->curstate.sequence, frame, j, k );
	}

	if (pseqdesc->motiontype & STUDIO_X)
	{
		pos[pseqdesc->motionbone][0] = 0.0;
	}
	if (pseqdesc->motiontype & STUDIO_Y)
	{
		pos[pseqdesc->motionbone][1] = 0.0;
	}
	if (pseqdesc->motiontype & STUDIO_Z)
	{
		pos[pseqdesc->motionbone][2] = 0.0;
	}

	s = 0 * ((1.0 - (f - (int)(f))) / (pseqdesc->numframes)) * m_pCurrentEntity->curstate.framerate;

	if (pseqdesc->motiontype & STUDIO_LX)
	{
		pos[pseqdesc->motionbone][0] += s * pseqdesc->linearmovement[0];
	}
	if (pseqdesc->motiontype & STUDIO_LY)
	{
		pos[pseqdesc->motionbone][1] += s * pseqdesc->linearmovement[1];
	}
	if (pseqdesc->motiontype & STUDIO_LZ)
	{
		pos[pseqdesc->motionbone][2] += s * pseqdesc->linearmovement[2];
	}
}




//==========================
// StudioEstimateInterpolant
//==========================

float CStudioModelRenderer::StudioEstimateInterpolant( void )
{
	float dadt = 1.0;

	if ( m_fDoInterp && ( m_pCurrentEntity->curstate.animtime >= m_pCurrentEntity->latched.prevanimtime + 0.01 ) )
	{
		dadt = (m_clTime - m_pCurrentEntity->curstate.animtime) / 0.1;
		if (dadt > 2.0)
		{
			dadt = 2.0;
		}
	}
	return dadt;
}




//====================
// StudioEstimateFrame
//====================

float CStudioModelRenderer::StudioEstimateFrame( mstudioseqdesc_t *pseqdesc )
{
	double				dfdt, f;

	if ( m_fDoInterp )
	{
		if ( m_clTime < m_pCurrentEntity->curstate.animtime )
		{
			dfdt = 0;
		}
		else
		{
			dfdt = (m_clTime - m_pCurrentEntity->curstate.animtime) * m_pCurrentEntity->curstate.framerate * pseqdesc->fps;

		}
	}
	else
	{
		dfdt = 0;
	}

	if (pseqdesc->numframes <= 1)
	{
		f = 0;
	}
	else
	{
		f = (m_pCurrentEntity->curstate.frame * (pseqdesc->numframes - 1)) / 256.0;
	}
 	
	f += dfdt;

	if (pseqdesc->flags & STUDIO_LOOPING) 
	{
		if (pseqdesc->numframes > 1)
		{
			f -= (int)(f / (pseqdesc->numframes - 1)) *  (pseqdesc->numframes - 1);
		}
		if (f < 0) 
		{
			f += (pseqdesc->numframes - 1);
		}
	}
	else 
	{
		if (f >= pseqdesc->numframes - 1.001) 
		{
			f = pseqdesc->numframes - 1.001;
		}
		if (f < 0.0) 
		{
			f = 0.0;
		}
	}
	return f;
}





















//====================================================================================================
// PROCEDURES AFFECTING MODEL ROTATION START HERE
//====================================================================================================



















#define D 0.0174532
#define CarLength 32


//#define TN 1 // Debug Particle Lines


bool CStudioModelRenderer::TerrainTrace ( float *n, float *vOfs, float *vUp, bool interior, float *endpos) 
{
	//float       ang;
	vec3_t src_down, dest_down, org;
	pmtrace_t trace;
	float testdepth = 2;

#ifdef TN 
	vec3_t tempvec;
#endif

	if(interior)
	{
		cl_entity_t *pthisplayer = gEngfuncs.GetLocalPlayer();
		VectorCopy(pthisplayer->origin, org);
	}
	else
		VectorCopy(m_pCurrentEntity->origin, org);


	VectorScale(vUp, 2, src_down);
	VectorAdd(src_down, org, src_down);
	VectorAdd(src_down, vOfs,          src_down);

	VectorScale(vUp, -testdepth, dest_down);
	VectorAdd(dest_down, org, dest_down);
	VectorAdd(dest_down, vOfs,          dest_down);

//	trace = pmove->PM_PlayerTrace (src_down, dest_down, PM_NORMAL, -1 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( src_down, dest_down, PM_NORMAL, -1, &trace );					

	VectorCopy (trace.plane.normal, n); // old system

	VectorCopy (trace.endpos, endpos);

	g_TraceDist = trace.fraction;

	if(trace.ent == -1)
	{
	//	gEngfuncs.Con_DPrintf("air\n\n");
		return true;
	}
	else
		return false;


#ifdef TN
	VectorScale (trace.plane.normal, 32, tempvec);
	VectorAdd   (trace.endpos, tempvec, tempvec);
//	PM_ParticleLine (trace.endpos, tempvec, 255, pmove->frametime*1.5, 0);
	gEngfuncs.pEfxAPI->R_ParticleLine (trace.endpos, tempvec, 255, 255, 255, pmove->frametime*1.5);
#endif


}

// SaRcaZm - V8 - Start
// PVS Entity Tracking
bool iWasInPVS[2][MAX_PLAYERS];
int iPVSIndex = 0;
int dPVSTime = -1;

bool WasInPVS (int index)
{
	return iWasInPVS[(iPVSIndex) ? 0 : 1][index];
}

void SetIsInPVS (int index, int time)
{
	if (dPVSTime != time)	// New frame
	{
		iPVSIndex = (iPVSIndex) ? 0 : 1;
		dPVSTime = time;

		for (int i = 0; i < MAX_PLAYERS; i++)
			iWasInPVS[iPVSIndex][i] = false;
	}

	iWasInPVS[iPVSIndex][index] = true;
}
// SaRcaZm - V8 - End

//======================
// StudioRallyRender
//======================
// Creme: 5-Aug-2003

void CStudioModelRenderer::StudioRallyRender( void )
{

	static float roll_hist;
	static float pitch_hist;

	vec3_t			angles;

	vec3_t		vel, r, n;
	vec3_t		f1,  f2, b1, b2, c1, tAngles;
	vec3_t		df1,df2,db1,db2,dc1, du, dv, dn, debugPline;

	float ang, spd;
	vec3_t		vForward, vRight, vUp, vTemp, vOfs;

	bool airborne = false;
	bool isLocal = false;
	bool isInterior = false;

	// Creme: Over-ride the hull setting
	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetTraceHull( 0 );

	cl_entity_t *pthisplayer = gEngfuncs.GetLocalPlayer();

	if(!m_pCurrentEntity->player)
	{
		angles[ROLL] = m_pCurrentEntity->curstate.angles[ROLL];
		angles[PITCH] = m_pCurrentEntity->curstate.angles[PITCH];
		angles[YAW] = m_pCurrentEntity->curstate.angles[YAW]; 
		// Will be replaced below
		// if the player is us
	}


	// Calculate forward speed!
	VectorCopy (pmove->velocity, n);  n[2] = 0;  spd = Length(pmove->velocity);  // n);
	currentspeed = spd;

	// Is the Entity A Player?
	if(m_pCurrentEntity->player)
	{

		//==================================
		// NON-LAGGY YAW HACK (FragMented)
		//==================================
		if(pthisplayer->index == m_pCurrentEntity->index)	// Is the player us?
		{
			angles[YAW] = RallyYaw;
			isLocal = true;

			AngleVectors(pmove->vuser1, vForward, vRight, vUp);

		} else {
			// Creme: 4-Aug-2003, othercars speed-fix
			spd = Length(m_pCurrentEntity->baseline.basevelocity);

			// SaRcaZm - V8 - Start
			angles[YAW] = m_pCurrentEntity->angles[YAW];
			m_pCurrentEntity->baseline.vuser1[YAW] = angles[YAW];
			//if (!WasInPVS (m_pCurrentEntity->index))
			//{
				m_pCurrentEntity->baseline.vuser1[PITCH] = 
				m_pCurrentEntity->baseline.vuser1[ROLL] = 0;
				//gEngfuncs.Con_DPrintf ("Wasn't in PVS - %s\n", g_PlayerInfoList[m_pCurrentEntity->index].name);
			//}

			//gEngfuncs.Con_DPrintf ("Angles: %f, %f, %f\n", m_pCurrentEntity->baseline.vuser1[0], m_pCurrentEntity->baseline.vuser1[1], m_pCurrentEntity->baseline.vuser1[2]);
			AngleVectors(m_pCurrentEntity->baseline.vuser1, vForward, vRight, vUp);
			// SaRcaZm - V8 - End

			isLocal = false;
		}


		//========================================
		// CLIENT SIDE ROTATION (FragMented, Crem)
		//========================================


		vOfs[0]=0; vOfs[1]=0; vOfs[2]=0;
		airborne = TerrainTrace (c1, vOfs, vUp, false, dc1); //Centre

		if(isLocal)
		{
			VectorScale(vRight, 8, vRight);
			VectorScale(vForward, 12, vForward);

			// Creme: Ok theres some serious trickery Ive used here to optimise, 
			// but I'm fairly certain theres an even faster way to work all these out.

			VectorAdd(vRight, vForward, vOfs); // Front, Right
			if (!TerrainTrace (f1, vOfs, vUp, false, df1)) {
				airborne = false;
			}

			VectorInverse(vOfs);
			if (!TerrainTrace (b2, vOfs, vUp, false, db2)) { // Back, Left
				airborne = false;
			}

			VectorInverse(vRight);
			VectorAdd(vRight, vForward, vOfs);
			if (!TerrainTrace (f2,  vOfs, vUp, false, df2)) { // Front, Left
				airborne = false;
			}

			VectorInverse(vOfs);
			if (!TerrainTrace (b1, vOfs, vUp, false, db1)) { // Back, Right
				airborne = false;
			}

		}


		if (!airborne) 
		{
			if(isLocal)
			{
				// First triangle
				//df1, df2, db2    and du, dv for x-product, dn for normal

				VectorSubtract (db2, df1, du);
				VectorSubtract (db2, df2, dv);

				CrossProduct (du, dv, dn); 

				// Second triangle
				//db2, db1, df1    and du, dv for x-product, dn for normal

				VectorSubtract (df1, db2, du);
				VectorSubtract (df1, db1, dv);

				CrossProduct (du, dv, n);

				// Add normals together and average

				VectorAdd(dn, n, dn); // reuse dn
				VectorScale(dn, 0.5, n); // returns n our car angle!


				//VectorAdd(dn, modelpos, debugPline)

				//gEngfuncs.pEfxAPI->R_ParticleLine 
				//	(modelpos, debugPline, 255, 255, 255, pmove->frametime*1.5);
	
			
			}
			else
			{
				n[0] = c1[0];
				n[1] = c1[1];
				n[2] = c1[2];
			}



			ang = angles[YAW] * D;


			// Do rotation to orientate terrain to model
			r[0] = (n[0] * cos(ang)) + (n[1] * sin(ang));
			r[1] = -(n[0]* sin(ang)) + (n[1] * cos(ang));
			r[2] = n[2];
			
			float rough_mul = 0.006;

			vec3_t start, end;
			VectorCopy( m_pCurrentEntity->origin, start );
			VectorCopy( m_pCurrentEntity->origin, end );
			end[2] -= 64;

			char *TextureName = (char *)gEngfuncs.pEventAPI->EV_TraceTexture( 0, start, end);

			if(TextureName)
			{
				char *tmp = TextureName;
				while (*tmp)
				{
					*tmp = tolower (*tmp);
					tmp++;
				}

				if (!(strstr(TextureName, "dirt") || strstr(TextureName, "gras") || strstr(TextureName, "snow"))) {
					if (strstr(TextureName, "road")) {

						rough_mul = 0.002;

					}
				}
//				pmove->Con_DPrintf ("      Car is on TextureName: %s\n", TextureName);
			}


			tAngles[PITCH] = (atan2(r[2], r[0]) * 180 / M_PI - 90);
			tAngles[ROLL]  = (atan2(r[2], r[1]) * 180 / M_PI - 90);
			
			float rough_spd_thresh = 300;

			if (spd < rough_spd_thresh) {
				tAngles[PITCH] += (gEngfuncs.pfnRandomFloat(spd * -rough_mul, spd * rough_mul));
				tAngles[ROLL]  += (gEngfuncs.pfnRandomFloat(spd * -rough_mul, spd * rough_mul));
			} else {
				tAngles[PITCH] += (gEngfuncs.pfnRandomFloat(rough_spd_thresh  * -rough_mul, rough_spd_thresh  * rough_mul));
				tAngles[ROLL]  += (gEngfuncs.pfnRandomFloat(rough_spd_thresh  * -rough_mul, rough_spd_thresh  * rough_mul));
			}			

			if(isLocal)
			{
				angles[PITCH] = (tAngles[PITCH]);// - pitch_hist*1) / 2;
				angles[ROLL]  = (tAngles[ROLL]);  //+ roll_hist*1)  / 2;
			}
			else
			{
				angles[PITCH] = tAngles[PITCH];
				angles[ROLL]  = tAngles[ROLL];
			}
			angles[PITCH] =- angles[PITCH]; // Invert


			//===============================================
			// Roll with mouse_x and pitch with accel / brake
			//===============================================

			m_pCvarRotatefx = IEngineStudio.GetCvar( "rally_physrotate" );

			if(isLocal && spd > 20 && m_pCvarRotatefx->value)
			{
				float mx_roll;

				// Work out the dot product between Right and Velocity
				mx_roll = DotProduct(pmove->velocity, pmove->right) / Length(pmove->velocity);

				// If Dot is more than this, max roll
				float roll_limit = 0.2;

				// Amount to roll at max.
				float limit_multiplier = 25;

				// if less than limit, roll lots
				if (fabs(mx_roll) < roll_limit) {
					mx_roll *= limit_multiplier;
				} else {
					mx_roll *= (limit_multiplier/10);

					if (mx_roll > 0) {
						mx_roll += roll_limit * limit_multiplier;
					} else {
						mx_roll -= roll_limit * limit_multiplier;
					}

				}

				angles[ROLL] += mx_roll;

				if (g_braking || g_handbraking)
					angles[PITCH] += spd / 150;

				if (spd < 300) {
					if(g_accelerating && (g_tractionloss == 0))
						angles[PITCH] -= 3.7 - (spd / 200);
				}

			}

			if(isLocal)
			{
				angles[PITCH] = (angles[PITCH] + pitch_hist*4) / 5;
				angles[ROLL]  = (angles[ROLL]  + roll_hist*3)  / 4;

			
				AngleVectors(angles, vForward, vRight, vUp);
				VectorScale(vRight, 8, vRight);
				VectorScale(vForward, 12, vForward);

				VectorAdd(vRight, vForward, vOfs); // Front, Right
				TerrainTrace (f1, vOfs, vUp, false, df1);
				frontRight = (g_TraceDist - 0.5) * 4;

				VectorInverse(vOfs);
				TerrainTrace (b2, vOfs, vUp, false, db2); // Back, Left
				backLeft = (g_TraceDist - 0.5) * 4;

				VectorInverse(vRight);
				VectorAdd(vRight, vForward, vOfs);
				TerrainTrace (f2,  vOfs, vUp, false, df2); // Front, Left
				frontLeft = (g_TraceDist - 0.5) * 4;
						
				VectorInverse(vOfs);
				TerrainTrace (b1, vOfs, vUp, false, db1); // Back, Right
				backRight = (g_TraceDist - 0.5) * 4;

			}


		}

		// Air Borne!
		else
		{
			if (isLocal)
			{
				angles[PITCH] = pitch_hist + (12.4 * pmove->frametime);
				angles[ROLL] = roll_hist;

				AngleVectors(angles, vForward, vRight, vUp);
				VectorScale(vRight, 8, vRight);
				VectorScale(vForward, 12, vForward);

				VectorAdd(vRight, vForward, vOfs); // Front, Right
				TerrainTrace (f1, vOfs, vUp, false, df1);
				frontRight = (g_TraceDist - 0.5) * 4;

				VectorInverse(vOfs);
				TerrainTrace (b2, vOfs, vUp, false, db2); // Back, Left
				backLeft = (g_TraceDist - 0.5) * 4;

				VectorInverse(vRight);
				VectorAdd(vRight, vForward, vOfs);
				TerrainTrace (f2,  vOfs, vUp, false, df2); // Front, Left
				frontLeft = (g_TraceDist - 0.5) * 4;
							
				VectorInverse(vOfs);
				TerrainTrace (b1, vOfs, vUp, false, db1); // Back, Right
				backRight = (g_TraceDist - 0.5) * 4;
			}
			else
			{
				angles[PITCH] = m_pCurrentEntity->prevstate.angles[PITCH] + (20.4 * pmove->frametime);
				angles[ROLL] = m_pCurrentEntity->prevstate.angles[ROLL];
			}

			if (angles[ROLL] > 0)
				angles[ROLL] += 12.4 * pmove->frametime;
			else
				angles[ROLL] -= 12.4 * pmove->frametime;
		}


		if(isLocal)
		{
			roll_hist = angles[ROLL];
			pitch_hist = angles[PITCH];
		}

	} // End Player Code


	//==============================================
	// STEER THE INTERIOR WHEEL WITH MX (FragMented)
	//==============================================
	else if(!strcmpi("models/interior.mdl", m_pCurrentEntity->model->name))
	{
		float flYaw;	 // view direction relative to movement

		isInterior = isLocal = true;

		m_pCurrentEntity->curstate.controller[0] = m_pCurrentEntity->latched.prevcontroller[0];

		// Speedo
		float ftemp = (-spd / 3.1) + 12;
//		pmove->Con_DPrintf ("      spd: %f  byte spd: %i\n", ftemp, byte(ftemp) );
		if (ftemp < -250) { ftemp = -250; }
		if (ftemp > -5) { ftemp = -5; }
		m_pCurrentEntity->curstate.controller[1] = ftemp;


		// RPM Needle
		ftemp = -rpms *1.2;
//		pmove->Con_DPrintf ("      rpms: %f  byte rpm: %i\n", ftemp, byte(ftemp) );
		if (ftemp < -255) { ftemp = -255; }
		m_pCurrentEntity->curstate.controller[2] = ftemp;

		// View angles
		if (pmove->onground != -1) 
		{
			AngleVectors(pmove->vuser1, vForward, vRight, vUp);

			vOfs[0]=0; vOfs[1]=0; vOfs[2]=0;
			TerrainTrace (c1, vOfs, vUp, true, dc1); //Centre
				
			VectorScale(vRight, 8, vRight);
			VectorScale(vForward, 12, vForward);

			// Creme: Ok theres some serious trickery Ive used here to optimise, 
			// but I'm fairly certain theres an even faster way to work all these out.

			VectorAdd(vRight, vForward, vOfs); // Front, Right
			TerrainTrace (f1, vOfs, vUp, true, df1);

			VectorInverse(vOfs);
			TerrainTrace (b2, vOfs, vUp, true, db2); // Back, Left

			VectorInverse(vRight);
			VectorAdd(vRight, vForward, vOfs);
			TerrainTrace (f2,  vOfs, vUp, true, df2); // Front, Left
					
			VectorInverse(vOfs);
			TerrainTrace (b1, vOfs, vUp, true, db1);// Back, Right

			// First triangle
			//df1, df2, db2    and du, dv for x-product, dn for normal

			VectorSubtract (db2, df1, du);
			VectorSubtract (db2, df2, dv);

			CrossProduct (du, dv, dn); 

			// Second triangle
			//db2, db1, df1    and du, dv for x-product, dn for normal

			VectorSubtract (df1, db2, du);
			VectorSubtract (df1, db1, dv);

			CrossProduct (du, dv, n);

			// Add normals together and average

			VectorAdd(dn, n, dn); // reuse dn
			VectorScale(dn, 0.5, n); // returns n our car angle!

			ang = RallyYaw * D;

			// Do rotation to orientate terrain to model
			r[0] = (n[0] * cos(ang)) + (n[1] * sin(ang));
			r[1] = -(n[0]* sin(ang)) + (n[1] * cos(ang));
			r[2] = n[2];


			float rough_mul = 0.006;

			vec3_t start, end;
			VectorCopy( m_pCurrentEntity->origin, start );
			VectorCopy( m_pCurrentEntity->origin, end );
			end[2] -= 64;

			char *TextureName = (char *)gEngfuncs.pEventAPI->EV_TraceTexture( 0, start, end);

			if(TextureName)
			{
				char *tmp = TextureName;
				while (*tmp)
				{
					*tmp = tolower (*tmp);
					tmp++;
				}

				if (!(strstr(TextureName, "dirt") || strstr(TextureName, "gras") || strstr(TextureName, "snow"))) {
					if (strstr(TextureName, "road")) {

						rough_mul = 0.003;

					}
				}
//				pmove->Con_DPrintf ("      Car is on TextureName: %s\n", TextureName);
			}


			if (spd < 10) {
				flYaw = (steer_wheel_angle * 15) + 127;
			} else if (spd < 500) {
				flYaw = (steer_wheel_angle * 15) + 127 + pmove->RandomFloat((-rough_mul*2) * spd, (rough_mul*2) * spd);
			} else {
				flYaw = (steer_wheel_angle * 15) + 127 + pmove->RandomFloat((-rough_mul*2) * 500, (rough_mul*2) * 500);
			}

			tAngles[PITCH] = (atan2(r[2], r[0]) * 180 / M_PI - 90);
			tAngles[ROLL]  = (atan2(r[2], r[1]) * 180 / M_PI - 90);
			
			float rough_spd_thresh = 300;

			if (spd < rough_spd_thresh) {
				tAngles[PITCH] += (gEngfuncs.pfnRandomFloat(spd * -rough_mul, spd * rough_mul));
				tAngles[ROLL]  += (gEngfuncs.pfnRandomFloat(spd * -rough_mul, spd * rough_mul));
			} else {
				tAngles[PITCH] += (gEngfuncs.pfnRandomFloat(rough_spd_thresh  * -rough_mul, rough_spd_thresh  * rough_mul));
				tAngles[ROLL]  += (gEngfuncs.pfnRandomFloat(rough_spd_thresh  * -rough_mul, rough_spd_thresh  * rough_mul));
			}			


		//	m_pCvarRotatefx = IEngineStudio.GetCvar( "rally_physrotate" );

			if(spd > 20)
			{
				float mx_roll;

				// Work out the dot product between Right and Velocity
				mx_roll = DotProduct(pmove->velocity, pmove->right) / Length(pmove->velocity);

				// If Dot is more than this, max roll
				float roll_limit = 0.2;

				// Amount to roll at max.
				float limit_multiplier = 25;

				// if less than limit, roll lots
				if (fabs(mx_roll) < roll_limit)
				{
					mx_roll *= limit_multiplier;
				}
				else
				{
					mx_roll *= (limit_multiplier/10);

					if (mx_roll > 0)
						mx_roll += roll_limit * limit_multiplier;
					else
						mx_roll -= roll_limit * limit_multiplier;
				}

				tAngles[ROLL] += mx_roll;

				if (g_braking || g_handbraking)
					tAngles[PITCH] += spd / 150;

				if (spd < 300) {
					if(g_accelerating)
						tAngles[PITCH] -= 3.7 - (spd / 200);
				}

			}

			// Yes this IS necessary due to the jerks
			view_pitch = (tAngles[PITCH]  + pitch_hist*9) / 10;
			view_roll  = (tAngles[ROLL]  + roll_hist*9) / 10;

//			gEngfuncs.Con_DPrintf ("Roll %.2f RollHist: %.2f\n", tAngles[ROLL], v_roll_hist);
			pitch_hist = view_pitch;
			roll_hist = view_roll;

		}
		// Else not onground
		else
		{
			// Can still steer in the air if we want ;)
			flYaw = (steer_wheel_angle * 15) + 127;

			view_pitch = pitch_hist - (4.4 * pmove->frametime);
//			view_roll = v_roll_hist;

			//v_roll_hist = view_roll;
			pitch_hist = view_pitch;
		}

		if (flYaw > 255)
			flYaw = 255;
		
		else if (flYaw < 0)
			flYaw = 0;

		flYaw = (flYaw + LastflYaw) / 2;

		m_pCurrentEntity->curstate.controller[0] = flYaw;
		LastflYaw = flYaw;

		m_pCurrentEntity->latched.prevcontroller[0] = m_pCurrentEntity->curstate.controller[0];
		m_pCurrentEntity->latched.prevcontroller[1] = m_pCurrentEntity->curstate.controller[1];
		m_pCurrentEntity->latched.prevcontroller[2] = m_pCurrentEntity->curstate.controller[2];



		angles[PITCH] = view_pitch - 9;
		view_pitch    = -view_pitch;
		angles[PITCH] = -angles[PITCH];

		angles[ROLL]  = view_roll;

		
		VectorCopy (angles, m_pCurrentEntity->curstate.angles);

	} // End if incar view

	//gEngfuncs.Con_DPrintf("     view_pitch: %.2f, angles[PITCH]: %.4f, tAngles[PITCH]: %f\n", view_pitch, angles[PITCH], tAngles[PITCH]);


//	m_pCurrentEntity->angles[YAW] = angles[YAW];
	VectorCopy(angles, m_pCurrentEntity->angles);
	// Store 'opponenet cars' vuser1 for next frame
	VectorCopy(angles, m_pCurrentEntity->baseline.angles);
	VectorCopy(angles, m_pCurrentEntity->baseline.vuser1);

/*	m_pCvarShadowOpponentsfx = IEngineStudio.GetCvar( "rally_opponentshadows" );

	// Creme: if its another player and shadowoppoents not null, draw their shadows!
	if(!isLocal && m_pCvarShadowOpponentsfx->value && m_pCurrentEntity->player) {
		//Creme: Render a shadow anyway!
		pmtrace_t tr;
		vec3_t			m_vCorners[4];
		vec3_t			forward, right, up;

		AngleVectors(angles, forward, right, up);

		// Don't draw if we can't see the player
		if (m_pCurrentEntity->curstate.effects & EF_NODRAW)
			return;

		// Creme: set the corners to this player.
		m_vCorners[FRONT_LEFT]  = m_pCurrentEntity->origin + (forward * 25) + (right * -11);
		m_vCorners[BACK_LEFT]   = m_pCurrentEntity->origin + (forward * -25) + (right * -11);
		m_vCorners[BACK_RIGHT]  = m_pCurrentEntity->origin + (forward * -22) + (right * 11);
		m_vCorners[FRONT_RIGHT] = m_pCurrentEntity->origin + (forward * 22) + (right * 11);

		HSPRITE hsprTexture = LoadSprite( "sprites/r_shadow.spr" ); // Load sprite, as normal;
		const model_s *pTexture = gEngfuncs.GetSpritePointer( hsprTexture ); // Get pointer to texture;
		vec3_t corners[4];

		// Determine the corners of the shadow
		corners[0] = m_vCorners[0];
		corners[1] = m_vCorners[1];
		corners[2] = m_vCorners[2];
		corners[3] = m_vCorners[3];

		// If we are off the ground, shrink the shadow according to the height of the ground
		if (airborne)
		{
			vec3_t origin;
			float height;

			// We must shrink the size of the shadow based on the height above the ground
			// Trace down from the origin to get the distance to the ground
			origin = m_pCurrentEntity->origin + Vector (0, 0, TRACE_HEIGHT);
			gEngfuncs.pEventAPI->EV_PlayerTrace( origin, origin + Vector (0, 0, -96), PM_STUDIO_BOX | PM_WORLD_ONLY, -1, &tr );
			height = (m_pCurrentEntity->origin - tr.endpos).Length();

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
		float inc_width = width / m_pCvarShadowOpponentsfx->value;
		float inc_length = length / m_pCvarShadowOpponentsfx->value;

		// rally_shadows now determines how much detail goes into the shadows
		// i.e. The number of subdivisions of the shadow

		// Setup rendering
		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)pTexture, 0 );
		gEngfuncs.pTriAPI->RenderMode( kRenderTransTexture );
		gEngfuncs.pTriAPI->CullFace( TRI_NONE );
		gEngfuncs.pTriAPI->Brightness( 1 );

		for (int i = 0; i < m_pCvarShadowOpponentsfx->value; i++)
		{
			vec3_t vertex_top, vertex_bottom;
			float tex_top = (1.0f / m_pCvarShadowOpponentsfx->value) * i;
			float tex_bottom = (1.0f / m_pCvarShadowOpponentsfx->value) * (i + 1);

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
			for (int j = 1; j <= m_pCvarShadowOpponentsfx->value; j++)
			{
				float tex_width = (1.0f / m_pCvarShadowOpponentsfx->value) * j;
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

	}*/

	// Add the HLRally Special Effects
	if (m_pCurrentEntity->player || isInterior) 
	{
		CRallyEffects::getSingleton()->renderPlayer(m_pCurrentEntity, m_nPlayerIndex, isInterior);
	}

	CRallyVGUIFX::getSingleton()->DoCountdown();

	// Creme: Restore the hull setting
	gEngfuncs.pEventAPI->EV_PopPMStates();

}

//======================
// StudioSetUpTransform
//======================

void CStudioModelRenderer::StudioSetUpTransform (int trivial_accept)
{
	int				i;
	vec3_t			angles;
	vec3_t			modelpos;


	VectorCopy( m_pCurrentEntity->origin, modelpos );

	//==================================
	// Copy All the Angles into Matrix
	//==================================

	if(!m_pCurrentEntity->player)
	{
		angles[ROLL] = m_pCurrentEntity->curstate.angles[ROLL];
		angles[YAW] = m_pCurrentEntity->curstate.angles[YAW]; 
		angles[PITCH] = m_pCurrentEntity->curstate.angles[PITCH];
	} else {
		VectorCopy(m_pCurrentEntity->angles, angles);
	}
											  

	AngleMatrix (angles, (*m_protationmatrix));

	if ( !IEngineStudio.IsHardware() )
	{
		static float viewmatrix[3][4];

		VectorCopy (m_vRight, viewmatrix[0]);
		VectorCopy (m_vUp, viewmatrix[1]);
		VectorInverse (viewmatrix[1]);
		VectorCopy (m_vNormal, viewmatrix[2]);

		(*m_protationmatrix)[0][3] = modelpos[0] - m_vRenderOrigin[0];
		(*m_protationmatrix)[1][3] = modelpos[1] - m_vRenderOrigin[1];
		(*m_protationmatrix)[2][3] = modelpos[2] - m_vRenderOrigin[2];

		ConcatTransforms (viewmatrix, (*m_protationmatrix), (*m_paliastransform));

		if (trivial_accept)
		{
			for (i=0 ; i<4 ; i++)
			{
				(*m_paliastransform)[0][i] *= m_fSoftwareXScale *
						(1.0 / (ZISCALE * 0x10000));
				(*m_paliastransform)[1][i] *= m_fSoftwareYScale *
						(1.0 / (ZISCALE * 0x10000));
				(*m_paliastransform)[2][i] *= 1.0 / (ZISCALE * 0x10000);

			}
		}
	}

	(*m_protationmatrix)[0][3] = modelpos[0];
	(*m_protationmatrix)[1][3] = modelpos[1];
	(*m_protationmatrix)[2][3] = modelpos[2];



}












//====================
// StudioProcessGait
//====================

void CStudioModelRenderer::StudioProcessGait( entity_state_t *pplayer )
{
	// Declaration of Variables
	mstudioseqdesc_t	*pseqdesc;

	float dt;
	int iDiv;
	float flYaw, tempflYaw;	 // view direction relative to movement
	float velocity;

	vec3_t  tempvec;


	dt = (m_clTime - m_clOldTime);
	if (dt < 0)
		dt = 0;
	else if (dt > 1.0)
		dt = 1;






	//===========================
	// Wheels Bone Controller
	//===========================
	m_pCurrentEntity->curstate.controller[0] = m_pCurrentEntity->latched.prevcontroller[0];
	m_pCurrentEntity->curstate.controller[1] = m_pCurrentEntity->latched.prevcontroller[1];
	m_pCurrentEntity->curstate.controller[2] = m_pCurrentEntity->latched.prevcontroller[2];
	m_pCurrentEntity->curstate.controller[3] = m_pCurrentEntity->latched.prevcontroller[3];


	cl_entity_t *pthisplayer = gEngfuncs.GetLocalPlayer();

	if(pthisplayer->index == m_pCurrentEntity->index)	// Is the player us?
	{
		tempflYaw = m_pCurrentEntity->latched.prevcontroller[0] - 127;

		//gEngfuncs.Con_DPrintf("flYaw: %.2f, Mx: %.4i\n", tempflYaw, pmove->mousex);


		iDiv = 30;


		//	gEngfuncs.Con_DPrintf("  %.2f\n", flYaw);

		flYaw = (steer_wheel_angle*-20) + 127;

		if (flYaw > 255) {
			flYaw = 255;
		} else if (flYaw < 0) {
			flYaw = 0;
		}

		//	gEngfuncs.Con_DPrintf("1: %i, 2: %i \n", m_pCurrentEntity->curstate.controller[1], m_pCurrentEntity->latched.prevcontroller[2]);

		m_pCurrentEntity->curstate.controller[0] = flYaw;
	
		if (pmove->onground != -1) 
		{
			if(pmove->velocity[1] > 100)
			{
				m_pCurrentEntity->curstate.controller[1] = ((m_pCurrentEntity->curstate.controller[1] * 5) + 100 + pmove->RandomLong(0, 40)) / 6;
				m_pCurrentEntity->curstate.controller[2] = ((m_pCurrentEntity->curstate.controller[2] * 6) + 100 + pmove->RandomLong(0, 40)) / 7;
			}
			else 
			{
				m_pCurrentEntity->curstate.controller[1] = ((m_pCurrentEntity->curstate.controller[1] * 5) + 100) / 6;
				m_pCurrentEntity->curstate.controller[2] = ((m_pCurrentEntity->curstate.controller[2] * 6) + 100) / 7;
			}
		}
		else
		{
			m_pCurrentEntity->curstate.controller[1] = ((m_pCurrentEntity->curstate.controller[1] * 20) + 255) / 21;
			m_pCurrentEntity->curstate.controller[2] = ((m_pCurrentEntity->curstate.controller[2] * 30) + 255) / 31;
		}

		m_pCurrentEntity->latched.prevcontroller[0] = m_pCurrentEntity->curstate.controller[0];
		m_pCurrentEntity->latched.prevcontroller[1] = m_pCurrentEntity->curstate.controller[1];
		m_pCurrentEntity->latched.prevcontroller[2] = m_pCurrentEntity->curstate.controller[2];
		m_pCurrentEntity->latched.prevcontroller[3] = m_pCurrentEntity->curstate.controller[3];

	} else { 
		
		// Creme added this section to reset controllers to normal 
		// on other cars. Eventually we'll need to try and suss these.

		m_pCurrentEntity->curstate.controller[0] = 127;
		m_pCurrentEntity->curstate.controller[1] = 127;
		m_pCurrentEntity->curstate.controller[2] = 127;
		m_pCurrentEntity->curstate.controller[3] = 127;

		m_pCurrentEntity->latched.prevcontroller[0] = m_pCurrentEntity->curstate.controller[0];
		m_pCurrentEntity->latched.prevcontroller[1] = m_pCurrentEntity->curstate.controller[1];
		m_pCurrentEntity->latched.prevcontroller[2] = m_pCurrentEntity->curstate.controller[2];
		m_pCurrentEntity->latched.prevcontroller[3] = m_pCurrentEntity->curstate.controller[3];

	}

	// FragMented
	pplayer->gaitsequence = 3;


	pseqdesc = (mstudioseqdesc_t *)((byte *)m_pStudioHeader + m_pStudioHeader->seqindex) + pplayer->gaitsequence;

	// calc gait frame
	if (pseqdesc->linearmovement[0] > 0)
		m_pPlayerInfo->gaitframe += (m_flGaitMovement / pseqdesc->linearmovement[0]) * pseqdesc->numframes;
	else
		m_pPlayerInfo->gaitframe += pseqdesc->fps * dt;

	
	if(pthisplayer->index == m_pCurrentEntity->index)	// Is the player us?
	{

		VectorCopy (pmove->velocity, tempvec); tempvec[2] = 0;
		velocity = Length(tempvec);

		// Crem - Spin those wheels!! (based *very crudely* on the cars general velocity)
		// FragMented! - Brake haxor!
		static int braketime;

		if(!g_braking || g_accelerating)
		{
			braketime = 0;
			pseqdesc->fps = velocity / 2.5f;
		}
		else
		{
			braketime ++;
			if(braketime < (velocity / 2.5f))
			{
				pseqdesc->fps = (velocity / 2.5f) - braketime;
			}
			else
				pseqdesc->fps = 0;
		}

		if(g_reversing)
		{
			pseqdesc->fps =- pseqdesc->fps;
		}

		if(g_handbraking)
		{
			pseqdesc->fps = 0;
		}

		if(g_tractionloss)
		{
			pseqdesc->fps *= 5;
		}


	} else {  // not us! pull ent velocity from rallyfx

		// Creme: 4-Aug-2003: use baseline.basevelocity) for speeds now

		velocity = m_pCurrentEntity->baseline.origin[1]; //Length(m_pCurrentEntity->baseline.basevelocity);
//		pmove->Con_DPrintf ("      wheel spd: %f\n", Length(m_pCurrentEntity->baseline.basevelocity));

		if ((velocity) && (velocity > 0)) {
			pseqdesc->fps = velocity / 2.5f;
		} else {
			pseqdesc->fps = 0;
		}

	}

	// do modulo
	m_pPlayerInfo->gaitframe = m_pPlayerInfo->gaitframe - (int)(m_pPlayerInfo->gaitframe / pseqdesc->numframes) * pseqdesc->numframes;
	if (m_pPlayerInfo->gaitframe < 0)
		m_pPlayerInfo->gaitframe += pseqdesc->numframes;
}






































//====================================================================================================
// PROCEDURES AFFECTING MODEL ROTATION END HERE
//====================================================================================================














































/*
====================
StudioDrawPlayer

====================
*/
int CStudioModelRenderer::StudioDrawPlayer( int flags, entity_state_t *pplayer )
{
	alight_t lighting;
	vec3_t dir;

	m_pCurrentEntity = IEngineStudio.GetCurrentEntity();
	IEngineStudio.GetTimes( &m_nFrameCount, &m_clTime, &m_clOldTime );
	IEngineStudio.GetViewInfo( m_vRenderOrigin, m_vUp, m_vRight, m_vNormal );
	IEngineStudio.GetAliasScale( &m_fSoftwareXScale, &m_fSoftwareYScale );

	// gEngfuncs.Con_DPrintf("DrawPlayer %d\n", m_pCurrentEntity->model );

	// Con_DPrintf("DrawPlayer %d %d (%d)\n", m_nFrameCount, pplayer->player_index, m_pCurrentEntity->curstate.sequence );

	// Con_DPrintf("Player %.2f %.2f %.2f\n", pplayer->velocity[0], pplayer->velocity[1], pplayer->velocity[2] );

	m_nPlayerIndex = pplayer->number - 1;

	if (m_nPlayerIndex < 0 || m_nPlayerIndex >= gEngfuncs.GetMaxClients())
		return 0;

	m_pRenderModel = IEngineStudio.SetupPlayerModel( m_nPlayerIndex );
	if (m_pRenderModel == NULL)
		return 0;

	m_pStudioHeader = (studiohdr_t *)IEngineStudio.Mod_Extradata (m_pRenderModel);
	IEngineStudio.StudioSetHeader( m_pStudioHeader );
	IEngineStudio.SetRenderModel( m_pRenderModel );


	if (flags & STUDIO_EVENTS)
	{
		StudioCalcAttachments( );
		if (gEngfuncs.GetLocalPlayer()->index == m_pCurrentEntity->index) {
			IEngineStudio.StudioClientEvents( );
		}
		// copy attachments into global entity array
		if ( m_pCurrentEntity->index > 0 )
		{
			cl_entity_t *ent = gEngfuncs.GetEntityByIndex( m_pCurrentEntity->index );

			memcpy( ent->attachment, m_pCurrentEntity->attachment, sizeof( vec3_t ) * 4 );
		}
	}

	/*if (pplayer->gaitsequence)
	{*/
		vec3_t orig_angles;
		m_pPlayerInfo = IEngineStudio.PlayerInfo( m_nPlayerIndex );

//		VectorCopy( m_pCurrentEntity->angles, orig_angles );
	
		// Creme: 5-Aug-2003 moved stuff out of setuptransform to 
		//		 RallyRender(), so that variables are updated before processgait

		StudioRallyRender ( );

		StudioProcessGait( pplayer );

		//m_pPlayerInfo->gaitsequence = pplayer->gaitsequence;
		m_pPlayerInfo = NULL;

		StudioSetUpTransform( 0 );
//		VectorCopy( orig_angles, m_pCurrentEntity->angles );
/*	}
	else
	{
		m_pCurrentEntity->curstate.controller[0] = 127;
		m_pCurrentEntity->curstate.controller[1] = 127;
		m_pCurrentEntity->curstate.controller[2] = 127;
		m_pCurrentEntity->curstate.controller[3] = 127;
		m_pCurrentEntity->latched.prevcontroller[0] = m_pCurrentEntity->curstate.controller[0];
		m_pCurrentEntity->latched.prevcontroller[1] = m_pCurrentEntity->curstate.controller[1];
		m_pCurrentEntity->latched.prevcontroller[2] = m_pCurrentEntity->curstate.controller[2];
		m_pCurrentEntity->latched.prevcontroller[3] = m_pCurrentEntity->curstate.controller[3];
		
		m_pPlayerInfo = IEngineStudio.PlayerInfo( m_nPlayerIndex );
		m_pPlayerInfo->gaitsequence = 0;

		StudioSetUpTransform( 0 );
	}*/

	if (flags & STUDIO_RENDER)
	{
		// see if the bounding box lets us trivially reject, also sets
		if (!IEngineStudio.StudioCheckBBox ())
			return 0;

		(*m_pModelsDrawn)++;
		(*m_pStudioModelCount)++; // render data cache cookie

		if (m_pStudioHeader->numbodyparts == 0)
			return 1;
	}

	m_pPlayerInfo = IEngineStudio.PlayerInfo( m_nPlayerIndex );
	StudioSetupBones( );
	StudioSaveBones( );
	m_pPlayerInfo->renderframe = m_nFrameCount;

	m_pPlayerInfo = NULL;



	if (flags & STUDIO_RENDER)
	{
		if (m_pCvarHiModels->value && m_pRenderModel != m_pCurrentEntity->model  )
		{
			// show highest resolution multiplayer model
			m_pCurrentEntity->curstate.body = 255;
		}

		if (!(m_pCvarDeveloper->value == 0 && gEngfuncs.GetMaxClients() == 1 ) && ( m_pRenderModel == m_pCurrentEntity->model ) )
		{
			m_pCurrentEntity->curstate.body = 1; // force helmet
		}

		lighting.plightvec = dir;
		IEngineStudio.StudioDynamicLight(m_pCurrentEntity, &lighting );

		IEngineStudio.StudioEntityLight( &lighting );

		// model and frame independant
		IEngineStudio.StudioSetupLighting (&lighting);

		m_pPlayerInfo = IEngineStudio.PlayerInfo( m_nPlayerIndex );

		// get remap colors
		m_nTopColor = m_pPlayerInfo->topcolor;
		if (m_nTopColor < 0)
			m_nTopColor = 0;
		if (m_nTopColor > 360)
			m_nTopColor = 360;
		m_nBottomColor = m_pPlayerInfo->bottomcolor;
		if (m_nBottomColor < 0)
			m_nBottomColor = 0;
		if (m_nBottomColor > 360)
			m_nBottomColor = 360;

		IEngineStudio.StudioSetRemapColors( m_nTopColor, m_nBottomColor );

		StudioRenderModel( );
		m_pPlayerInfo = NULL;

		if (pplayer->weaponmodel)
		{
			cl_entity_t saveent = *m_pCurrentEntity;

			model_t *pweaponmodel = IEngineStudio.GetModelByIndex( pplayer->weaponmodel );

			m_pStudioHeader = (studiohdr_t *)IEngineStudio.Mod_Extradata (pweaponmodel);
			IEngineStudio.StudioSetHeader( m_pStudioHeader );

			// FragMented
			if(m_pCurrentEntity->curstate.weaponmodel == gEngfuncs.pEventAPI->EV_FindModelIndex( "models/hp_bomb.mdl"))
			{
				static cvar_t *m_pCvarHPfx = IEngineStudio.GetCvar( "rally_hpdisplay" );

				if(m_pCvarHPfx->value == 0 || m_pCvarHPfx->value == 1 || m_pCvarHPfx->value == 3) 
					pplayer->weaponmodel=NULL;
			}

			StudioMergeBones( pweaponmodel);
		}

		//==============================
		// CHROME EFFECTS (FragMented!)
		//==============================
		// And the Lord said, "Let there be Chrome"
		m_pCvarChromefx = IEngineStudio.GetCvar( "rally_chrome" );

		if(m_pCurrentEntity->player && m_pCvarChromefx->value)
		{
			char szChromeModel[80], szDir[512]; 
			sprintf (szChromeModel, "models/player/%s/%s_chrome.mdl", g_PlayerInfoList[m_pCurrentEntity->index].model, g_PlayerInfoList[m_pCurrentEntity->index].model); 
			gEngfuncs.COM_ExpandFilename (szChromeModel, szDir, sizeof (szDir)); 

			// Load Model into Model_t Struct
			model_t *chromeModel = IEngineStudio.GetModelByIndex(gEngfuncs.pEventAPI->EV_FindModelIndex( szChromeModel));

			if(chromeModel)
			{
				// Configure Renderer
				gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
				m_pStudioHeader = (studiohdr_t *)IEngineStudio.Mod_Extradata (chromeModel);
				IEngineStudio.StudioSetHeader( m_pStudioHeader );

				// Snap to model
				StudioMergeBones( chromeModel );
			}

			// Creme: Added some ambient to the chrome, 
			// makes the car appear a lil shiny in the dark! (looks good to me ;)
			lighting.ambientlight = 24;
			IEngineStudio.StudioSetupLighting (&lighting);

			// Render Model
			StudioRenderModel();

			// Reset the rendermode
			gEngfuncs.pTriAPI->RenderMode( kRenderNormal );

		}

// Moved this up before rally stuff
		StudioCalcAttachments( );

	}

	// SaRcaZm - V8 - Start
	SetIsInPVS (m_pCurrentEntity->index, m_nFrameCount);
	// SaRcaZm - V8 - End

	return 1;
}

/*
====================
StudioCalcAttachments

====================
*/
void CStudioModelRenderer::StudioCalcAttachments( void )
{
	int i;
	mstudioattachment_t *pattachment;

	if ( m_pStudioHeader->numattachments > 4 )
	{
		gEngfuncs.Con_DPrintf( "Too many attachments on %s\n", m_pCurrentEntity->model->name );
		exit( -1 );
	}

	// calculate attachment points
	pattachment = (mstudioattachment_t *)((byte *)m_pStudioHeader + m_pStudioHeader->attachmentindex);
	for (i = 0; i < m_pStudioHeader->numattachments; i++)
	{
		VectorTransform( pattachment[i].org, (*m_plighttransform)[pattachment[i].bone],  m_pCurrentEntity->attachment[i] );
	}
}







/*
====================
StudioDrawModel

====================
*/
int CStudioModelRenderer::StudioDrawModel( int flags )
{
	alight_t lighting;
	vec3_t dir;

	m_pCurrentEntity = IEngineStudio.GetCurrentEntity();
	IEngineStudio.GetTimes( &m_nFrameCount, &m_clTime, &m_clOldTime );
	IEngineStudio.GetViewInfo( m_vRenderOrigin, m_vUp, m_vRight, m_vNormal );
	IEngineStudio.GetAliasScale( &m_fSoftwareXScale, &m_fSoftwareYScale );

	if (m_pCurrentEntity->curstate.renderfx == kRenderFxDeadPlayer)
	{
		entity_state_t deadplayer;

		int result;
		int save_interp;

		if (m_pCurrentEntity->curstate.renderamt <= 0 || m_pCurrentEntity->curstate.renderamt > gEngfuncs.GetMaxClients() )
			return 0;

		// get copy of player
		deadplayer = *(IEngineStudio.GetPlayerState( m_pCurrentEntity->curstate.renderamt - 1 )); //cl.frames[cl.parsecount & CL_UPDATE_MASK].playerstate[m_pCurrentEntity->curstate.renderamt-1];

		// clear weapon, movement state
		deadplayer.number = m_pCurrentEntity->curstate.renderamt;
		deadplayer.weaponmodel = 0;
		deadplayer.gaitsequence = 0;

		deadplayer.movetype = MOVETYPE_NONE;
//		VectorCopy( m_pCurrentEntity->curstate.angles, deadplayer.angles ); FragMented! We still want it on the ground
		VectorCopy( m_pCurrentEntity->curstate.origin, deadplayer.origin );

		save_interp = m_fDoInterp;
		m_fDoInterp = 0;
		
		// draw as though it were a player
		result = StudioDrawPlayer( flags, &deadplayer );
		
		m_fDoInterp = save_interp;
		return result;
	}

	m_pRenderModel = m_pCurrentEntity->model;
	m_pStudioHeader = (studiohdr_t *)IEngineStudio.Mod_Extradata (m_pRenderModel);
	IEngineStudio.StudioSetHeader( m_pStudioHeader );
	IEngineStudio.SetRenderModel( m_pRenderModel );

	if (flags & STUDIO_EVENTS)
	{
		StudioCalcAttachments( );
		IEngineStudio.StudioClientEvents( );
		// copy attachments into global entity array
		if ( m_pCurrentEntity->index > 0 )
		{
			cl_entity_t *ent = gEngfuncs.GetEntityByIndex( m_pCurrentEntity->index );

			memcpy( ent->attachment, m_pCurrentEntity->attachment, sizeof( vec3_t ) * 4 );
		}
	}

	if(!strcmpi("models/interior.mdl", m_pCurrentEntity->model->name)) {
		StudioRallyRender ( );
	}
	StudioSetUpTransform( 0 );

	if (flags & STUDIO_RENDER)
	{
		// see if the bounding box lets us trivially reject, also sets
		if (!IEngineStudio.StudioCheckBBox ())
			return 0;

		(*m_pModelsDrawn)++;
		(*m_pStudioModelCount)++; // render data cache cookie

		if (m_pStudioHeader->numbodyparts == 0)
			return 1;
	}

	if (m_pCurrentEntity->curstate.movetype == MOVETYPE_FOLLOW)
	{
		StudioMergeBones( m_pRenderModel );
	}
	else
	{
		StudioSetupBones( );
	}
	StudioSaveBones( );



	if (flags & STUDIO_RENDER)
	{
		lighting.plightvec = dir;
		IEngineStudio.StudioDynamicLight(m_pCurrentEntity, &lighting );

		IEngineStudio.StudioEntityLight( &lighting );

		// model and frame independant
		IEngineStudio.StudioSetupLighting (&lighting);

		// get remap colors
		m_nTopColor = m_pCurrentEntity->curstate.colormap & 0xFF;
		m_nBottomColor = (m_pCurrentEntity->curstate.colormap & 0xFF00) >> 8;

		IEngineStudio.StudioSetRemapColors( m_nTopColor, m_nBottomColor );

		StudioRenderModel( );

		//==============================
		// CHROME EFFECTS (FragMented!)
		//==============================
		// And the Lord said, "Let there be Chrome" (TempEnts)
		if (m_pCvarChromefx->value)
		{
			char szChromeModel[80], szDir[512]; 
			char ModelName[32] = "";

			for(int i = 7; i <= int(strlen(m_pCurrentEntity->model->name)-5); i++)
				ModelName[strlen(ModelName)] = m_pCurrentEntity->model->name[i];

			// SaRcaZm - V8 - Start
			// Transparent model slowdown HACK fix
			// Only do chrome on the player models and the interior
			if ((strstr (ModelName, "player") == NULL) && (strstr (ModelName, "interior") == NULL))
				return 1;
			// SaRcaZm - V8 - End

			sprintf (szChromeModel, "models/%s_chrome.mdl", ModelName); 
			gEngfuncs.COM_ExpandFilename (szChromeModel, szDir, sizeof (szDir)); 

			// Load Model into Model_t Struct
			model_t *chromeModel = IEngineStudio.GetModelByIndex(gEngfuncs.pEventAPI->EV_FindModelIndex(szChromeModel));

		//	gEngfuncs.Con_DPrintf(szChromeModel);

			if (chromeModel)
			{
				// Configure Renderer
				IEngineStudio.GL_SetRenderMode( kRenderTransAdd );
				m_pStudioHeader = (studiohdr_t *)IEngineStudio.Mod_Extradata (chromeModel);
				IEngineStudio.StudioSetHeader( m_pStudioHeader );

				// Snap to model
				StudioMergeBones( chromeModel );

				StudioRenderModel( );

				IEngineStudio.GL_SetRenderMode( kRenderNormal );
			}
		}
	}


	return 1;
}





/*
====================
StudioRenderModel

====================
*/
void CStudioModelRenderer::StudioRenderModel( void )
{
	IEngineStudio.SetChromeOrigin();
	IEngineStudio.SetForceFaceFlags( 0 );

	if ( m_pCurrentEntity->curstate.renderfx == kRenderFxGlowShell )
	{
		m_pCurrentEntity->curstate.renderfx = kRenderFxNone;
		StudioRenderFinal( );
		
		if ( !IEngineStudio.IsHardware() )
		{
			gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
		}

		IEngineStudio.SetForceFaceFlags( STUDIO_NF_CHROME );

		gEngfuncs.pTriAPI->SpriteTexture( m_pChromeSprite, 0 );
		m_pCurrentEntity->curstate.renderfx = kRenderFxGlowShell;

		StudioRenderFinal( );
		if ( !IEngineStudio.IsHardware() )
		{
			gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
		}
	}
	else
	{
		StudioRenderFinal( );
		gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
	}

}

/*
====================
StudioRenderFinal_Software

====================
*/
void CStudioModelRenderer::StudioRenderFinal_Software( void )
{
	int i;

	// Note, rendermode set here has effect in SW
	IEngineStudio.SetupRenderer( 0 ); 

	if (m_pCvarDrawEntities->value == 2)
	{
		IEngineStudio.StudioDrawBones( );
	}
	else if (m_pCvarDrawEntities->value == 3)
	{
		IEngineStudio.StudioDrawHulls( );
	}
	else
	{
		for (i=0 ; i < m_pStudioHeader->numbodyparts ; i++)
		{
			IEngineStudio.StudioSetupModel( i, (void **)&m_pBodyPart, (void **)&m_pSubModel );
			IEngineStudio.StudioDrawPoints( );
		}
	}

	if (m_pCvarDrawEntities->value == 4)
	{
		gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
		IEngineStudio.StudioDrawHulls( );
		gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
	}

	if (m_pCvarDrawEntities->value == 5)
	{
		IEngineStudio.StudioDrawAbsBBox( );
	}
	
	IEngineStudio.RestoreRenderer();
}

/*
====================
StudioRenderFinal_Hardware

====================
*/
void CStudioModelRenderer::StudioRenderFinal_Hardware( void )
{
	int i;
	int rendermode;

	rendermode = IEngineStudio.GetForceFaceFlags() ? kRenderTransAdd : m_pCurrentEntity->curstate.rendermode;
	IEngineStudio.SetupRenderer( rendermode );
	
	if (m_pCvarDrawEntities->value == 2)
	{
		IEngineStudio.StudioDrawBones();
	}
	else if (m_pCvarDrawEntities->value == 3)
	{
		IEngineStudio.StudioDrawHulls();
	}
	else
	{
		for (i=0 ; i < m_pStudioHeader->numbodyparts ; i++)
		{
			IEngineStudio.StudioSetupModel( i, (void **)&m_pBodyPart, (void **)&m_pSubModel );

			if (m_fDoInterp)
			{
				// interpolation messes up bounding boxes.
				m_pCurrentEntity->trivial_accept = 0; 
			}

			IEngineStudio.GL_StudioDrawShadow();
			IEngineStudio.GL_SetRenderMode( rendermode );
			IEngineStudio.StudioDrawPoints();

		}
	}

	if ( m_pCvarDrawEntities->value == 4 )
	{
		gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
		IEngineStudio.StudioDrawHulls( );
		gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
	}

	IEngineStudio.RestoreRenderer();
}

/*
====================
StudioRenderFinal

====================
*/
void CStudioModelRenderer::StudioRenderFinal(void)
{
	if ( IEngineStudio.IsHardware() )
	{
		StudioRenderFinal_Hardware();
	}
	else
	{
		StudioRenderFinal_Software();
	}
}

