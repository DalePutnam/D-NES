/*
 * nrom.cc
 *
 *  Created on: Jul 8, 2014
 *      Author: Dale
 */

#include "nrom.h"

Cart::MirrorMode NROM::GetMirrorMode()
{
    return Mirroring;
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
        if (PrgSize == 0x4000)
        {
            return Prg[(address - 0x2000) % 0x4000];
        }
        else
        {
            return Prg[address - 0x2000];
        }
    }
}

void NROM::PrgWrite(uint8_t M, uint16_t address)
{

}

uint8_t NROM::ChrRead(uint16_t address)
{
    if (ChrSize != 0)
    {
        return Chr[address];
    }
    else
    {
        return ChrRam[address];
    }
}

void NROM::ChrWrite(uint8_t M, uint16_t address)
{
    if (ChrSize == 0)
    {
        ChrRam[address] = M;
    }
}

NROM::NROM(const std::string& fileName, const std::string& saveDir)
    : MapperBase(fileName, saveDir)
{
}

NROM::~NROM()
{
}

