#pragma once

#include <memory>
#include <atomic>

class IAudioPlatform;

class AudioBackend
{
public:
	AudioBackend(uint32_t sampleRate = DEFAULT_SAMPLE_RATE);
	~AudioBackend();

	void Reset();
	uint32_t GetNumPendingSamples();
	void SubmitSample(float sample);
	uint32_t GetSampleRate();

	static const int DEFAULT_SAMPLE_RATE;

private:
	std::unique_ptr<IAudioPlatform> _backend;
};