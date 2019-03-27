#pragma once

#include <cstdint>
#include <mutex>

#include "cart.h"
#include "state_save.h"

class CPU;
class PPU;

class MapperBase
{
public:
    MapperBase(const std::string& fileName, const std::string& saveDir);
    virtual ~MapperBase();

    const std::string& GetGameName();
    void SetSaveDirectory(const std::string& saveDir);
    void AttachCPU(CPU* cpu);
    void AttachPPU(PPU* ppu);

    virtual void SaveState(StateSave::Ptr& state);
    virtual void LoadState(const StateSave::Ptr& state);

    Cart::MirrorMode GetMirrorMode();

    virtual uint8_t PrgRead(uint16_t address) = 0;
    virtual void PrgWrite(uint8_t M, uint16_t address) = 0;

    virtual uint8_t ChrRead(uint16_t address) = 0;
    virtual void ChrWrite(uint8_t M, uint16_t address) = 0;

protected:
    int PrgSize;
    const uint8_t* Prg;
    int ChrSize;
    uint8_t* Chr;
    int WramSize;
    uint8_t* Wram;
    bool HasSaveMem;

    Cart::MirrorMode Mirroring;

    std::string GameName;
    std::string SaveFile;

    std::mutex MapperMutex;

    CPU* Cpu;
    PPU* Ppu;
};
