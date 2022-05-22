/*

rally.c

 Creme, michael_danzo@hotmail.com
 Sarcazm, many tweaks and updates
 Fragmented, tweaks


*/ 

#include <stdio.h>  // NULL
#include <math.h>
#include <assert.h>
#include <string.h> 
#include "mathlib.h"
#include "const.h"
#include "usercmd.h"
#include "pm_defs.h"
#include "pm_shared.h"
#include "pm_movevars.h"
#include "pm_debug.h"

#include "rally.h"

extern rally_t *g_rv;

#define D 0.0174532

#define PM_TRACEMETHOD PM_NORMAL


#define	PITCH	0
#define	YAW		1
#define	ROLL	2 

extern playermove_t *pmove;

#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )

//#define TN 1
#define D 0.0174532

#ifdef CLIENT_DLL
	#include "../cl_dll/fmod/api/inc/fmod.h"
#endif

float CL_CrashAmount;



// ======================================================================
// External functions from pm_shared.c
// ======================================================================
qboolean PM_AddToTouched(pmtrace_t tr, vec3_t impactvelocity);
int PM_ClipVelocity (vec3_t in, vec3_t normal, vec3_t out, float overbounce);
void InterpolateAngles( float *start, float *end, float *output, float frac );


void PlaySound (char *soundfile)
{
#ifdef CLIENT_DLL
/*	int channel1;
	char sz[256];
	FSOUND_SAMPLE samp1;
	sprintf( sz, "hlrally/%s", soundfile );
	//FSOUND_STREAM *stream;

	//stream = FSOUND_Stream_OpenFile(sz, FSOUND_LOOP_NORMAL, 64);
	//FSOUND_Stream_Play(5, stream);

	*samp1 = NULL;

	samp1 = FSOUND_Sample_Load(5, sz, FSOUND_LOOP_NORMAL, 0);
	channel1 = FSOUND_PlaySound(5, samp1);*/
#endif
}



// ======================================================================
//#define TN 1 // Debug Particle Lines
void TerrainTrace (struct playermove_s *pmove, float *n, float *vOfs, float *vUp, float *endpos) {

	//float       ang;
	vec3_t src_down, dest_down;
	pmtrace_t trace;

#ifdef TN 
	vec3_t tempvec;
#endif


	VectorScale(vUp, 4, src_down);
	VectorAdd(src_down, pmove->origin, src_down);
	VectorAdd(src_down, vOfs,          src_down);

	VectorScale(vUp, -16, dest_down);
	VectorAdd(dest_down, pmove->origin, dest_down);
	VectorAdd(dest_down, vOfs,          dest_down);

	trace = pmove->PM_PlayerTrace (src_down, dest_down, PM_TRACEMETHOD, -1 );
	VectorCopy (trace.plane.normal, n);

	VectorCopy (trace.endpos, endpos);



#ifdef TN
	VectorScale (trace.plane.normal, 32, tempvec);
	VectorAdd   (trace.endpos, tempvec, tempvec);
	PM_ParticleLine (trace.endpos, tempvec, 255, pmove->frametime*1.5, 0);
#endif


}


/*void VectorAngles( const vec3_t forward, vec3_t angles )
{
	float	tmp, yaw, pitch;
	
	if (forward[1] == 0 && forward[0] == 0)
	{
		yaw = 0;
		if (forward[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		yaw = (atan2(forward[1], forward[0]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;

		tmp = sqrt (forward[0]*forward[0] + forward[1]*forward[1]);
		pitch = (atan2(forward[2], tmp) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}
	
	angles[0] = pitch;
	angles[1] = yaw;
	angles[2] = 0;
}
*/

void PM_PlayerAngles (void) 
{ 
	float		ang;
	vec3_t		r, n;
	vec3_t		f1, f2, b1, b2, tAngles;
	vec3_t		df1,df2,db1,db2;
	vec3_t		du, dv, dn;
	vec3_t		vForward, vRight, vUp, vOfs; // vTemp, 

	// if on the ground, do the terrain travelling magic!
	if (pmove->onground != -1) {
		AngleVectors(pmove->vuser1, vForward, vRight, vUp);

		vOfs[0]=0; vOfs[1]=0; vOfs[2]=0;
//		TerrainTrace (pmove, c1, vOfs, vUp, dc1); //Centre
		
		VectorScale(vRight, 8, vRight);
		VectorScale(vForward, 12, vForward);

		// Creme: Ok theres some serious trickery Ive used here to optimise, 
		// but I'm fairly certain theres an even faster way to work all these out.

		VectorAdd(vRight, vForward, vOfs); // Front, Right
		TerrainTrace (pmove, f1, vOfs, vUp, df1);

		VectorInverse(vOfs);
		TerrainTrace (pmove, b2, vOfs, vUp, db2); // Back, Left

		VectorInverse(vRight);
		VectorAdd(vRight, vForward, vOfs);
		TerrainTrace (pmove, f2,  vOfs, vUp, df2); // Front, Left
			
		VectorInverse(vOfs);
		TerrainTrace (pmove, b1, vOfs, vUp, db1);// Back, Right

		// First triangle
		//df1, df2, db2    and du, dv for x-product, dn for normal

		VectorSubtract (db2, df1, du);
		VectorSubtract (db2, df2, dv);
		CrossProduct (du, dv, dn); 

		// Second triangle
		//db2, db1, df1    and du, dv for x-product, dn for normal

		VectorSubtract (df1, db2, du);
		VectorSubtract (df1, db1, dv);
		CrossProduct (du, dv, n);

		// Add normals together and average
		VectorAdd(dn, n, dn); // reuse dn
		VectorScale(dn, 0.5, n); // returns n our car angle!

		
		ang = pmove->angles[YAW] * D;

		// Do rotation to orientate terrain to model
		r[0] = (n[0] * cos(ang)) + (n[1] * sin(ang));
		r[1] = -(n[0]* sin(ang)) + (n[1] * cos(ang));
		r[2] = n[2];
		
		tAngles[PITCH] = (atan2(r[2], r[0]) * 180 / M_PI - 90) * -1;
		tAngles[ROLL]  = atan2(r[2], r[1]) * 180 / M_PI - 90;

		pmove->angles[0] = (tAngles[0]);// + (pmove->vuser1[0]*5)) / 6;
		pmove->angles[2] = (tAngles[2]);// + (pmove->vuser1[2]*5)) / 6;

		// Store the old angles
		VectorCopy (pmove->angles, pmove->vuser1);

		// Crem: if we are in trouble, try smoothly flattening the angle
		/*if (pmove->onground == -1) {
			if (pmove->vuser1[0] > 180) {
				pmove->vuser1[0] -= 180;
			}

			if (fabs(pmove->vuser1[0]) > 70) {
				pmove->vuser1[0] = 0;
			} else {
				tAngles[0] = pmove->vuser1[0] - (pmove->vuser1[0] * 0.25);
				pmove->vuser1[0] -= (tAngles[0] * pmove->frametime);
			}

			if (pmove->vuser1[2] > 180) {
				pmove->vuser1[2] -= 180;
			}

			if (fabs(pmove->vuser1[2]) > 70) {
				pmove->vuser1[2] = 0;
			} else {
				tAngles[2] = pmove->vuser1[2] - (pmove->vuser1[2] * 0.25);
				pmove->vuser1[2] -= (tAngles[2] * pmove->frametime);
			}

		}*/


	} else {

		// Flying, set to cat mode: pitch/yaw reset to horizontal.

//		pmove->angles[0] = pmove->vuser1[0];
//		pmove->angles[2] = pmove->vuser1[2];

		pmove->vuser1[0] = 0;
		pmove->vuser1[2] = 0;

	}

}

#include <memory.h>
// From mathlib.c
double VectorLength(vec3_t v)
{
	int		i;
	double	length;
	
	length = 0;
	for (i=0 ; i< 3 ; i++)
		length += v[i]*v[i];
	length = sqrt (length);		// FIXME

	return length;
}


#define TRACE_GAP				8
#define CarLength				45
#define CarWidth				18
#define CarHeight				16

// indexes- front:0, right:1, back:2, left:3, up:4, down:5;
#define s_front 0
#define s_right 1
#define s_back  2
#define s_left  3
#define s_top    4
#define s_bottom  5

#define inc_u 0
#define inc_v 1


//#define ShowDots 1

// Creme: 27/Dec/02 Re-wrote this routine from scratch to optimise.
float PM_CarWorldCollision (vec3_t origin, pmtrace_t *tr) {

	float mintrace = 1.0f;
	vec3_t vForward, vRight, vUp;
	vec3_t vForwardDist, vRightDist, vUpDist;

	// 3d offset co-ordinates
	int u, v;
	int i;
	
	// Center of mass, current trace dest
	vec3_t center, current_dest;

	// Sides to process 
	// indexes- front:0, right:1, back:2, left:3, top:4, bottom:5;
	qboolean process_side[6] = {true, true, true, true, false, false};
	int current_side;

	// Array of scanning start positions (each side/face)
	vec3_t scan_start[6];
	// Array of scan vectors which define directions to scan.
	vec3_t scan_direction[6][2];
	
	// CarLength, Width, etc
	float scan_length[6] = { 
		CarWidth*1.4, 
		CarLength-TRACE_GAP, 
		CarWidth*1.4, 
		CarLength-TRACE_GAP, 
		CarLength, 
		CarLength};

	vec3_t vTemp;
	vec3_t vStartHeight;


	// *** Commence some pre-calc stuff ***
	//
	// Work out the scan starts
	//

	AngleVectors (pmove->angles, vForward, vRight, vUp);

	VectorScale(vForward, CarLength / 2, vForwardDist);
	VectorScale(vRight,  -CarWidth / 2,  vRightDist);
	VectorScale(vUp,      CarHeight,     vUpDist);

	VectorScale(vUp, pmove->movevars->stepsize, vStartHeight);

	VectorAdd (vForwardDist, vRightDist, vTemp);
	VectorAdd (origin, vTemp, scan_start[s_front]);

	// Add stepsize height to get over bumps
	VectorAdd (scan_start[s_front], vStartHeight, scan_start[s_front]);

	VectorCopy(scan_start[s_front], scan_start[s_left]);
	//crem: todo fix me, this moves s_left to adjacent rear corner
	VectorSubtract(scan_start[s_left], vForwardDist, scan_start[s_left]);
	VectorSubtract(scan_start[s_left], vForwardDist, scan_start[s_left]);

	// Opposite corner
	VectorInverse (vTemp);
	VectorAdd (origin, vTemp, scan_start[s_back]);

	// Add stepsize height to get over bumps
	VectorAdd (scan_start[s_back], vStartHeight, scan_start[s_back]);

	// Back and right start at the same corner.
	VectorCopy(scan_start[s_back], scan_start[s_right]);

	//
	// Work out the increments
	//
	VectorScale(vForward, TRACE_GAP, vForwardDist);
	VectorScale(vRight,   TRACE_GAP*0.6, vRightDist);
	VectorScale(vUp,      TRACE_GAP, vUpDist);

	// Front and Right increments positively
	VectorCopy (vRightDist,   scan_direction[s_front][inc_u]);
	VectorCopy (vForwardDist, scan_direction[s_right][inc_u]);
	
	// Back and Left increments start backwards (negatively)
	VectorInverse (vRightDist);
	VectorCopy (vRightDist,   scan_direction[s_back][inc_u]);

//	VectorInverse (vForwardDist);
	VectorCopy (vForwardDist, scan_direction[s_left][inc_u]);
	
	// Set first 4 inc_v's to vUp * TraceGap;
	for (i = 0; i < 4; i++) {
		VectorCopy (vUpDist, scan_direction[i][inc_v]);
	}

	// Give left and right startpoint an increment, so that the 
	// same scan point isnt checked twice
	//
	VectorAdd (scan_start[s_left], 
		scan_direction[s_left][inc_u],
		scan_start[s_left]);
	VectorAdd (scan_start[s_right], 
		scan_direction[s_right][inc_u],
		scan_start[s_right]);

	// Add special offsets to the front and back to center to scan points
	VectorScale(vRight,   2, vRightDist);
	VectorAdd (scan_start[s_front], 
		vRightDist,
		scan_start[s_front]);

	VectorInverse(vRightDist);
	VectorAdd (scan_start[s_back], 
		vRightDist,
		scan_start[s_back]);

	// TODO, top and bottom faces


	// Set centre of mass
	VectorCopy (origin, center);
	VectorAdd  (center, vUpDist, center);


	// Work out which sides to cull based on velocity direction
	// Theres no point tracing away from the movement..
	if (Length(pmove->velocity) > 10) {

		if (DotProduct(vForward, pmove->velocity) < 0) {
			process_side[s_front] = false;
		} else {
			process_side[s_back] = false;
		}

		if (DotProduct(vRight, pmove->velocity) < 0) {
			process_side[s_right] = false;
		} else {
			process_side[s_left] = false;
		}

	}



// Lets start tracing!
// 
// Creme: the all important Psuedo-Code
//
// for each side { 
//   if this side is to be processed {
//     set the scan height
//       scan along that row
//         if we hit something, check for mintrace
//       repeat until next row
//	   }
//	 }
// }

//	pmove->usehull = 3;

	for (current_side = 0; current_side < 6; current_side++) {
		if (process_side[current_side]) {

			vec3_t vec_v = { 0,0,0 };

			for (v = pmove->movevars->stepsize; v < CarHeight; v += TRACE_GAP) {

				VectorCopy (scan_start[current_side], current_dest);
				VectorAdd (current_dest, vec_v, current_dest);
				
				// Increment the current v for the next scan-line
				VectorAdd (vec_v, scan_direction[current_side][inc_v], vec_v);

				for (u = 0; u < scan_length[current_side]; u += TRACE_GAP) {

					pmtrace_t trace;

// For debugging
#if defined( ShowDots )
					pmove->PM_Particle(current_dest, 256, pmove->frametime*2, 0, 0);
#endif
					
					trace = pmove->PM_PlayerTrace (center, current_dest, PM_TRACEMETHOD, -1 );

					// Check to see if this is a new minimum
					if (trace.fraction < mintrace)
					{
						memcpy (tr, &trace, sizeof (pmtrace_t));	// Copy it to the return structure
						// Store the attempted position for later
						VectorCopy(current_dest, tr->deltavelocity);
						mintrace = trace.fraction;

					}

					// Update position for next trace
					VectorAdd (current_dest, 
						scan_direction[current_side][inc_u], 
						current_dest);

				}

			}

		}

	}

	return mintrace;

}


void PM_DriveMove ()
{
	int i;

	vec3_t		wishvel;
	float       spd;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;

	// Copy movement amounts
	fmove = pmove->cmd.forwardmove;
	smove = pmove->cmd.sidemove;
	
	VectorNormalize (pmove->forward);  // Normalize remainder of vectors.
	VectorNormalize (pmove->right);

	for (i=0 ; i<3 ; i++)       // Determine x and y parts of velocity
		wishvel[i] = pmove->forward[i]*fmove + pmove->right[i]*smove;
	
//	wishvel[2] = 0;             // Zero out z part of velocity

	VectorCopy (wishvel, wishdir);   // Determine maginitude of speed of move
	wishspeed = VectorNormalize(wishdir);

	// Clamp to server defined max speed
	if (wishspeed > pmove->maxspeed)
	{
		VectorScale (wishvel, pmove->maxspeed/wishspeed, wishvel);
		wishspeed = pmove->maxspeed;
	}

	// Set pmove velocity
	PM_Accelerate (wishdir, wishspeed, pmove->movevars->accelerate);

	// Add in any base velocity to the current velocity.
	VectorAdd (pmove->velocity, pmove->basevelocity, pmove->velocity );

	spd = Length( pmove->velocity );

	if (spd < 1.0f)
	{
//		VectorClear( pmove->velocity );
		return;
	}

	PM_MoveToDest ();
}

void PM_MoveToDest ()
{
	int			clip, i;
	int			oldonground;
	vec3_t dest, start;//, dir;
	vec3_t original, originalvel;
	vec3_t down, downvel;
	float downdist, updist;//, dist;
	pmtrace_t trace;

	vec3_t vUp, vNDummy; // Used for stepsize


	oldonground = pmove->onground;

	dest[0] = pmove->origin[0] + pmove->velocity[0]*pmove->frametime;
	dest[1] = pmove->origin[1] + pmove->velocity[1]*pmove->frametime;	
	dest[2] = pmove->origin[2] + pmove->velocity[2]*pmove->frametime; 


	// first try moving directly to the next spot
	VectorCopy (dest, start);
	trace = pmove->PM_PlayerTrace (pmove->origin, dest, PM_TRACEMETHOD, -1 );


	// If we made it all the way, then copy trace end as new player position.
	if (trace.fraction == 1)
	{
		VectorCopy (trace.endpos, pmove->origin);
		return;
	}

	if (oldonground == -1 && pmove->waterlevel == 0)
		return;

	// Try sliding forward both on ground and up 16 pixels take the move that goes farthest
	VectorCopy (pmove->origin, original);       // Save our original pos &
	VectorCopy (pmove->velocity, originalvel);  //  velocity.

	// Slide move
	clip = PM_FlyMove ();

	// Copy the results out
	VectorCopy (pmove->origin, down);
	VectorCopy (pmove->velocity, downvel);


// ************ ************ ************ ************ *********** 
//***** ************ ************ ************ ************ *******

//   ** Commence Stage 2 of FlyMode check

//***** ************ ************ ************ ************ *******
// ************ ************ ************ ************ ***********

	
	// Restore original values.
	VectorCopy (original, pmove->origin);
	VectorCopy (originalvel, pmove->velocity);

	// Start out up one stair height
	VectorCopy (pmove->origin, dest);

// Another 2D hack by valve
//	dest[2] += pmove->movevars->stepsize; // another 

	AngleVectors(pmove->vuser1, vNDummy, vNDummy, vUp);
	
	for (i=0 ; i<3 ; i++) {
		dest[i] = dest[i] + (vUp[i] * pmove->movevars->stepsize);
	}

	trace = pmove->PM_PlayerTrace (pmove->origin, dest, PM_TRACEMETHOD, -1 );

	//  run another move try.
	if (!trace.startsolid && !trace.allsolid)
	{
		VectorCopy (trace.endpos, pmove->origin);
	}

	// Try moving from a Stepped-Up initial position
	clip = PM_FlyMove ();

	// Now try going back down from the end point.. press down the stepheight
	VectorCopy (pmove->origin, dest);

// Another 2D hack by valve
//	dest[2] -= pmove->movevars->stepsize;

	for (i=0 ; i<3 ; i++) {
		dest[i] = dest[i] + (vUp[i] * -pmove->movevars->stepsize);
	}
	
	trace = pmove->PM_PlayerTrace (pmove->origin, dest, PM_TRACEMETHOD, -1 );

	// If we are not on the ground any more then
	//  use the original movement attempt
	//if ( trace.plane.normal[2] < 0.7)
	//	goto usedown;
	// If the trace ended up in empty space, copy the end
	//  over to the origin.
	if (!trace.startsolid && !trace.allsolid)
	{
		VectorCopy (trace.endpos, pmove->origin);
	}
	// Copy this origion to up.
	VectorCopy (pmove->origin, pmove->up);  // Creme - this is a nasty hack you know.. 
											// i wanted to use the pmove->up, and wondered 
											// why it wasnt keeping its value...sigh valve

	// decide which one went farther
	downdist = (down[0] - original[0])*(down[0] - original[0])
			 + (down[1] - original[1])*(down[1] - original[1]);
	updist   = (pmove->up[0]   - original[0])*(pmove->up[0]   - original[0])
			 + (pmove->up[1]   - original[1])*(pmove->up[1]   - original[1]);

	if (downdist > updist)
	{
//usedown:
		VectorCopy (down   , pmove->origin);
		VectorCopy (downvel, pmove->velocity);
	} else // copy z value from slide move
		pmove->velocity[2] = downvel[2]; 

}


void PM_Crash(void)
{

	int i;
	pmtrace_t trace;
	vec3_t penetrate_vec;
	vec3_t origin;

	// Figure out the % of how far we can get there without 
	// hitting anything at all. CollisionTrace returns value 0 to 1;
	CL_CrashAmount = 0;

	origin[0] = pmove->origin[0] + pmove->velocity[0]*pmove->frametime;
	origin[1] = pmove->origin[1] + pmove->velocity[1]*pmove->frametime;	
	origin[2] = pmove->origin[2] + pmove->velocity[2]*pmove->frametime; // It wasnt adding this before

	if (PM_CarWorldCollision (origin, &trace) != 1.0)
	{
		vec3_t pene_dest, OldVelocity, t1, t2;
		float backoff = DotProduct (pmove->velocity, trace.plane.normal);

		vec3_t center;
		vec3_t vUptemp;
		vec3_t vNDummy, vUp;

		//CL_CrashAmount = backoff;
		// Creme: 8-Apr-2004
		// Dont play crashamount(sound) stuff if its ground (fix me eventually)
		//pmove->Con_DPrintf("normal[2]: %f \n", trace.plane.normal[2]);
		if (trace.plane.normal[2] < 0.3)
		{
			CL_CrashAmount = backoff;
		}

//		pmove->Con_DPrintf("      backoff: %f; velocity: %f; dotproduct: %f; Server: %i\n", backoff, Length(pmove->velocity), (backoff / Length(pmove->velocity)), pmove->server);

		AngleVectors(pmove->angles, vNDummy, vNDummy, vUp);
		VectorScale(vUp, TRACE_GAP, vUptemp);

		// Creme: this is largely another ugly hack, 
		// to stop 'car floating' when wheels cant reach ground
		// basically: if we are 'flying' and we hit a poly that is ground, 
		// then ignore car - collision with this poly.
//		if ((pmove->onground == -1) && (DotProduct(trace.plane.normal, vUp) > 0.75)) {
		if ((pmove->onground == -1) && (trace.plane.normal[2] > 0.3)) {
			//pmove->onground = trace.ent;
//			pmove->Con_DPrintf("      IGNORING WORLD COLLISION: DotProduct : %f;\n", DotProduct(trace.plane.normal, vUp) );
//			//penetrate_vec[2] = -2;
		} else {

			VectorCopy(pmove->velocity, OldVelocity);
			// Slide along the wall
			for (i=0 ; i<3 ; i++) {

				float change = trace.plane.normal[i] * backoff * 1.2; // Multiply for bounce!
				pmove->velocity[i] -= change;

			}

			VectorCopy(pmove->velocity, t1);
			VectorNormalize(t1);
			VectorCopy(OldVelocity, t2);
			VectorNormalize(t2);

			// Work out the new difference
			backoff = DotProduct(t1, t2);

			// Creme: Move penetration calculation out to here, so its
			// only calculated once on the resulting shortest trace.

			VectorCopy (pmove->origin, center);
			VectorAdd  (center, vUptemp, center);

			for (i = 0; i < 3; i++) {
				penetrate_vec[i] = trace.endpos[i] - trace.deltavelocity[i];
			}

			for (i=0 ; i<3 ; i++) {
				pmove->velocity[i] = ((pmove->velocity[i] * backoff) + (pmove->velocity[i] * 2)) / 3;

				// Creme this is thrown in here to save a 'for i<3;' loop ;)
				pene_dest[i] = pmove->origin[i] + penetrate_vec[i];

			}

			// Trace to check that the new penetrated backoff location is in the world
			trace = pmove->PM_PlayerTrace (pmove->origin, pene_dest, PM_TRACEMETHOD, -1);
			
			// Copy it anyway
			VectorCopy (trace.endpos, pmove->origin);

		} // if onground
	
	} // if Colliding != 1.0

}

/*
============
PM_FlyMove

The basic solid body movement clip that slides along multiple planes
============
*/
int PM_FlyMove (void)
{
	int			bumpcount, numbumps;
	vec3_t		dir;
	float		d;
	int			numplanes;
	vec3_t		planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity, original_velocity;
	vec3_t      new_velocity;
	int			i, j;
	pmtrace_t	trace;
	vec3_t		end;
	float		time_left, allFraction;
	int			blocked;
		
	numbumps  = 4;           // Bump up to four times
	
	blocked   = 0;           // Assume not blocked
	numplanes = 0;           //  and not sliding along any planes
	VectorCopy (pmove->velocity, original_velocity);  // Store original velocity
	VectorCopy (pmove->velocity, primal_velocity);
	
	allFraction = 0;
	time_left = pmove->frametime;   // Total time for this movement operation.

	for (bumpcount=0 ; bumpcount<numbumps ; bumpcount++)
	{
		if (!pmove->velocity[0] && !pmove->velocity[1] && !pmove->velocity[2])
			break;

		// Assume we can move all the way from the current origin to the
		//  end point.
		for (i=0 ; i<3 ; i++)
			end[i] = pmove->origin[i] + time_left * pmove->velocity[i];

		// See if we can make it from origin to end point.
		trace = pmove->PM_PlayerTrace (pmove->origin, end, PM_TRACEMETHOD, -1 );

		allFraction += trace.fraction;
		// If we started in a solid object, or we were in solid space
		//  the whole way, zero out our velocity and return that we
		//  are blocked by floor and wall.
		if (trace.allsolid)
		{	// entity is trapped in another solid
			VectorCopy (vec3_origin, pmove->velocity);
			//Con_DPrintf("Trapped 4\n");
			return 4;
		}

		// If we moved some portion of the total distance, then
		//  copy the end position into the pmove->origin and 
		//  zero the plane counter.
		if (trace.fraction > 0)
		{	// actually covered some distance
			VectorCopy (trace.endpos, pmove->origin);
			VectorCopy (pmove->velocity, original_velocity);
			numplanes = 0;
		}

		// If we covered the entire distance, we are done
		//  and can return.
		if (trace.fraction == 1)
			 break;		// moved the entire distance

		// Save entity that blocked us (since fraction was < 1.0)
		//  for contact
		// Add it if it's not already in the list!!!
		PM_AddToTouched(trace, pmove->velocity);

		// If the plane we hit has a high z component in the normal, then
		//  it's probably a floor
/* Creme: this blocked rubbish baffles me.. im disabling it everywhere
		if (trace.plane.normal[2] > 0.7)
		{
			blocked |= 1;		// floor
		}
		// If the plane has a zero z component in the normal, then it's a 
		//  step or wall
		if (!trace.plane.normal[2])
		{
			blocked |= 2;		// step / wall
			//Con_DPrintf("Blocked by %i\n", trace.ent);
		}
*/
		// Reduce amount of pmove->frametime left by total time left * fraction
		//  that we covered.
		time_left -= time_left * trace.fraction;
		
		// Did we run out of planes to clip against?
		if (numplanes >= MAX_CLIP_PLANES)
		{	// this shouldn't really happen
			//  Stop our movement if so.
			VectorCopy (vec3_origin, pmove->velocity);
			//Con_DPrintf("Too many planes 4\n");

			break;
		}

		// Set up next clipping plane
		VectorCopy (trace.plane.normal, planes[numplanes]);
		numplanes++;

		// modify original_velocity so it parallels all of the clip planes
		if ( pmove->movetype == MOVETYPE_WALK &&
			((pmove->onground == -1) || (pmove->friction != 1)) )	// reflect player velocity
		{
			for ( i = 0; i < numplanes; i++ )
			{
				if ( planes[i][2] > 0.9  )  // was 0.7
				{// floor or slope
					PM_ClipVelocity( original_velocity, planes[i], new_velocity, 1 );
					VectorCopy( new_velocity, original_velocity );
				}
				else															
					PM_ClipVelocity( original_velocity, planes[i], new_velocity, 1.1 );
//					PM_ClipVelocity( original_velocity, planes[i], new_velocity, 1.0 + pmove->movevars->bounce * (1-pmove->friction) );
			}

			VectorCopy( new_velocity, pmove->velocity );
			VectorCopy( new_velocity, original_velocity );
		}
		else
		{
			for (i=0 ; i<numplanes ; i++)
			{
				PM_ClipVelocity (
					original_velocity,
					planes[i],
					pmove->velocity,
					1.1);
				for (j=0 ; j<numplanes ; j++)
					if (j != i)
					{
						// Are we now moving against this plane?
						if (DotProduct (pmove->velocity, planes[j]) < 0)
							break;	// not ok
					}
				if (j == numplanes)  // Didn't have to clip, so we're ok
					break;
			}
			
			// Did we go all the way through plane set
			if (i != numplanes)
			{	// go along this plane
				// pmove->velocity is set in clipping call, no need to set again.
				;  
			}
			else
			{	// go along the crease
				if (numplanes != 2)
				{
					//Con_Printf ("clip velocity, numplanes == %i\n",numplanes);
					VectorCopy (vec3_origin, pmove->velocity);
					//Con_DPrintf("Trapped 4\n");

					break;
				}
				CrossProduct (planes[0], planes[1], dir);
				d = DotProduct (dir, pmove->velocity);
				VectorScale (dir, d, pmove->velocity );
			}

			// if original velocity is against the original velocity, stop dead
			// to avoid tiny occilations in sloping corners
			if (DotProduct (pmove->velocity, primal_velocity) <= 0)
			{
				//pmove->Con_DPrintf("Back\n");
				VectorCopy (vec3_origin, pmove->velocity);
				break;
			}
		}
	}

	if ( allFraction == 0 )
	{
		VectorCopy (vec3_origin, pmove->velocity);
	}

	return blocked;
}




/*
==============
PM_Accelerate
Creme: 29-May-2002 moved PM_Accelerate to rally.c
==============
*/
void PM_Accelerate (vec3_t wishdir, float wishspeed, float accel)
{
	int			i;
	float		addspeed, accelspeed, currentspeed;

	// Dead player's don't accelerate
	if (pmove->dead)
		return;

	// If waterjumping, don't accelerate
	if (pmove->waterjumptime)
		return;

	// See if we are changing direction a bit
	currentspeed = DotProduct (pmove->velocity, wishdir);

	// Reduce wishspeed by the amount of veer.
	addspeed = wishspeed - currentspeed;

	// If not going to add any speed, done.
	if (addspeed <= 0)
		return;

	// Determine amount of accleration.
	accelspeed = accel * pmove->frametime * wishspeed * pmove->friction;

	// Cap at addspeed
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	if (pmove->cmd.forwardmove < 0) {
		if (Length(pmove->velocity) > 100) {
			accelspeed = 0;
		}
	}

	
		// Adjust velocity.
	for (i=0 ; i<3 ; i++)
	{
		pmove->velocity[i] += accelspeed * wishdir[i];	
	}
}

/*
==================
PM_Friction

Handles both ground friction and water friction

Creme: 18-May-2002 moved PM_Friction to rally.c
==================
*/
void PM_Friction (void)
{

	float	speed;
	float	latfric;
	float LAT_MAX;
	float LATERAL_FRICTION;
	float sliding_ofs;
	int i;
	// Amount that surface type reduces speed
	float surface_drag = 0;
	vec3_t velocity_normal;

	LAT_MAX = pmove->movevars->friction * 300;
	
	speed = Length(pmove->velocity);

	LATERAL_FRICTION = (LAT_MAX * (1 / (speed / 30)));
	sliding_ofs = DotProduct(pmove->velocity, pmove->forward) / speed;


	//if (speed > 5.0f) {

		// Creme: if player is driving in reverse, give them some friction
		if (pmove->cmd.forwardmove < 0) {
			sliding_ofs = fabs(sliding_ofs);
		}

		// Error check the Dot Product (sliding_ofs) 
		// Incase car is going in reverse or programmer brain failure.

		if (sliding_ofs < 0.05) { 
			sliding_ofs = 0.05f; 
		} else if (sliding_ofs > 1) {
			sliding_ofs = 1;
		}

		// Increase friction if braking
		if (pmove->cmd.buttons & IN_BACK) {

			LATERAL_FRICTION *= 0.3f;

		}

		// Destroy friction if handbraking (uses jump key)
		if (pmove->cmd.buttons & IN_JUMP) {

			LATERAL_FRICTION *= 0.05f;

		}

		// Decrease friction if accelerating
		if (pmove->cmd.buttons & IN_FORWARD) {

			LATERAL_FRICTION *= 0.85f;

		}
		
		// Decrease lateral friction if accelerating and turning
		if ((pmove->cmd.buttons & IN_FORWARD) && 
			(pmove->cmd.buttons & IN_LEFT ||
			pmove->cmd.buttons & IN_RIGHT)) {

			LATERAL_FRICTION *= 0.8 * sliding_ofs;

		}

		// Reduce Lateral friction if sliding!
		LATERAL_FRICTION *= (sliding_ofs * sliding_ofs * sliding_ofs);

	//}

	// SaRcaZm - V4 - Start
	// Customise the lateral friction to give better / worse steering
	LATERAL_FRICTION *= fabs(pmove->vuser4[1] - 40) / 50.0f;
	// SaRcaZm - V4 - End


	//	pmove->Con_DPrintf(" Lateral Friction : %4.2f; Sliding_ofs : %4.2f;\n", LATERAL_FRICTION, sliding_ofs);

	if (pmove->sztexturename) {

		if (strstr(pmove->sztexturename, "dirt") || strstr(pmove->sztexturename, "mud")) {

			LATERAL_FRICTION *= 0.5;
			surface_drag += 0.04;

		} else if (strstr(pmove->sztexturename, "gras")) {

			LATERAL_FRICTION *= 0.35;
			surface_drag += 0.075;


		} else if(strstr(pmove->sztexturename, "sand") || strstr(pmove->sztexturename, "desert")) {


			LATERAL_FRICTION *= 0.4;
			surface_drag += 0.05;

		} else if(strstr(pmove->sztexturename, "snow")) {

			LATERAL_FRICTION *= 0.315;
			surface_drag += 0.05;

		} 

		if (strstr(pmove->sztexturename, "road") || strstr(pmove->sztexturename, "street") || strstr(pmove->sztexturename, "rock")) {
			//LATERAL_FRICTION *= 1.2;
			surface_drag += -0.1;
		}

	}

	if (LATERAL_FRICTION > LAT_MAX) { LATERAL_FRICTION = LAT_MAX; }

//	pmove->Con_DPrintf(" Lateral Friction : %4.2f; Sliding_ofs : %4.2f;\n", LATERAL_FRICTION, sliding_ofs);

	// If too slow, return
/*	if (speed < 0.1f)
		return;*/

	VectorNormalize ( pmove->forward ); 
	VectorNormalize ( pmove->right );

	VectorCopy(pmove->velocity, velocity_normal);
	VectorNormalize(velocity_normal);

	for (i=0 ; i < 3 ; i++) {
		if (pmove->onground != -1)  
			pmove->velocity[i] -= velocity_normal[i] * pmove->movevars->friction * speed * pmove->frametime;
	}

	speed = Length(pmove->velocity);

	latfric = DotProduct(pmove->velocity, pmove->right);// / Length(pmove->velocity);

	for (i=0 ; i<3 ; i++) {
		// Remove lateral friction, by slowing down velocity in the lateral direction
		pmove->velocity[i] -= pmove->right[i] * latfric * LATERAL_FRICTION * pmove->frametime;
	}

	speed = speed - Length(pmove->velocity); // extra

	for (i=0 ; i<3 ; i++) {
		pmove->velocity[i] += pmove->forward[i] * speed;
		// Slow down if going sideways!
		pmove->velocity[i] -= (pmove->velocity[i] * (-sliding_ofs+1) * pmove->frametime) * 0.25; 
		pmove->velocity[i] -= (pmove->velocity[i] * surface_drag) * pmove->frametime; 
	}

	VectorCopy (pmove->velocity, velocity_normal);
	VectorNormalize (velocity_normal);

	// Creme: Slow her down a tad if braking or handbraking
	if (pmove->cmd.forwardmove >= 0) {
		if ((pmove->cmd.buttons & IN_BACK) || (pmove->cmd.buttons & IN_JUMP)) {

			for (i=0 ; i<3 ; i++) {
				pmove->velocity[i] -= (velocity_normal[i] * 150) * sliding_ofs * pmove->frametime;

			}
		}
	}
}