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
    
    void Flush();
    void SetEnabled(bool enabled);
    void SetFiltersEnabled(bool enabled);
    void operator<<(float sample);

    static constexpr uint32_t SAMPLE_RATE = 44100;
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
    void InitializeXAudio2();
    void CleanUpXAudio2();
    void FlushXAudio2();
    void SetEnabledXAudio2(bool enabled);
    void ProcessSampleXAudio2(float sample);
    IXAudio2* XAudio2Instance;
    IXAudio2MasteringVoice* XAudio2MasteringVoice;
    IXAudio2SourceVoice* XAudio2SourceVoice;
    uint32_t BufferIndex;
    uint32_t CurrentBuffer;
    float** OutputBuffers;

    static constexpr uint32_t BUFFER_SIZE = 147;
    static constexpr uint32_t NUM_BUFFERS = SAMPLE_RATE / BUFFER_SIZE;
#elif __linux
    void InitializeAlsa();
    void CleanUpAlsa();
    void FlushAlsa();
    void SetEnabledAlsa(bool enabled);
    void ProcessSampleAlsa(float sample);

    snd_pcm_t* AlsaHandle;
    uint32_t SampleBufferSize;
    uint32_t SampleBufferIndex;
    float* SampleBuffer;
    static constexpr uint32_t MAX_BUFFER_SIZE = 2048;
    static constexpr uint32_t MIN_FRAME_SIZE = 256;
#endif

    Filter HighPass90Hz;
    Filter HighPass440Hz;
    Filter LowPass14KHz;
};
