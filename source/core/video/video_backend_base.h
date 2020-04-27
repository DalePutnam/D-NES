#pragma once

#include <cstdint>
#include <string>

#include "video_manager.h"

class VideoBackendBase
{
public:
    VideoBackendBase(VideoManager& videoOutput)
        : _videoOutput(videoOutput) {} 
    virtual ~VideoBackendBase() = default;

    virtual int Prepare() = 0;
    virtual void Finalize() = 0;

    virtual void SubmitFrame(uint8_t* frameBuffer) = 0;
    virtual void ShowMessage(const std::string& message, uint32_t duration) = 0;

protected:
    VideoManager& GetVideoOutput() { return _videoOutput; }

private:
    VideoManager& _videoOutput;
};
