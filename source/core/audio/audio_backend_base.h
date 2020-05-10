#pragma once

#include <cstdint>
#include <string>

#include "nes.h"

class AudioBackendBase
{
public:
    AudioBackendBase(NES& nes): _nes(nes) {};
    virtual ~AudioBackendBase() = default;

    virtual void Initialize() = 0;
    virtual void CleanUp() noexcept = 0;

    virtual void Reset() noexcept = 0;
    virtual void SubmitSample(float sample) noexcept = 0;

    static constexpr int DEFAULT_SAMPLE_RATE = 44100;

protected:
    NES& _nes;
    uint32_t _sampleRate{DEFAULT_SAMPLE_RATE};

public:
    void SetSampleRate(uint32_t sampleRate) noexcept { _sampleRate = sampleRate; }
    uint32_t GetSampleRate() noexcept { return _sampleRate; }

    void SwapSettings(AudioBackendBase& other) noexcept
    {
        std::swap(_sampleRate, other._sampleRate);
    }
};
