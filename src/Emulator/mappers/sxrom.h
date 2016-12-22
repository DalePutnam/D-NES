#pragma once

#include <boost/iostreams/device/mapped_file.hpp>

#include "mapper_base.h"

class CPU;

class SXROM : public MapperBase
{
public:
    SXROM(boost::iostreams::mapped_file_source* file, const std::string& gameName);
    ~SXROM();

    Cart::MirrorMode GetMirrorMode() override;

    uint8_t PrgRead(uint16_t address) override;
    void PrgWrite(uint8_t M, uint16_t address) override;

    uint8_t ChrRead(uint16_t address) override;
    void ChrWrite(uint8_t M, uint16_t address) override;

private:
    boost::iostreams::mapped_file* save;
    std::string gameName;

    unsigned long long lastWriteCycle;
    uint8_t counter;
    uint8_t tempRegister;
    uint8_t controlRegister;
    uint8_t chrRegister1;
    uint8_t chrRegister2;
    uint8_t prgRegister;

    int8_t* wram;
    int8_t* chrRam;
};
