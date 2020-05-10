#include "nes_exception.h"
#include "error_handling.h"

#include "alsa_backend.h"
#include <cstring>

static constexpr uint32_t NUM_PERIODS = 4;
static constexpr uint32_t STANDARD_FRAME_RATE = 60;

void AlsaBackend::Initialize()
{
    int32_t rc;
    snd_pcm_hw_params_t* alsaHwParams = nullptr;
    snd_pcm_sw_params_t* alsaSwParams = nullptr;
    snd_pcm_uframes_t bufferSize, periodSize;

    _alsaHandle = nullptr;
    _numOutputBuffers = NUM_PERIODS;

    rc = snd_pcm_open(&_alsaHandle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (rc < 0) goto FailedExit;

    // Set hardware parameters

    rc = snd_pcm_hw_params_malloc(&alsaHwParams);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_hw_params_any(_alsaHandle, alsaHwParams);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_hw_params_set_access(_alsaHandle, alsaHwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_hw_params_set_format(_alsaHandle, alsaHwParams, SND_PCM_FORMAT_FLOAT);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_hw_params_set_rate(_alsaHandle, alsaHwParams, _sampleRate, 0);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_hw_params_set_channels(_alsaHandle, alsaHwParams, 2);
    if (rc < 0) goto FailedExit;

    // Target period size is a quarter frame
    periodSize = _sampleRate / STANDARD_FRAME_RATE / NUM_PERIODS;

    rc = snd_pcm_hw_params_set_period_size_near(_alsaHandle, alsaHwParams, &periodSize, nullptr);
    if (rc < 0) goto FailedExit;

    // Target buffer size is four periods (a whole frame)
    bufferSize = periodSize * NUM_PERIODS;

    rc = snd_pcm_hw_params_set_buffer_size_near(_alsaHandle, alsaHwParams, &bufferSize);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_hw_params(_alsaHandle, alsaHwParams);
    if (rc < 0) goto FailedExit;

    snd_pcm_hw_params_free(alsaHwParams);

    // Set software parameters

    rc = snd_pcm_sw_params_malloc(&alsaSwParams);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_sw_params_current(_alsaHandle, alsaSwParams);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_sw_params_set_start_threshold(_alsaHandle, alsaSwParams, bufferSize);
    if (rc < 0) goto FailedExit;

    rc = snd_pcm_sw_params(_alsaHandle, alsaSwParams);
    if (rc < 0) goto FailedExit;

    snd_pcm_sw_params_free(alsaSwParams);

    _sampleBufferSize = periodSize * 2;

    _outputBuffers = new float*[_numOutputBuffers];
    for (size_t i = 0; i < _numOutputBuffers; ++i)
    {
        _outputBuffers[i] = new float[_sampleBufferSize];
    }

    _sampleBufferIndex = 0;

    snd_pcm_prepare(_alsaHandle);

    _readIndex = 0;
    _writeIndex = 0;
    _overlapped = false;

    _running = true;
    _thread = std::thread(&AlsaBackend::StreamWorker, this);

    return;

FailedExit:
    if (_alsaHandle) snd_pcm_close(_alsaHandle);
    if (alsaHwParams) snd_pcm_hw_params_free(alsaHwParams);
    if (alsaSwParams) snd_pcm_sw_params_free(alsaSwParams);

    SPDLOG_LOGGER_ERROR(_nes.GetLogger(), "Failed to initialize ALSA: {}", snd_strerror(rc));
    throw NesException(ERROR_INITIALIZE_ALSA_FAILED);
}

void AlsaBackend::CleanUp() noexcept
{
    _running = false;
    _cv.notify_all();
    _thread.join();

    snd_pcm_drop(_alsaHandle);
    
    for (size_t i = 0; i < _numOutputBuffers; ++i)
    {
        delete[] _outputBuffers[i];
    }
    delete[] _outputBuffers;

    snd_pcm_close(_alsaHandle);
}

void AlsaBackend::Reset() noexcept
{
    std::unique_lock<std::mutex> lock(_mutex);

    snd_pcm_drop(_alsaHandle);

    _sampleBufferIndex = 0;

    _readIndex = 0;
    _writeIndex = 0;
    _overlapped = false;

    snd_pcm_prepare(_alsaHandle);

    _cv.notify_all();
}

void AlsaBackend::SubmitSample(float sample) noexcept
{
    std::unique_lock<std::mutex> lock(_mutex);

    float* buffer = _outputBuffers[_writeIndex];

    buffer[_sampleBufferIndex++] = sample;
    buffer[_sampleBufferIndex++] = sample;

    if (_sampleBufferIndex == _sampleBufferSize)
    {
        _writeIndex++;
        if (_writeIndex >= _numOutputBuffers)
        {
            _overlapped = true;
            _writeIndex = 0;
        }

        _cv.notify_all();

        if (_writeIndex == _readIndex && _overlapped)
        {
            _cv.wait(lock);
        }

        _sampleBufferIndex = 0;
    }
}

void AlsaBackend::StreamWorker()
{
    float* localBuffer = new float[_sampleBufferSize];

    while (_running)
    {
        {
            std::unique_lock<std::mutex> lock(_mutex);

            if (_writeIndex == _readIndex && !_overlapped)
            {
                _cv.wait(lock);

                if (!_running)
                {
                    break;
                }

                if (_writeIndex == _readIndex && !_overlapped)
                {
                    continue;
                }
            }

            memcpy(localBuffer, _outputBuffers[_readIndex], _sampleBufferSize * sizeof(float));

            _readIndex++;
            if (_readIndex >= _numOutputBuffers)
            {
                _overlapped = false;
                _readIndex = 0;
            }
            
            _cv.notify_all();
        }

        int rc = snd_pcm_writei(_alsaHandle, localBuffer, _sampleBufferSize / 2);
        if (rc == -EPIPE)
        {
            snd_pcm_prepare(_alsaHandle);
        }
    }

    delete [] localBuffer;
}
