/*
 * nrom.cc
 *
 *  Created on: Jul 8, 2014
 *      Author: Dale
 */

#include "nrom.h"

Cart::MirrorMode NROM::GetMirrorMode()
{
    return mirroring;
}

uint8_t NROM::PrgRead(uint16_t address)
{
    // Battery backed memory, not implemented
    if (address >= 0x0000 && address < 0x2000)
    {
        return 0x00;
    }
    else
    {
        if (prgSize == 0x4000)
        {
            return prg[(address - 0x2000) % 0x4000];
        }
        else
        {
            return prg[address - 0x2000];
        }
    }
}

void NROM::PrgWrite(uint8_t M, uint16_t address)
{

}

uint8_t NROM::ChrRead(uint16_t address)
{
    if (chrSize != 0)
    {
        return chr[address];
    }
    else
    {
        return chrRam[address];
    }
}

void NROM::ChrWrite(uint8_t M, uint16_t address)
{
    if (chrSize == 0)
    {
        chrRam[address] = M;
    }
}

NROM::NROM(const std::string& fileName, const std::string& saveDir)
    : MapperBase(fileName, saveDir)
{
}

NROM::~NROM()
{
}

