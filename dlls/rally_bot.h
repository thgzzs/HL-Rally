#ifndef BOT_H
#define BOT_H

#define		NUM_ANGLES		5

extern cvar_t bot_skill;

// Bot header
class CBot : public CBasePlayer //Derive a bot class from CBasePlayer
{
	public:
		void Spawn( void );
		void EXPORT BotThink( void );
		void Killed(entvars_t *pevAttacker, int iGib);
		void PlayerDeathThink( void );

		// Bots should return FALSE for this, they can't receive NET messages
		virtual BOOL IsNetClient( void ) { return FALSE; }
		int BloodColor() { return DONT_BLEED; }

		bool m_bKicked;
		float m_fSkill;

	private:
		void HornThink (void);
		void SetupWaypointData (void);
		void MoveToWaypoint (void);
		byte ThrottledMsec (void);
		void BotThinkMain (void);

		// Bot things
		float m_fPrevSpeed, m_fSpeed;
		int m_iOffset;
		float	m_fNewSpeed;

		// Waypoint things
		bool m_bFirstWaypoint;
		vec3_t m_vTarg, m_vTargCentre;
		vec3_t m_vNextWay;
		float m_fNextWayAngle, m_fNextWaySpeed;

		// Pathing
		int m_iCurrentPath;

		// Not turning back
		int m_iWrongWay;
		float m_fLastSkip;
		float m_fPrevDist, m_fPrevNextDist;

		// MSEC
		float	m_flPreviousCommandTime;
};

#endif // BOT_H
