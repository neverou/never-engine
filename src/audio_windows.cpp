#include "audio.h"

#ifdef PLATFORM_WINDOWS

#include "error.h"
#include "logger.h"
#include "maths.h"

#include <combaseapi.h>
#include <xaudio2.h>
#include <stdint.h>
#include <math.h>


IXAudio2* xAudio = NULL;
IXAudio2MasteringVoice* masteringVoice = NULL;
IXAudio2SourceVoice* sourceVoice = NULL;

bool InitAudioBackend()
{
    // Start COM
    HRESULT comInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    if (comInit == 0)
    {
        Log("[audio] Started COM!");
    }
    else if (comInit == 1)
    {
        LogWarn("[audio] COM is probably already started on this thread");
        return 1;
    }
    else
    {
        LogWarn("[audio] Another COM issue, probably that the apartment threading mode changed");
        return 1;
    }

    // Create xaudio2
    HRESULT xAudioInit = XAudio2Create(&xAudio, 0, XAUDIO2_DEFAULT_PROCESSOR);

    if (xAudioInit == 0)
    {
        Log("[audio] Initialized XAudio2");
    }
    else
    {
        FatalError("[audio] Failed to initialize XAudio2");
    }

    // Create mastering voice
    HRESULT masteringVoiceInit = xAudio->CreateMasteringVoice(&masteringVoice);
    if (masteringVoiceInit == 0)
    {
        Log("[audio] Created Mastering Voice");
    }
    else
    {
        FatalError("[audio] Failed to create Mastering Voice");
    }

    WAVEFORMATEXTENSIBLE waveFormat{};

    waveFormat.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    waveFormat.Format.nChannels = Audio_Channels;
    waveFormat.Format.nSamplesPerSec = Audio_Output_Sample_Rate;
    waveFormat.Format.nAvgBytesPerSec = Audio_Channels * 2 * Audio_Output_Sample_Rate;
    waveFormat.Format.nBlockAlign = Audio_Channels * 2;
    waveFormat.Format.wBitsPerSample = 16;
    waveFormat.Format.cbSize = 22;
    waveFormat.Samples.wValidBitsPerSample = 16;
    waveFormat.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
    waveFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

    // Create source voice
    HRESULT sourceVoiceCreation = xAudio->CreateSourceVoice(&sourceVoice, (WAVEFORMATEX*)&waveFormat);
    if (sourceVoiceCreation == 0)
    {
        Log("[audio] Created Source Voice");
    }
    else
    {
        LogWarn("[audio] Failed to create Source Voice: %lu", sourceVoiceCreation);

        return 1;
    }

    HRESULT startVoice = sourceVoice->Start();
    if (startVoice == 0)
    {
        Log("[audio] Started Source Voice!");
    }
    else
    {
        LogWarn("[audio] Failed to start Source Voice");
    }

    return true;
}

bool ReadyForBuffer()
{
    XAUDIO2_VOICE_STATE state;
    sourceVoice->GetState(&state);
    //Log("Buffers queued: %d\n", state.BuffersQueued);

    return (state.BuffersQueued < 2);
}

bool EndAudioBackend()
{
    xAudio->Release();
    CoUninitialize();
    return 0;
}

void SubmitBuffer(AudioBuffer buffer)
{

    XAUDIO2_BUFFER xAudioBuffer{};

    xAudioBuffer.AudioBytes = Audio_Channels * buffer.sampleCount * Audio_Bytes_Per_Sample;
    xAudioBuffer.pAudioData = (BYTE*)buffer.samples;
    xAudioBuffer.Flags = 0;

    HRESULT submitSourceBuffer = sourceVoice->SubmitSourceBuffer(&xAudioBuffer);
    if (submitSourceBuffer != 0)
    {
        //LogWarn("[audio] Failed to submit Source Voice");
    }

    
}

#endif