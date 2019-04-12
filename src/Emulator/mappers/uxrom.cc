#include "uxrom.h"

UXROM::UXROM(iNesFile& file)
    : MapperBase(file)
    , _register(0)
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

uint8_t UXROM::CpuRead(uint16_t address)
{
    if (address >= 0x6000 && address < 0x8000)
    {
        return _prgRam[address - 0x6000];
    }
    else if (address >= 0x8000)
    {
        if (address < 0xC000)
        {
            return _prgRom[(address - 0x8000) + (0x4000 * _register)];
        }
        else
        {   
            return _prgRom[(address - 0xC000) + (_prgRomSize - 0x4000)];
        }
    }
    else 
    {
        return 0x00;
    }
}

void UXROM::CpuWrite(uint8_t M, uint16_t address)
{
    if (address >= 0x6000 && address < 0x8000)
    {
        _prgRam[address - 0x6000] = M;
    }
    else if (address >= 0x8000)
    {
        _register = M;
    } 
}

uint8_t UXROM::PpuRead()
{
    if (_ppuAddress < 0x2000)
    {
        return _chrRam[_ppuAddress];
    }
    else
    {
        return DefaultNameTableRead();
    }
}

void UXROM::PpuWrite(uint8_t M)
{
    if (_ppuAddress < 0x2000)
    {
        _chrRam[_ppuAddress] = M;
    }
    else
    {
        DefaultNameTableWrite(M);
    }
}