#pragma once

#include <cstdint>

#include "../cart.h"

class CPU;

class MapperBase
{
public:
    MapperBase(const std::string& fileName, const std::string& saveDir);
    virtual ~MapperBase();

    virtual const std::string& GetGameName() final;
    virtual void AttachCPU(CPU* cpu) final;

    virtual Cart::MirrorMode GetMirrorMode() = 0;

    virtual uint8_t PrgRead(uint16_t address) = 0;
    virtual void PrgWrite(uint8_t M, uint16_t address) = 0;

    virtual uint8_t ChrRead(uint16_t address) = 0;
    virtual void ChrWrite(uint8_t M, uint16_t address) = 0;

protected:
    int prgSize;
    const uint8_t* prg;
    int chrSize;
    const uint8_t* chr;
    uint8_t* chrRam;
    int wramSize;
    uint8_t* wram;
    bool hasSaveMem;

    Cart::MirrorMode mirroring;

    std::string gameName;
    const std::string saveDir;

    CPU* cpu;
};