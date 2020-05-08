#pragma once

#include "audio_backend_base.h"

class NullAudioBackend : public AudioBackendBase
{
public:
    NullAudioBackend() = default;
    ~NullAudioBackend() = default;

    virtual void Initialize() override {};
    virtual void CleanUp() override {};
    virtual void Reset() override {};
    virtual void SubmitSample(float sample) override {};
};