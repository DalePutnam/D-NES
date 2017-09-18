#include <cmath>
#include <iostream>

#include "audio_backend.h"

//**********************************************************************
// Audio Backend
//**********************************************************************

const uint32_t AudioBackend::SampleRate = 44100;

AudioBackend::AudioBackend()
    : Enabled(true)
    , FiltersEnabled(false)
    , HighPass90Hz(SampleRate, 90.0f, 1.0f, false)
    , HighPass440Hz(SampleRate, 440.0f, 1.0f, false)
    , LowPass14KHz(SampleRate, 14000.0f, 1.0f, true)
{
#ifdef _WIN32
    InitializeXAudio2();
#elif __linux
    InitializeAlsa();
#endif
}

AudioBackend::~AudioBackend()
{
#ifdef _WIN32
    CleanUpXAudio2();
#elif __linux
    CleanUpAlsa();
#endif
}

void AudioBackend::Flush()
{
    std::unique_lock<std::mutex> lock(Mutex);

#ifdef _WIN32
    FlushXAudio2();
#elif defined(__linux)
    FlushAlsa();
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
        HighPass90Hz.Reset();
        HighPass440Hz.Reset();
        LowPass14KHz.Reset();
    }

    FiltersEnabled = enabled;
}

void AudioBackend::operator<<(float sample)
{
    std::unique_lock<std::mutex> lock(Mutex);

    if (Enabled)
    {
        if (FiltersEnabled)
        {
            sample = LowPass14KHz(HighPass440Hz(HighPass90Hz(sample)));
        }

#ifdef _WIN32
        ProcessSampleXAudio2(sample);
#elif __linux
        ProcessSampleAlsa(sample);
#endif
    }
}

#ifdef _WIN32
const uint32_t AudioBackend::BufferSize = 128;
const uint32_t AudioBackend::NumBuffers = SampleRate / BufferSize;

void AudioBackend::InitializeXAudio2()
{
    XAudio2Instance = nullptr;
    XAudio2MasteringVoice = nullptr;
    XAudio2SourceVoice = nullptr;
    BufferIndex = 0;
    CurrentBuffer = 0;

    WAVEFORMATEX WaveFormat = { 0 };
    WaveFormat.nChannels = 1;
    WaveFormat.nSamplesPerSec = SampleRate;
    WaveFormat.wBitsPerSample = sizeof(float) * 8;
    WaveFormat.nAvgBytesPerSec = SampleRate * sizeof(float);
    WaveFormat.nBlockAlign = sizeof(float);
    WaveFormat.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;

    HRESULT hr;
    hr = XAudio2Create(&XAudio2Instance, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr)) goto FailedExit;

    hr = XAudio2Instance->CreateMasteringVoice(&XAudio2MasteringVoice);
    if (FAILED(hr)) goto FailedExit;

    hr = XAudio2Instance->CreateSourceVoice(&XAudio2SourceVoice, &WaveFormat);
    if (FAILED(hr)) goto FailedExit;

    OutputBuffers = new float*[NumBuffers];
    for (size_t i = 0; i < NumBuffers; ++i)
    {
        OutputBuffers[i] = new float[BufferSize];
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

    for (size_t i = 0; i < NumBuffers; ++i)
    {
        delete[] OutputBuffers[i];
    }
    delete[] OutputBuffers;
}

void AudioBackend::FlushXAudio2()
{
    XAudio2SourceVoice->Stop(0);
    XAudio2SourceVoice->FlushSourceBuffers();
    XAudio2SourceVoice->Start(0);
    CurrentBuffer = 0;
    BufferIndex = 0;
}

void AudioBackend::SetEnabledXAudio2(bool enabled)
{
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

    if (BufferIndex == BufferSize)
    {
        XAUDIO2_BUFFER XAudio2Buffer = { 0 };
        XAudio2Buffer.AudioBytes = BufferSize * sizeof(float);
        XAudio2Buffer.pAudioData = reinterpret_cast<BYTE*>(Buffer);
        XAudio2SourceVoice->SubmitSourceBuffer(&XAudio2Buffer);

        CurrentBuffer = (CurrentBuffer + 1) % NumBuffers;
        BufferIndex = 0;
    }
}

#elif __linux
void AudioBackend::InitializeAlsa()
{
    int rc, dir;
    snd_pcm_hw_params_t* AlsaHwParams = nullptr;
    snd_pcm_sw_params_t* AlsaSwParams = nullptr;
    snd_pcm_uframes_t bufferSize;

    AlsaHandle = nullptr;
    PeriodSize = 128;
    BufferIndex = 0;

    rc = snd_pcm_open(&AlsaHandle, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_hw_params_malloc(&AlsaHwParams);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_hw_params_any(AlsaHandle, AlsaHwParams);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_hw_params_set_access(AlsaHandle, AlsaHwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_hw_params_set_format(AlsaHandle, AlsaHwParams, SND_PCM_FORMAT_FLOAT_LE);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_hw_params_set_channels(AlsaHandle, AlsaHwParams, 1);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_hw_params_set_rate(AlsaHandle, AlsaHwParams, SampleRate, 0);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_hw_params_set_period_size_min(AlsaHandle, AlsaHwParams, &PeriodSize, 0);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_hw_params_set_period_size_first(AlsaHandle, AlsaHwParams, &PeriodSize, &dir);
    if (rc < 0) goto FailedExit;

    bufferSize = SampleRate / 30; // Two (video) frames worth of audio data
    rc = snd_pcm_hw_params_set_buffer_size_min(AlsaHandle, AlsaHwParams, &bufferSize);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_hw_params_set_buffer_size_first(AlsaHandle, AlsaHwParams, &bufferSize);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_hw_params(AlsaHandle, AlsaHwParams);
    if (rc < 0) goto FailedExit;

    snd_pcm_hw_params_free(AlsaHwParams);

    SampleBuffer = new float[PeriodSize];

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
    BufferIndex = 0;
    snd_pcm_drop(AlsaHandle);
    snd_pcm_prepare(AlsaHandle);
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
        BufferIndex = 0;
    }
}

void AudioBackend::ProcessSampleAlsa(float sample)
{
    SampleBuffer[BufferIndex++] = sample;
    if (BufferIndex == PeriodSize)
    {
        uint64_t frames = 0;
        while (frames < PeriodSize)
        {
            int rc = snd_pcm_writei(AlsaHandle, SampleBuffer + (frames*sizeof(float)), PeriodSize - frames);
            if (rc == -EPIPE)
            {
                snd_pcm_prepare(AlsaHandle);
                rc = snd_pcm_writei(AlsaHandle, SampleBuffer + (frames*sizeof(float)), PeriodSize - frames);
            }

            frames += rc;
        }

        BufferIndex = 0;
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
