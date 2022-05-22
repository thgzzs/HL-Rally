/*
********************************************************************************
*
*	Copyright (c) HL Rally Team 2001
*	
*   Author: FragMented!
*  
*   Purpose: HL Rally Key Round Code
*
********************************************************************************
*/ 

#ifndef __Rally_Rounds_H__
#define __Rally_Rounds_H__

// Define Time restraints
// SaRcaZm - Start
#define ROUNDTIME_LIMIT			6000		// 100 minutes
//#define ROUNDTIME_LIMIT			180		// 3 minutes (2*60)
//#define ROUNDTIME_SETUP			15		// 15 secs
// SaRcaZm - End
#define ROUNDTIME_LIMIT_HP		20		// 20 secs plus variation
#define ROUNDTIME_CHECK			10		// 10 secs
#define ROUNDTIME_DELAY			10		// 10 secs
// SaRcaZm - V5 - Start
#define COUNTDOWN_TIME			3.6
// SaRcaZm - V5 - End

// Define Scoring
#define TSCORE_TEAM1			2		// team1 gets 2 points
#define TSCORE_TEAM2			2		// team2 gets 2 points
#define TSCORE_DRAW				0		// team1&2 get 0 points

#define ISCORE_NORMAL			1		// player gets 1 frag
#define ISCORE_SUICIDE			-1		// player loses 1 frag
#define ISCORE_TEAM				-1		// player loses 1 frag

// Round flow control enums
enum
{
	ROUND_NOTSTARTED = 0,	// round is preparing to start
	ROUND_STARTED,			// round is going
	ROUND_END,				// round is stopped (prepare to prepare?)

	// SaRcaZm - V7 - Start
	ROUND_COUNTDOWN,
	// SaRcaZm - V7 - End
};

// Scoring enums:
enum
{
	TIME_UP = 0,	// time up
	ALL_FINISHED,	// Everyone is finished all the laps
	AB_FINISH,		// If a player finishes an AB_ map
	HP_END,			// HP_ mode player blew up
};

extern cvar_t numlaps;
extern cvar_t prerace;
extern int gmsgSetTimer;
extern int gmsgRaceInfo;
extern int gmsgFixAngle;
extern int gmsgCheckpoint;
extern int gmsgLapEnd;
extern int gmsgRaceEnd;
extern int m_iTotalCheckPoints;
// SaRcaZm - V5 - Start
extern int gmsgCountdown;
// SaRcaZm - V5 - End
// SaRcaZm - V6 - Start
extern cvar_t direction;
extern float fGlobalRaceStartTime;

#define MAX_PLAYERS	64
#define MAX_LAPS	99//10
// SaRcaZm - V6 - End

// ------------------------------------------------------------------------
// Class + Function Declaration
// ------------------------------------------------------------------------
class CRallyCheckPoint : public CBaseToggle
{
public:
	// SaRcaZm - V7 - Start
	CRallyCheckPoint (void) { m_iTimeExtension = 10; };
	// SaRcaZm - V7 - End

	void Touch(CBaseEntity *pBase);
	void Spawn(void);
	void KeyValue(KeyValueData *pkvd);

	float Time;

	int m_iScoreIncrement;
	float m_fCurrentCheckPoint;

	void ClearPlaces (void);

	// SaRcaZm - V6 - Start
	int m_iTimeExtension;
	// SaRcaZm - V6 - End
	// SaRcaZm - V8 - Start
	int m_iTimeExtensionReverse;
	// SaRcaZm - V8 - End

private:
	// SaRcaZm - V4 - Start
	//int m_iPlaces[10];		// Store the place of the current person (Max 10 laps)
	CBasePlayer *m_pPlaces[MAX_LAPS][MAX_PLAYERS];
	// SaRcaZm - V4 - End
};
// SaRcaZm - V2 - End

// SaRcaZm - V7 - Start
class CRallyWayPoint : public CBaseToggle
{
public:
	void Spawn(void);
	void KeyValue(KeyValueData *pkvd);

	int number;
	float offset;
	int path;
	// SaRcaZm - V8 - Start
	int iPaths[10];		// Maximum number of paths
	int iReversePaths[10];
	int iNumPaths, iNumReversePaths;
	float speedfactor;
	// SaRcaZm - V8 - End

private:
};
// SaRcaZm - V7 - End

// SaRcaZm - V6 - Start
// Moved this here from subs.cpp
class CBaseDMStart : public CPointEntity
{
public:
	CBaseDMStart (void)
	{
		m_iPosition = 0;	// Set this to a default of no position
	}

	void		KeyValue( KeyValueData *pkvd );
	BOOL		IsTriggered( CBaseEntity *pEntity );

	// SaRcaZm - V7 - Start
	int			m_iPosition;
	// SaRcaZm - V7 - End

private:
};

class CBaseDMRespawn : public CBaseDMStart
{
public:
	void KeyValue (KeyValueData *pkvd)
	{
		if (FStrEq(pkvd->szKeyName, "checkpoint"))
			m_fCheckPoint = atof (pkvd->szValue);
	}

	int m_fCheckPoint;
};

extern cvar_t racemode;
// SaRcaZm - V6 - End

#endif // __Rally_Rounds_H__