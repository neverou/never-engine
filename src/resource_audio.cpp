#include "resource.h"

struct WavHeader
{
	char magic[4];
	u32 fileSize;
	char typeHeader[4];
	char formatChunkMarker[4];
	u32 formatChunkLength;
	u16 formatType;
	u16 channelCount;
	u32 sampleRate;
	u32 srbpsc8;
	u16 bytesPerSampleTimesChannels;
	u16 bitsPerSample;
	char dataChunkHeader[4];
	u32 dataSize;
};

AudioResource* LoadAudio(StringView name)
{
	{
		auto* existing = SearchResource(&resourceManager.audios, name);
		if (existing) {
			// Log("Existing model! %s", name.data);
			return existing;
		}
	}


	File file;
	if (Open(&file, TPrint("%s", name.data), FILE_READ)) {

		Size length = FileLength(&file);

		void* data = TempAlloc(length);
		
		Read(&file, data, length);
		Close(&file);

		WavHeader* header = (WavHeader*)data;
		void* audioData = header + 1;
		
		// error checking
		if (sizeof(WavHeader) > length)
		{
			LogWarn("[res] Invalid audio file");
			return NULL;
		}
		if (memcmp("RIFF", header->magic, sizeof(header->magic)) != 0)
		{
			LogWarn("[res] Unrecognized audio file format, expecting RIFF");
			return NULL;
		}
		if (memcmp("WAVE", header->typeHeader, sizeof(header->typeHeader)) != 0)
		{
			LogWarn("[res] Unrecgnozied audio file type header, expected WAVE");
			return NULL;
		}
		if (header->formatType != 1)
		{
			LogWarn("[res] Unrecognized audio data format, expected 1 (PCM)");
			return NULL;
		}
		if (header->bitsPerSample != 16)
		{
			LogWarn("[res] Tried to load audio file with non 16 bits per sample");
			return NULL;
		}
		if (header->dataSize + sizeof(WavHeader) > length)
		{
			LogWarn("[res] Audio file ended too early");
			return NULL;
		}

		AudioBuffer buffer{};

		void* bufferData = Alloc(header->dataSize);
		Memcpy(bufferData, audioData, header->dataSize);

		buffer.samples = (PcmSample*)bufferData;
		buffer.channelCount = header->channelCount;
		buffer.sampleRate = header->sampleRate;
		buffer.sampleCount = header->dataSize / header->bytesPerSampleTimesChannels;
		
		AudioResource audio = {};
		audio.name = CopyString(name);
		audio.buffer = buffer;

		Log("[res] loaded audio (%s)", name.data);
		return BucketArrayAdd(&resourceManager.audios, audio);
	}

	LogWarn("[res] Tried to read audio that does not exist!");
	return NULL;
}

