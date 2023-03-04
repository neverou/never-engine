#include "audio.h"
#ifdef PLATFORM_LINUX

#include "logger.h"
 
#include <pulse/simple.h>
#include <pulse/error.h>

 
// Backend, Platform Dependent implementation



intern pa_simple *pulseAudio = NULL; 

bool InitAudioBackend()
{
	pa_sample_spec sampleSpec = {
		.format = PA_SAMPLE_S16LE,
		.rate = Audio_Output_Sample_Rate,
		.channels = Audio_Channels,
	};

	int error;
    if (!(pulseAudio = pa_simple_new(NULL, "Never Engine", PA_STREAM_PLAYBACK, NULL, "playback", &sampleSpec, NULL, NULL, &error)))
	{
        LogWarn("[audio] Failed to init PulseAudio, pa_simple_new() failed: %s", pa_strerror(error));
		return false;
    }

	return true;
}

bool ReadyForBuffer()
{
	return true;
}

bool EndAudioBackend()
{
	if (pulseAudio)
		pa_simple_free(pulseAudio);
	return true;
}

void SubmitBuffer(AudioBuffer buffer)
{
	int error;
	if (pa_simple_write(pulseAudio, buffer.samples, buffer.sampleCount * Audio_Bytes_Per_Sample * Audio_Channels, &error) < 0) {
		LogWarn("[audio] PulseAudio error, pa_simple_write() failed: %s", pa_strerror(error));
	}
}

#endif