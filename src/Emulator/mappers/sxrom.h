#pragma once

#include "mapper_base.h"

class CPU;

class SXROM : public MapperBase
{
public:
    SXROM(const std::string& fileName, const std::string& saveDir);
    ~SXROM();

    int GetStateSize();
    void SaveState(char* state);
    void LoadState(const char* state);

    Cart::MirrorMode GetMirrorMode() override;

    uint8_t PrgRead(uint16_t address) override;
    void PrgWrite(uint8_t M, uint16_t address) override;

    uint8_t ChrRead(uint16_t address) override;
    void ChrWrite(uint8_t M, uint16_t address) override;

private:
    uint64_t LastWriteCycle;
    uint8_t Counter;
    uint8_t TempRegister;
    uint8_t ControlRegister;
    uint8_t ChrRegister1;
    uint8_t ChrRegister2;
    uint8_t PrgRegister;
};
