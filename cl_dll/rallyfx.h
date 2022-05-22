// rallyfx.cpp
// Renders HLRally Special effects
// By SaRcaZm, Creme, FragMented
#if !defined ( RALLYFX_H )
#define RALLYFX_H
#if defined( _WIN32 )
#pragma once
#endif

// Definitions
#define		FRONT_LEFT	0
#define		BACK_LEFT	1
#define		BACK_RIGHT	2
#define		FRONT_RIGHT	3

// Structures
// Structure for saving info about the players skidmarks
typedef struct skidpers_s
{
	vec3_t			last_pos[4], last_right;
	bool			bFirstTime;

	// SaRcaZm - V6 - Start
	bool			bOnGround;
	// SaRcaZm - V6 - End
} skidpers_t;

// Structure for saving info about each player
typedef struct playerpers_s {
	// SaRcaZm - V5 - Start
	//cl_entity_t		*pEnt;
	int				pEnt;
	// SaRcaZm - V5 - End
	skidpers_t		skid;
	vec3_t			lastorigin;
} playerpers_t;

// SaRcaZm - V6 - Start
typedef enum
{
	SKIDTEX_DIRT,
	SKIDTEX_SAND,
	SKIDTEX_SNOW,
	SKIDTEX_NONE,
	SKIDTEX_COUNT
};
extern char *skidtex_names[SKIDTEX_COUNT];
// SaRcaZm - V6 - End

// Structure for the skid mark linked list
typedef struct skidlist_s
{
	vec3_t	vertices[4];
	float	time;
	// SaRcaZm - V6 - Start
	int		texture;
	// SaRcaZm - V6 - End
	struct skidlist_s *next;
} skidlist_t;

// SaRcaZm - V5 - Start
// This is for custom animations

// SaRcaZm - V5 - End

// Structures for the life of the temporary models


// Variables to save the life of the temporary models
//extern modellife_t vehicles[2];
//extern modellife_t *vehicles;
//extern int curvehicle;

// SaRcaZm - V5 - Start
#define MAX_PATH_NODES	8
#define MAX_ANIMATIONS			20

#define NUM_ANIM				0
#define CUST_NO_ANIM			1
#define	CUST_TOPBUTTON_ANIM		2	// There are MAX_CUST_BUTTONS of these animations
#define CUST_RETURN_ANIM		MAX_ANIMATIONS - 1


// Changes by FragMented!
// Structure for the light list
typedef struct playerlight_s
{
	vec3_t origin;
	vec3_t angles;
	short headlightOn;
	float hBrightness; // Headlight Brightness
	float bBrightness; // Brakelight / Reverse Light Brightness
	short isBrake;
	short isReverse;
	bool wasBrake;
	//bool headlightOn;
	float prevSpeed;

	int teamidx;
	int modelidx;

	// SaRcaZm - V6 - Start
	float lastupdate;	// Update from Frag
	// SaRcaZm - V6 - End
} playerlight_t;



// SaRcaZm - V5 - End

// Class Definitions
class CRallyFX
{
public:
	CRallyFX (void);
	~CRallyFX (void);
	void Init (void);
	void Render (cl_entity_s *pplayer);
	void Free_Skidlist (void);
	void RenderTriangles (void);	// Called from tri.cpp
	void VGUI (char *model);
	void VGUIBackground (void);
	void SetMode (int iMode);
	int GetMode (void);
	void ClearVGUIModels (void);

	// Creme, added this stuff from tri.cpp, 11 June, 2002
	void RenderFog ( void );
	void RenderHUDNeedles (void);
	// End Creme 11 June, 2002

	// SaRcaZm - V5 - Start
	void StartCountdown (float fStartTime);
	void DoCountdown (void);
	int CustomAnim (char *szModel, int iAnimIndex);
	void SetCustomAnim (int iAnimNum, int iAnimIndex);
	void StopCustomAnim (int iAnimNum);
	void SetHeadlightStatus (int iIndex, int iState);
	// SaRcaZm - V5 - End

	// Changes by FragMented!
	void Lights (vec3_t angles);

	// SaRcaZm - V8 - Start
	void			AddHorn (int index);
private:
	bool			m_bHorn[MAX_PLAYERS];
public:
	// SaRcaZm - V8 - End

	// SaRcaZm - V6 - Start
	// I wanna access this elsewhere, hence public
	cvar_t			*m_pCvarManual; // FragMented - Gear System
	// SaRcaZm - V6 - End

	vec3_t			m_vVelocity, m_vOrigin, m_vAngles;
	float			m_fSpeed;

private:
	void CalcVariables (void);
	void CalcSkidMarks (void);
	void ParticleEffects (void);

	void RenderSkidMarks (void);
	void Shadows (void);
	void Dust (void);
	void Exhaust (void);
	void HP (void);
	void WheelAnimation (void);

	cl_entity_t		*m_pCurrentEntity;
	cvar_t			*m_pCvarSkidfx;
	cvar_t			*m_pCvarShadowfx;
	cvar_t			*m_pCvarShadowOpponentsfx;
	cvar_t			*m_pCvarSkidTime;
	cvar_t			*m_pCvarDustfx;
	cvar_t			*m_pCvarFogfx; // FragMented - RC7
	cvar_t			*m_pCvarHPfx;
	cvar_t			*m_pCvarHPfx_r;
	cvar_t			*m_pCvarHPfx_g;
	cvar_t			*m_pCvarHPfx_b;

	// SaRcaZm - V7 - Start
	cvar_t			*m_pCvarSparkfx;
	// SaRcaZm - V7 - End

	vec3_t			forward, right, up;
	vec3_t			m_vRenderOrigin, m_vNormal, m_vRight, m_vUp;
	vec3_t			m_vCorners[4];
	bool			m_bLocalPlayer;
	float			m_fCurrentTime;
	float			m_gear;

	playerpers_t	m_pPers[MAX_PLAYERS];
	int				m_iPersMax;
	skidlist_t		*m_pSkidlist;
	playerpers_t	*m_pCurrentPers;
	int				m_iMode;

	// SaRcaZm - V7 - Start
	// Made these public so I can access outside this class
public:
	// Creme vgui car positioning (TEMP!!)

private:
	// SaRcaZm - V7 - End

	// SaRcaZm - V5 - Start
	cvar_t			*cl_2dcountdown;
	float			m_fCountdownStart;
	int				m_iCountdownDone;
	// SaRcaZm - V5 - End

	// SaRcaZm - V5 - Start
	// Changes by FragMented!
	void RenderLights (void);
	playerlight_t	m_pLightList[MAX_PLAYERS];
	bool			m_bIsOnGround;
	// SaRcaZm - V5 - End
	bool			m_bIsSkidding;
	bool			m_bInteriorView;
	vec3_t			m_vBackFireOrigin;
};
//extern CRallyFX m_pRallyFX;

// SaRcaZm - V5 - Start
// FragMented sound
#include "fmod/api/inc/fmod.h"
#include "fmod/api/inc/fmod_errors.h"
#include "rally_sound.h"
// SaRcaZm - V5 - End

#endif // RALLYFX_H
