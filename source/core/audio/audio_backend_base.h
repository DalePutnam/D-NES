#pragma once

#include <cstdint>
#include <string>

class AudioBackendBase
{
public:
    AudioBackendBase() = default;
    virtual ~AudioBackendBase() = default;

    virtual void Initialize() = 0;
    virtual void CleanUp() = 0;

    virtual void Reset() = 0;
    virtual void SubmitSample(float sample) = 0;

    static constexpr int DEFAULT_SAMPLE_RATE = 44100;

protected:
    uint32_t _sampleRate{DEFAULT_SAMPLE_RATE};

public:
    void SetSampleRate(uint32_t sampleRate) { _sampleRate = sampleRate; }
    uint32_t GetSampleRate() { return _sampleRate; }

    void SwapSettings(AudioBackendBase& other)
    {
        std::swap(_sampleRate, other._sampleRate);
    }
};
