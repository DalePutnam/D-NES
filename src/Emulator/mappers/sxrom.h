#pragma once

#include "mapper_base.h"

class CPU;

class SXROM : public MapperBase
{
public:
    SXROM(const std::string& fileName, const std::string& saveDir);
    ~SXROM();

    Cart::MirrorMode GetMirrorMode() override;

    uint8_t PrgRead(uint16_t address) override;
    void PrgWrite(uint8_t M, uint16_t address) override;

    uint8_t ChrRead(uint16_t address) override;
    void ChrWrite(uint8_t M, uint16_t address) override;

private:
    unsigned long long lastWriteCycle;
    uint8_t counter;
    uint8_t tempRegister;
    uint8_t controlRegister;
    uint8_t chrRegister1;
    uint8_t chrRegister2;
    uint8_t prgRegister;
};
