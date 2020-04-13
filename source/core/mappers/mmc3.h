#pragma once

#include "mapper_base.h"

class MMC3 : public MapperBase
{
public:
    MMC3(NES& nes, iNesFile& file);

    void SaveState(StateSave::Ptr& state) override;
    void LoadState(const StateSave::Ptr& state) override;

    uint8_t CpuRead(uint16_t address) override;
    void CpuWrite(uint8_t M, uint16_t address) override;

    void SetPpuAddress(uint16_t address) override;
    uint8_t PpuRead() override;
    void PpuWrite(uint8_t M) override;

    uint8_t PpuPeek(uint16_t address) override;

    bool CheckIRQ() override;

private:
    void ClockIRQCounter(uint16_t address);

    // Register 0x8000 fields
    uint8_t _prgMode;
    uint8_t _chrMode;
    uint8_t _registerAddress;

    // 0x8001 registers
    uint8_t _chrReg0;
    uint8_t _chrReg1;
    uint8_t _chrReg2;
    uint8_t _chrReg3;
    uint8_t _chrReg4;
    uint8_t _chrReg5;
    uint8_t _prgReg0;
    uint8_t _prgReg1;

    bool _prgRamEnabled;
    bool _prgRamWriteProtect;

    bool _lastReadHigh;
    uint64_t _lastRiseCycle;
    uint8_t _irqCounter;
    uint8_t _irqReloadValue;
    bool _irqEnabled;
    bool _irqPending;
};