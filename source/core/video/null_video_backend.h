#pragma once

#include "video_backend_base.h"

class NullVideoBackend final : public VideoBackendBase
{
public:
    NullVideoBackend(NES& nes): VideoBackendBase(nes) {}
    ~NullVideoBackend() = default;

    void Initialize() {}
    void CleanUp() noexcept {}

    void SubmitFrame(uint8_t* frameBuffer) noexcept {}
    void ShowMessage(const std::string& message, uint32_t duration) noexcept {}
};