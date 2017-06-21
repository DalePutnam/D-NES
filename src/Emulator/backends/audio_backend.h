#pragma once

#include <mutex>

#ifdef _WIN32
#include <xaudio2.h>
#elif __linux
#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>
#endif

class AudioBackend
{
public:
    AudioBackend();
    ~AudioBackend();
    void SetEnabled(bool enabled);
    void SetFiltersEnabled(bool enabled);
    void operator<<(float sample);

    static const uint32_t SampleRate;
private:
    // Butterworth filter implementation shamelessly stolen from
    // http://stackoverflow.com/questions/8079526/lowpass-and-high-pass-filter-in-c-sharp
    class Filter
    {
    public:
        Filter(int sampleRate, float frequency, float resonance, bool isLowPass);
        void Reset();
        float operator()(float sample);

    private:
        float c, a1, a2, a3, b1, b2;
        float InputHistory[2];
        float OutputHistory[2];
    };

    std::mutex Mutex;

    bool Enabled;
    bool FiltersEnabled;

#ifdef _WIN32
    static const uint32_t BufferSize;
    static const uint32_t NumBuffers;
    void InitializeXAudio2();
    void CleanUpXAudio2();
    void SetEnabledXAudio2(bool enabled);
    void ProcessSampleXAudio2(float sample);
    IXAudio2* XAudio2Instance;
    IXAudio2MasteringVoice* XAudio2MasteringVoice;
    IXAudio2SourceVoice* XAudio2SourceVoice;
    uint32_t BufferIndex;
    uint32_t CurrentBuffer;
    float** OutputBuffers;
#elif __linux
    void InitializeAlsa();
    void CleanUpAlsa();
    void SetEnabledAlsa(bool enabled);
    void ProcessSampleAlsa(float sample);
    snd_pcm_t* AlsaHandle;
    snd_pcm_uframes_t PeriodSize;
    snd_pcm_uframes_t BufferIndex;
    float* SampleBuffer;
#endif

    Filter HighPass90Hz;
    Filter HighPass440Hz;
    Filter LowPass14KHz;
};
