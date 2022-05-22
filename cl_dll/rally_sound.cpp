// Rally_Sound.cpp
// Hopefully will play some sound!

// SaRcaZm - V8 - Start
// Consider most of this file altered somehow

#include "hud.h"
#include "cl_util.h"
#include "vgui_TeamFortressViewport.h"
#include "rally_fmod.h"
#include "rally_sound.h"
#include "rally_effects.h"
#include "com_model.h"

#define MAX_SOUNDS		32
#define FMOD_HL_SCALE	100

#define VOL_MULT		200

typedef struct rally_sound_s {
	char szName[80];
	FSOUND_SAMPLE *stream;
} rally_sound_t;
rally_sound_t rally_sounds[MAX_SOUNDS];
int iSndIndex = 0;
static int iWhineChannel = -1;

typedef struct rally_channel_s {
	char szModel[80];
	int iChannel;
	FSOUND_STREAM *stream;
} rally_channel_t;
rally_channel_t local_channel;
rally_channel_t remote_channels[MAX_PLAYERS];
rally_channel_t mp3;

CFModFuncs *m_cFMod;

void CRallySound::fmodInit()
{
	m_cFMod = new CFModFuncs ();
	if (!m_cFMod->Init ())
	{
		ConsolePrint ("Failed to load FMOD\n");
		m_cFMod = NULL;
		return;
	}

	m_cFMod->FSOUND_SetMinHardwareChannels (MAX_PLAYERS);	// 1 hardware channel for each player
	m_cFMod->FSOUND_SetOutput(FSOUND_OUTPUT_DSOUND);
	m_cFMod->FSOUND_SetBufferSize(50);
	m_cFMod->FSOUND_Stream_Net_SetBufferProperties(64000, 60, 80);
	m_cFMod->FSOUND_SetDriver(0);
	m_cFMod->FSOUND_Init(44100, 16, 0);						// 16 Software channels

	// This does nothing!
	//m_cFMod->FSOUND_3D_SetDistanceFactor (FMOD_HL_SCALE);	// HL units / metre
	//m_cFMod->FSOUND_3D_SetDopplerFactor (5);					// Exaggerate the doppler effect
	//m_cFMod->FSOUND_3D_SetRolloffFactor (0.5);				// Sounds carry 2x further

	for(int chan = 0; chan <= m_cFMod->FSOUND_GetMaxChannels(); chan++)
	{
		m_cFMod->FSOUND_SetVolume(chan, int(CVAR_GET_FLOAT("volume")*VOL_MULT));
	}
	//m_cFMod->FSOUND_SetSFXMasterVolume (int(CVAR_GET_FLOAT("volume")*VOL_MULT));

	// Init the snd channels array
	local_channel.iChannel = -1;
	local_channel.stream = NULL;
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		remote_channels[i].iChannel = -1;
		remote_channels[i].stream = NULL;

		iRemoteSoundPlayed[i] = 0;
	}
	mp3.iChannel = -1;
	mp3.stream = NULL;

	hw3d = gEngfuncs.pfnRegisterVariable( "rally_sound_hw3d", "1", FCVAR_ARCHIVE );
	sound_channels = gEngfuncs.pfnRegisterVariable( "rally_sound_3dchannels", "4", FCVAR_ARCHIVE );

	m_iMP3Volume = 255;
	m_iMP3Balance = 128;
}

// Returns the index of the sound in the rally_sounds array
// If the sound isn't loaded, it returns -1
int CRallySound::IsSoundLoaded (char *sound)
{
	for (int i = 0; i < iSndIndex; i++)
		if (!stricmp (sound, rally_sounds[i].szName))
			return i;

	return -1;
}

FSOUND_SAMPLE *CRallySound::LoadSound (char *sound, unsigned int mode)
{
	// Check to see if it is loaded already
	int i = IsSoundLoaded (sound);
	if (i != -1)
		return rally_sounds[i].stream;

	// Load the sound into memory
	strcpy (rally_sounds[iSndIndex].szName, sound);
	rally_sounds[iSndIndex].stream = m_cFMod->FSOUND_Sample_Load(FSOUND_FREE, sound, mode, 0, 0);

	return rally_sounds[iSndIndex].stream;
}

void CRallySound::fmodPlay (char *sound, bool loop, int pitch)
{
	fmodPlay (sound, loop, pitch, 1.0);
}

void CRallySound::fmodPlay (char *sound, bool loop, int pitch, float vol)
{
	if (m_cFMod == NULL)
		return;

	char sz[256];
	sprintf( sz, "hlrally/sound/%s", sound );

	FSOUND_SAMPLE *stream;

	// This now loads them into memory once
	stream = LoadSound (sz, (loop ? FSOUND_LOOP_NORMAL : FSOUND_LOOP_OFF) | FSOUND_2D);

	int channel = m_cFMod->FSOUND_PlaySound(FSOUND_FREE, stream);

	if (channel != -1)
	{
		m_cFMod->FSOUND_SetVolume(channel, CVAR_GET_FLOAT("volume") * vol * VOL_MULT);
		if (pitch)
			m_cFMod->FSOUND_SetFrequency(channel, pitch);
	}
}

void CRallySound::fmodPlay3D (char *sound, bool loop, int pitch, vec3_t pos, vec3_t vel)
{
	fmodPlay3D (sound, loop, pitch, 1.0, pos, vel);
}

void CRallySound::fmodPlay3D (char *sound, bool loop, int pitch, float vol, vec3_t pos, vec3_t vel)
{
	if (m_cFMod == NULL)
		return;

	char sz[256];
	sprintf( sz, "hlrally/sound/%s", sound );

	FSOUND_SAMPLE *stream;

	// This now loads them into memory once
	stream = LoadSound (sz, (loop ? FSOUND_LOOP_NORMAL : FSOUND_LOOP_OFF) | (CVAR_GET_FLOAT ("rally_sound_hw3d") ? FSOUND_HW3D : 0));

	// We have to shrink the 3D coords back into metres
	vec3_t fmod_pos, fmod_vel;
	for (int i = 0; i < 3; i++)
	{
		fmod_pos[i] = pos[i] / FMOD_HL_SCALE;
		fmod_vel[i] = vel[i] / FMOD_HL_SCALE;
	}

	int channel = m_cFMod->FSOUND_PlaySound(FSOUND_FREE, stream);

	if (channel != -1)
	{
		// Update the 3D position of the sound
		m_cFMod->FSOUND_3D_SetAttributes (channel, fmod_pos, fmod_vel);

		m_cFMod->FSOUND_SetVolume(channel, CVAR_GET_FLOAT("volume") * vol * VOL_MULT);
		if (pitch)
			m_cFMod->FSOUND_SetFrequency(channel, pitch);
	}
}

// Update from Frag
// This is also changed now
void CRallySound::fmodEngineStart (int channel, char *model)
{
    char sz[256];
    sprintf( sz, "hlrally/sound/engine/%s_engine.mp3", model );

    FILE * fp;
    int  length;

	// Locate the correct channel
	rally_channel_t *chan;
	if (channel == -1)		// -1 indicates the local player
		chan = &local_channel;
	else
		chan = &remote_channels[channel];

	// See if we should unload the previously playing stream
	if (chan->stream != NULL)
	{
		m_cFMod->FSOUND_Stream_Stop (chan->stream);
		m_cFMod->FSOUND_Stream_Close (chan->stream);
	}

	// Open the new sound
    fp = fopen(sz, "rb");
    if (!fp)
    {
		// Search for .wav
		sprintf( sz, "hlrally/sound/engine/%s_engine.wav", model );
		fp = fopen (sz, "rb");
		if (!fp)
		{
			// Default back to the 206
		    sprintf (sz, "hlrally/sound/engine/206_engine.mp3");
//		    sprintf (sz, "hlrally/sound/engine/evo7_engine.wav");
//		    sprintf (sz, "hlrally/sound/engine/racing_whine.wav");
			fp = fopen (sz, "rb");

			if (!fp)	// Now we have some problems
			{
				m_cFMod->FSOUND_Close ();
				return;
			}
		}
    }

    fseek(fp, 0, SEEK_END);
    length = ftell(fp);
    fclose(fp);

	chan->stream = m_cFMod->FSOUND_Stream_Open(sz, (CVAR_GET_FLOAT ("rally_sound_hw3d") ? FSOUND_HW3D : 0) | FSOUND_LOOP_NORMAL , 0, length);
	if (chan->stream != NULL)
		chan->iChannel = m_cFMod->FSOUND_Stream_Play(FSOUND_FREE, chan->stream);
}

// Creme: Start rally:whine hax
void CRallySound::fmodStartWhine (void)
{
	char szWhileFile[] = "hlrally/sound/engine/racing_whine.wav";
    FSOUND_STREAM *stream;
    FILE * fp;
    int  length;

    fp = fopen(szWhileFile, "rb");

	if (!fp)	// Now we have some problems
	{
		m_cFMod->FSOUND_Close();
		return;
	}

	fseek(fp, 0, SEEK_END);
	length = ftell(fp);

	fclose(fp);


    stream = m_cFMod->FSOUND_Stream_Open(szWhileFile, FSOUND_HW3D | FSOUND_LOOP_NORMAL, 0, length);

    // hack hack for testing
	iWhineChannel = m_cFMod->FSOUND_Stream_Play(FSOUND_FREE, stream);

	// set this to zero straight away in case something happens with the update cycle
	m_cFMod->FSOUND_SetVolume (iWhineChannel, 0);
	m_cFMod->FSOUND_3D_SetAttributes (iWhineChannel, Vector(0,0,0), Vector(0,0,0));

	// Creme end whine hax
}

void CRallySound::fmodEngineChangePitch (int rpm, int playernum, int accelerating, vec3_t pos, vec3_t vel, bool isLocal)
{
	char szModel[30];

	if (m_cFMod == NULL)
		return;

	// If this isn't the local player, shove it into the list
	if (!isLocal)
	{
		// See if we've played this one already
		if (!iRemoteSoundPlayed[playernum])
		{
			iRemoteSoundPlayed[playernum] = 1;
			fmodAddRemoteEngine (rpm, pos, vel);
		}

		return;
	}

	// The rest of this function is for the local player only
	if (!g_PlayerInfoList[CRallyEffects::getSingleton()->m_PlayerList[playernum].pPlayerEnt->index].model)
		strcpy (szModel, "206");		// Set up the default
	else
		strcpy (szModel, g_PlayerInfoList[CRallyEffects::getSingleton()->m_PlayerList[playernum].pPlayerEnt->index].model);

	if (!m_cFMod->FSOUND_IsPlaying(local_channel.iChannel) || stricmp (szModel, local_channel.szModel))
	{
		// They have changed their model, so load the new engine noise
		m_cFMod->FSOUND_StopSound (local_channel.iChannel);							// Stop previous sounds
		fmodEngineStart(-1, szModel);
		strcpy (local_channel.szModel, szModel);	// Save for later
	}

	// We have to shrink the 3D coords back into metres
	vec3_t fmod_pos, fmod_vel;
	for (int i = 0; i < 3; i++)
	{
		fmod_pos[i] = pos[i] / FMOD_HL_SCALE;
		fmod_vel[i] = vel[i] / FMOD_HL_SCALE;
	}

	// Update the 3D position of the sound
	m_cFMod->FSOUND_3D_SetAttributes (local_channel.iChannel, fmod_pos, fmod_vel);

	if (isLocal)
	{
		if (iWhineChannel == -1)
			fmodStartWhine();

		m_cFMod->FSOUND_3D_SetAttributes (iWhineChannel, fmod_pos, fmod_vel);
	}
	// *** Creme: whine hax ^^^;)

	float rpm_vol;
	if (accelerating > 0)
		rpm_vol = 40 + (rpm/2);
	else
		rpm_vol = 20 + (rpm/3);

	if (rpm_vol > 255)
		rpm_vol = 255;

	m_cFMod->FSOUND_SetVolume (local_channel.iChannel, rpm_vol * (float(CVAR_GET_FLOAT("volume"))) );
	m_cFMod->FSOUND_SetFrequency (local_channel.iChannel, 2000 + rpm*260);

// Creme: start rally-whine attribute hax
	if (Length(vel) < 500)
		m_cFMod->FSOUND_SetVolume (iWhineChannel, (10 + (rpm/3)) * (float(CVAR_GET_FLOAT("volume"))) /* * (Length(vel)/600)*/);
	else
	{
		float tVol = (10 + (rpm/3)) * (float(CVAR_GET_FLOAT("volume"))) * (((-Length(vel)*2)+1500)/600);
		if (tVol < 0)
			tVol = 0;

		m_cFMod->FSOUND_SetVolume (iWhineChannel, tVol);
	}

	m_cFMod->FSOUND_SetFrequency(iWhineChannel, 4000 + rpm*30 + (Length(vel) * 80));
// Creme: end rallywhine hax
}

void CRallySound::fmodStopEngine (void)
{
	if (m_cFMod == NULL)
		return;

//	for (int i = 0; i < MAX_PLAYERS; i++)
//		m_cFMod->FSOUND_SetMute (remote_channels[i].iChannel, 1);

	m_cFMod->FSOUND_SetMute (local_channel.iChannel, 1);
	m_cFMod->FSOUND_SetMute (iWhineChannel, 1);
}

void CRallySound::fmodStartEngine (void)
{
	if (m_cFMod == NULL)
		return;

	m_cFMod->FSOUND_SetMute (FSOUND_ALL, 0);
}

void CRallySound::fmodShutDown (void)
{
	if (m_cFMod == NULL)
		return;

	// Stop and close the MP3 stream if necessary
	//fmodStopMp3 ();
	int open = m_cFMod->FSOUND_Stream_GetOpenState (mp3.stream);
	if ((open != -1) && (open != -3))
	{
		// We have a valid stream
		while (!m_cFMod->FSOUND_Stream_Stop (mp3.stream)) ;
		while (!m_cFMod->FSOUND_Stream_Close (mp3.stream)) ;
		mp3.stream = NULL;
		mp3.iChannel = -1;
	}

	// Unload any streams
	// Samples are automagically unloaded by FMOD
	if (local_channel.stream != NULL)
		m_cFMod->FSOUND_Stream_Close (local_channel.stream);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (remote_channels[i].stream != NULL)
			m_cFMod->FSOUND_Stream_Close (remote_channels[i].stream);
	}

	m_cFMod->FSOUND_Close();

	delete m_cFMod;
	m_cFMod = NULL;	// Prevent any further calls
}

signed char F_CALLBACKAPI metacallback(char *name, char *value, int userdata)
{
	char tmp[128];

	if (!strcmp("ARTIST", name))
	{
		sprintf (tmp, "artist %s", value);
		//gEngfuncs.pfnClientCmd (tmp);
		gViewPort->mp3Command (tmp);

		return TRUE;
	}
	else if (!strcmp("TITLE", name))
	{
		sprintf (tmp, "title %s", value);
		//gEngfuncs.pfnClientCmd (tmp);
		gViewPort->mp3Command (tmp);

		return TRUE;
	}

	return TRUE;
}

void CRallySound::fmodFrame (vec3_t position, vec3_t velocity, vec3_t angles)
{
	if (m_cFMod == NULL)
		return;

	vec3_t vForward, vRight, vUp;
	AngleVectors(angles, vForward, vRight, vUp);

	// We have to shrink the 3D coords back into metres
	vec3_t fmod_pos, fmod_vel;
	for (int i = 0; i < 3; i++)
	{
		fmod_pos[i] = position[i] / FMOD_HL_SCALE;
		fmod_vel[i] = velocity[i] / FMOD_HL_SCALE;
	}

	m_cFMod->FSOUND_3D_Listener_SetAttributes(fmod_pos, fmod_vel, vForward[0], vForward[1], vForward[2], vUp[0], vUp[1], vUp[2]); // update 'ears'
	m_cFMod->FSOUND_Update(); // needed to update 3d engine, once per frame

	// See if we have an mp3 that needs playing
	if (mp3.iChannel < 0)
	{
		mp3.iChannel = m_cFMod->FSOUND_Stream_Play (FSOUND_FREE, mp3.stream);
		m_cFMod->FSOUND_Stream_Net_SetMetadataCallback (mp3.stream, metacallback, 0);

		// Update the volume and balance
		m_cFMod->FSOUND_SetPan (mp3.iChannel, m_iMP3Balance);
		m_cFMod->FSOUND_SetVolumeAbsolute (mp3.iChannel, m_iMP3Volume);
	}
}

#define NUM_PLAYER_SOUND	4

void CRallySound::fmodAddRemoteEngine (int rpm, vec3_t pos, vec3_t vel)
{
	vec3_t localorg = gEngfuncs.GetLocalPlayer()->origin;
	float dist = Length (pos - localorg);

//	char tmp[64];
//	sprintf (tmp, "Adding %f, %f, %f\n", pos[0], pos[1], pos[2]);
//	ConsolePrint (tmp);

	if (i3dSoundNumAlloc < i3dSoundNumChannels)
	{
		// Just slot it in
		t3dSoundInfo[i3dSoundNumAlloc].rpms = rpm;
		t3dSoundInfo[i3dSoundNumAlloc].pos = pos;
		t3dSoundInfo[i3dSoundNumAlloc].vel = vel;
		t3dSoundInfo[i3dSoundNumAlloc].dist = dist;

		i3dSoundNumAlloc++;

		// Update the max dist
		f3dSoundMaxDist = 0;
		for (int i = 0; i < i3dSoundNumAlloc; i++)
		{
			if (t3dSoundInfo[i].dist > f3dSoundMaxDist)
				f3dSoundMaxDist = t3dSoundInfo[i].dist;
		}
	}
	else
	{
		// See if this is closer than the greatest one
		if (dist < f3dSoundMaxDist)
		{
			// Find the old one
			for (int i = 0; i < i3dSoundNumChannels; i++)
			{
				if (t3dSoundInfo[i].dist == f3dSoundMaxDist)
				{
					// Overwrite it
					t3dSoundInfo[i].rpms = rpm;
					t3dSoundInfo[i].pos = pos;
					t3dSoundInfo[i].vel = vel;
					t3dSoundInfo[i].dist = dist;
					break;
				}
			}

			// Update the max dist
			f3dSoundMaxDist = 0;
			for (i = 0; i < i3dSoundNumAlloc; i++)
			{
				if (t3dSoundInfo[i].dist > f3dSoundMaxDist)
					f3dSoundMaxDist = t3dSoundInfo[i].dist;
			}
		}
	}
}

void CRallySound::fmodPlayRemoteEngines (void)
{
	if (m_cFMod == NULL)
		return;

	// We did add someone so loop through
	for (int i = 0; i < i3dSoundNumAlloc; i++)
	{
		if (remote_channels[i].iChannel == -1 && remote_channels[i].stream == NULL && !m_cFMod->FSOUND_IsPlaying(remote_channels[i].iChannel))
		{

			// This is the first time this channel has been played
			fmodEngineStart(i, "206");						// All remote cars sound like the 206
			strcpy (remote_channels[i].szModel, "206");		// Save for later
		}

		// We have to shrink the 3D coords back into metres
		vec3_t fmod_pos, fmod_vel;
		for (int j = 0; j < 3; j++)
		{
			fmod_pos[j] = t3dSoundInfo[i].pos[j] / FMOD_HL_SCALE;
			fmod_vel[j] = t3dSoundInfo[i].vel[j] / FMOD_HL_SCALE;
		}

		// Calculate the volume
		float rpm_vol = 25 + (t3dSoundInfo[i].rpms / 4);
		if (rpm_vol > 75)	// Sanity Check so they aren't too loud
			rpm_vol = 75;

		// Update the sound channel
		m_cFMod->FSOUND_SetMute (remote_channels[i].iChannel, 0);
		m_cFMod->FSOUND_3D_SetAttributes (remote_channels[i].iChannel, fmod_pos, fmod_vel);
		m_cFMod->FSOUND_SetVolume (remote_channels[i].iChannel, rpm_vol * (float(CVAR_GET_FLOAT("volume"))) );
		m_cFMod->FSOUND_SetFrequency(remote_channels[i].iChannel, 2000 + t3dSoundInfo[i].rpms * 220);
	}

	// Mute any channels that haven't been used (i.e. Not enough cars in PVS)
	for (i = i3dSoundNumAlloc; (i >= 0) && (i < i3dSoundNumChannels); i++)
		m_cFMod->FSOUND_SetMute (remote_channels[i].iChannel, 1);

	// Check to see if any previously used channels aren't being used anymore
	int iNewNumChannels = CVAR_GET_FLOAT ("rally_sound_3dchannels");

	if (iNewNumChannels < i3dSoundNumChannels)
	{
		// Stop the channels
		for (i = i3dSoundNumChannels - 1; i >= iNewNumChannels; i--)
		{
			m_cFMod->FSOUND_StopSound (remote_channels[i].iChannel);		// Stop previous sounds
		}
	}

	// Reset the variables for the next frame
	f3dSoundMaxDist = 8196;
	i3dSoundNumAlloc = 0;
	i3dSoundNumChannels = iNewNumChannels;

	for (i = 0; i < MAX_PLAYERS; i++)
		iRemoteSoundPlayed[i] = 0;
}

void CRallySound::fmodStopRemoteEngines (void)
{
	if (m_cFMod == NULL)
		return;

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (remote_channels[i].stream != NULL)
			m_cFMod->FSOUND_Stream_Stop (remote_channels[i].stream);
	}
}

//
// MP3 Functions
//
void CRallySound::fmodPlayMp3 (char *file)
{
	if (m_cFMod == NULL || file == NULL)
		return;

	if (!stricmp (mp3.szModel, file) && m_cFMod->FSOUND_GetPaused (mp3.iChannel))
	{
		m_cFMod->FSOUND_SetPaused (mp3.iChannel, false);
		return;
	}
	else if (m_cFMod->FSOUND_IsPlaying (mp3.iChannel))
		fmodStopMp3 ();

	strcpy (mp3.szModel, file);
	mp3.stream = m_cFMod->FSOUND_Stream_Open(file, FSOUND_MPEGACCURATE | FSOUND_NONBLOCKING, 0, 0);
	mp3.iChannel = -1;
}

void CRallySound::fmodStopMp3 (void)
{
	if (m_cFMod == NULL)
		return;

	if (fmodIsPlayingMp3 ())
	{
		m_cFMod->FSOUND_Stream_Stop (mp3.stream);
		m_cFMod->FSOUND_Stream_Close (mp3.stream);
		mp3.iChannel = -1;
		mp3.stream = NULL;
	}
}

void CRallySound::fmodPauseMp3 (void)
{
	if (m_cFMod == NULL)
		return;

	if (fmodIsPlayingMp3 ())
		m_cFMod->FSOUND_SetPaused (mp3.iChannel, true);
}

bool CRallySound::fmodIsPausedMp3 (void)
{
	if (m_cFMod == NULL)
		return false;

	return m_cFMod->FSOUND_GetPaused (mp3.iChannel);
}

bool CRallySound::fmodIsPlayingMp3 (void)
{
	if (m_cFMod == NULL)
		return false;

	return m_cFMod->FSOUND_IsPlaying (mp3.iChannel);
}

bool CRallySound::fmodIsInternetRadioPlaying (void)
{
	if (m_cFMod == NULL)
		return false;

	int ret = m_cFMod->FSOUND_Stream_GetOpenState (mp3.stream);

	if (ret == 0)
		return m_cFMod->FSOUND_IsPlaying (mp3.iChannel);
	else
		return ((ret == -4) || (ret == -5));	// Waiting for reply / Buffering
}

bool CRallySound::fmodIsOpenMp3 (void)
{
	if (m_cFMod == NULL)
		return false;

	int ret = m_cFMod->FSOUND_Stream_GetOpenState (mp3.stream);

	return ((ret == 0) || (ret == -4) || (ret == -5));	// -4, -5 are for internet radio
}

void CRallySound::fmodSetMp3Balance (int bal)
{
	if (m_cFMod == NULL)
		return;

	if (bal < 0)
		bal = 0;
	else if (bal > 255)
		bal = 255;

	m_iMP3Balance = bal;
	m_cFMod->FSOUND_SetPan (mp3.iChannel, m_iMP3Balance);
}

void CRallySound::fmodSetMp3Volume (int vol)
{
	if (m_cFMod == NULL)
		return;

	if (vol < 0)
		vol = 0;
	else if (vol > 255)
		vol = 255;

	m_iMP3Volume = vol;
	m_cFMod->FSOUND_SetVolumeAbsolute (mp3.iChannel, m_iMP3Volume);
}

int CRallySound::fmodGetMp3Position (void)
{
	if (m_cFMod == NULL)
		return 0;

	if (!fmodIsPlayingMp3 ())
		return 0;

	return m_cFMod->FSOUND_Stream_GetTime (mp3.stream);
}

void CRallySound::fmodSetMp3Position (int milliseconds)
{
	if (m_cFMod == NULL)
		return;

	m_cFMod->FSOUND_Stream_SetTime (mp3.stream, milliseconds);
}

int CRallySound::fmodGetMp3Length (void)
{
	if (m_cFMod == NULL)
		return 0;

	if (!fmodIsPlayingMp3 ())
		return 0;

	return m_cFMod->FSOUND_Stream_GetLengthMs (mp3.stream);
}

char tmp2[2048];	// TODO: Don't declare this here
char *CRallySound::fmodGetMp3Tag (char *tagname)
{
	if (m_cFMod == NULL)
		return NULL;

	int numinfo;
	m_cFMod->FSOUND_Stream_GetNumTagFields(mp3.stream, &numinfo);

	for (int i = 0; i < numinfo; i++)
	{
		char *name, *value;
		int  type, length;

		if (m_cFMod->FSOUND_Stream_GetTagField(mp3.stream, i, &type, &name, (void **)&value, &length))
		{
			if (type == FSOUND_TAGFIELD_ID3V2 && ((name[0] == 'T' && value[0] == 0) || !strncmp(name, "COMM", 4)))
			{
				int offset = 0;

				if (name[0] == 'T' && value[0] == 0)
					offset = 1;
				else if (!strncmp(name, "COMM", 4) && length > 8)
					offset = 8;     // a quick hack to skip the COMM tag stuff at the start (language etc), check id3.org for more

				strncpy(tmp2, value + offset, length - offset);
				tmp2[length - 1] = 0;

				if (!stricmp (name, tagname))
					return tmp2;
			}
			else if (type == FSOUND_TAGFIELD_ID3V1)
			{
				if (!stricmp (name, tagname))
					return value;
			}
		}
	}

	return NULL;
}

int CRallySound::fmodGetMp3Kbps (void)
{
	if (m_cFMod == NULL)
		return 0;

	unsigned int lenms, lenbytes;
	lenms = m_cFMod->FSOUND_Stream_GetLengthMs (mp3.stream);
	lenbytes = m_cFMod->FSOUND_Stream_GetLength (mp3.stream);

	if (!lenms)		// Avoid divide by 0 error
		return 0;

	return (lenbytes / (lenms / 1000) /1000 * 8);
}

int CRallySound::fmodGetMp3Frequency (void)
{
	if (m_cFMod == NULL)
		return 0;

	return m_cFMod->FSOUND_GetFrequency (mp3.iChannel);
}

bool CRallySound::fmodIsStereoMp3 (void)
{
	if (m_cFMod == NULL)
		return 0;

	unsigned int mode = m_cFMod->FSOUND_Stream_GetMode (mp3.stream);
	return (mode & FSOUND_STEREO);
}
// SaRcaZm - V8 - End
