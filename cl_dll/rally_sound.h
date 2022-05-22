// Rally MP3 Header

// SaRcaZm - V7
// Consider most of this file altered somehow
#ifndef __Rally_Sound_h__
#define __Rally_Sound_h__

#include "rally_fmod.h"

// SaRcaZm - V8 - Start
typedef struct remote_sound_info_s
{
	int rpms;
	vec3_t pos, vel;
	float dist;	// Could technically work this out, but easier to store it
} remote_sound_info_t;

class CRallySound {
public:
	void fmodInit();
	void fmodPlay (char *sound, bool loop, int pitch);
	void fmodPlay (char *sound, bool loop, int pitch, float vol);
	void fmodEngineStart(int channel, char *model);
	void fmodEngineChangePitch (int rpm, int playernum, int accelerating, vec3_t pos, vec3_t vel, bool isLocal);

	void fmodStartEngine (void);
	void fmodStopEngine (void);

	//void fmodStop (char *sound);
	void fmodShutDown(void);

	void fmodFrame (vec3_t position, vec3_t velocity, vec3_t angles);
	void fmodPlay3D (char *sound, bool loop, int pitch, vec3_t pos, vec3_t vel);
	void fmodPlay3D (char *sound, bool loop, int pitch, float vol, vec3_t pos, vec3_t vel);
	void fmodAddRemoteEngine (int rpm, vec3_t pos, vec3_t vel);
	void fmodPlayRemoteEngines (void);
	void fmodStopRemoteEngines (void);
	void fmodStartWhine (void);

	// Mp3 functions
	void fmodPlayMp3 (char *file);
	void fmodStopMp3 (void);
	void fmodPauseMp3 (void);
	void fmodSetMp3Balance (int bal);
	void fmodSetMp3Volume (int vol);
	void fmodSetMp3Position (int milliseconds);

	bool fmodIsPlayingMp3 (void);
	bool fmodIsInternetRadioPlaying (void);
	bool fmodIsOpenMp3 (void);
	bool fmodIsStereoMp3 (void);
	bool fmodIsPausedMp3 (void);
	char *fmodGetMp3Tag (char *tagname);

	int fmodGetMp3Length (void);
	int fmodGetMp3Position (void);
	int fmodGetMp3Kbps (void);
	int fmodGetMp3Frequency (void);

private:

	int IsSoundLoaded (char *sound);
	FSOUND_SAMPLE *LoadSound (char *sound, unsigned int mode);

	cvar_t *hw3d, *sound_channels;
	float f3dSoundMaxDist;
	int i3dSoundNumAlloc, i3dSoundNumChannels;
	remote_sound_info_t t3dSoundInfo[MAX_PLAYERS];

	int iRemoteSoundPlayed[MAX_PLAYERS];
	int m_iMP3Volume, m_iMP3Balance;
};
// SaRcaZm - V8 - End

extern CRallySound *RallySound;
#endif	// __Rally_Sound_h__
