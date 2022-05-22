// cl.input.c  -- builds an intended movement command to send to the server

//xxxxxx Move bob and pitch drifting code here and other stuff from view if needed

// Quake is a trademark of Id Software, Inc., (c) 1996 Id Software, Inc. All
// rights reserved.
#include "hud.h"
#include "cl_util.h"
#include "camera.h"
extern "C"
{
#include "kbutton.h"
}
#include "cvardef.h"
#include "usercmd.h"
#include "const.h"
#include "camera.h"
#include "in_defs.h"
#include "view.h"
#include <string.h>
#include <ctype.h>

// Crem pm
#include "pm_defs.h"
#include "pm_debug.h"
#include "pmtrace.h"

#include "vgui_TeamFortressViewport.h"

// Frag Vangles
float RallyYaw;

extern "C" 
{
	struct kbutton_s DLLEXPORT *KB_Find( const char *name );
	void DLLEXPORT CL_CreateMove ( float frametime, struct usercmd_s *cmd, int active );
	void DLLEXPORT HUD_Shutdown( void );
	int DLLEXPORT HUD_Key_Event( int eventcode, int keynum, const char *pszCurrentBinding );
	void PM_ParticleLine( float *start, float *end, int pcolor, float life, float vert);
}
extern "C" void Global_DLLEXPORT_Info( float time, const char *func_name );

extern "C" playermove_t *pmove;

extern int g_iAlive;

extern int g_weaponselect;
extern cl_enginefunc_t gEngfuncs;
extern cvar_t	*joy_forwardsensitivity;

// Defined in pm_math.c
extern "C" float anglemod( float a );

void IN_Init (void);
float IN_Move ( float frametime, usercmd_t *cmd);
void IN_Shutdown( void );
void V_Init( void );
void VectorAngles( const float *forward, float *angles );
int CL_ButtonBits( int );

// xxx need client dll function to get and clear impuse
extern cvar_t *in_joystick;

extern float fPlayerAngles;
int	in_impulse	= 0;
int	in_cancel	= 0;

float g_accelerating = 0;
float g_braking = 0;
float g_reversing = 0;
float g_handbraking = 0;
float g_tractionloss = 0;

int g_backfire = 0;

// Creme: Joystick forward axis, for export
float JoyMoved = 0;


cvar_t	*m_pitch;
cvar_t	*m_yaw;
cvar_t	*m_forward;
cvar_t	*m_side;

cvar_t	*lookstrafe;
cvar_t	*lookspring;
cvar_t	*cl_pitchup;
cvar_t	*cl_pitchdown;
cvar_t	*cl_upspeed;
cvar_t	*cl_forwardspeed;
cvar_t	*cl_backspeed;
cvar_t	*cl_sidespeed;
cvar_t	*cl_movespeedkey;
cvar_t	*cl_yawspeed;
cvar_t	*cl_pitchspeed;
cvar_t	*cl_anglespeedkey;
cvar_t	*cl_vsmoothing;


// Creme: V9 start
cvar_t	*cl_steer_center_speed;
cvar_t	*cl_steer_threshold;
cvar_t	*cl_steer_delta_mul;
cvar_t	*cl_steer_delta_dec;
// Creme: V9 end

// FragMented - RC7
cvar_t	*rally_rearview;

// SaRcaZm - V6 - Start
int gear;
extern float rpms;
// SaRcaZm - V6 - End

/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as a parameter to the command so it can be matched up with
the release.

state bit 0 is the current state of the key
state bit 1 is edge triggered on the up to down transition
state bit 2 is edge triggered on the down to up transition

===============================================================================
*/


kbutton_t	in_mlook;
kbutton_t	in_klook;
kbutton_t	in_jlook;
kbutton_t	in_left;
kbutton_t	in_right;
kbutton_t	in_forward;
kbutton_t	in_back;
kbutton_t	in_lookup;
kbutton_t	in_lookdown;
kbutton_t	in_moveleft;
kbutton_t	in_moveright;
kbutton_t	in_strafe;
kbutton_t	in_speed;
kbutton_t	in_use;
kbutton_t	in_jump;
kbutton_t	in_attack;
kbutton_t	in_attack2;
kbutton_t	in_up;
kbutton_t	in_down;
kbutton_t	in_duck;
kbutton_t	in_reload;
kbutton_t	in_alt1;
kbutton_t	in_score;
kbutton_t	in_break;
kbutton_t	in_graph;  // Display the netgraph

typedef struct kblist_s
{
	struct kblist_s *next;
	kbutton_t *pkey;
	char name[32];
} kblist_t;

kblist_t *g_kbkeys = NULL;

#define	SLOW_SPEED_CUTOFF	20

bool fixangle;

#include "rally_effects.h"
#include "rally_sound.h"

float steer_delta = 0;  // The amount to update the steering position.

static float steer_accel = 0 ;  // The amount to update the delta.
float steer_delta_accel = 0;  // The amount to update the steering delta!
float steer_wheel_angle = 0; // The angle of the wheel.

static bool setangles = false;
static vec3_t oldangles;

/*
============
KB_ConvertString

Removes references to +use and replaces them with the keyname in the output string.  If
 a binding is unfound, then the original text is retained.
NOTE:  Only works for text with +word in it.
============
*/
int KB_ConvertString( char *in, char **ppout )
{
	char sz[ 4096 ];
	char binding[ 64 ];
	char *p;
	char *pOut;
	char *pEnd;
	const char *pBinding;

	if ( !ppout )
		return 0;

	*ppout = NULL;
	p = in;
	pOut = sz;
	while ( *p )
	{
		if ( *p == '+' )
		{
			pEnd = binding;
			while ( *p && ( isalnum( *p ) || ( pEnd == binding ) ) && ( ( pEnd - binding ) < 63 ) )
			{
				*pEnd++ = *p++;
			}

			*pEnd =  '\0';

			pBinding = NULL;
			if ( strlen( binding + 1 ) > 0 )
			{
				// See if there is a binding for binding?
				pBinding = gEngfuncs.Key_LookupBinding( binding + 1 );
			}

			if ( pBinding )
			{
				*pOut++ = '[';
				pEnd = (char *)pBinding;
			}
			else
			{
				pEnd = binding;
			}

			while ( *pEnd )
			{
				*pOut++ = *pEnd++;
			}

			if ( pBinding )
			{
				*pOut++ = ']';
			}
		}
		else
		{
			*pOut++ = *p++;
		}
	}

	*pOut = '\0';

	pOut = ( char * )malloc( strlen( sz ) + 1 );
	strcpy( pOut, sz );
	*ppout = pOut;

	return 1;
}

/*
============
KB_Find

Allows the engine to get a kbutton_t directly ( so it can check +mlook state, etc ) for saving out to .cfg files
============
*/
struct kbutton_s DLLEXPORT *KB_Find( const char *name )
{
	kblist_t *p;
	p = g_kbkeys;
	while ( p )
	{
		if ( !stricmp( name, p->name ) )
			return p->pkey;

		p = p->next;
	}
	return NULL;
}

/*
============
KB_Add

Add a kbutton_t * to the list of pointers the engine can retrieve via KB_Find
============
*/
void KB_Add( const char *name, kbutton_t *pkb )
{
	kblist_t *p;	
	kbutton_t *kb;

	kb = KB_Find( name );
	
	if ( kb )
		return;

	p = ( kblist_t * )malloc( sizeof( kblist_t ) );
	memset( p, 0, sizeof( *p ) );

	strcpy( p->name, name );
	p->pkey = pkb;

	p->next = g_kbkeys;
	g_kbkeys = p;
}

/*
============
KB_Init

Add kbutton_t definitions that the engine can query if needed
============
*/
void KB_Init( void )
{
	g_kbkeys = NULL;

	KB_Add( "in_graph", &in_graph );
	KB_Add( "in_mlook", &in_mlook );
	KB_Add( "in_jlook", &in_jlook );
}

/*
============
KB_Shutdown

Clear kblist
============
*/
void KB_Shutdown( void )
{
	kblist_t *p, *n;
	p = g_kbkeys;
	while ( p )
	{
		n = p->next;
		free( p );
		p = n;
	}
	g_kbkeys = NULL;
}

/*
============
KeyDown
============
*/
void KeyDown (kbutton_t *b)
{
	int		k;
	char	*c;

	c = gEngfuncs.Cmd_Argv(1);
	if (c[0])
		k = atoi(c);
	else
		k = -1;		// typed manually at the console for continuous down

	if (k == b->down[0] || k == b->down[1])
		return;		// repeating key
	
	if (!b->down[0])
		b->down[0] = k;
	else if (!b->down[1])
		b->down[1] = k;
	else
	{
		gEngfuncs.Con_DPrintf ("Three keys down for a button '%c' '%c' '%c'!\n", b->down[0], b->down[1], c);
		return;
	}
	
	if (b->state & 1)
		return;		// still down
	b->state |= 1 + 2;	// down + impulse down
}

/*
============
KeyUp
============
*/
void KeyUp (kbutton_t *b)
{
	int		k;
	char	*c;
	
	c = gEngfuncs.Cmd_Argv(1);
	if (c[0])
		k = atoi(c);
	else
	{ // typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->state = 4;	// impulse up
		return;
	}

	if (b->down[0] == k)
		b->down[0] = 0;
	else if (b->down[1] == k)
		b->down[1] = 0;
	else
		return;		// key up without coresponding down (menu pass through)
	if (b->down[0] || b->down[1])
	{
		//Con_Printf ("Keys down for button: '%c' '%c' '%c' (%d,%d,%d)!\n", b->down[0], b->down[1], c, b->down[0], b->down[1], c);
		return;		// some other key is still holding it down
	}

	if (!(b->state & 1))
		return;		// still up (this should not happen)

	b->state &= ~1;		// now up
	b->state |= 4; 		// impulse up
}

/*
============
HUD_Key_Event

Return 1 to allow engine to process the key, otherwise, act on it as needed
============
*/
int DLLEXPORT HUD_Key_Event( int down, int keynum, const char *pszCurrentBinding )
{
	if (gViewPort)
		return gViewPort->KeyInput(down, keynum, pszCurrentBinding);
	
	return 1;
}

void IN_BreakDown( void ) { KeyDown( &in_break );};
void IN_BreakUp( void ) { KeyUp( &in_break ); };
void IN_KLookDown (void) {KeyDown(&in_klook);}
void IN_KLookUp (void) {KeyUp(&in_klook);}
void IN_JLookDown (void) {KeyDown(&in_jlook);}
void IN_JLookUp (void) {KeyUp(&in_jlook);}
void IN_MLookDown (void) {KeyDown(&in_mlook);}
void IN_UpDown(void) {KeyDown(&in_up);}
void IN_UpUp(void) {KeyUp(&in_up);}
void IN_DownDown(void) {KeyDown(&in_down);}
void IN_DownUp(void) {KeyUp(&in_down);}
void IN_LeftDown(void) {KeyDown(&in_left);}
void IN_LeftUp(void) {KeyUp(&in_left);}
void IN_RightDown(void) {KeyDown(&in_right);}
void IN_RightUp(void) {KeyUp(&in_right);}
void IN_ForwardDown(void) {KeyDown(&in_forward);}
void IN_ForwardUp(void) {KeyUp(&in_forward);}
void IN_BackDown(void) {KeyDown(&in_back);}
void IN_BackUp(void) {KeyUp(&in_back);}
void IN_LookupDown(void) {KeyDown(&in_lookup);}
void IN_LookupUp(void) {KeyUp(&in_lookup);}
void IN_LookdownDown(void) {KeyDown(&in_lookdown);}
void IN_LookdownUp(void) {KeyUp(&in_lookdown);}
void IN_MoveleftDown(void) {KeyDown(&in_moveleft);}
void IN_MoveleftUp(void) {KeyUp(&in_moveleft);}
void IN_MoverightDown(void) {KeyDown(&in_moveright);}
void IN_MoverightUp(void) {KeyUp(&in_moveright);}
void IN_SpeedDown(void) {KeyDown(&in_speed);}
void IN_SpeedUp(void) {KeyUp(&in_speed);}
void IN_StrafeDown(void) {KeyDown(&in_strafe);}
void IN_StrafeUp(void) {KeyUp(&in_strafe);}

// SaRcaZm - V8 - Start
void IN_GearUp(void)
{
	if (CRallyEffects::getSingleton()->m_pCvarManual->value && gear < 6)
	{
		gear++;
	}
}

void IN_GearDown(void)
{
	if (CRallyEffects::getSingleton()->m_pCvarManual->value && gear > -1)
	{
		g_backfire = 2;
		gear--;
	}
}
// SaRcaZm - V8 - End

// needs capture by hud/vgui also
extern void __CmdFunc_InputPlayerSpecial(void);
void IN_Attack2Down(void) 
{
	KeyDown(&in_attack2);
	__CmdFunc_InputPlayerSpecial();
}

void IN_Attack2Up(void) {KeyUp(&in_attack2);}
void IN_UseDown (void) {KeyDown(&in_use);}
void IN_UseUp (void) {KeyUp(&in_use);}
void IN_JumpDown (void) {KeyDown(&in_jump);}
void IN_JumpUp (void) {KeyUp(&in_jump);}
void IN_DuckDown(void) {KeyDown(&in_duck);}
void IN_DuckUp(void) {KeyUp(&in_duck);}
void IN_ReloadDown(void) {KeyDown(&in_reload);}
void IN_ReloadUp(void) {KeyUp(&in_reload);}
void IN_Alt1Down(void) {KeyDown(&in_alt1);}
void IN_Alt1Up(void) {KeyUp(&in_alt1);}
void IN_GraphDown(void) {KeyDown(&in_graph);}
void IN_GraphUp(void) {KeyUp(&in_graph);}

void IN_AttackDown(void)
{
	KeyDown( &in_attack );
}

void IN_AttackUp(void)
{
	KeyUp( &in_attack );
	in_cancel = 0;
}

// Special handling
void IN_Cancel(void)
{
	in_cancel = 1;
}

void IN_Impulse (void)
{
	in_impulse = atoi( gEngfuncs.Cmd_Argv(1) );
}

void IN_ScoreDown(void)
{
	KeyDown(&in_score);
	if ( gViewPort )
	{
		gViewPort->ShowScoreBoard();
	}
}

void IN_ScoreUp(void)
{
	KeyUp(&in_score);
	if ( gViewPort )
	{
		gViewPort->HideScoreBoard();
	}
}

void IN_MLookUp (void)
{
	KeyUp( &in_mlook );
	if ( !( in_mlook.state & 1 ) && lookspring->value )
	{
		V_StartPitchDrift();
	}
}

/*
===============
CL_KeyState

Returns 0.25 if a key was pressed and released during the frame,
0.5 if it was pressed and held
0 if held then released, and
1.0 if held for the entire time
===============
*/
float CL_KeyState (kbutton_t *key)
{
	float		val = 0.0;
	int			impulsedown, impulseup, down;
	
	impulsedown = key->state & 2;
	impulseup	= key->state & 4;
	down		= key->state & 1;
	
	if ( impulsedown && !impulseup )
	{
		// pressed and held this frame?
		val = down ? 0.5 : 0.0;
	}

	if ( impulseup && !impulsedown )
	{
		// released this frame?
		val = down ? 0.0 : 0.0;
	}

	if ( !impulsedown && !impulseup )
	{
		// held the entire frame?
		val = down ? 1.0 : 0.0;
	}

	if ( impulsedown && impulseup )
	{
		if ( down )
		{
			// released and re-pressed this frame
			val = 0.75;	
		}
		else
		{
			// pressed and released this frame
			val = 0.25;	
		}
	}

	// clear impulses
	key->state &= 1;		
	return val;
}

/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
void CL_AdjustAngles ( float frametime, float *viewangles )
{
	float	speed;//, velocity;
//	float	up, down;

//	velocity = Length(pmove->velocity);

/*	if (in_speed.state & 1)	{

		speed = frametime * cl_anglespeedkey->value;

	} else {
*/
		speed = frametime;

//	}

	// Creme, we'll keep this old var, but partially anti-haxor it.
	if (cl_yawspeed->value > 400) {
		cl_yawspeed->value = 400;
	}


	if (!(in_strafe.state & 1))	{
//		if (velocity > 20) {
			viewangles[YAW] -= speed*cl_yawspeed->value*CL_KeyState (&in_right);
			viewangles[YAW] += speed*cl_yawspeed->value*CL_KeyState (&in_left);
			viewangles[YAW] = anglemod(viewangles[YAW]);
//		}
	}


}

/*
================
CL_CreateMove

Send the intended movement message to the server
if active == 1 then we are 1) not playing back demos ( where our commands are ignored ) and
2 ) we have finished signing on to server
================
*/

float PowerOf(float num, float power)
{
	int i;
	float number=num;

	if (power > 1) {
		for (i = 2; i <= power; i++) {
			number *= num;
		}
	} 

	return number;
}

// SaRcaZm - V5 - Start
extern int ab_finish;
// SaRcaZm - V5 - End



float Limit (float n, float min, float max) {

	if (n > max) n = max;
	if (n < min) n = min;

	return n;

}

#include "com_model.h"
void FixYaw (void)
{
	cl_entity_t *pthisplayer = gEngfuncs.GetLocalPlayer();
	cl_entity_t *pEnt = gEngfuncs.GetEntityByIndex( 0 );	// get world model

	vec3_t position, angle, savedangle;
	position = angle = savedangle = Vector (0, 0, 0);

	int min = 8196;

	if ( pEnt )
	{
		char			*data;
		char			keyname[256];
		int				n;
		char			token[1024];

		for (int i = 0; i < 3; i++)
		{
			for ( data = gEngfuncs.COM_ParseFile( pEnt->model->entities, token) ;	// cl.worldmodel->entities
				data && (*data) && (*token != '}');
				data = gEngfuncs.COM_ParseFile(data, token) ) // TODO check this for null pointer crashes
			{
				
				if (token[0] != '{')
					return;
				
				while (1)
				{	
					// parse key
					data = gEngfuncs.COM_ParseFile(data, token);
					if (token[0] == '}')
						break;
					if (!data)
						return;
					
					strcpy (keyname, token);
					// another hack to fix keynames with trailing spaces
					n = strlen(keyname);
					while (n && keyname[n-1] == ' ')
					{
						keyname[n-1] = 0;
						n--;
					}
					
					// parse value
					data = gEngfuncs.COM_ParseFile(data, token);
					
					if (!data)
						return;
					
					else if (token[0] == '}')
						return;
					
					else if (!stricmp(keyname,"classname"))
					{
						if (!stricmp(token,"info_player_start") || !stricmp(token,"info_player_start_reverse"))
						{
							int length = (pthisplayer->origin - position).Length ();
							//int length = (pmove->origin - position).Length ();
							if (length < min)
							{
								savedangle = angle;
								min = length;
							}
						}
					}

					else if (!stricmp(keyname,"origin"))
					{
						UTIL_StringToVector(position, token);
					}

					else if (!stricmp(keyname,"angles"))
					{
						UTIL_StringToVector(angle, token);
					}

				} // while (1)

				if (strlen(data) < 2) { 
					break;
				}
			}
		}
	}

	//gEngfuncs.Con_DPrintf ("Origin: %f, %f, %f\nAngles: %f, %f, %f\n", pthisplayer->origin[0], pthisplayer->origin[1], pthisplayer->origin[2], savedangle[0], savedangle[1], savedangle[2]);
	oldangles = savedangle;
	setangles = true;
}

void DLLEXPORT CL_CreateMove ( float frametime, struct usercmd_s *cmd, int active )
{	
	float spd;
	vec3_t viewangles;
	float	velocity;
	vec3_t  tempvec;
	float sliding_ofs;
	float reversing = 1;

	// Creme: new steering acceleration system.


	float max_steer_angle = 45; // Self explanetory;

	Global_DLLEXPORT_Info(gEngfuncs.GetClientTime(), "CL_CreateMove");

	//gEngfuncs.Con_DPrintf("CreateMove (input) (%f)\n", fPlayerAngles);

	tempvec = pmove->velocity; tempvec[2] = 0;
	velocity = Length(tempvec);

	// SaRcaZm - V2 - Start
	//if ( active )
	// Can't create move if we aren't on the ground
	// Creme; actually you can still update the model turning in mid-air
	//        so i'll do seperate checks for onground as we go

	// SaRcaZm - V5 - Start
	vec3_t ab_viewangles;
	if (ab_finish && (velocity > 20))
	{
		cmd->buttons = 0;
		cmd->buttons |= IN_BACK;
		g_braking = 1;

		in_forward.state = 0;
		in_back.state = 1;

		gEngfuncs.GetViewAngles ((float *) ab_viewangles);
	}
	else if (ab_finish)
	{
		g_braking = 0;
		cmd->buttons = 0;
		in_back.state = 0;
		gEngfuncs.GetViewAngles ((float *) ab_viewangles);
	}
	else if (setangles)	// Set the view angles based on oldangles
	{
		setangles = false;
		gEngfuncs.SetViewAngles ((float *) oldangles);
	}
	else if (fixangle)	// This means the engine has set the view angles and we shouldn't touch them
	{					// That means you Creme! (this was working till you terrorized my CL_CreateMove)
		fixangle = false;

		// SaRcaZm - V7 - Start
		// We also want to reset to neutral
		gear = 0;
		steer_delta = 0; // Creme/Sarc - reset delta.
		// SaRcaZm - V7 - End

		gEngfuncs.GetViewAngles ((float *) oldangles);
	}
	else
	// SaRcaZm - V5 - End

	if (active) // && (pmove->onground != -1))
	// SaRcaZm - V2 - End
	{
		gEngfuncs.GetViewAngles( (float *)viewangles );
		CL_AdjustAngles ( frametime, viewangles );
		gEngfuncs.SetViewAngles( (float *)viewangles );

		// Can't create move if we aren't on the ground
		memset (cmd, 0, sizeof(*cmd));

		// SaRcaZm - V5 - Start
		// Moved FragMented!'s code up from the "if (pmove->onground != -1)"
		// So the brake lights go on even if you are in the air
		bool isReverse = false;

		if ((velocity < 40 && CL_KeyState(&in_back) > 0 && !CRallyEffects::getSingleton()->m_pCvarManual->value) ||
			(gear == -1 && CRallyEffects::getSingleton()->m_pCvarManual->value))
		{
			isReverse = true;
		}

		if (CL_KeyState(&in_back) > 0) {
			g_braking = 1;
		} else {
			g_braking = 0;
		}

/*		if ((g_backfire == 1) || (g_backfire == 2)) {
			g_backfire = -1;
		} else {
//			g_backfire = 0;
		}
*/

		if ((CL_KeyState(&in_forward) > 0) &&
			(!CRallyEffects::getSingleton()->m_pCvarManual->value || (CRallyEffects::getSingleton()->m_pCvarManual->value && gear != -1)))
		{
			isReverse = false;
			if(g_accelerating == 2) {
				g_accelerating = 1;
				if (rpms > 140) { g_backfire = 1; }
			} else {
				g_accelerating = 1; 
			}
		}
		else
			g_accelerating = 0;
		// SaRcaZm - V5 - End

		// Creme: g_backfire = 2 is when there is a general/override
		// to force a backfire, like on a gear change down

		// Allow mice and other controllers to add their inputs

		// SaRcaZm, completely reworked by Creme
		// Now that we have all inputs, cut off the amount of turn allowed
		// We must get the new view angles which include the mouse and joystick movement

		// Creme; forward axis of joystick is returned.
		JoyMoved = IN_Move ( frametime, cmd );

		JoyMoved = JoyMoved * joy_forwardsensitivity->value;

		//gEngfuncs.Con_DPrintf("JoyMoved: (%f)\n", JoyMoved);

		// Creme: this needs more work.
		// This code is re-done at the bottom, still no effect
		// Add 0.1 of 'dead-zone'
		if (JoyMoved > 0.1) {
			
			if (cmd->buttons & IN_FORWARD) {
			} else {
				cmd->buttons |= IN_FORWARD;
			}

		} else if (JoyMoved < 0.1) {

			if (cmd->buttons & IN_BACK) {
			} else {
				cmd->buttons |= IN_BACK;
			}
		}

		if (pmove->onground != -1) {

			// SaRcaZm - V6 - Start
			if (isReverse)
			{
				g_braking = 0;
				g_reversing = 1;
				cl_forwardspeed->value = 10000;
				if (!CRallyEffects::getSingleton()->m_pCvarManual->value)
					cmd->forwardmove -= cl_forwardspeed->value * CL_KeyState (&in_back);
				else
					cmd->forwardmove -= cl_forwardspeed->value * CL_KeyState (&in_forward);

				// SaRcaZm - V8 - Start
				if (CRallyEffects::getSingleton()->m_pCvarManual->value && CL_KeyState (&in_forward))
					g_reversing = 2;
				else if (CL_KeyState (&in_back))
					g_reversing = 2;
				// SaRcaZm - V8 - End
			}
			else
			{
				cl_forwardspeed->value = 10000;
				g_reversing = 0;
			}

			if ( !(in_klook.state & 1 ) &&
				(
					(
						(gear > 0) && (CRallyEffects::getSingleton()->m_pCvarManual->value)
					)
					||
					(
						!CRallyEffects::getSingleton()->m_pCvarManual->value
					)
				)
			)
			{
				vec3_t vel;
				VectorCopy(pmove->velocity, vel);

				// SaRcaZm
				// This was stopping us from accelerating at low speed 
				// whilst revving the engine

				// SaRcaZm - V8 - Start
				if (velocity == 0)
					cl_forwardspeed->value = 10000;
				else if (rpms < 200 || (velocity < 50))
					cl_forwardspeed->value = sqrt(vel[0]*vel[0] + vel[1]*vel[1]) + ((rpms*30) / (velocity/78));
				else
					cl_forwardspeed->value = sqrt(vel[0]*vel[0] + vel[1]*vel[1]);
				// SaRcaZm - V8 - End

				// Creme to fix new prob with auto-gearbox.
				if (g_reversing) {
					// Reversing speedlimit.
					cmd->forwardmove += cl_forwardspeed->value * CL_KeyState (&in_forward) * 0.5;
				} else {
					cmd->forwardmove += cl_forwardspeed->value * CL_KeyState (&in_forward);
				}
			}
		}

		gEngfuncs.GetViewAngles( (float *)viewangles );

		steer_accel = oldangles[YAW] - viewangles[YAW];

		if (fabs(oldangles[YAW] - viewangles[YAW]) > 200) {
			if (oldangles[YAW] < viewangles[YAW]) {
				steer_accel += 360;
			} else {
				steer_accel -= 360;
			}
		}

		// Auto-recenter wheels for mouse/keyboard steering
		if ((steer_wheel_angle > 0) && (steer_wheel_angle <  cl_steer_threshold->value)) { 
			steer_wheel_angle -= cl_steer_center_speed->value * frametime; 
		}
		if ((steer_wheel_angle < 0) && (steer_wheel_angle > -cl_steer_threshold->value)) { 
			steer_wheel_angle += cl_steer_center_speed->value * frametime; 
		}

		float low_angle_mul;

		if (fabs(steer_wheel_angle) < 3) {
			low_angle_mul = 1.5;
		} else {
			low_angle_mul = 1;
		}


		// if speeding, decrease the angle
		if (velocity > 400) { 
//			steer_wheel_angle = steer_wheel_angle / (velocity / 100); 

			if (steer_wheel_angle < 5) {
				steer_wheel_angle += (steer_accel * low_angle_mul) / 10;
			} else {
				steer_wheel_angle += (steer_accel * low_angle_mul) / 15;
			}

		} else {
			steer_wheel_angle += (steer_accel * low_angle_mul) / 10;
		}

		steer_wheel_angle = Limit (steer_wheel_angle, -10, 10);

		// Creme: 10 Aug 2003 
		// Sliding direction, 
		// -ive for Left  +ive for right
		//
		float SlidingDirection = 0;
		float FWSlidingDirection = 0;
		
		if (pmove->onground != -1) {
			vec3_t tangles, vForward, vRight, vUp;

			tangles = pmove->vuser1;
			tangles[YAW] = viewangles[YAW];
			AngleVectors (tangles, vForward, vRight, vUp);

			sliding_ofs = DotProduct(pmove->velocity, vForward) / Length(pmove->velocity);
			SlidingDirection = DotProduct(pmove->velocity, vRight) / Length(pmove->velocity);

			// Work out the front wheels sliding direction
			tangles[YAW] = viewangles[YAW] + steer_wheel_angle * 2.5;
			AngleVectors (tangles, vForward, vRight, vUp);
			FWSlidingDirection = DotProduct(pmove->velocity, vRight) / Length(pmove->velocity);

			// Sanity check the result, sometimes its NaN at the start of a game.
			if (!SlidingDirection) { SlidingDirection = 0; }
			if (!FWSlidingDirection) { FWSlidingDirection = 0; }

			// Choose the lowest sliding direct, front wheels or rear.
			if (fabs(FWSlidingDirection) < fabs(SlidingDirection)) { 
//				gEngfuncs.Con_DPrintf("Front wheels chosen! %f\n", FWSlidingDirection);
				SlidingDirection = FWSlidingDirection; 
			} else {
//				gEngfuncs.Con_DPrintf("Rear wheels chosen! %f\n", SlidingDirection);
			}

			// Sanity check the result, sometimes its NaN at the start of a game.
			if (!sliding_ofs) { sliding_ofs = 0; }

			// Creme: set a multiplier to reverse the steering if applicable
			if (sliding_ofs < 0) {
				reversing = -1;
				//steer_accel = -steer_accel;
			} else {
				reversing = 1;
			}
			//gEngfuncs.Con_DPrintf("                Accel %f; Delta: %f;\n", steer_accel, steer_delta);
			//gEngfuncs.Con_DPrintf("                Angles Old %f; New: %f;\n", oldangles[YAW], viewangles[YAW]);

			if ((g_braking) && (reversing != -1)) {
				steer_accel *= 1.5;
			}

			// Creme: Jump key used for handbrake.
			if (CL_KeyState (&in_jump) > 0) {
//				steer_accel *= 3;
				if (fabs(SlidingDirection) < 0.65) {
					steer_delta = steer_delta - (SlidingDirection * pmove->frametime * 6);// * reversing);
				} else {
					steer_delta = steer_delta - (SlidingDirection * pmove->frametime * 3);// * reversing);
				}
				g_handbraking = 1;
			} else {
				g_handbraking = 0;
			}


			sliding_ofs = fabs(sliding_ofs);
			float inv_sliding_ofs = -sliding_ofs + 1;

			int skidding_factor;

			if (sliding_ofs < 0.96) {
				skidding_factor = 2;
				//gEngfuncs.Con_DPrintf("sliding!\n");

			} else {
				skidding_factor = 0;
			} 


//			float mp_arcadetraction = CVAR_GET_FLOAT("mp_arcadetraction");
			float mp_arcadetraction = -1;

			if (mp_arcadetraction < 0) mp_arcadetraction = 0;
			if (mp_arcadetraction > 1) mp_arcadetraction = 1;

//			mp_arcadetraction = -mp_arcadetraction+1;

//			gEngfuncs.Con_DPrintf("mp_arcadetraction: %f\n", mp_arcadetraction);

//			gEngfuncs.Con_DPrintf("Length(pmove->velocity) %f\n", Length(pmove->velocity));
			if (mp_arcadetraction != 1) {
				if (velocity < 500) {

					if ((((rpms > 145) && g_accelerating) || 
						(rpms > 165)) && 
						(fabs(SlidingDirection) > 0.05) && 
						(fabs(SlidingDirection) < 0.85) && 
						(gear != 0)) 
					{
						steer_delta = steer_delta - (SlidingDirection * pmove->frametime * 7.5 * reversing);// * mp_arcadetraction);
						if ((fabs(SlidingDirection) > 0.3)) {
							g_tractionloss = 1;
						} else {
							g_tractionloss = fabs(SlidingDirection);
						}
					} else {
						g_tractionloss = 0;
					}
				} else {

					if ((((rpms > 150) && g_accelerating) || 
						(rpms > 170)) && 
						(fabs(SlidingDirection) > 0.2) && 
						(fabs(SlidingDirection) < 0.85) && 
						(gear != 0)) 
					{
						steer_delta = steer_delta - (SlidingDirection * pmove->frametime * 4.5 * reversing);// * mp_arcadetraction);
						if ((fabs(SlidingDirection) > 0.4)) {
							g_tractionloss = 1;
						} else {
							g_tractionloss = fabs(SlidingDirection/3);
						}
					} else {
						g_tractionloss = 0;
					}
				}
			}


// dodgy old frame averaging technique. bad for low framerate!
			steer_delta = 
				(

					(
						((steer_accel*reversing) * PowerOf(sliding_ofs, 6))  
						+ (steer_delta*(-sliding_ofs + 1))
					) 
					+ (steer_delta * (29 + skidding_factor))

				) / (30 + skidding_factor);

			
/*			float dec_speed_amt = 1;
			if (velocity < 500) {
				dec_speed_amt = 1;
			} else {
				dec_speed_amt = 0.5;
			}
			if (steer_delta > 0) {
/*				if (steer_delta > 1) {
					steer_delta = steer_delta - (cl_steer_delta_dec->value * sliding_ofs * frametime);
				} else */
/*					steer_delta = steer_delta - (cl_steer_delta_dec->value * sliding_ofs * frametime * dec_speed_amt);
			
				if (steer_delta < 0) steer_delta = 0;
			} else {
				steer_delta = steer_delta + (cl_steer_delta_dec->value * sliding_ofs * frametime * dec_speed_amt);
				if (steer_delta > 0) steer_delta = 0;
			}


			if (velocity < 500) {
				if (fabs(steer_wheel_angle) < 3) {
					low_angle_mul = 3.5;
				} else {
					low_angle_mul = 1;
				}

				steer_delta += (((steer_wheel_angle*low_angle_mul*reversing) * PowerOf(sliding_ofs, 4)) 
					* cl_steer_delta_mul->value * frametime);
			} else {
				gEngfuncs.Con_DPrintf("steer_accel: %f\n", steer_accel);

				steer_delta += ((((steer_wheel_angle*0.6+steer_accel*0.4)*low_angle_mul*reversing*0.55) * PowerOf(sliding_ofs, 4)) 
					* cl_steer_delta_mul->value * frametime);
			}
*/

		}

		float SlowTurnLimit = 100;
		float TurnMultiplier = 25;

		if ((velocity > 5)) {
			if (velocity < SlowTurnLimit) {

				// Creme: If less than SlowTurnLimit, 
				// make it turn a lesser amount based on the limit.
				viewangles[YAW] = oldangles[YAW] - 
					((steer_delta * (velocity / SlowTurnLimit)) 
					* TurnMultiplier * frametime);

			} else {
				viewangles[YAW] = oldangles[YAW] - 
					(steer_delta * TurnMultiplier * frametime);
			}
		} else {
			viewangles[YAW] = oldangles[YAW];
		}

		float currenttime = gEngfuncs.GetClientTime ();
	
		// Wrap the YAW back into 360 degrees
		if (viewangles[YAW] < 0.0f)
			viewangles[YAW] += 360.0f;
		else if (viewangles[YAW] > 360.0f)
			viewangles[YAW] -= 360.0f;

		// Update the view angles to account for all the changes
		gEngfuncs.SetViewAngles ((float *) viewangles);

		if (pmove->onground != -1) {

			// clip to maxspeed
			spd = gEngfuncs.GetClientMaxspeed();
			if ( spd != 0.0 )
			{
				// scale the 3 speeds so that the total velocity is not > cl.maxspeed
				float fmov = sqrt( (cmd->forwardmove*cmd->forwardmove) + (cmd->sidemove*cmd->sidemove) + (cmd->upmove*cmd->upmove) );

				if ( fmov > spd )
				{
					float fratio = spd / fmov;
					cmd->forwardmove *= fratio;
					cmd->sidemove *= fratio;
					cmd->upmove *= fratio;
				}
			}
		}

	}

	// SaRcaZm - V5 - Start
	if (ab_finish)
		gEngfuncs.SetViewAngles ((float *)ab_viewangles);
	// SaRcaZm - V5 - End

	cmd->impulse = in_impulse;
	in_impulse = 0;

	cmd->weaponselect = g_weaponselect;
	g_weaponselect = 0;
	//
	// set button and flag bits
	//
	cmd->buttons = CL_ButtonBits( 1 );

	// Using joystick?
	if ( in_joystick->value )
	{
		if ( cmd->forwardmove > 0 )
		{
			cmd->buttons |= IN_FORWARD;
		}
		else if ( cmd->forwardmove < 0 )
		{
			cmd->buttons |= IN_BACK;
		}
	}

	gEngfuncs.GetViewAngles( (float *)viewangles );
	// Set current view angles.

	if ( g_iAlive )
	{
		VectorCopy( viewangles, cmd->viewangles );
		VectorCopy( viewangles, oldangles );
	}
	else
	{
		VectorCopy( oldangles, cmd->viewangles );
	}

	// FragMented
	RallyYaw = cmd->viewangles[YAW];
}

/*
============
CL_IsDead

Returns 1 if health is <= 0
============
*/
int	CL_IsDead( void )
{
	return ( gHUD.m_Health.m_iHealth <= 0 ) ? 1 : 0;
}

/*
============
CL_ButtonBits

Returns appropriate button info for keyboard and mouse state
Set bResetState to 1 to clear old state info
============
*/
int CL_ButtonBits( int bResetState )
{
	int bits = 0;

	if ( in_attack.state & 3 )
	{
		bits |= IN_ATTACK;
	}
	
	if (in_duck.state & 3)
	{
		bits |= IN_DUCK;
	}
 
	if (in_jump.state & 3)
	{
		bits |= IN_JUMP;
	}

	if ( in_forward.state & 3 )
	{
		bits |= IN_FORWARD;
	}
	
	if (in_back.state & 3)
	{
		bits |= IN_BACK;
	}

	if (in_use.state & 3)
	{
		bits |= IN_USE;
	}

	if (in_cancel)
	{
		bits |= IN_CANCEL;
	}

	if ( in_left.state & 3 )
	{
		bits |= IN_LEFT;
	}
	
	if (in_right.state & 3)
	{
		bits |= IN_RIGHT;
	}
	
	if ( in_moveleft.state & 3 )
	{
		bits |= IN_MOVELEFT;
	}
	
	if (in_moveright.state & 3)
	{
		bits |= IN_MOVERIGHT;
	}

	if (in_attack2.state & 3)
	{
		bits |= IN_ATTACK2;
	}

	if (in_reload.state & 3)
	{
		bits |= IN_RELOAD;
	}

	if (in_alt1.state & 3)
	{
		bits |= IN_ALT1;
	}

	if ( in_score.state & 3 )
	{
		bits |= IN_SCORE;
	}

	// Dead or in intermission? Shore scoreboard, too
	if ( CL_IsDead() || gHUD.m_iIntermission )
	{
		bits |= IN_SCORE;
	}

	if ( bResetState )
	{
		in_attack.state &= ~2;
		in_duck.state &= ~2;
		in_jump.state &= ~2;
		in_forward.state &= ~2;
		in_back.state &= ~2;
		in_use.state &= ~2;
		in_left.state &= ~2;
		in_right.state &= ~2;
		in_moveleft.state &= ~2;
		in_moveright.state &= ~2;
		in_attack2.state &= ~2;
		in_reload.state &= ~2;
		in_alt1.state &= ~2;
		in_score.state &= ~2;
	}

	return bits;
}

/*
============
CL_ResetButtonBits

============
*/
void CL_ResetButtonBits( int bits )
{
	int bitsNew = CL_ButtonBits( 0 ) ^ bits;

	// Has the attack button been changed
	if ( bitsNew & IN_ATTACK )
	{
		// Was it pressed? or let go?
		if ( bits & IN_ATTACK )
		{
			KeyDown( &in_attack );
		}
		else
		{
			// totally clear state
			in_attack.state &= ~7;
		}
	}
}

/*
============
InitInput
============
*/
void InitInput (void)
{
	gEngfuncs.pfnAddCommand ("+moveup",IN_UpDown);
	gEngfuncs.pfnAddCommand ("-moveup",IN_UpUp);
	gEngfuncs.pfnAddCommand ("+movedown",IN_DownDown);
	gEngfuncs.pfnAddCommand ("-movedown",IN_DownUp);

	// SaRcaZm - V5 - Start
	// Anti-Haxor Stuff
	/*
	gEngfuncs.pfnAddCommand ("+left",IN_LeftDown);
	gEngfuncs.pfnAddCommand ("-left",IN_LeftUp);
	gEngfuncs.pfnAddCommand ("+right",IN_RightDown);
	gEngfuncs.pfnAddCommand ("-right",IN_RightUp);
	gEngfuncs.pfnAddCommand ("+forward",IN_ForwardDown);
	gEngfuncs.pfnAddCommand ("-forward",IN_ForwardUp);
	gEngfuncs.pfnAddCommand ("+back",IN_BackDown);
	gEngfuncs.pfnAddCommand ("-back",IN_BackUp);
	*/
	gEngfuncs.pfnAddCommand (clientConst.szPlus_Left,IN_LeftDown);
	gEngfuncs.pfnAddCommand (clientConst.szMinus_Left,IN_LeftUp);
	gEngfuncs.pfnAddCommand (clientConst.szPlus_Right,IN_RightDown);
	gEngfuncs.pfnAddCommand (clientConst.szMinus_Right,IN_RightUp);
	gEngfuncs.pfnAddCommand (clientConst.szPlus_Forward,IN_ForwardDown);
	gEngfuncs.pfnAddCommand (clientConst.szMinus_Forward,IN_ForwardUp);
	gEngfuncs.pfnAddCommand (clientConst.szPlus_Back,IN_BackDown);
	gEngfuncs.pfnAddCommand (clientConst.szMinus_Back,IN_BackUp);
	// SaRcaZm - V5 - End

	gEngfuncs.pfnAddCommand ("+lookup", IN_LookupDown);
	gEngfuncs.pfnAddCommand ("-lookup", IN_LookupUp);
	gEngfuncs.pfnAddCommand ("+lookdown", IN_LookdownDown);
	gEngfuncs.pfnAddCommand ("-lookdown", IN_LookdownUp);
	gEngfuncs.pfnAddCommand ("+strafe", IN_StrafeDown);
	gEngfuncs.pfnAddCommand ("-strafe", IN_StrafeUp);
	gEngfuncs.pfnAddCommand ("+moveleft", IN_MoveleftDown);
	gEngfuncs.pfnAddCommand ("-moveleft", IN_MoveleftUp);
	gEngfuncs.pfnAddCommand ("+moveright", IN_MoverightDown);
	gEngfuncs.pfnAddCommand ("-moveright", IN_MoverightUp);
	gEngfuncs.pfnAddCommand ("+speed", IN_SpeedDown);
	gEngfuncs.pfnAddCommand ("-speed", IN_SpeedUp);
	gEngfuncs.pfnAddCommand ("+attack", IN_AttackDown);
	gEngfuncs.pfnAddCommand ("-attack", IN_AttackUp);
	gEngfuncs.pfnAddCommand ("+attack2", IN_Attack2Down);
	gEngfuncs.pfnAddCommand ("-attack2", IN_Attack2Up);
	gEngfuncs.pfnAddCommand ("+use", IN_UseDown);
	gEngfuncs.pfnAddCommand ("-use", IN_UseUp);
	gEngfuncs.pfnAddCommand ("+jump", IN_JumpDown);
	gEngfuncs.pfnAddCommand ("-jump", IN_JumpUp);
	gEngfuncs.pfnAddCommand ("impulse", IN_Impulse);
	gEngfuncs.pfnAddCommand ("+klook", IN_KLookDown);
	gEngfuncs.pfnAddCommand ("-klook", IN_KLookUp);
	gEngfuncs.pfnAddCommand ("+mlook", IN_MLookDown);
	gEngfuncs.pfnAddCommand ("-mlook", IN_MLookUp);
	gEngfuncs.pfnAddCommand ("+jlook", IN_JLookDown);
	gEngfuncs.pfnAddCommand ("-jlook", IN_JLookUp);
	gEngfuncs.pfnAddCommand ("+duck", IN_DuckDown);
	gEngfuncs.pfnAddCommand ("-duck", IN_DuckUp);
	gEngfuncs.pfnAddCommand ("+reload", IN_ReloadDown);
	gEngfuncs.pfnAddCommand ("-reload", IN_ReloadUp);
	gEngfuncs.pfnAddCommand ("+alt1", IN_Alt1Down);
	gEngfuncs.pfnAddCommand ("-alt1", IN_Alt1Up);
	gEngfuncs.pfnAddCommand ("+score", IN_ScoreDown);
	gEngfuncs.pfnAddCommand ("-score", IN_ScoreUp);
	gEngfuncs.pfnAddCommand ("+showscores", IN_ScoreDown);
	gEngfuncs.pfnAddCommand ("-showscores", IN_ScoreUp);
	gEngfuncs.pfnAddCommand ("+graph", IN_GraphDown);
	gEngfuncs.pfnAddCommand ("-graph", IN_GraphUp);
	gEngfuncs.pfnAddCommand ("+break",IN_BreakDown);
	gEngfuncs.pfnAddCommand ("-break",IN_BreakUp);

	// SaRcaZm - V6 - Start
	gEngfuncs.pfnAddCommand ("gearup", IN_GearUp);
	gEngfuncs.pfnAddCommand ("geardown", IN_GearDown);
	// SaRcaZm - V6 - End

	lookstrafe			= gEngfuncs.pfnRegisterVariable ( "lookstrafe", "0", FCVAR_ARCHIVE );
	lookspring			= gEngfuncs.pfnRegisterVariable ( "lookspring", "0", FCVAR_ARCHIVE );
	cl_anglespeedkey	= gEngfuncs.pfnRegisterVariable ( "cl_anglespeedkey", "0.67", 0 );

	// SaRcaZm - V5 - Start
	// Anti-Haxor Stuff
	//cl_yawspeed			= gEngfuncs.pfnRegisterVariable ( "cl_yawspeed", "210", 0 );
	cl_yawspeed = gEngfuncs.pfnRegisterVariable (clientConst.szCvar_Cl_YawSpeed, clientConst.szCvarValue_Cl_YawSpeed, 0);
	// SaRcaZm - V5 - End

	rally_rearview		= gEngfuncs.pfnRegisterVariable ( "rally_rearview", "1", FCVAR_ARCHIVE );

	cl_pitchspeed		= gEngfuncs.pfnRegisterVariable ( "cl_pitchspeed", "225", 0 );
	cl_upspeed			= gEngfuncs.pfnRegisterVariable ( "cl_upspeed", "320", 0 );

	// SaRcaZm - V5 - Start
	// Anti-Haxor Stuff
	cl_forwardspeed = gEngfuncs.pfnRegisterVariable (clientConst.szCvar_Cl_ForwardSpeed, clientConst.szCvarValue_Cl_ForwardSpeed, FCVAR_ARCHIVE);
	//cl_forwardspeed		= gEngfuncs.pfnRegisterVariable ( "cl_forwardspeed", "400", FCVAR_ARCHIVE );
	// SaRcaZm - V5 - End

	cl_backspeed		= gEngfuncs.pfnRegisterVariable ( "cl_backspeed", "400", FCVAR_ARCHIVE );
	cl_sidespeed		= gEngfuncs.pfnRegisterVariable ( "cl_sidespeed", "400", 0 );
	cl_movespeedkey		= gEngfuncs.pfnRegisterVariable ( "cl_movespeedkey", "0.3", 0 );
	cl_pitchup			= gEngfuncs.pfnRegisterVariable ( "cl_pitchup", "89", 0 );
	cl_pitchdown		= gEngfuncs.pfnRegisterVariable ( "cl_pitchdown", "89", 0 );

	cl_vsmoothing		= gEngfuncs.pfnRegisterVariable ( "cl_vsmoothing", "0.05", FCVAR_ARCHIVE );

	// SaRcaZm - V5 - Start
	// Anti-Haxor Stuff
	m_pitch = gEngfuncs.pfnRegisterVariable (clientConst.szCvar_M_Pitch, clientConst.szCvarValue_M_Pitch, FCVAR_ARCHIVE);
	m_yaw = gEngfuncs.pfnRegisterVariable (clientConst.szCvar_M_Yaw, clientConst.szCvarValue_M_Yaw, FCVAR_ARCHIVE);
	//m_pitch			    = gEngfuncs.pfnRegisterVariable ( "m_pitch","0.022", FCVAR_ARCHIVE );
	//m_yaw				= gEngfuncs.pfnRegisterVariable ( "m_yaw","0.022", FCVAR_ARCHIVE );
	// SaRcaZm - V5 - End

	m_forward			= gEngfuncs.pfnRegisterVariable ( "m_forward","1", FCVAR_ARCHIVE );
	m_side				= gEngfuncs.pfnRegisterVariable ( "m_side","0.8", FCVAR_ARCHIVE );

	// Creme: V9 start
	cl_steer_center_speed = gEngfuncs.pfnRegisterVariable ( "cl_steer_center_speed", "20", FCVAR_ARCHIVE );
	cl_steer_threshold	  = gEngfuncs.pfnRegisterVariable ( "cl_steer_threshold", "12", FCVAR_ARCHIVE );

	cl_steer_delta_mul = gEngfuncs.pfnRegisterVariable ( "cl_steer_delta_mul", "1.8", FCVAR_ARCHIVE );
	cl_steer_delta_dec = gEngfuncs.pfnRegisterVariable ( "cl_steer_delta_dec", "7", FCVAR_ARCHIVE );

	// Creme: V9 end

	// Initialize third person camera controls.
	CAM_Init();
	// Initialize inputs
	IN_Init();
	// Initialize keyboard
	KB_Init();
	// Initialize view system
	V_Init();
}


/*
============
ShutdownInput
============
*/
void ShutdownInput (void)
{
	IN_Shutdown();
	KB_Shutdown();
}

// SaRcaZm - V8 - Start
#include "vgui_int.h"
// SaRcaZm - V8 - End
void DLLEXPORT HUD_Shutdown( void )
{
	ShutdownInput();

	// SaRcaZm - V8 - Start
	RallySound->fmodShutDown();
	VGui_Shutdown ();
	delete RallySound;
	// SaRcaZm - V8 - End
}
