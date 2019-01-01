#if defined(_WIN32)

#include "xaudio2_platform.h"
#include <string>

#pragma comment(lib, "XAudio2.lib")

XAudio2Platform::XAudio2Platform()
	: _xaudio2Instance(nullptr)
	, _xaudio2MasteringVoice(nullptr)
	, _xaudio2SourceVoice(nullptr)
	, _sampleRate(0)
	, _currentBuffer(0)
	, _currentBufferOffset(0)
	, _numOutputBuffers(0)
	, _bufferSize(0)
	, _outputBuffers(nullptr)
{}

bool XAudio2Platform::Initialize(uint32_t sampleRate)
{
	_xaudio2Instance = nullptr;
	_xaudio2MasteringVoice = nullptr;
	_xaudio2SourceVoice = nullptr;
	_currentBufferOffset = 0;
	_currentBuffer = 0;
	_sampleRate = sampleRate;
	_bufferSize = _sampleRate / 60;
	_numOutputBuffers = 10;

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

	hr = _xaudio2Instance->CreateSourceVoice(&_xaudio2SourceVoice, &WaveFormat);
	if (FAILED(hr)) goto FailedExit;

	_outputBuffers = new float*[_numOutputBuffers];
	for (size_t i = 0; i < _numOutputBuffers; ++i)
	{
		_outputBuffers[i] = new float[_bufferSize];
	}

	_xaudio2SourceVoice->Start(0);

	return true;

FailedExit:
	if (_xaudio2SourceVoice) _xaudio2SourceVoice->DestroyVoice();
	if (_xaudio2MasteringVoice) _xaudio2MasteringVoice->DestroyVoice();
	if (_xaudio2Instance) _xaudio2Instance->Release();
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
	_xaudio2SourceVoice->Start(0);
	_currentBuffer = 0;
	_currentBufferOffset = 0;
}

void XAudio2Platform::Flush()
{
	std::unique_lock<std::mutex> lock(_mutex);

	PrivateFlush();
}

 uint32_t XAudio2Platform::GetNumPendingSamples()
{
	std::unique_lock<std::mutex> lock(_mutex);

	return _currentBufferOffset;
}

void XAudio2Platform::SubmitSample(float sample)
{
	std::unique_lock<std::mutex> lock(_mutex);

	float* buffer = _outputBuffers[_currentBuffer];
	buffer[_currentBufferOffset++] = sample;

	if (_currentBufferOffset == _bufferSize)
	{
		PrivateFlush();
	}
}

uint32_t XAudio2Platform::GetSampleRate()
{
	return _sampleRate;
}

void XAudio2Platform::PrivateFlush()
{
	float* Buffer = _outputBuffers[_currentBuffer];

	XAUDIO2_BUFFER xaudio2Buffer = { 0 };
	xaudio2Buffer.AudioBytes = _currentBufferOffset * sizeof(float);
	xaudio2Buffer.pAudioData = reinterpret_cast<BYTE*>(Buffer);
	_xaudio2SourceVoice->SubmitSourceBuffer(&xaudio2Buffer);

	_currentBuffer = (_currentBuffer + 1) % _numOutputBuffers;
	_currentBufferOffset = 0;
}

#endif