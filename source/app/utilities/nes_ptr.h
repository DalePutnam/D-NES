#pragma once

#include <memory>

#include <dnes/dnes.h>

class NESDeleter
{
public:
    void operator()(dnes::INES* nes)
    {
        dnes::DestroyNES(nes);
    }
};

using NESPtr = std::unique_ptr<dnes::INES, NESDeleter>;