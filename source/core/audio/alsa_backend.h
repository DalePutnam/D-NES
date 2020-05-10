#pragma once

#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <alsa/asoundlib.h>

#include "audio_backend_base.h"

class AlsaBackend : public AudioBackendBase
{
public:
    AlsaBackend(NES& nes): AudioBackendBase(nes) {}
    ~AlsaBackend() = default;

    virtual void Initialize() override;
    virtual void CleanUp() noexcept override;
    virtual void Reset() noexcept override;
    virtual void SubmitSample(float sample) noexcept override;
private:
    void StreamWorker();

    snd_pcm_t* _alsaHandle;
    uint32_t _sampleBufferSize;
    uint32_t _sampleBufferIndex;

    uint32_t _numOutputBuffers;
    uint32_t _writeIndex;
    uint32_t _readIndex;
    bool _overlapped;
    float** _outputBuffers;

    std::mutex _mutex;
    std::condition_variable _cv;
    std::thread _thread;

    std::atomic<bool> _running;
};