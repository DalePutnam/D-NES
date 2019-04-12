/*
 * nrom.cc
 *
 *  Created on: Jul 8, 2014
 *      Author: Dale
 */

#include "nrom.h"

uint8_t NROM::CpuRead(uint16_t address)
{
    // Battery backed memory, not implemented
    if (address < 0x8000)
    {
        return 0x00;
    }
    else
    {
        if (_prgRomSize == 0x4000)
        {
            return _prgRom[(address - 0x8000) % 0x4000];
        }
        else
        {
            return _prgRom[address - 0x8000];
        }
    }
}

void NROM::CpuWrite(uint8_t M, uint16_t address)
{
}

uint8_t NROM::PpuRead()
{
    if (_ppuAddress < 0x2000)
    {
        return _chr[_ppuAddress];
    }
    else
    {
        return DefaultNameTableRead();
    }
    
}

void NROM::PpuWrite(uint8_t M)
{
    if (_ppuAddress < 0x2000)
    {
        if (_chrRomSize == 0)
        {
            _chr[_ppuAddress] = M;
        }
    }
    else
    {
        DefaultNameTableWrite(M);
    }
}

NROM::NROM(iNesFile& file)
    : MapperBase(file)
    , _chr(_chrRomSize == 0 ? _chrRam.get() : _chrRom.get())
{
}

