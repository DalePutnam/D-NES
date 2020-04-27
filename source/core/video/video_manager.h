#pragma once

#include <cstdint>
#include <string>
#include <memory>

class VideoBackendBase;

class VideoManager
{
public:
    VideoManager();

    void SetBackend(std::unique_ptr<VideoBackendBase>&& backend);

    int Prepare();
    void Finalize();

    void SubmitFrame(uint8_t* frameBuffer);
    void ShowMessage(const std::string& message, uint32_t duration);

public:
    void SetOverscanEnabled(bool enabled);
    bool GetOverscanEnabled();

    void SetShowFps(bool showFps);
    bool GetShowFps();

    uint32_t GetCurrentFps();

private:
    bool _overscanEnabled{false};
    bool _showFps{false};
    uint32_t _currentFps{0};

    std::unique_ptr<VideoBackendBase> _backend;
};