#pragma once

#include <cstdint>
#include <fstream>
#include <mutex>

#include "cart.h"
#include "ines.h"
#include "state_save.h"

class CPU;
class PPU;

class MapperBase
{
public:
    MapperBase(iNesFile& file);
    virtual ~MapperBase();

    void AttachCPU(CPU* cpu);
    void AttachPPU(PPU* ppu);

    virtual void SaveNativeSave(std::ofstream& stream);
    virtual void LoadNativeSave(std::ifstream& stream);

    virtual void SaveState(StateSave::Ptr& state);
    virtual void LoadState(const StateSave::Ptr& state);

    Cart::MirrorMode GetMirrorMode();

    virtual uint8_t PrgRead(uint16_t address) = 0;
    virtual void PrgWrite(uint8_t M, uint16_t address) = 0;

    virtual uint8_t ChrRead(uint16_t address) = 0;
    virtual void ChrWrite(uint8_t M, uint16_t address) = 0;

    virtual bool CheckIRQ();

protected:
    uint32_t _prgRomSize;
    uint32_t _chrRomSize;
    uint32_t _miscRomSize;
    uint32_t _prgRamSize;
    uint32_t _prgNvRamSize;
    uint32_t _chrRamSize;
    uint32_t _chrNvRamSize;

    bool _hasNonVolatileMemory;

    std::unique_ptr<uint8_t[]> _prgRom;
    std::unique_ptr<uint8_t[]> _chrRom;
    std::unique_ptr<uint8_t[]> _miscRom;
    std::unique_ptr<uint8_t[]> _prgRam;
    std::unique_ptr<uint8_t[]> _chrRam;

    Cart::MirrorMode _mirroring;

    CPU* _cpu;
    PPU* _ppu;
};
