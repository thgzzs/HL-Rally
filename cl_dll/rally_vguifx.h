//
// rally_vguifx.h
//
// Header file for rally_vguifx.cpp
//
// By SaRcaZm
//

#if !defined ( RALLY_VGUIFX_H )
#define RALLY_VGUIFX_H
#if defined( _WIN32 )
#pragma once
#endif

// Definitions
#define		FRONT_LEFT	0
#define		BACK_LEFT	1
#define		BACK_RIGHT	2
#define		FRONT_RIGHT	3

// Modes that the fx can be in
// FragMented! v2
typedef enum {
	MODE_NORM = 0,
	MODE_VGUI,
	MODE_SHOWROOM,
	MODE_COUNTDOWN,
	NUM_MODES
} rally_modes;

// This is for custom animations
typedef struct numpath_s {
	vec3_t position;
	vec3_t angles;
	float time;
} numpath_t;

// Structures for the life of the temporary models
typedef struct modellife_s {
	char name[16];
	float life;
	vec3_t angle, origin;
	bool mousedown;

	struct modellife_s *next;

	int anim;
	int reference;
	float animstart;
	int ret;
	numpath_t retpath;
} modellife_t;


typedef struct rally_modelinfo_s {
	char szModelName[20];
	char szWeight[50];
	char szTransmission[50];
	char szHP[50];
	char szTorque[50];
	char szGearBox[50];
	int iStats[3];
	float fBrakeLightDst;
	float fBrakeLightWidth;
	float fBrakeLightHeight;
	float fHeadLightDst;
	float fHeadLightWidth;
	float fHeadLightHeight;

	// SaRcaZm - V8 - Start
	char szDisplayName[30];
	// SaRcaZm - V8 - End
} rally_modelinfo_t;


typedef struct rally_teaminfo_s {
	char szTeamName[20];
	rally_modelinfo_t szModels[3];
} rally_teaminfo_t;

//extern rally_teaminfo_t rallyinfo[MAX_TEAMS];

// Variables to save the life of the temporary models
extern modellife_t *vehicles;

#define MAX_PATH_NODES			8
#define MAX_ANIMATIONS			20

#define NUM_ANIM				0
#define CUST_NO_ANIM			1
#define	CUST_TOPBUTTON_ANIM		2	// There are MAX_CUST_BUTTONS of these animations
#define CUST_RETURN_ANIM		MAX_ANIMATIONS - 1

typedef struct anim_paths_s {
	vec3_t origin, angles;
	numpath_t nodes[MAX_PATH_NODES];
	int num_nodes;
	float length;
	float scale;
} anim_paths_t;
extern anim_paths_t animpaths[MAX_ANIMATIONS];

class CRallyVGUIFX
{
public:

	//	Singleton
	static CRallyVGUIFX *createSingleton()
	{
		if(!ms_pSingleton)
			ms_pSingleton = new CRallyVGUIFX();
		return ms_pSingleton;
	};

	static CRallyVGUIFX *getSingleton()
	{
		return ms_pSingleton;
	};

	static void freeSingleton()
	{
		delete ms_pSingleton;
	};


	void Init (void);
	void VGUI (char *model);
	void VGUIBackground (void);

	void SetMode (int iMode);
	int GetMode (void);

	void ClearVGUIModels (void);

	void StartCountdown (float fStartTime);
	void DoCountdown (void);

	int CustomAnim (char *szModel, int iAnimIndex);
	void SetCustomAnim (int iAnimNum, int iAnimIndex);
	void StopCustomAnim (int iAnimNum);

	// Creme vgui car positioning (TEMP!!)
	cvar_t			*vg_x;
	cvar_t			*vg_y;
	cvar_t			*vg_z;

private:

	// Singleton (FragMented v2)
	CRallyVGUIFX (void);
	~CRallyVGUIFX (void);

	//	Singleton instance
	static CRallyVGUIFX *ms_pSingleton;

	cvar_t			*cl_2dcountdown;
	float			m_fCountdownStart;
	int				m_iCountdownDone;
	int				m_iMode;
};

#endif // RALLY_VGUIFX_H