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
    int PrgSize;
    const uint8_t* Prg;
    int ChrSize;
    const uint8_t* Chr;
    uint8_t* ChrRam;
    int WramSize;
    uint8_t* Wram;
    bool HasSaveMem;

    Cart::MirrorMode Mirroring;

    std::string GameName;
    const std::string SaveDir;

    CPU* Cpu;
};