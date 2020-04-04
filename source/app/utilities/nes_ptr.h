#pragma once

#include <memory>

#include <dnes/dnes.h>

class NESDeleter
{
public:
    void operator()(dnes::NES* nes)
    {
        dnes::DestroyNES(nes);
    }
};

using NESPtr = std::unique_ptr<dnes::NES, NESDeleter>;