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
    
    void Reset();
	void Flush();
    void SetEnabled(bool enabled);
    void SetFiltersEnabled(bool enabled);
	int GetSampleRate();

    AudioBackend& operator<<(float sample);

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
	int SampleRate;

#ifdef _WIN32
    void InitializeXAudio2(int requestedSampleRate);
    void CleanUpXAudio2();
    void ResetXAudio2();
	void FlushXAudio2Samples();
    void SetEnabledXAudio2(bool enabled);
    void ProcessSampleXAudio2(float sample);
    IXAudio2* XAudio2Instance;
    IXAudio2MasteringVoice* XAudio2MasteringVoice;
    IXAudio2SourceVoice* XAudio2SourceVoice;
    uint32_t BufferIndex;
    uint32_t CurrentBuffer;
    float** OutputBuffers;
	int NumOutputBuffers;
	IXAudio2VoiceCallback* XAudio2VoiceCallback;
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

    std::unique_ptr<Filter> HighPass90Hz;
	std::unique_ptr<Filter> HighPass440Hz;
	std::unique_ptr<Filter> LowPass14KHz;
};
