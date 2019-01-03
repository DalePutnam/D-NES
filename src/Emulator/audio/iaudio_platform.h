#pragma once

#include <cstdint>
#include <memory>

class IAudioPlatform
{
public:
	static std::unique_ptr<IAudioPlatform> CreateAudioPlatform();

	virtual bool Initialize(uint32_t sampleRate) = 0;
	virtual void CleanUp() = 0;
	virtual void Reset() = 0;
	virtual uint32_t GetNumPendingSamples() = 0;
	virtual void SubmitSample(float sample) = 0;
	virtual uint32_t GetSampleRate() = 0;

	virtual ~IAudioPlatform() {}
};