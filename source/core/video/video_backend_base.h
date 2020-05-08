#pragma once

#include <cstdint>
#include <string>

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
    bool _overscanEnabled{false};
    bool _showFps{false};

public:
    void SetOverscanEnabled(bool enabled) { _overscanEnabled = enabled; }

    void SetShowFps(bool show) { _showFps = show; }

    void SwapSettings(VideoBackendBase& other)
    {
        std::swap(_overscanEnabled, other._overscanEnabled);
        std::swap(_showFps, other._showFps);
    }
};
