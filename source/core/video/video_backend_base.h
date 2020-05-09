#pragma once

#include <cstdint>
#include <string>
#include <atomic>

class VideoBackendBase
{
public:
    VideoBackendBase() = default;
    virtual ~VideoBackendBase() = default;

    virtual int Prepare() = 0;
    virtual void Finalize() = 0;

    virtual void SubmitFrame(uint8_t* frameBuffer) = 0;
    virtual void ShowMessage(const std::string& message, uint32_t duration) = 0;

protected:
    std::atomic<bool> _overscanEnabled{false};
    std::atomic<bool> _showFps{false};

public:
    void SetOverscanEnabled(bool enabled) { _overscanEnabled = enabled; }

    void SetShowFps(bool show) { _showFps = show; }

    void SwapSettings(VideoBackendBase& other)
    {
        _overscanEnabled.exchange(other._overscanEnabled);
        _showFps.exchange(other._showFps);
    }
};
