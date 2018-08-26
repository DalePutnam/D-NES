#include <cmath>
#include <iostream>
#include <string>

#include "audio_backend.h"

#if defined(_WIN32)
#pragma comment(lib, "XAudio2.lib")
#endif

//**********************************************************************
// Audio Backend
//**********************************************************************

namespace
{
constexpr uint32_t DEFAULT_SAMPLE_RATE = 44100;
}

AudioBackend::AudioBackend()
    : Enabled(true)
    , FiltersEnabled(false)
{
#ifdef _WIN32
    InitializeXAudio2(DEFAULT_SAMPLE_RATE);
#elif __linux
    InitializeAlsa();
#endif

	HighPass90Hz = std::unique_ptr<Filter>(new Filter(GetSampleRate(), 90.0f, 1.0f, false));
	HighPass440Hz = std::unique_ptr<Filter>(new Filter(GetSampleRate(), 440.0f, 1.0f, false));
	LowPass14KHz = std::unique_ptr<Filter>(new Filter(GetSampleRate(), 14000.0f, 1.0f, true));
}

AudioBackend::~AudioBackend()
{
#ifdef _WIN32
    CleanUpXAudio2();
#elif __linux
    CleanUpAlsa();
#endif
}

void AudioBackend::Reset()
{
    std::unique_lock<std::mutex> lock(Mutex);

#ifdef _WIN32
    ResetXAudio2();
#elif defined(__linux)
    FlushAlsa();
#endif
}

void AudioBackend::Flush()
{
	std::unique_lock<std::mutex> lock(Mutex);

#ifdef _WIN32
	FlushXAudio2Samples();
#endif
}

void AudioBackend::SetEnabled(bool enabled)
{
    std::unique_lock<std::mutex> lock(Mutex);

#ifdef _WIN32
    SetEnabledXAudio2(enabled);
#elif __linux
    SetEnabledAlsa(enabled);
#endif

    Enabled = enabled;
}

void AudioBackend::SetFiltersEnabled(bool enabled)
{
    std::unique_lock<std::mutex> lock(Mutex);

    if (enabled && !FiltersEnabled)
    {
        HighPass90Hz->Reset();
        HighPass440Hz->Reset();
        LowPass14KHz->Reset();
    }

    FiltersEnabled = enabled;
}

int AudioBackend::GetSampleRate()
{
	return SampleRate;
}

AudioBackend& AudioBackend::operator<<(float sample)
{
    std::unique_lock<std::mutex> lock(Mutex);

    if (Enabled)
    {
        if (FiltersEnabled)
        {
            sample = (*LowPass14KHz)((*HighPass440Hz)((*HighPass90Hz)(sample)));
        }

#ifdef _WIN32
        ProcessSampleXAudio2(sample);
#elif __linux
        ProcessSampleAlsa(sample);
#endif
    }

	return *this;
}

#ifdef _WIN32
class VoiceCallback : public IXAudio2VoiceCallback
{
public:
	VoiceCallback() {}

	void OnStreamEnd() { }

	//Unused methods are stubs
	void OnVoiceProcessingPassEnd() { }
	void OnVoiceProcessingPassStart(UINT32 SamplesRequired) { }
	void OnBufferEnd(void * pBufferContext) { }
	void OnBufferStart(void * pBufferContext) { }
	void OnLoopEnd(void * pBufferContext) { }
	void OnVoiceError(void * pBufferContext, HRESULT Error) { }
};

namespace
{
constexpr int BUFFER_SIZE = 147;
}

void AudioBackend::InitializeXAudio2(int requestedSampleRate)
{
    XAudio2Instance = nullptr;
    XAudio2MasteringVoice = nullptr;
    XAudio2SourceVoice = nullptr;
    BufferIndex = 0;
    CurrentBuffer = 0;
	SampleRate = requestedSampleRate;
	NumOutputBuffers = (SampleRate / BUFFER_SIZE) * 2;

    WAVEFORMATEX WaveFormat = { 0 };
    WaveFormat.nChannels = 1;
	WaveFormat.nSamplesPerSec = SampleRate;
    WaveFormat.wBitsPerSample = sizeof(float) * 8;
    WaveFormat.nAvgBytesPerSec = SampleRate * sizeof(float);
	WaveFormat.nBlockAlign = sizeof(float);
    WaveFormat.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;

    HRESULT hr;
    hr = XAudio2Create(&XAudio2Instance);
    if (FAILED(hr)) goto FailedExit;

    hr = XAudio2Instance->CreateMasteringVoice(&XAudio2MasteringVoice, 1, SampleRate);
    if (FAILED(hr)) goto FailedExit;

	XAudio2VoiceCallback = new VoiceCallback();
    hr = XAudio2Instance->CreateSourceVoice(&XAudio2SourceVoice, &WaveFormat, 0, XAUDIO2_DEFAULT_FREQ_RATIO, XAudio2VoiceCallback);
    if (FAILED(hr)) goto FailedExit;

    OutputBuffers = new float*[NumOutputBuffers];
    for (size_t i = 0; i < NumOutputBuffers; ++i)
    {
        OutputBuffers[i] = new float[BUFFER_SIZE];
    }

    XAudio2SourceVoice->Start(0);

    return;

FailedExit:
    if (XAudio2SourceVoice) XAudio2SourceVoice->DestroyVoice();
    if (XAudio2MasteringVoice) XAudio2MasteringVoice->DestroyVoice();
    if (XAudio2Instance) XAudio2Instance->Release();
    throw std::runtime_error("APU: Failed to initialize XAudio2");
}

void AudioBackend::CleanUpXAudio2()
{
    XAudio2SourceVoice->Stop();
    XAudio2SourceVoice->FlushSourceBuffers();
    XAudio2SourceVoice->DestroyVoice();
    XAudio2MasteringVoice->DestroyVoice();
    XAudio2Instance->Release();

    for (size_t i = 0; i < NumOutputBuffers; ++i)
    {
        delete[] OutputBuffers[i];
    }
    delete[] OutputBuffers;

	delete XAudio2VoiceCallback;
}

void AudioBackend::ResetXAudio2()
{
    XAudio2SourceVoice->Stop(0);
    XAudio2SourceVoice->FlushSourceBuffers();
    XAudio2SourceVoice->Start(0);
    CurrentBuffer = 0;
    BufferIndex = 0;
}

void AudioBackend::FlushXAudio2Samples()
{
	XAUDIO2_VOICE_STATE state;
	XAudio2SourceVoice->GetState(&state);
	OutputDebugString(std::to_string(state.BuffersQueued).c_str());
	OutputDebugString("\n");

	float* Buffer = OutputBuffers[CurrentBuffer];

	XAUDIO2_BUFFER XAudio2Buffer = { 0 };
	XAudio2Buffer.AudioBytes = BufferIndex * sizeof(float);
	XAudio2Buffer.pAudioData = reinterpret_cast<BYTE*>(Buffer);
	XAudio2SourceVoice->SubmitSourceBuffer(&XAudio2Buffer);

	CurrentBuffer = (CurrentBuffer + 1) % NumOutputBuffers;
	BufferIndex = 0;
}

void AudioBackend::SetEnabledXAudio2(bool enabled)
{
	return;

    if (!enabled && Enabled)
    {
        XAudio2SourceVoice->Stop(0);
        XAudio2SourceVoice->FlushSourceBuffers();
    }
    else if (enabled && !Enabled)
    {
        XAudio2SourceVoice->Start(0);
        CurrentBuffer = 0;
        BufferIndex = 0;
    }
}

void AudioBackend::ProcessSampleXAudio2(float sample)
{
    float* Buffer = OutputBuffers[CurrentBuffer];
    Buffer[BufferIndex++] = sample;

    if (BufferIndex == BUFFER_SIZE)
    {
		FlushXAudio2Samples();
    }
}

#elif __linux
void AudioBackend::InitializeAlsa()
{
    int32_t rc, dir;
    snd_pcm_hw_params_t* AlsaHwParams = nullptr;
    snd_pcm_sw_params_t* AlsaSwParams = nullptr;
    snd_pcm_uframes_t bufferSize, frameSize;
    uint32_t numPeriods, sampleRate;

    AlsaHandle = nullptr;
    bufferSize = MAX_BUFFER_SIZE;
    frameSize = MIN_FRAME_SIZE;
    sampleRate = SAMPLE_RATE;

    rc = snd_pcm_open(&AlsaHandle, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
    if (rc < 0) goto FailedExit;

    // Set hardware parameters

    rc = snd_pcm_hw_params_malloc(&AlsaHwParams);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_hw_params_any(AlsaHandle, AlsaHwParams);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_hw_params_set_access(AlsaHandle, AlsaHwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_hw_params_set_format(AlsaHandle, AlsaHwParams, SND_PCM_FORMAT_FLOAT);
    if (rc < 0) goto FailedExit;

    dir = 0;
    rc = snd_pcm_hw_params_set_rate_near(AlsaHandle, AlsaHwParams, &sampleRate, &dir);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_hw_params_set_channels(AlsaHandle, AlsaHwParams, 1);
    if (rc < 0) goto FailedExit;

    dir = 0;
    numPeriods = bufferSize / frameSize;
    rc = snd_pcm_hw_params_set_periods_max(AlsaHandle, AlsaHwParams, &numPeriods, &dir);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_hw_params_set_buffer_size_max(AlsaHandle, AlsaHwParams, &bufferSize);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_hw_params(AlsaHandle, AlsaHwParams);
    if (rc < 0) goto FailedExit;

    snd_pcm_hw_params_free(AlsaHwParams);

    // Get selected values for the buffer size and number of periods

    rc = snd_pcm_hw_params_get_buffer_size(AlsaHwParams, &bufferSize);
    if (rc < 0) goto FailedExit;

    dir = 0;
    rc = snd_pcm_hw_params_get_periods_max(AlsaHwParams, &numPeriods, &dir);
    if (rc < 0) goto FailedExit;

    // Set software parameters

    rc = snd_pcm_sw_params_malloc(&AlsaSwParams);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_sw_params_current(AlsaHandle, AlsaSwParams);
    if (rc < 0) goto FailedExit;

    // Set the start threshold to a 2 video frame delay (735 samples per frame * 2 = 1470)
    rc = snd_pcm_sw_params_set_start_threshold(AlsaHandle, AlsaSwParams, 1470);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_sw_params(AlsaHandle, AlsaSwParams);
    if (rc < 0) goto FailedExit;

    snd_pcm_sw_params_free(AlsaSwParams);

    SampleBufferSize = bufferSize / numPeriods;
    if (SampleBufferSize < MIN_FRAME_SIZE)
    {
        SampleBufferSize = MIN_FRAME_SIZE;
    }

    SampleBuffer = new float[SampleBufferSize];
    SampleBufferIndex = 0;

    snd_pcm_prepare(AlsaHandle);

    return;

FailedExit:
    if (AlsaHandle) snd_pcm_close(AlsaHandle);
    if (AlsaHwParams) snd_pcm_hw_params_free(AlsaHwParams);
    if (AlsaSwParams) snd_pcm_sw_params_free(AlsaSwParams);
    throw std::runtime_error(std::string("Failed to initialize ALSA. ") + snd_strerror(rc));
}

void AudioBackend::CleanUpAlsa()
{
    snd_pcm_drop(AlsaHandle);
    snd_pcm_close(AlsaHandle);

    delete [] SampleBuffer;
}

void AudioBackend::FlushAlsa()
{
    snd_pcm_drop(AlsaHandle);
    snd_pcm_prepare(AlsaHandle);
    SampleBufferIndex = 0;
}

void AudioBackend::SetEnabledAlsa(bool enabled)
{
    if (!enabled && Enabled)
    {
        snd_pcm_drop(AlsaHandle);
    }
    else if (enabled && !Enabled)
    {
        snd_pcm_prepare(AlsaHandle);
        SampleBufferIndex = 0;
    }
}

void AudioBackend::ProcessSampleAlsa(float sample)
{
    SampleBuffer[SampleBufferIndex++] = sample;
    if (SampleBufferIndex == SampleBufferSize)
    {
        SampleBufferIndex = 0;

        int rc = snd_pcm_writei(AlsaHandle, SampleBuffer, SampleBufferSize);
        if (rc == -EPIPE)
        {
            snd_pcm_prepare(AlsaHandle);
            snd_pcm_writei(AlsaHandle, SampleBuffer, SampleBufferSize);
        }
    }
}

#endif

//**********************************************************************
// Butterworth filter
//**********************************************************************

AudioBackend::Filter::Filter(int sampleRate, float frequency, float resonance, bool isLowPass)
{
    static constexpr float pi = 3.14159265f;

    if (isLowPass)
    {
        c = 1.0f / std::tan(pi * frequency / sampleRate);
        a1 = 1.0f / (1.0f + resonance * c + c * c);
        a2 = 2.0f * a1;
        a3 = a1;
        b1 = 2.0f * (1.0f - c * c) * a1;
        b2 = (1.0f - resonance * c + c * c) * a1;
    }
    else
    {
        c = std::tan(pi * frequency / sampleRate);
        a1 = 1.0f / (1.0f + resonance * c + c * c);
        a2 = -2.0f * a1;
        a3 = a1;
        b1 = 2.0f * (c * c - 1.0f) * a1;
        b2 = (1.0f - resonance * c + c * c) * a1;
    }

    memset(InputHistory, 0, sizeof(float) * 2);
    memset(OutputHistory, 0, sizeof(float) * 2);
}

void AudioBackend::Filter::Reset()
{
    memset(InputHistory, 0, sizeof(float) * 2);
    memset(OutputHistory, 0, sizeof(float) * 2);
}

float AudioBackend::Filter::operator()(float sample)
{
    float output = (a1 * sample) + (a2 * InputHistory[0]) + (a3 * InputHistory[1]) - (b1 * OutputHistory[0]) - (b2 * OutputHistory[1]);

    InputHistory[1] = InputHistory[0];
    InputHistory[0] = sample;

    OutputHistory[1] = OutputHistory[0];
    OutputHistory[0] = output;

    return output;
}
