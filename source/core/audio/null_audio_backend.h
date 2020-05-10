#pragma once

#include "audio_backend_base.h"

class NullAudioBackend : public AudioBackendBase
{
public:
    NullAudioBackend(NES& nes): AudioBackendBase(nes) {};
    ~NullAudioBackend() = default;

    virtual void Initialize() override {};
    virtual void CleanUp() noexcept override {};
    virtual void Reset() noexcept override {};
    virtual void SubmitSample(float sample) noexcept override {};
};