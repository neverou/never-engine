#pragma once

#include "core.h"
#include "entity.h"
#include "maths.h"

typedef s16 PcmSample;
constexpr u32 Audio_Output_Sample_Rate = 100000;
constexpr u32 Audio_Bytes_Per_Sample = sizeof(PcmSample);
constexpr u32 Audio_Buffer_Size = 4096; // @TODO - Make it possible to change sample size during the program
constexpr u32 Audio_Channels = 2;

// note: latency in ms is (1000)/(bufferSize/sampleRate)

struct AudioBuffer {
	u32 sampleRate;
	Size sampleCount;
	PcmSample* samples;
	u16 channelCount;
};

void StartAudioThread();
void StopAudioThread();
void UpdateAudio();

void SetAudioListener(EntityId entity);

enum AudioPlayerFlags
{
	AudioPlayer_Flags_Loop = 1 << 0,
};

struct AudioPlayer {
	u32 flags;
	double position;
	float volume;
	AudioBuffer buffer;

	Vec3 sourcePosition;
	// ~Todo store the state of the audio player (isPlaying and isPaused)
};

// Backend, Platform Dependent implementation

bool InitAudioBackend();
bool ReadyForBuffer();
bool EndAudioBackend();

void SubmitBuffer(AudioBuffer buffer);