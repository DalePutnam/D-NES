#include "uxrom.h"

UXROM::UXROM(iNesFile& file)
    : MapperBase(file)
    , _register(0)
{
}

UXROM::~UXROM()
{
}

void UXROM::SaveState(StateSave::Ptr& state)
{
    MapperBase::SaveState(state);

    state->StoreValue(_register);
}

void UXROM::LoadState(const StateSave::Ptr& state)
{
    MapperBase::LoadState(state);

    state->StoreValue(_register);
}

uint8_t UXROM::PrgRead(uint16_t address)
{
    if (address >= 0x2000)
    {
        uint16_t addr = address - 0x2000;
        if (addr < 0x4000)
        {
            return _prgRom[addr + (0x4000 * _register)];
        }
        else
        {   
            addr -= 0x4000;
            return _prgRom[addr + (_prgRomSize - 0x4000)];
        }
    }
    else
    {
        return _prgRam[address];
    }
}

void UXROM::PrgWrite(uint8_t M, uint16_t address)
{
    if (address >= 0x2000)
    {
        _register = M;
    }
    else
    {
        _prgRam[address] = M;
    }
}

uint8_t UXROM::ChrRead(uint16_t address)
{
    return _chrRam[address];
}

void UXROM::ChrWrite(uint8_t M, uint16_t address)
{
    _chrRam[address] = M;
}