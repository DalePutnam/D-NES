#pragma once

#include <cstdint>
#include <string>
#include <atomic>

#include "nes.h"

class VideoBackendBase
{
public:
    virtual ~VideoBackendBase() = default;

    virtual void Initialize() = 0;
    virtual void CleanUp() noexcept = 0;

    virtual void SubmitFrame(uint8_t* frameBuffer) noexcept = 0;
    virtual void ShowMessage(const std::string& message, uint32_t duration) noexcept = 0;

protected:
    NES& _nes;
    std::atomic<bool> _overscanEnabled{false};
    std::atomic<bool> _showFps{false};

public:
    VideoBackendBase(NES& nes): _nes(nes) {}

    void SetOverscanEnabled(bool enabled) noexcept { _overscanEnabled = enabled; }

    void SetShowFps(bool show) noexcept { _showFps = show; }

    void SwapSettings(VideoBackendBase& other) noexcept
    {
        _overscanEnabled.exchange(other._overscanEnabled);
        _showFps.exchange(other._showFps);
    }
};
