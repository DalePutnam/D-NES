#pragma once

#include "mapper_base.h"

class CNROM : public MapperBase
{
public:
    CNROM(iNesFile& file);

    void SaveState(StateSave::Ptr& state) override;
    void LoadState(const StateSave::Ptr& state) override;

    uint8_t CpuRead(uint16_t address) override;
    void CpuWrite(uint8_t M, uint16_t address) override;

    uint8_t PpuRead() override;
    void PpuWrite(uint8_t M) override;

    uint8_t PpuPeek(uint16_t address) override;
    
private:
    uint8_t _register;
};