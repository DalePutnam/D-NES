/*
 * nrom.cc
 *
 *  Created on: Jul 8, 2014
 *      Author: Dale
 */

#include "nrom.h"

uint8_t NROM::PrgRead(uint16_t address)
{
    // Battery backed memory, not implemented
    if (address >= 0x0000 && address < 0x2000)
    {
        return 0x00;
    }
    else
    {
        if (_prgRomSize == 0x4000)
        {
            return _prgRom[(address - 0x2000) % 0x4000];
        }
        else
        {
            return _prgRom[address - 0x2000];
        }
    }
}

void NROM::PrgWrite(uint8_t M, uint16_t address)
{

}

uint8_t NROM::ChrRead(uint16_t address)
{
    return _chr[address];
}

void NROM::ChrWrite(uint8_t M, uint16_t address)
{
    if (_chrRomSize == 0)
    {
        _chr[address] = M;
    }
}

NROM::NROM(iNesFile& file)
    : MapperBase(file)
    , _chr(_chrRomSize == 0 ? _chrRam.get() : _chrRom.get())
{
}

NROM::~NROM()
{
}
