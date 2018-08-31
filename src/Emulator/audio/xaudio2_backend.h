#pragma once

#include <cstdint>
#include <mutex>
#include <condition_variable>
#include <xaudio2.h>

#include "iaudio_backend.h"

class XAudio2Backend : public IAudioBackend
{
public:
	XAudio2Backend();
	virtual ~XAudio2Backend() {};

	virtual bool Initialize(int sampleRate) override;
	virtual void CleanUp() override;
	virtual void Reset() override;
	virtual void Flush() override;
	virtual uint32_t GetNumPendingSamples() override;
	virtual void SubmitSample(float sample) override;
	virtual uint32_t GetSampleRate() override;

private:
	void PrivateFlush();

	IXAudio2* _xaudio2Instance;
	IXAudio2MasteringVoice* _xaudio2MasteringVoice;
	IXAudio2SourceVoice* _xaudio2SourceVoice;

	int _sampleRate;
	int _currentBuffer;
	int _currentBufferOffset;
	int _numOutputBuffers;
	int _bufferSize;
	float** _outputBuffers;

	std::mutex _mutex;
};