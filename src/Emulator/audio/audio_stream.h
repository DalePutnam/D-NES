#pragma once

#include <memory>
#include <atomic>

class IAudioBackend;

class AudioStream
{
public:
	AudioStream(uint32_t sampleRate = DEFAULT_SAMPLE_RATE);
	~AudioStream();

	void Reset();
	void Flush();
	uint32_t GetNumPendingSamples();
	void SubmitSample(float sample);
	uint32_t GetSampleRate();

	static const int DEFAULT_SAMPLE_RATE;

private:
	std::unique_ptr<IAudioBackend> _backend;
};