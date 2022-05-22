/***********************************************************************
*
*   File: Rally_Effects.h
*    
*   Purpose: Renders and controls Half-Life Rally Special Effects
*
*   Author(s): FragMented!, Creme, SaRcaZm
*
*   Copyright: 2001-2004 HL Rally Team
*
**************************************************************************
*/


// Definitions
#define		FRONT_LEFT	0
#define		BACK_LEFT	1
#define		BACK_RIGHT	2
#define		FRONT_RIGHT	3


// Class
class CRallyEffects
{
public:

	//	Structures
	typedef struct playerEffectsInfo_s
	{
		int				iPlayerIndex;					// Player index (equal to the index in the m_PlayerList array)
		cl_entity_t		*pPlayerEnt;					// Pointer to related HL player / client

		bool			bIsInPVS;
		bool			bIsLocal;						// Is this the local player
		bool			bIsInterior;					// Is this the interior model
		bool			bIsSkidding;					// Is the player currently skidding
		bool			bIsOnGround;					// Is the player on the ground

		vec3_t			vVelocity;						// 3D velocity of the player
		float			fVelocity2D;					// 2D velocity / speed of the player

		bool			bIsHorn;						//	Is the player horny
		bool			bIsBraking;						//	Is the player braking
		bool			bIsReversing;					//	Is the player reversing
		bool			bWasBraking;					//	Was the player braking / reversing
		bool			bIsHeadlightOn;					//	Is the player's headlight on
		float			fBrightness;					//	Has the player previously been braking/reversing, and is fading
		float			fBrightnessHeadlight;			//	Has the player had their lights on

		vec3_t			vSkid_last_pos[4], vSkid_last_right;
		bool			bSkid_FirstTime;
		int				pSkidEnt;
		vec3_t			vLastOrigin;


		vec3_t vForward, vRight, vUp;					//	Angle Vectors (stored here for optimisation)
		vec3_t vCorners[4];								//	Corners of local player's vehicle

	} playerEffectsInfo_t;


	//	Singleton
	static CRallyEffects *createSingleton()
	{
		if(!ms_pSingleton)
			ms_pSingleton = new CRallyEffects();

		return ms_pSingleton;
	};

	static CRallyEffects *getSingleton()
	{
		return ms_pSingleton;
	};

	static void freeSingleton()
	{
		delete ms_pSingleton;
	};


	//	Rally Effects Entry Points
	void VidInit						();		// Used to precache sprites
	void render							();
	void renderTransTriangles			();
	void renderPlayer					(cl_entity_t *pPlayerEnt, int iPlayerIndex, bool bIsInterior);
	void renderPlayerTransTriangles		(playerEffectsInfo_t *pPlayerEnt);

	void freeSkidmarks();

	//	Player List
	playerEffectsInfo_t m_PlayerList[MAX_PLAYERS];

	//	CVars
	cvar_t			*m_pCvarManual;
	cvar_t			*m_pCvarPartslist;

	//	Sprites
	struct model_s	*m_pSprShadow;
	struct model_s	*m_pSprHudNeedle;
	struct model_s	*m_pSprBrakeLight;

private:

	
	//	Only Constructed / Destructed by Singleton
	CRallyEffects();
	~CRallyEffects();

	//	Singleton instance
	static CRallyEffects	*ms_pSingleton;


	//	Specific Render Methods
	void renderCalculateRPMs();
	void renderFog();
	void renderSkidmarks();


	//	Specific Render Player Methods
	void renderPlayerCalcVariables		(playerEffectsInfo_t *pPlayer);
	void renderPlayerWheels				(playerEffectsInfo_t *pPlayer);
	void renderPlayerDust				(playerEffectsInfo_t *pPlayer);
	void renderPlayerBrakeLights		(playerEffectsInfo_t *pPlayer);
	void renderPlayerHeadLights			(playerEffectsInfo_t *pPlayer);
	void renderPlayerExhaust			(playerEffectsInfo_t *pPlayer);
	void renderPlayerShadows			(playerEffectsInfo_t *pPlayer);
	void renderPlayerSounds				(playerEffectsInfo_t *pPlayer);
	void renderPlayerHP					(playerEffectsInfo_t *pPlayer);
	void renderPlayerCheckSkid			(playerEffectsInfo_t *pPlayer);


	//	CVars
	cvar_t					*m_pCvarSkidfx;
	cvar_t					*m_pCvarShadowfx;
	cvar_t					*m_pCvarShadowOpponentsfx;
	cvar_t					*m_pCvarSkidTime;
	cvar_t					*m_pCvarDustfx;
	cvar_t					*m_pCvarFogfx;
	cvar_t					*m_pCvarSparkfx;
	cvar_t					*m_pCvarHPfx_r;
	cvar_t					*m_pCvarHPfx_g;
	cvar_t					*m_pCvarHPfx_b;
	cvar_t					*m_pCvarHeadlightsOpponents;


	//	Skidmarks structure for the skid mark linked list
	typedef struct skidlist_s
	{
		vec3_t	vertices[4];
		float	time;
		struct	model_s		*texture;
		struct	skidlist_s	*next;
	} skidlist_t;


	//	Linked list head node
	skidlist_t				*m_pSkidlist;


	//	Other Variables
	vec3_t		m_vBackFireOrigin;				//	Origin of local player's exhaust

};
