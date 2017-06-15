#include <cmath>
#include <iostream>

#include "audio_backend.h"

//**********************************************************************
// Audio Backend
//**********************************************************************

const uint32_t AudioBackend::BufferSize = 256;

AudioBackend::AudioBackend()
    : Enabled(true)
    , FiltersEnabled(false)
    , SampleRate(44100)
    , BufferIndex(0)
    , CurrentBuffer(0)
#ifdef _WIN32
    , XAudio2Instance(nullptr)
    , XAudio2MasteringVoice(nullptr)
    , XAudio2SourceVoice(nullptr)
#elif __linux
    , AlsaHandle(nullptr)
#endif
{
#ifdef _WIN32
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

    XAudio2SourceVoice->Start(0);

#elif __linux
    int rc;
    snd_pcm_hw_params_t* AlsaHwParams = nullptr;
    snd_pcm_uframes_t periodSize, bufferSize;

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

    periodSize = BufferSize;
    rc = snd_pcm_hw_params_set_period_size_near(AlsaHandle, AlsaHwParams, &periodSize, 0);
    if (rc < 0) goto FailedExit;

    //bufferSize = BufferSize * 4;
    bufferSize = (SampleRate / 60);
    rc = snd_pcm_hw_params_set_buffer_size_min(AlsaHandle, AlsaHwParams, &bufferSize);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_hw_params_set_buffer_size_first(AlsaHandle, AlsaHwParams, &bufferSize);
    if (rc < 0) goto FailedExit;

    std::cout << bufferSize << std::endl;

    rc = snd_pcm_hw_params(AlsaHandle, AlsaHwParams);
    if (rc < 0) goto FailedExit;

    snd_pcm_hw_params_free(AlsaHwParams);

#endif

    HighPass90Hz = Filter(SampleRate, 90.0f, 1.0f, false);
    HighPass440Hz = Filter(SampleRate, 440.0f, 1.0f, false);
    LowPass14KHz = Filter(SampleRate, 14000.0f, 1.0f, true);

    NumBuffers = SampleRate / BufferSize;

    OutputBuffers = new float*[NumBuffers];
    for (size_t i = 0; i < NumBuffers; ++i)
    {
        OutputBuffers[i] = new float[BufferSize];
    }

    return;

FailedExit:
#ifdef _WIN32
    if (XAudio2SourceVoice) XAudio2SourceVoice->DestroyVoice();
    if (XAudio2MasteringVoice) XAudio2MasteringVoice->DestroyVoice();
    if (XAudio2Instance) XAudio2Instance->Release();

    throw std::runtime_error("APU: Failed to initialize XAudio2");
#elif __linux
    if (AlsaHandle) snd_pcm_close(AlsaHandle);
    if (AlsaHwParams) snd_pcm_hw_params_free(AlsaHwParams);

    throw std::runtime_error("APU: Failed to initialize ALSA");
#endif
}

AudioBackend::~AudioBackend()
{
#ifdef _WIN32
    XAudio2SourceVoice->Stop();
    XAudio2SourceVoice->FlushSourceBuffers();

    XAudio2SourceVoice->DestroyVoice();
    XAudio2MasteringVoice->DestroyVoice();
    XAudio2Instance->Release();
#elif __linux
    snd_pcm_drop(AlsaHandle);
    snd_pcm_close(AlsaHandle);
#endif

    for (size_t i = 0; i < NumBuffers; ++i)
    {
        delete[] OutputBuffers[i];
    }
    delete[] OutputBuffers;
}

uint32_t AudioBackend::GetSampleRate()
{
    return SampleRate;
}

void AudioBackend::SetEnabled(bool enabled)
{
    std::unique_lock<std::mutex> lock(Mutex);

    if (!enabled && Enabled)
    {
#ifdef _WIN32
        XAudio2SourceVoice->Stop(0);
        XAudio2SourceVoice->FlushSourceBuffers();
#elif __linux
        snd_pcm_drop(AlsaHandle);
#endif
    }
    else if (enabled && !Enabled)
    {
#ifdef _WIN32
        XAudio2SourceVoice->Start(0);
#elif __linux
        snd_pcm_prepare(AlsaHandle);
#endif
        CurrentBuffer = 0;
        BufferIndex = 0;
    }

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

        float* Buffer = OutputBuffers[CurrentBuffer];
        Buffer[BufferIndex++] = sample;

        if (BufferIndex == BufferSize)
        {
#ifdef _WIN32
            XAUDIO2_BUFFER XAudio2Buffer = { 0 };
            XAudio2Buffer.AudioBytes = BufferSize * sizeof(float);
            XAudio2Buffer.pAudioData = reinterpret_cast<BYTE*>(Buffer);
            XAudio2SourceVoice->SubmitSourceBuffer(&XAudio2Buffer);
#elif __linux
            int rc;
            do
            {
                rc = snd_pcm_writei(AlsaHandle, reinterpret_cast<void*>(Buffer), BufferSize);
                if (rc == -EPIPE)
                {
                    snd_pcm_prepare(AlsaHandle);
                    rc = snd_pcm_writei(AlsaHandle, reinterpret_cast<void*>(Buffer), BufferSize);
                }
            } while (rc == -EAGAIN);
#endif
            CurrentBuffer = (CurrentBuffer + 1) % NumBuffers;
            BufferIndex = 0;
        }
    }
}

//**********************************************************************
// Butterworth filter
//**********************************************************************

AudioBackend::Filter::Filter()
{
}

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
