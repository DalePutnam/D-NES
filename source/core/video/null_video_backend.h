#pragma once

#include "video_backend_base.h"

class NullVideoBackend final : public VideoBackendBase
{
public:
    NullVideoBackend(VideoManager& videoOutput)
        : VideoBackendBase(videoOutput)
    {}

    int Prepare() { return 0; }
    void Finalize() {}

    void SubmitFrame(uint8_t* frameBuffer) {}
    void ShowMessage(const std::string& message, uint32_t duration) {}
};