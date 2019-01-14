#pragma once

#include <exception>

class NESCallback
{
public:
    virtual void OnFrameComplete() = 0;
    virtual void OnError(std::exception_ptr eptr) = 0;

    virtual ~NESCallback() = default;
};
