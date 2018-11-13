#pragma once

#include <cstdint>

class IAudioBackend
{
public:
	virtual ~IAudioBackend() {}

	virtual bool Initialize(int sampleRate) = 0;
	virtual void CleanUp() = 0;
	virtual void Reset() = 0;
	virtual void Flush() = 0;
	virtual uint32_t GetNumPendingSamples() = 0;
	virtual void SubmitSample(float sample) = 0;
	virtual uint32_t GetSampleRate() = 0;
};