#include "hud.h"
#include "cl_util.h"
#include "camera.h"
#include "kbutton.h"
#include "cvardef.h"
#include "usercmd.h"
#include "const.h"
#include "camera.h"
#include "in_defs.h"
#include "cl_entity.h"

#include "windows.h"

float CL_KeyState (kbutton_t *key);

#define M_PI			3.14159265358979323846

// Crem pm
#include "pm_defs.h"
#include "pm_debug.h"
#include "pmtrace.h"

#include "entity_types.h"
// these are included for the math functions
#include "com_model.h"
#include "studio_util.h"
#include "hud_spectator.h"

// SaRcaZm - V7 - Start
#include "rally_vguifx.h"

extern int g_iUser1;
extern int g_iUser2;
// SaRcaZm - V7 - End

extern "C" 
{
	void DLLEXPORT CAM_Think( void );
	int DLLEXPORT CL_IsThirdPerson( void );
	void DLLEXPORT CL_CameraOffset( float *ofs );

	extern playermove_t *pmove;
	void PM_ParticleLine( float *start, float *end, int pcolor, float life, float vert);
}
extern "C" void Global_DLLEXPORT_Info( float time, const char *func_name );


extern cl_enginefunc_t gEngfuncs;
extern vec3_t realvieworg;
vec3_t cam_origin;
vec3_t cam_angles;

extern float g_lastFOV;

//-------------------------------------------------- Constants

#define CAM_DIST_DELTA 1.0
#define CAM_ANGLE_DELTA 2.5
#define CAM_ANGLE_SPEED 2.5
#define CAM_MIN_DIST 30.0
#define CAM_ANGLE_MOVE .5
#define MAX_ANGLE_DIFF 10.0
#define PITCH_MAX 90.0
#define PITCH_MIN 0
#define YAW_MAX  135.0
#define YAW_MIN	 -135.0

enum ECAM_Command
{
	CAM_COMMAND_NONE = 0,
	CAM_COMMAND_TOTHIRDPERSON = 1,
	CAM_COMMAND_TOFIRSTPERSON = 2
};

//-------------------------------------------------- Global Variables

cvar_t	*cam_command;
cvar_t	*cam_snapto;
cvar_t	*cam_idealyaw;
cvar_t	*cam_idealpitch;
cvar_t	*cam_idealdist;
cvar_t	*cam_wishdist;  // thirdperson wished distance
cvar_t	*cam_wishyaw;   // thirdperson wished yaw (for panning)
cvar_t	*cam_wishpitch; // thirdperson wished pitch (for slope adjustment)
cvar_t	*cam_contain;
cvar_t	*cam_viewmode;  // thirdperson view mode, tv, chase etc

cvar_t	*c_maxpitch;
cvar_t	*c_minpitch;
cvar_t	*c_maxyaw;
cvar_t	*c_minyaw;
cvar_t	*c_maxdistance;
cvar_t	*c_mindistance;

// pitch, yaw, dist
vec3_t cam_ofs;


float cam_originx, cam_originy, cam_originz;
float cam_anglesx, cam_anglesy, cam_anglesz;

// In third person
int cam_thirdperson;
int cam_mousemove; //true if we are moving the cam with the mouse, False if not
int iMouseInUse=0;
int cam_distancemove;
extern int mouse_x, mouse_y;  //used to determine what the current x and y values are
int cam_old_mouse_x, cam_old_mouse_y; //holds the last ticks mouse movement
POINT		cam_mouse;
//-------------------------------------------------- Local Variables

static kbutton_t cam_pitchup, cam_pitchdown, cam_yawleft, cam_yawright;
static kbutton_t cam_in, cam_out, cam_move;

//-------------------------------------------------- Prototypes

void CAM_ToThirdPerson(void);
void CAM_ToFirstPerson(void);
void CAM_StartDistance(void);
void CAM_EndDistance(void);
// SaRcaZm - V7 - Start
void CAM_TV_Mode (vec3_t targetorg);
void CAM_VGUI (void);
void CAM_Load_TV (void);
void CAM_Load_Spec (void);
vec3_t vgui_origin;
vec3_t vgui_angles;

int old_iuser1 = 0;
// SaRcaZm - V7 - End


//-------------------------------------------------- Local Functions

float MoveToward( float cur, float goal, float maxspeed )
{
	if( cur != goal )
	{
		if( abs( cur - goal ) > 180.0 )
		{
			if( cur < goal )
				cur += 360.0;
			else
				cur -= 360.0;
		}

		if( cur < goal )
		{
			if( cur < goal - 1.0 )
				cur += ( goal - cur ) / 4.0;
			else
				cur = goal;
		}
		else
		{
			if( cur > goal + 1.0 )
				cur -= ( cur - goal ) / 4.0;
			else
				cur = goal;
		}
	}


	// bring cur back into range
	if( cur < 0 )
		cur += 360.0;
	else if( cur >= 360 )
		cur -= 360;

	return cur;
}

void VectorAngles( const vec3_t forward, float *angles )
{
	float	tmp, yaw, pitch;
	
	if (forward[1] == 0 && forward[0] == 0)
	{
		yaw = 0;
		if (forward[2] > 0)
			pitch = 90;
		else
			pitch = -90;
	}
	else
	{
		yaw = (atan2(forward[1], forward[0]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;

		tmp = sqrt (forward[0]*forward[0] + forward[1]*forward[1]);
		pitch = (atan2(forward[2], tmp) * 180 / M_PI);
	}
	
	angles[0] = pitch;
	angles[1] = yaw;
	angles[2] = 0;
}

//-------------------------------------------------- Gobal Functions

typedef struct
{
	vec3_t		boxmins, boxmaxs;// enclose the test object along entire move
	float		*mins, *maxs;	// size of the moving object
	vec3_t		mins2, maxs2;	// size when clipping against mosnters
	float		*start, *end;
	trace_t		trace;
	int			type;
	edict_t		*passedict;
	qboolean	monsterclip;
} moveclip_t;

extern trace_t SV_ClipMoveToEntity (edict_t *ent, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end);

void DLLEXPORT CAM_Think( void )
{
	vec3_t origin;
	vec3_t ext, pnt, pntoriginal, camForward, camRight, camUp, cam_origin, tempvec, tempAngles;
	moveclip_t	clip;
	float dist;
	vec3_t camAngles;
	float flSensitivity, flTemp;
//	int i, sv;
	vec3_t viewangles;
	cl_entity_t pEnt;
	pmtrace_t trace, trace_down, trace_left, trace_right;
	float lowest_trace;
	vec3_t porigin;

	Global_DLLEXPORT_Info(gEngfuncs.GetClientTime(), "CAM_Think");

	switch( (int) cam_command->value )
	{
		case CAM_COMMAND_TOTHIRDPERSON:
			CAM_ToThirdPerson();
			break;

		case CAM_COMMAND_TOFIRSTPERSON:
			CAM_ToFirstPerson();
			break;

		case CAM_COMMAND_NONE:
		default:
			break;
	}

	// SaRcaZm - V8 - Start
	// Sanity check the cam_viewmode
	if ((cam_viewmode->value < 0) || (cam_viewmode->value > 3))
		cam_viewmode->value = 0;
	// SaRcaZm - V8 - End

	// SaRcaZm - V7 - Start
	static char szCamMap[80];
	if (stricmp (gEngfuncs.pfnGetLevelName(), szCamMap))
	{
		// Kick them into normal 3rd person mode on map change
		CAM_ToThirdPerson ();
		cam_viewmode->value = 0;
		g_iUser1 = 0;
		strcpy (szCamMap, gEngfuncs.pfnGetLevelName());
	}

	// Don't move the camera whilst in the VGUI
	if ((CRallyVGUIFX::getSingleton()->GetMode () == MODE_VGUI) || (CRallyVGUIFX::getSingleton()->GetMode () == MODE_SHOWROOM))
	{
		realvieworg = vgui_origin;
		cam_anglesx = vgui_angles[PITCH];
		cam_anglesy = vgui_angles[YAW];
		cam_anglesz = 0;
		g_lastFOV = 90;
		return;
	}

	if (g_iUser1)
	{
		// Save off our camera values so we can restore them for the next level
		if (g_iUser1 != old_iuser1)
		{
			extern float cam_prevmode;
			extern bool pers_thirdperson;
			pers_thirdperson = (cam_thirdperson != 0);
			cam_prevmode = CVAR_GET_FLOAT ("cam_viewmode");
		}

		CAM_ToThirdPerson ();
		cam_viewmode->value = 1;
	}
	old_iuser1 = g_iUser1;

	if (!cam_thirdperson || cam_viewmode->value >= 2)
	{
		gEngfuncs.GetViewAngles( (float *)viewangles );
		cam_anglesx = viewangles[YAW];
		cam_anglesy = viewangles[PITCH];
		return;
	}
	// SaRcaZm - V7 - End

// Crem
//	gEngfuncs.GetViewAngles( (float *)viewangles );

	//pEnt = gEngfuncs.GetLocalPlayer;


	camAngles[ PITCH ] = cam_idealpitch->value;
	camAngles[ YAW ] = cam_idealyaw->value;
	dist = cam_idealdist->value;

	//
	// Movement of the Camera with the mouse
	//
    // Get windows cursor position
	if (cam_mousemove) {
	  GetCursorPos (&cam_mouse);

	  // Don't do any movement of the cam using YAW/PITCH if we are zooming in/out the camera	
	  if (!cam_distancemove) {
		
		// Keep the camera within certain limits around the player 
		// (ie avoid certain bad viewing angles)  
		if (cam_mouse.x > gEngfuncs.GetWindowCenterX()) {
			if (camAngles[YAW] < c_maxyaw->value)	{

				flTemp = (CAM_ANGLE_MOVE) *
					     ((cam_mouse.x - gEngfuncs.GetWindowCenterX()) / 2) / 4;

				camAngles[YAW]     += flTemp;
				cam_wishyaw->value += flTemp;

			}
			if (camAngles[YAW] > c_maxyaw->value)	{
				camAngles[YAW] = c_maxyaw->value;
			}
		} else if (cam_mouse.x < gEngfuncs.GetWindowCenterX()) {
			if (camAngles[YAW] > c_minyaw->value)	{

				flTemp = (CAM_ANGLE_MOVE) *
				         ((gEngfuncs.GetWindowCenterX() - cam_mouse.x) / 2) / 4;

				camAngles[YAW]     -= flTemp;
				cam_wishyaw->value -= flTemp;

			}
			if (camAngles[YAW] < c_minyaw->value) {
				camAngles[YAW] = c_minyaw->value;
			}
		}

		if (cam_mouse.y > gEngfuncs.GetWindowCenterY()) {
			if(camAngles[PITCH] < c_maxpitch->value) {
				flTemp = (CAM_ANGLE_MOVE) * 
					                ((cam_mouse.y - gEngfuncs.GetWindowCenterY()) / 2) / 4;

				camAngles[PITCH]	+= flTemp;
				cam_wishpitch->value += flTemp;

			}
			if (camAngles[PITCH] > c_maxpitch->value)	{
				camAngles[PITCH] = c_maxpitch->value;
			}
		} else if (cam_mouse.y < gEngfuncs.GetWindowCenterY()) {
			if (camAngles[PITCH] > c_minpitch->value) {
				flTemp = (CAM_ANGLE_MOVE) *
				                   ((gEngfuncs.GetWindowCenterY() - cam_mouse.y) / 2) / 4;

				camAngles[PITCH]	-= flTemp;
				cam_wishpitch->value -= flTemp;

			}
			if (camAngles[PITCH] < c_minpitch->value) {
				camAngles[PITCH] = c_minpitch->value;
			}
		}

		// Set old mouse coordinates to current mouse coordinates
		// Since we are done with the mouse

		if ((flSensitivity = gHUD.GetSensitivity()) != 0) {
			cam_old_mouse_x = cam_mouse.x * flSensitivity;
			cam_old_mouse_y = cam_mouse.y * flSensitivity;
		} else {
			cam_old_mouse_x = cam_mouse.x;
			cam_old_mouse_y = cam_mouse.y;
		}
		SetCursorPos (gEngfuncs.GetWindowCenterX(), gEngfuncs.GetWindowCenterY());
	  }
	}

	if (cam_wishyaw->value > 360) {
		cam_wishyaw->value -= 360;
	} else if (cam_wishyaw->value < -360) {
		cam_wishyaw->value += 360;
	}


	//Nathan code here
	if (CL_KeyState(&cam_pitchup)) {

		camAngles[PITCH] += CAM_ANGLE_DELTA;

	} else if (CL_KeyState(&cam_pitchdown)) {

		camAngles[PITCH] -= CAM_ANGLE_DELTA;

	}
	if (CL_KeyState(&cam_yawleft)) {

		camAngles[YAW] -= CAM_ANGLE_DELTA;
		cam_wishyaw->value -= CAM_ANGLE_DELTA;

	} else if (CL_KeyState(&cam_yawright)) {

		camAngles[YAW] += CAM_ANGLE_DELTA;
		cam_wishyaw->value += CAM_ANGLE_DELTA;

	}

	if( CL_KeyState( &cam_in ) ) {
		dist -= CAM_DIST_DELTA;
		if( dist < CAM_MIN_DIST ) {
			// If we go back into first person, reset the angle
			camAngles[ PITCH ] = 0;
			camAngles[ YAW ] = 0;
			dist = CAM_MIN_DIST;
		}
	} else if( CL_KeyState( &cam_out ) ) {
		dist += CAM_DIST_DELTA;
	}

	if (cam_distancemove) {
		if (cam_mouse.y>gEngfuncs.GetWindowCenterY()) {
			if (dist < c_maxdistance->value) {
			    dist += CAM_DIST_DELTA * ((cam_mouse.y - gEngfuncs.GetWindowCenterY()) / 2);
			}
			if (dist > c_maxdistance->value) {
				dist = c_maxdistance->value;
			}
		} else if (cam_mouse.y<gEngfuncs.GetWindowCenterY()) {
			if (dist > c_mindistance->value) {
			    dist -= (CAM_DIST_DELTA)*((gEngfuncs.GetWindowCenterY() - cam_mouse.y) / 2);
			}
			if (dist < c_mindistance->value) {
				dist = c_mindistance->value;
			}
		}
		//set old mouse coordinates to current mouse coordinates
		//since we are done with the mouse

		cam_wishdist->value = dist;
		cam_old_mouse_x=cam_mouse.x*gHUD.GetSensitivity();
		cam_old_mouse_y=cam_mouse.y*gHUD.GetSensitivity();
		SetCursorPos (gEngfuncs.GetWindowCenterX(), gEngfuncs.GetWindowCenterY());
	}

	
	VectorCopy (pmove->velocity, tempvec); tempvec[2] = 0;

	if (Length(tempvec) > 5) {
		VectorAngles(pmove->velocity, tempAngles);
	} else {
		gEngfuncs.GetViewAngles( (float *)viewangles );
		VectorCopy(viewangles, tempAngles);
	}

//		gEngfuncs.Con_DPrintf ("x: %.2f; y: %.2f; z: %.2f;\n", tempAngles[0], tempAngles[1], tempAngles[2]);
		camAngles[YAW]  = tempAngles[YAW];
//		camAngles[YAW]  = atan2(pmove->velocity[1], pmove->velocity[0]) * 180 / M_PI;
		camAngles[PITCH]= ((-tempAngles[PITCH] + cam_wishpitch->value) + (cam_idealpitch->value * 8)) / 9;
	

//    gEngfuncs.Con_DPrintf ("Length: %.2f;", dist);

  //  gEngfuncs.Con_DPrintf ("Pitch: %.2f;\n", pmove->angles[0]);

	// SaRcaZm - V7 - Start
		if ((CRallyVGUIFX::getSingleton()->GetMode() == MODE_VGUI) || (CRallyVGUIFX::getSingleton()->GetMode() == MODE_SHOWROOM))
	{
		camAngles[YAW] = 0;
	}
	// SaRcaZm - V7 - End

	// update ideal
	cam_idealpitch->value = camAngles[ PITCH ];
	cam_idealyaw->value = camAngles[ YAW ];
	cam_idealdist->value = dist;

	// Crem - cam_contain routine practically re-written from scratch
	if( cam_contain->value ) {

		tempvec[0] = cam_idealpitch->value;
		tempvec[1] = cam_idealyaw->value + cam_wishyaw->value;
		tempvec[2] = 0;

		AngleVectors (tempvec, camForward, camRight, camUp);

		// Point for traceline routine - pnt

		pnt[0] = pmove->origin[0] - (camForward[0] * cam_wishdist->value);
		pnt[1] = pmove->origin[1] - (camForward[1] * cam_wishdist->value);
		pnt[2] = pmove->origin[2] - (camForward[2] * cam_wishdist->value);

		VectorCopy (pnt, pntoriginal);
		// Crem: needs 16 coz you still go thru the ceiling a bit
		VectorAdd(pntoriginal, camUp * 16, pnt);

		// Crem: origin is on the floor under the car, so come up a bit
		VectorCopy( pmove->origin, porigin );
		porigin[2] += 8;


		trace = pmove->PM_PlayerTrace (porigin, pnt, PM_NORMAL, -1 );

		if (trace.startsolid || trace.allsolid ) {

			cam_idealdist->value = cam_ofs[2];
			//gEngfuncs.Con_DPrintf ("Warning: (Camera) hit invalid plane or out of bounds\n");
			return;

		} else {

			// Crem: if we got this far lets check if the cam clips downwards too
			
			VectorCopy (pntoriginal, pnt);
	
//			VectorSubtract(pntoriginal, camUp, pnt);  // Crem: floor only needs -2
			trace_down = pmove->PM_PlayerTrace (porigin, pnt, PM_NORMAL, -1 );

			if (trace.fraction < trace_down.fraction) {
				lowest_trace = trace.fraction;
			} else {
				lowest_trace = trace_down.fraction;
			}

			if( lowest_trace < 1.0 ) {

				cam_idealdist->value = (cam_wishdist->value * lowest_trace) - 8;

				if (cam_idealdist->value < c_mindistance->value) {
					cam_idealdist->value = c_mindistance->value;
				} else 
				if (cam_idealdist->value > c_maxdistance->value) {
					cam_idealdist->value = c_maxdistance->value;
				}

			} else {

				cam_idealdist->value = cam_wishdist->value;
			}

		}


//		Crem: more debug shit
/*		VectorSubtract (porigin, trace.endpos, pnt);
	    gEngfuncs.Con_DPrintf (" Traced: %.2f; Wish: %.2f; Ideal: %.2f\n", Length(pnt), cam_wishdist->value, cam_idealdist->value);
/*
		pnt[0] = pmove->origin[0] - ((pcosy - (psinx * pcosy)) * cam_idealdist->value);
		pnt[1] = pmove->origin[1] - ((psiny - (psinx * psiny)) * cam_idealdist->value);
		pnt[2] = pmove->origin[2] + (psinx * cam_idealdist->value);
		PM_ParticleLine((float *)&pmove->origin, (float *)&pnt, 60, 1.0, 0.0 );
	    gEngfuncs.Con_DPrintf ("Trace: %.2f, Dist %.2f\n", trace.fraction, cam_idealdist->value);
*/


	}

	// Smoothen move towards ideal
	// ==================
	
	// camAngles = cam_ofs;   // Values of Angles/dist from previous frame
	VectorCopy( cam_ofs, camAngles );

//	gEngfuncs.GetViewAngles( (float *)viewangles );

	// Smoothen distance alterations
	if( abs(camAngles[2] - cam_idealdist->value) < 0.5) {
		camAngles[2] = cam_idealdist->value;
	} else {
		if (cam_idealdist->value < camAngles[2]) { // if its less move in quicker
			camAngles[2] = ((cam_ofs[2] * 5) + (cam_idealdist->value)) / 6;
		} else {
			camAngles[2] = ((cam_ofs[2] * 29) + (cam_idealdist->value)) / 30;
		}
	}

	// Snappy Tom, or Smooth operator? 
	if( cam_snapto->value )	{
		camAngles[ YAW ] = cam_idealyaw->value + cam_wishyaw->value;
		camAngles[ PITCH ] = cam_idealpitch->value; 
//		camAngles[ 2 ] = cam_idealdist->value;
	} else {
		if( camAngles[ YAW ] != cam_idealyaw->value + cam_wishyaw->value )
			camAngles[ YAW ] = MoveToward( camAngles[ YAW ], cam_idealyaw->value + cam_wishyaw->value, CAM_ANGLE_SPEED );

		if( camAngles[ PITCH ] != cam_idealpitch->value )
			camAngles[ PITCH ] = MoveToward( camAngles[ PITCH ], cam_idealpitch->value, CAM_ANGLE_SPEED );

	}

// Le Big Update
//=======================================================================================
//

/*	tempvec[0] = camAngles[PITCH];
	tempvec[1] = camAngles[YAW];
	tempvec[2] = 0;

	AngleVectors(tempvec, camForward, camRight, camUp);

	cam_originx = cam_origin[0] = pmove->origin[0] - (camForward[0] * camAngles[2]);
	cam_originy = cam_origin[1] = pmove->origin[1] - (camForward[1] * camAngles[2]);
	cam_originz = cam_origin[2] = pmove->origin[2] - (camForward[2] * camAngles[2]);
*/
//	PM_ParticleLine((float *)&pmove->origin, (float *)&cam_origin, 5, 0.025, 0.0 );

//=======================================================================================
//
	
	vec3_t n;
	VectorCopy (pmove->velocity, n);
	n[2] = 0;
	float spd = Length(n);

	spd /= 120;

	if(spd > 2)
		spd = 2;

	camAngles[2] += spd;


	cam_anglesx = cam_ofs[ 0 ] = camAngles[ 0 ];
	cam_anglesy = cam_ofs[ 1 ] = camAngles[ 1 ];
	              cam_ofs[ 2 ] = camAngles[ 2 ];
	cam_anglesz = 0;

	extern float g_time;

//	gEngfuncs.Con_DPrintf ("             gHUD.m_flTime: %.2f; \n", gHUD.m_flTime);
	
//	if ((g_time > 10000) && // on changelevel...
	// SaRcaZm - V7 - Start
    if(  (cam_viewmode->value == 1) || g_iUser1) {
		vec3_t tv_targ;
		if (g_iUser1)
		{
			cl_entity_t *pl = gEngfuncs.GetEntityByIndex (g_iUser2);
			if (pl)
				tv_targ = pl->origin;
		}
		else
			tv_targ = pmove->origin;

		CAM_TV_Mode(tv_targ);

		vec3_t tv_vect;

		tv_vect[0] = tv_targ[0] - realvieworg[0];
		tv_vect[1] = tv_targ[1] - realvieworg[1];
		tv_vect[2] = tv_targ[2] - realvieworg[2];
		// SaRcaZm - V7 - End

		cam_ofs[YAW] = atan2(tv_vect[1], tv_vect[0]) * 180 / M_PI;

		float tmp;
		tmp = sqrt (tv_vect[0]*tv_vect[0] + tv_vect[1]*tv_vect[1]);
		cam_ofs[PITCH] = -(atan2(tv_vect[2], tmp) * 180 / M_PI);

		float tLength = Length(tv_vect);
		if ((!tLength) || (tLength < 1)) { tLength = 1; }

/*		if (tLength < 100) {
			g_lastFOV = (tLength / -100) + 90;  // 90 to 89
		} else if (tLength < 500) {
			g_lastFOV = (tLength / -16) + 95.25;  // 89 to 64
		} else {
			g_lastFOV = (tLength / -10) + 114;  // 64 to ...
		}*/

		float distance1 = 50;
		float distance2 = 250;
		float distance3 = 850;

		float fov1 = 105;
		float fov2 = 20;
		float fov3 = 11;
		float fov4 = 10;

		float TransitionDistance = 200;

		if (tLength < distance1) {
			g_lastFOV = fov1;
		} else if (tLength < (distance1 + TransitionDistance)) {
			g_lastFOV = fov1 - ((tLength - distance1) * ((fov1-fov2)/TransitionDistance));

		} else if (tLength < distance2) {
			g_lastFOV = fov2;
		} else if (tLength < (distance2 + TransitionDistance)) {   
			g_lastFOV = fov2 - ((tLength - distance2) * ((fov2-fov3)/TransitionDistance));

		} else if (tLength < distance3) {
			g_lastFOV = fov3;
		} else if (tLength < (distance3 + TransitionDistance)) {
			g_lastFOV = fov3 - ((tLength - distance3) * ((fov3-fov4)/TransitionDistance));

		} else  {
			g_lastFOV = fov4;
		}

		if (g_lastFOV > 100) { g_lastFOV = 100; }
		if (g_lastFOV < 2.5)  { g_lastFOV = 2.5; }
//	    gEngfuncs.Con_DPrintf ("             FOV: %.2f; Distance: %.2f\n", g_lastFOV, tLength);
	}
}

// SaRcaZm - V7 - Start
#define MAX_CAMS 150
vec3_t cam_origins[MAX_CAMS];
int iNumCams;

void CAM_Load_TV (void)
{
	iNumCams = 0;
	float position[3] = {0,0,0};
	cl_entity_t * pEnt = gEngfuncs.GetEntityByIndex( 0 );	// get world model

	if ( pEnt && pEnt->model )
	{
		char			*data;
		char			keyname[256];
		int				n;
		char			token[1024];

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
				
				if (token[0] == '}')
					return;
				
				if (!strcmp(keyname,"classname"))
				{
					if (!strcmp(token,"info_camera"))
					{
						// origin should already be in SpectatorPos
						cam_origins[iNumCams++] = position;
					}
				};
				
				if (!strcmp(keyname,"origin"))
				{
					UTIL_StringToVector(position, token);

				};
				
			} // while (1)

			if (strlen(data) < 2) { 
				break;
			}
		}
	}
}

void CAM_Load_Spec (void)
{
	float position[3] = {0,0,0};
	float angles[3] = {0, 0, 0};
	cl_entity_t * pEnt = gEngfuncs.GetEntityByIndex( 0 );	// get world model

	vgui_origin = Vector (0, 0, 0);
	vgui_angles = Vector (0, 0, 0);

	if ( pEnt )
	{
		char			*data;
		char			keyname[256];
		int				n;
		char			token[1024];
		char szClassName[3][30] = { "info_vgui_start", "info_player_start", "info_player_deathmatch" };

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
					
					if (token[0] == '}')
						return;
					
					if (!stricmp(keyname,"classname"))
					{
						if (!stricmp(token,szClassName[i]))
						{
							// origin should already be in SpectatorPos
							vgui_origin = position;
							if (i != 0)		// This isn't an info_vgui_start
								vgui_origin[2] += 80;
							vgui_angles = angles;
							return;
						}
					}

					if (!stricmp(keyname,"origin"))
					{
						UTIL_StringToVector(position, token);
					}

					if (!stricmp(keyname,"angles"))
					{
						UTIL_StringToVector(angles, token);
					}
				} // while (1)

				if (strlen(data) < 2) { 
					break;
				}
			}
		}
	}
}

void CAM_TV_Mode (vec3_t origin)
{
	static char szCamMap[80];
	float position[3] = {0,0,0};
	float closest = 999999;

	if (stricmp (gEngfuncs.pfnGetLevelName(), szCamMap))
	{
		CAM_Load_TV ();
		strcpy (szCamMap, gEngfuncs.pfnGetLevelName());
	}

	realvieworg[0] = 0;
	realvieworg[1] = 0;
	realvieworg[2] = 100;

	for (int i = 0; i < iNumCams; i++)
	{
		vec3_t v_camdist = origin - cam_origins[i];

		if (Length(v_camdist) < closest)
		{
			realvieworg = cam_origins[i];
			closest = Length (v_camdist);
		}
	}
}
// SaRcaZm - V7 - End

extern void KeyDown (kbutton_t *b);	// HACK
extern void KeyUp (kbutton_t *b);	// HACK

void CAM_PitchUpDown(void) { KeyDown( &cam_pitchup ); }
void CAM_PitchUpUp(void) { KeyUp( &cam_pitchup ); }
void CAM_PitchDownDown(void) { KeyDown( &cam_pitchdown ); }
void CAM_PitchDownUp(void) { KeyUp( &cam_pitchdown ); }
void CAM_YawLeftDown(void) { KeyDown( &cam_yawleft ); }
void CAM_YawLeftUp(void) { KeyUp( &cam_yawleft ); }
void CAM_YawRightDown(void) { KeyDown( &cam_yawright ); }
void CAM_YawRightUp(void) { KeyUp( &cam_yawright ); }
void CAM_InDown(void) { KeyDown( &cam_in ); }
void CAM_InUp(void) { KeyUp( &cam_in ); }
void CAM_OutDown(void) { KeyDown( &cam_out ); }
void CAM_OutUp(void) { KeyUp( &cam_out ); }

void CAM_ToThirdPerson(void)
{ 
	vec3_t viewangles;

// SaRcaZm - V3 - Start
// We want to do this all the time
/*	#if !defined( _DEBUG )
	if ( gEngfuncs.GetMaxClients() > 1 )
	{
		// no thirdperson in multiplayer.
		return;
	}
#endif	*/
// SaRcaZm - V3 - End

	// SaRcaZm - V6 - Start
	g_lastFOV = 90;	// Update from Frag
	// SaRcaZm - V6 - End

	gEngfuncs.GetViewAngles( (float *)viewangles );

	if( !cam_thirdperson )
	{
		cam_thirdperson = 1; 
		
		cam_ofs[ YAW ] = viewangles[ YAW ]; 
		cam_ofs[ PITCH ] = viewangles[ PITCH ]; 
		cam_ofs[ 2 ] = CAM_MIN_DIST; 
	}

	gEngfuncs.Cvar_SetValue( "cam_command", 0 );
}

// SaRcaZm - V7 - Start
void CAM_VGUI (void)
{
	static char szVGUIMap[80];
	if (stricmp (gEngfuncs.pfnGetLevelName(), szVGUIMap))
	{
		CAM_Load_Spec ();
		strcpy (szVGUIMap, gEngfuncs.pfnGetLevelName());
	}

	gEngfuncs.Cvar_SetValue( "cam_command", 0 );
}
// SaRcaZm - V7 - End

void CAM_ToFirstPerson(void) 
{ 
	// SaRcaZm - V6 - Start
	g_lastFOV = 90;	// Update from Frag
	// SaRcaZm - V6 - End

	cam_thirdperson = 0;
	
	gEngfuncs.Cvar_SetValue( "cam_command", 0 );
}

void CAM_ToggleSnapto( void )
{ 
	cam_snapto->value = !cam_snapto->value;
}

void CAM_Init( void )
{
	gEngfuncs.pfnAddCommand( "+campitchup", CAM_PitchUpDown );
	gEngfuncs.pfnAddCommand( "-campitchup", CAM_PitchUpUp );
	gEngfuncs.pfnAddCommand( "+campitchdown", CAM_PitchDownDown );
	gEngfuncs.pfnAddCommand( "-campitchdown", CAM_PitchDownUp );
	gEngfuncs.pfnAddCommand( "+camyawleft", CAM_YawLeftDown );
	gEngfuncs.pfnAddCommand( "-camyawleft", CAM_YawLeftUp );
	gEngfuncs.pfnAddCommand( "+camyawright", CAM_YawRightDown );
	gEngfuncs.pfnAddCommand( "-camyawright", CAM_YawRightUp );
	gEngfuncs.pfnAddCommand( "+camin", CAM_InDown );
	gEngfuncs.pfnAddCommand( "-camin", CAM_InUp );
	gEngfuncs.pfnAddCommand( "+camout", CAM_OutDown );
	gEngfuncs.pfnAddCommand( "-camout", CAM_OutUp );
	gEngfuncs.pfnAddCommand( "thirdperson", CAM_ToThirdPerson );
	gEngfuncs.pfnAddCommand( "firstperson", CAM_ToFirstPerson );
	gEngfuncs.pfnAddCommand( "+cammousemove",CAM_StartMouseMove);
	gEngfuncs.pfnAddCommand( "-cammousemove",CAM_EndMouseMove);
	gEngfuncs.pfnAddCommand( "+camdistance", CAM_StartDistance );
	gEngfuncs.pfnAddCommand( "-camdistance", CAM_EndDistance );
	gEngfuncs.pfnAddCommand( "snapto", CAM_ToggleSnapto );

	cam_command				= gEngfuncs.pfnRegisterVariable ( "cam_command", "0", 0 );	 // tells camera to go to thirdperson
	cam_snapto				= gEngfuncs.pfnRegisterVariable ( "cam_snapto", "0", 0 );	 // snap to thirdperson view
	cam_idealyaw			= gEngfuncs.pfnRegisterVariable ( "cam_idealyaw", "90", 0 ); // thirdperson yaw
	cam_idealpitch			= gEngfuncs.pfnRegisterVariable ( "cam_idealpitch", "20", 0 ); // thirperson pitch
	cam_idealdist			= gEngfuncs.pfnRegisterVariable ( "cam_idealdist", "64", 0 ); // thirdperson distance
	cam_wishdist			= gEngfuncs.pfnRegisterVariable ( "cam_wishdist", "64", 0 ); // thirdperson wished distance
	cam_wishyaw				= gEngfuncs.pfnRegisterVariable ( "cam_wishyaw", "0", 0 );	 // thirdperson wished yaw (for panning)
	cam_wishpitch			= gEngfuncs.pfnRegisterVariable ( "cam_wishpitch", "20", 0 ); // thirdperson wished pitch (for slopes etc)
	cam_contain				= gEngfuncs.pfnRegisterVariable ( "cam_contain", "0", 0 );	// contain camera to world
	cam_viewmode			= gEngfuncs.pfnRegisterVariable ( "cam_viewmode", "0", 0 ); // thirdperson view mode, tv, chase etc

	c_maxpitch				= gEngfuncs.pfnRegisterVariable ( "c_maxpitch", "90.0", 0 );
	c_minpitch				= gEngfuncs.pfnRegisterVariable ( "c_minpitch", "0.0", 0 );
	c_maxyaw				= gEngfuncs.pfnRegisterVariable ( "c_maxyaw",   "135.0", 0 );
	c_minyaw				= gEngfuncs.pfnRegisterVariable ( "c_minyaw",   "-135.0", 0 );
	c_maxdistance			= gEngfuncs.pfnRegisterVariable ( "c_maxdistance",   "200.0", 0 );
	c_mindistance			= gEngfuncs.pfnRegisterVariable ( "c_mindistance",   "30.0", 0 );

	cam_viewmode->value = 0;

}

void CAM_ClearStates( void )
{
	vec3_t viewangles;

	gEngfuncs.GetViewAngles( (float *)viewangles );

	cam_pitchup.state = 0;
	cam_pitchdown.state = 0;
	cam_yawleft.state = 0;
	cam_yawright.state = 0;
	cam_in.state = 0;
	cam_out.state = 0;

	cam_thirdperson = 0;
	cam_command->value = 0;
	cam_mousemove=0;

	cam_snapto->value = 0;
	cam_distancemove = 0;

	cam_ofs[ 0 ] = 0.0;
	cam_ofs[ 1 ] = 0.0;
	cam_ofs[ 2 ] = CAM_MIN_DIST;

	cam_idealpitch->value = viewangles[ PITCH ];
	cam_idealyaw->value = viewangles[ YAW ];
	cam_idealdist->value = CAM_MIN_DIST;
}

void CAM_StartMouseMove(void)
{
	float flSensitivity;
		
	//only move the cam with mouse if we are in third person.
	if (cam_thirdperson)
	{
		//set appropriate flags and initialize the old mouse position
		//variables for mouse camera movement
		if (!cam_mousemove)
		{
			cam_mousemove=1;
			iMouseInUse=1;
			GetCursorPos (&cam_mouse);

			if ( ( flSensitivity = gHUD.GetSensitivity() ) != 0 )
			{
				cam_old_mouse_x=cam_mouse.x*flSensitivity;
				cam_old_mouse_y=cam_mouse.y*flSensitivity;
			}
			else
			{
				cam_old_mouse_x=cam_mouse.x;
				cam_old_mouse_y=cam_mouse.y;
			}
		}
	}
	//we are not in 3rd person view..therefore do not allow camera movement
	else
	{   
		cam_mousemove=0;
		iMouseInUse=0;
	}
}

//the key has been released for camera movement
//tell the engine that mouse camera movement is off
void CAM_EndMouseMove(void)
{
   cam_mousemove=0;
   iMouseInUse=0;
}


//----------------------------------------------------------
//routines to start the process of moving the cam in or out 
//using the mouse
//----------------------------------------------------------
void CAM_StartDistance(void)
{
	//only move the cam with mouse if we are in third person.
	if (cam_thirdperson)
	{
	  //set appropriate flags and initialize the old mouse position
	  //variables for mouse camera movement
	  if (!cam_distancemove)
	  {
		  cam_distancemove=1;
		  cam_mousemove=1;
		  iMouseInUse=1;
		  GetCursorPos (&cam_mouse);
		  cam_old_mouse_x=cam_mouse.x*gHUD.GetSensitivity();
		  cam_old_mouse_y=cam_mouse.y*gHUD.GetSensitivity();
	  }
	}
	//we are not in 3rd person view..therefore do not allow camera movement
	else
	{   
		cam_distancemove=0;
		cam_mousemove=0;
		iMouseInUse=0;
	}
}

//the key has been released for camera movement
//tell the engine that mouse camera movement is off
void CAM_EndDistance(void)
{
   cam_distancemove=0;
   cam_mousemove=0;
   iMouseInUse=0;
}

int DLLEXPORT CL_IsThirdPerson( void )
{
	return cam_thirdperson ? 1 : 0;
}

void DLLEXPORT CL_CameraOffset( float *ofs )
{
	Global_DLLEXPORT_Info(gEngfuncs.GetClientTime(), "CL_CameraOffset");
	VectorCopy( cam_ofs, ofs );
}