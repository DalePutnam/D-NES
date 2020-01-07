#pragma once

#if defined(_WIN32)

#include "iaudio_platform.h"

#include <cstdint>
#include <mutex>
#include <condition_variable>
#include <xaudio2.h>

class XAudio2Platform : public IAudioPlatform
{
public:
	XAudio2Platform();
	virtual ~XAudio2Platform() {};

	virtual void Initialize(uint32_t sampleRate) override;
	virtual void CleanUp() override;
	virtual void Reset() override;
	virtual uint32_t GetNumPendingSamples() override;
	virtual void SubmitSample(float sample) override;
	virtual uint32_t GetSampleRate() override;

private:
	class VoiceCallback;

	IXAudio2* _xaudio2Instance;
	IXAudio2MasteringVoice* _xaudio2MasteringVoice;
	IXAudio2SourceVoice* _xaudio2SourceVoice;
	VoiceCallback* _voiceCallback;

	uint32_t _sampleRate;
	uint32_t _currentBufferOffset;
	uint32_t _numOutputBuffers;
	uint32_t _bufferSize;

	uint32_t _writeIndex;
	uint32_t _readIndex;
	bool _overlapped;
	float** _outputBuffers;

	std::mutex _mutex;
	std::condition_variable _cv;
};

#endif