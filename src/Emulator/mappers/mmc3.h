#pragma once

#include "mapper_base.h"

class MMC3 : public MapperBase
{
public:
    MMC3(iNesFile& file);
    virtual ~MMC3() = default;

    void SaveState(StateSave::Ptr& state) override;
    void LoadState(const StateSave::Ptr& state) override;

    uint8_t PrgRead(uint16_t address) override;
    void PrgWrite(uint8_t M, uint16_t address) override;

    uint8_t PpuRead(uint16_t address) override;
    void PpuWrite(uint8_t M, uint16_t address) override;

    bool CheckIRQ() override;

protected:
    uint8_t ChrRead(uint16_t address) override;
    void ChrWrite(uint8_t M, uint16_t address) override;

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