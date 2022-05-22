#ifndef __Rally_Fmod_H__
#define __Rally_Fmod_H__

// Windows Includes
#include <windows.h>

// FMOD Includes
#include "fmod/api/inc/fmod.h"
#include "fmod/api/inc/fmod_errors.h"


// Class Defintion
class CFModFuncs {
public:
	// My Funcs
	CFModFuncs (void);
	~CFModFuncs (void);
	bool Init (void);

	// FMOD FUNCTION PTRS

	// FMod Funcs
	signed char (_stdcall *FSOUND_Init) (int mixrate, int maxsoftwarechannels, unsigned int flags);
	signed char (_stdcall *FSOUND_SetOutput) (int outputtype);
	signed char (_stdcall *FSOUND_SetBufferSize) (int len_ms);
	signed char (_stdcall *FSOUND_SetDriver) (int driver);
	signed char (_stdcall *FSOUND_SetMinHardwareChannels) (int min);
	void (_stdcall *FSOUND_SetSFXMasterVolume) (int volume);
	void (_stdcall *FSOUND_Update) (void);
	void (_stdcall *FSOUND_Close) (void);

	// Channel Funcs
	int (_stdcall *FSOUND_GetMaxChannels) (void);
	signed char (_stdcall *FSOUND_SetVolume) (int channel, int vol);
	signed char (_stdcall *FSOUND_SetVolumeAbsolute) (int channel, int vol);
	signed char (_stdcall *FSOUND_SetFrequency) (int channel, int freq);
	int (_stdcall *FSOUND_GetFrequency) (int channel);
	signed char (_stdcall *FSOUND_SetMute) (int channel, signed char mute);
	signed char (_stdcall *FSOUND_IsPlaying) (int channel);
	signed char (_stdcall *FSOUND_SetPaused) (int channel, signed char paused);
	signed char (_stdcall *FSOUND_GetPaused) (int channel);
	signed char (_stdcall *FSOUND_SetPan) (int channel, int pan);

	// Stream Funcs
	FSOUND_STREAM *(_stdcall *FSOUND_Stream_Open) (const char *filename, unsigned int mode, int offset, int memlength);
	int (_stdcall *FSOUND_Stream_Play) (int channel, FSOUND_STREAM *stream);
	signed char (_stdcall *FSOUND_Stream_Stop) (FSOUND_STREAM *stream);
	signed char (_stdcall *FSOUND_Stream_Close) (FSOUND_STREAM *stream);
	int (_stdcall *FSOUND_Stream_GetLengthMs) (FSOUND_STREAM *stream);
	int (_stdcall *FSOUND_Stream_GetLength) (FSOUND_STREAM *stream);
	int (_stdcall *FSOUND_Stream_GetTime) (FSOUND_STREAM *stream);
	signed char (_stdcall *FSOUND_Stream_SetTime) (FSOUND_STREAM *stream, int ms);
	int (_stdcall *FSOUND_Stream_GetOpenState) (FSOUND_STREAM *stream);
	unsigned int (_stdcall *FSOUND_Stream_GetMode) (FSOUND_STREAM *stream);
	signed char (_stdcall *FSOUND_Stream_GetNumTagFields) (FSOUND_STREAM *stream, int *num);
	signed char (_stdcall *FSOUND_Stream_GetTagField) (FSOUND_STREAM *stream, int num, int *type, char **name, void **value, int *length);

	// Internet Stream Funcs
	signed char (_stdcall *FSOUND_Stream_Net_SetBufferProperties) (int buffersize, int prebuffer_percent, int rebuffer_percent);
	signed char (_stdcall *FSOUND_Stream_Net_SetMetadataCallback) (FSOUND_STREAM *stream, FSOUND_METADATACALLBACK callback, int userdata);

	// 3D Sound Funcs
	void (_stdcall *FSOUND_3D_SetDopplerFactor) (float scale);
	void (_stdcall *FSOUND_3D_SetDistanceFactor) (float factor);
	void (_stdcall *FSOUND_3D_SetRolloffFactor) (float factor);
	signed char (_stdcall *FSOUND_3D_SetAttributes) (int channel, float *pos, float *vel);
	void (_stdcall *FSOUND_3D_Listener_SetAttributes) (float *pos, float *vel, float fx, float fy, float fz, float tx, float ty, float tz);

	// Sample Funcs
	FSOUND_SAMPLE *(_stdcall *FSOUND_Sample_Load) (int index, const char *name, unsigned int inputmode, int offset, int memlength);
	int (_stdcall *FSOUND_PlaySound) (int channel, FSOUND_SAMPLE *sptr);
	signed char (_stdcall *FSOUND_StopSound) (int channel);

private:
	HINSTANCE m_hFMod;
};

#endif	// __Rally_Fmod_H__