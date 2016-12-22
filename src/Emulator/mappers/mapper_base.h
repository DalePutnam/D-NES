#pragma once

#include <cstdint>
#include <boost/iostreams/device/mapped_file.hpp>

#include "../cart.h"

class CPU;

class MapperBase
{
public:
    MapperBase(boost::iostreams::mapped_file_source* file);
    virtual ~MapperBase();

    virtual void AttachCPU(CPU* cpu) final;
    virtual Cart::MirrorMode GetMirrorMode() = 0;

    virtual uint8_t PrgRead(uint16_t address) = 0;
    virtual void PrgWrite(uint8_t M, uint16_t address) = 0;

    virtual uint8_t ChrRead(uint16_t address) = 0;
    virtual void ChrWrite(uint8_t M, uint16_t address) = 0;

protected:
    int prgSize;
    const int8_t* prg;
    int chrSize;
    const int8_t* chr;

    CPU* cpu;
    boost::iostreams::mapped_file_source* romFile;;
};