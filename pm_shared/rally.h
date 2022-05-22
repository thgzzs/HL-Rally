/***
*
*	Copyright (c) 2001, Half-Life Rally Team
*	
*	Rally.h by Michael "Crem" Danzo and FragMented
*
****/
//
// rally.h
//
#ifndef RALLYH
#define RALLYH
#pragma once

typedef struct rally_s { 

	int speed;
	int gear;
	int mousex;

} rally_t;

//extern rally_t g_rv;

void PM_PlayerAngles (void);
//void PM_MotorSound (struct playermove_s *pmove, float spd, float sliding);
void PM_MoveToDest ();
void PM_Accelerate (vec3_t wisdir, float wishspeed, float accel);
void PM_Crash(void);
int PM_FlyMove ();
void PM_DriveMove ();
void PM_Friction (void);


#endif