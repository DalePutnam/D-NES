#pragma once

#include <cstdint>
#include <fstream>
#include <mutex>

#include "ines.h"
#include "state_save.h"

class CPU;
class PPU;

class MapperBase
{
public:
    MapperBase(iNesFile& file);
    virtual ~MapperBase() = default;

    void AttachCPU(CPU* cpu);
    void AttachPPU(PPU* ppu);

    virtual void SaveNativeSave(std::ofstream& stream);
    virtual void LoadNativeSave(std::ifstream& stream);

    virtual void SaveState(StateSave::Ptr& state);
    virtual void LoadState(const StateSave::Ptr& state);

    virtual uint8_t CpuRead(uint16_t address) = 0;
    virtual void CpuWrite(uint8_t M, uint16_t address) = 0;

    virtual void SetPpuAddress(uint16_t address);
    virtual uint8_t PpuRead() = 0;
    virtual void PpuWrite(uint8_t M) = 0;

    virtual uint8_t PpuPeek(uint16_t address) = 0;

    virtual bool CheckIRQ();

protected:
    uint8_t DefaultNameTableRead(uint16_t address);
    void DefaultNameTableWrite(uint8_t M, uint16_t address);

    uint16_t _ppuAddress;

    uint32_t _prgRomSize;
    uint32_t _chrRomSize;
    uint32_t _miscRomSize;
    uint32_t _prgRamSize;
    uint32_t _prgNvRamSize;
    uint32_t _chrRamSize;
    uint32_t _chrNvRamSize;

    bool _hasNonVolatileMemory;
    iNesFile::Mirroring _mirroring;

    std::unique_ptr<uint8_t[]> _prgRom;
    std::unique_ptr<uint8_t[]> _chrRom;
    std::unique_ptr<uint8_t[]> _miscRom;
    std::unique_ptr<uint8_t[]> _prgRam;
    std::unique_ptr<uint8_t[]> _chrRam;

    std::unique_ptr<uint8_t[]> _vram;

    CPU* _cpu;
    PPU* _ppu;
};
