#include "video_manager.h"
#include "video_backend_base.h"
#include "null_video_backend.h"


VideoManager::VideoManager()
    : _backend(std::make_unique<NullVideoBackend>(*this))
{}

void VideoManager::SetBackend(std::unique_ptr<VideoBackendBase>&& backend)
{
    if (backend == nullptr)
    {
        _backend = std::make_unique<NullVideoBackend>(*this);
    }

    _backend = std::move(backend);
}

int VideoManager::Prepare()
{
    return _backend->Prepare();
}

void VideoManager::Finalize()
{
    return _backend->Finalize();
}

void VideoManager::SubmitFrame(uint8_t* frameBuffer)
{
    return _backend->SubmitFrame(frameBuffer);
}

void VideoManager::ShowMessage(const std::string& message, uint32_t duration)
{
    return _backend->ShowMessage(message, duration);
}

void VideoManager::SetOverscanEnabled(bool enabled)
{
    _overscanEnabled = enabled;
}

bool VideoManager::GetOverscanEnabled()
{
    return _overscanEnabled;
}

void VideoManager::SetShowFps(bool enabled)
{
    _showFps = enabled;
}

bool VideoManager::GetShowFps()
{
    return _showFps;
}

uint32_t VideoManager::GetCurrentFps()
{
    return _currentFps;
}
