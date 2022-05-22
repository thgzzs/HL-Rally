/*
********************************************************************************
*
*	Copyright (c) HL Rally Team 2001
*	
*   Author: FragMented!
*  
*   Purpose: HL Rally Hot Potato Weapon Code
*
********************************************************************************
*/ 



#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "rally_rounds.h"

extern cvar_t hptime;

class CRallyHP : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);
	BOOL Deploy( void );
	int AddToPlayer (CBasePlayer *pPlayer);

private:
	void EXPORT SearchRadius (void);
	float m_fEndTime;
	int m_iHPTime;
};


#define MSG_COUNT 11

const char *szPassMessages[MSG_COUNT] =
{
	"%s set up %s the bomb\n",															//	1
	"%s passes the bomb to %s\n",														//	2
	"%s slips a bomb up %s's butt\n",													//	3
	"%s offloaded the bomb to %s\n",													//	4
	"%s hid the bomb in %s's boot\n",													//	5
	"%s slipped the bomb in %s's drink\n",												//	6
	"%s's bomb fell off onto %s's car\n",												//	7
	"%s wonders what c4 is and throws it at %s\n",										//	8
	"%s lost the bomb and found it on %s's car\n",										//	9
	"%s defuses the bomb, then plants the bomb on %s!\n"								//	10
	"%s hands %s a special package\n"													//	11
};


LINK_ENTITY_TO_CLASS( weapon_rallyhp, CRallyHP );

void CRallyHP::Spawn( )
{
	Precache( );
	m_iId = WEAPON_RALLY_HP;
	SET_MODEL(ENT(pev), "models/hp_bomb.mdl");
	m_iClip = -1;
	m_iHPTime = hptime.value;

	FallInit();// get ready to fall down.
}

void CRallyHP::Precache( void )
{
	extern DLL_GLOBAL	short	g_sModelIndexFireball;
	g_sModelIndexFireball = PRECACHE_MODEL ("sprites/zerogxplode.spr");
	PRECACHE_MODEL("models/hp_bomb.mdl");
	PRECACHE_SOUND("weapons/gren_cock1.wav");
	PRECACHE_SOUND("weapons/mine_deploy.wav");
	PRECACHE_SOUND("common/wpn_denyselect.wav");
}

int CRallyHP::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 0;
	p->iId = WEAPON_CROWBAR;
	p->iWeight = CROWBAR_WEIGHT;
	return 1;
}

int CRallyHP::AddToPlayer (CBasePlayer *pPlayer)
{
	m_pPlayer = pPlayer;
	SetThink (SearchRadius);
	pev->nextthink = gpGlobals->time + 0.1;
	m_fEndTime = gpGlobals->time + m_iHPTime;
	m_iHPTime -= 2;

	// Start the timer running again
	MESSAGE_BEGIN( MSG_ALL, gmsgSetTimer );
		WRITE_COORD (gpGlobals->time);
	MESSAGE_END();

	return 1;
}

BOOL CRallyHP::Deploy( )
{
	return DefaultDeploy( "models/interior.mdl", "models/hp_bomb.mdl", 1, "crowbar" );
}

void CRallyHP::SearchRadius (void)
{
	CBaseEntity *pEntity = NULL;
	TraceResult	tr;
	Vector vecSrc = m_pPlayer->pev->origin;

	// Check to see if the time has run out
	if (gpGlobals->time > m_fEndTime)
		g_pGameRules->RoundEnd (HP_END, (char *)STRING(m_pPlayer->pev->netname));

	// iterate on all entities in the vicinity.
	while ((pEntity = UTIL_FindEntityInSphere( pEntity, vecSrc, 40 )) != NULL)
	{
		if (pEntity == m_pPlayer)
			continue;

		if ( pEntity->pev->takedamage != DAMAGE_NO )
		{
			vec3_t vecSpot = pEntity->BodyTarget( vecSrc );

			UTIL_TraceLine ( vecSrc, vecSpot, dont_ignore_monsters, m_pPlayer->edict (), &tr );

			if ( tr.flFraction == 1.0 || tr.pHit == pEntity->edict() )
			{
				if (tr.fStartSolid)
				{
					// if we're stuck inside them, fixup the position and distance
					tr.vecEndPos = vecSrc;
					tr.flFraction = 0.0;
				}
				
				//ALERT( at_console, "hit %s\n", STRING( pEntity->pev->classname ) );

				// Hit!
				if (pEntity)
				{
					if (pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE)
					{
						EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/mine_deploy.wav", 1, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3)); 

						char sz[256];
						sprintf(sz, "%s gives %s the bomb\n", (char *)STRING(m_pPlayer->pev->netname), (char *)STRING(pEntity->pev->netname));
						UTIL_SayTextAll(sz, m_pPlayer);

						// Transfer the bomb!
						m_pPlayer->RemovePlayerItem(this);
						m_pPlayer->AddPoints(2, true);
						pEntity->AddPlayerItem(this);

						m_pPlayer = (CBasePlayer *) pEntity;
						pev->nextthink = gpGlobals->time + 1;	// No takesies backsies
						return;
					}
				}
			}
		}
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

