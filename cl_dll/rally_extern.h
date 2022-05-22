//
// rally_extern.h
//
// Contains references to common external variables
//

#if !defined ( RALLY_EXTERN_H )
#define RALLY_EXTERN_H
#if defined( _WIN32 )
#pragma once
#endif

// Creme (non laggy yaw hack, extended from frags stuff ;)
extern float RallyYaw;

// Creme: more stuff from the tri.cpp (hudneedle), not sure if all of this is used now.
extern float currentspeed;
extern float cam_originx, cam_originy, cam_originz;
extern float cam_anglesx, cam_anglesy, cam_anglesz;

extern vec3_t realvieworg;
extern vec3_t cam_origin;
extern cvar_t *cam_viewmode;

float oldrpms = 0;
float rpm_delta = 0;
float rpms;

// SaRcaZm - V6 - Start
extern int gear;
// SaRcaZm - V6 - End

extern float g_accelerating;
extern float g_braking;
extern float g_reversing;
extern float g_handbraking;

// Creme HACK HACK HACK!
extern float JoyMoved;

#endif	// RALLY_EXTERN_H
