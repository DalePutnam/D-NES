#pragma once

#if defined(__linux)

#include "iaudio_platform.h"

#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <alsa/asoundlib.h>

class AlsaPlatform : public IAudioPlatform
{
public:
	virtual bool Initialize(uint32_t sampleRate) override;
	virtual void CleanUp() override;
	virtual void Reset() override;
	virtual uint32_t GetNumPendingSamples() override;
	virtual void SubmitSample(float sample) override;
	virtual uint32_t GetSampleRate() override;
private:
	void StreamWorker();

	snd_pcm_t* _alsaHandle;
	uint32_t _sampleRate;
	uint32_t _sampleBufferSize;
	uint32_t _sampleBufferIndex;

	uint32_t _numOutputBuffers;
	uint32_t _writeIndex;
	uint32_t _readIndex;
	bool _overlapped;
	float** _outputBuffers;

	std::mutex _mutex;
	std::condition_variable _cv;
	std::thread _thread;

	std::atomic<bool> _running;
};

#endif