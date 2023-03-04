#include <thread>

#include "audio.h"
#include "logger.h"
#include "allocators.h"
#include "resource.h"

// #ifdef PLATFORM_WINDOWS
// #include <Windows.h>
// #endif

#include "game.h"

intern std::thread audioThread;
intern bool audioThreadRunning;

intern PcmSample* bufferRing;
intern PcmSample* samplesTempBuffer;
intern int selectedBuffer = 0;

intern EntityId audioListener = 0;
intern Mat4 audioListenerMatrix = CreateMatrix(1);



// ~Todo multithreading properly (add mutexes to audio player array)

void SetAudioListener(EntityId entity)
{
	audioListener = entity;
}

void UpdateAudio()
{
	// ~Todo gather audio player data here and put them on an audio thread-synchronized data structure
	// based off the loaded world

	Entity* listener = GetWorldEntity(&game->world, audioListener);
	audioListenerMatrix = EntityWorldXformMatrix(&game->world, listener);
}




intern void QuerySamplesFromAudioPlayer(AudioPlayer* player, PcmSample* samples, u32 sampleCount, u32 sampleRate)
{
	// ~Todo multichannel
	double stepAmount = 1.0 / double(sampleRate);
	for (u32 i = 0; i < sampleCount; ++i)
	{
		u32 sampleIdx = u32(player->position * player->buffer.sampleRate);
		double interp = player->position * player->buffer.sampleRate - sampleIdx;
		if (sampleIdx < player->buffer.sampleCount)
		{
			PcmSample sample0 = player->buffer.samples[player->buffer.channelCount * (sampleIdx)];
			PcmSample sample1 = player->buffer.samples[player->buffer.channelCount * ((sampleIdx + 1 > player->buffer.sampleCount) ? sampleIdx : (sampleIdx + 1))];
            
			double interpolated = sample1 * interp + sample0 * (1 - interp);
			samples[i] = (s32)interpolated;
			
			player->position += stepAmount;
		}
		else
		{
			if (player->flags & AudioPlayer_Flags_Loop)
			{
				player->position = 0;
			}
			// ~Todo else set to not playing or loop
			// also if loop is enabled then wrap around the samples
			break;
		}
	}
}


intern void AudioThread()
{
	InitAudioBackend();
    
    
	while (audioThreadRunning)
	{
		if (ReadyForBuffer())
		{ 
            s16* audioBuffer = &(bufferRing[Audio_Channels * Audio_Buffer_Size * selectedBuffer]);
			Memset(audioBuffer, Audio_Channels * Audio_Buffer_Size * Audio_Bytes_Per_Sample, 0);
            
			for (auto it = BucketArrayBegin(&game->world.audioPlayers); BucketIteratorValid(it); it = BucketIteratorNext(it))
			{
				auto* player = GetBucketIterator(it);
				QuerySamplesFromAudioPlayer(player, samplesTempBuffer, Audio_Buffer_Size, Audio_Output_Sample_Rate);
                
				// apply spatial audio
				Vec3 soundRelativePosition = Mul(player->sourcePosition, 1, Inverse(audioListenerMatrix));
				Vec3 soundDir = Normalize(soundRelativePosition);
				float soundVolume = 1.0 / Length(soundRelativePosition);
                
				// ~Todo Un-hardcode @@SurroundAudio
				// Maybe we just always do every speaker?
				float leftEar  = -soundDir.x * 0.5 + 0.5;
				float rightEar =  soundDir.x * 0.5 + 0.5;
                
				for (int i = 0; i < Audio_Buffer_Size; i++)
				{
					audioBuffer[i * Audio_Channels    ] += s32(samplesTempBuffer[i] * player->volume * soundVolume * leftEar);
					audioBuffer[i * Audio_Channels + 1] += s32(samplesTempBuffer[i] * player->volume * soundVolume * rightEar);
				}
			}
            
			selectedBuffer++;
			if (selectedBuffer > 2) selectedBuffer = 0;
            
			AudioBuffer buffer = {
				.sampleRate = Audio_Output_Sample_Rate,
				.sampleCount = Audio_Buffer_Size,
				.samples = audioBuffer,
			};
			SubmitBuffer(buffer);
		}
	}
    
	EndAudioBackend();
}

void StartAudioThread()
{
	// ~TODO @@WindowsCode replace with threading library wrapper
	// you may be asking whats broken, but im asking what isnt broken as there is c++ code involved...
    
	Log("[audio] Starting audio thread");
    
	audioThreadRunning = true;
	audioThread = std::thread(AudioThread);
    
	bufferRing = (PcmSample*)AceAlloc(Audio_Channels * Audio_Bytes_Per_Sample * Audio_Buffer_Size * 3, Engine_Arena);
	samplesTempBuffer = (PcmSample*)AceAlloc(Audio_Buffer_Size * Audio_Bytes_Per_Sample, Engine_Arena);
}

void StopAudioThread()
{
	Log("[audio] Stopping audio thread");
    
	audioThreadRunning = false;
	audioThread.join();
}

