#if defined(_WIN32)

#include "xaudio2_platform.h"
#include <string>

#pragma comment(lib, "XAudio2.lib")

static constexpr uint32_t BUFFERS_PER_FRAME = 4;
static constexpr uint32_t STANDARD_FRAME_RATE = 60;

class XAudio2Platform::VoiceCallback : public IXAudio2VoiceCallback
{
public:
	VoiceCallback(XAudio2Platform& platform)
		: _platform(platform)
	{}

	void OnBufferEnd(void * pBufferContext)
	{
		std::unique_lock<std::mutex> lock(_platform._mutex);

		if (_platform._writeIndex == _platform._readIndex && !_platform._overlapped)
		{
			_platform._xaudio2SourceVoice->Stop(0);
		}
		else
		{
			_platform._readIndex++;
			if (_platform._readIndex >= _platform._numOutputBuffers)
			{
				_platform._overlapped = false;
				_platform._readIndex = 0;
			}
		}

		_platform._cv.notify_all();
	}

	// Ununsed methods are stubs
	void OnStreamEnd() {}
	void OnVoiceProcessingPassEnd() {}
	void OnVoiceProcessingPassStart(UINT32 SamplesRequired) {}
	void OnBufferStart(void * pBufferContext) {}
	void OnLoopEnd(void * pBufferContext) {}
	void OnVoiceError(void * pBufferContext, HRESULT Error) {}

private:
	XAudio2Platform& _platform;
};

XAudio2Platform::XAudio2Platform()
	: _xaudio2Instance(nullptr)
	, _xaudio2MasteringVoice(nullptr)
	, _xaudio2SourceVoice(nullptr)
	, _voiceCallback(nullptr)
	, _sampleRate(0)
	, _currentBufferOffset(0)
	, _numOutputBuffers(0)
	, _bufferSize(0)
	, _writeIndex(0)
	, _readIndex(0)
	, _overlapped(false)
	, _outputBuffers(nullptr)
{}

bool XAudio2Platform::Initialize(uint32_t sampleRate)
{
	_xaudio2Instance = nullptr;
	_xaudio2MasteringVoice = nullptr;
	_xaudio2SourceVoice = nullptr;
	_voiceCallback = nullptr;
	_currentBufferOffset = 0;
	_sampleRate = sampleRate;
	_bufferSize = _sampleRate / STANDARD_FRAME_RATE / BUFFERS_PER_FRAME;
	_writeIndex = 0;
	_readIndex = 0;
	_overlapped = false;
	_numOutputBuffers = BUFFERS_PER_FRAME;

	WAVEFORMATEX WaveFormat = { 0 };
	WaveFormat.nChannels = 1;
	WaveFormat.nSamplesPerSec = _sampleRate;
	WaveFormat.wBitsPerSample = sizeof(float) * 8;
	WaveFormat.nAvgBytesPerSec = _sampleRate * sizeof(float);
	WaveFormat.nBlockAlign = sizeof(float);
	WaveFormat.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;

	HRESULT hr;
	hr = XAudio2Create(&_xaudio2Instance);
	if (FAILED(hr)) goto FailedExit;

	hr = _xaudio2Instance->CreateMasteringVoice(&_xaudio2MasteringVoice, 1, _sampleRate);
	if (FAILED(hr)) goto FailedExit;

	_voiceCallback = new VoiceCallback(*this);

	hr = _xaudio2Instance->CreateSourceVoice(&_xaudio2SourceVoice, &WaveFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, _voiceCallback);
	if (FAILED(hr)) goto FailedExit;

	_outputBuffers = new float*[_numOutputBuffers];
	for (size_t i = 0; i < _numOutputBuffers; ++i)
	{
		_outputBuffers[i] = new float[_bufferSize];
	}

	return true;

FailedExit:
	if (_xaudio2SourceVoice) _xaudio2SourceVoice->DestroyVoice();
	if (_xaudio2MasteringVoice) _xaudio2MasteringVoice->DestroyVoice();
	if (_xaudio2Instance) _xaudio2Instance->Release();
	if (_voiceCallback) delete _voiceCallback;
	throw std::runtime_error("APU: Failed to initialize XAudio2");
	return false;
}

void XAudio2Platform::CleanUp()
{
	_xaudio2SourceVoice->Stop();
	_xaudio2SourceVoice->FlushSourceBuffers();
	_xaudio2SourceVoice->DestroyVoice();
	_xaudio2MasteringVoice->DestroyVoice();
	_xaudio2Instance->Release();
	delete _voiceCallback;

	for (size_t i = 0; i < _numOutputBuffers; ++i)
	{
		delete[] _outputBuffers[i];
	}
	delete[] _outputBuffers;
}

void XAudio2Platform::Reset()
{
	std::unique_lock<std::mutex> lock(_mutex);

	_xaudio2SourceVoice->Stop(0);
	_xaudio2SourceVoice->FlushSourceBuffers();

	_writeIndex = 0;
	_readIndex = 0;
	_overlapped = false;
	_currentBufferOffset = 0;

	_cv.notify_all();
}

 uint32_t XAudio2Platform::GetNumPendingSamples()
{
	std::unique_lock<std::mutex> lock(_mutex);

	return _currentBufferOffset;
}

void XAudio2Platform::SubmitSample(float sample)
{
	std::unique_lock<std::mutex> lock(_mutex);

	float* buffer = _outputBuffers[_writeIndex];
	buffer[_currentBufferOffset++] = sample;

	if (_currentBufferOffset == _bufferSize)
	{
		XAUDIO2_BUFFER xaudio2Buffer = { 0 };
		xaudio2Buffer.AudioBytes = _currentBufferOffset * sizeof(float);
		xaudio2Buffer.pAudioData = static_cast<BYTE*>(static_cast<void*>(buffer));
		_xaudio2SourceVoice->SubmitSourceBuffer(&xaudio2Buffer);

		XAUDIO2_VOICE_STATE state;
		_xaudio2SourceVoice->GetState(&state);
		if (state.BuffersQueued >= _numOutputBuffers - 1)
		{
			_xaudio2SourceVoice->Start(0);
		}

		_writeIndex++;
		if (_writeIndex >= _numOutputBuffers)
		{
			_overlapped = true;
			_writeIndex = 0;
		}

		if (_writeIndex == _readIndex && _overlapped)
		{
			_cv.wait(lock);
		}

		_currentBufferOffset = 0;
	}
}

uint32_t XAudio2Platform::GetSampleRate()
{
	return _sampleRate;
}

#endif