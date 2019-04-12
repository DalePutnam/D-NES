#include "cnrom.h"

CNROM::CNROM(iNesFile& file)
    : MapperBase(file)
    , _register(0)
{
}

void CNROM::SaveState(StateSave::Ptr& state)
{
    MapperBase::SaveState(state);

    state->StoreValue(_register);
}

void CNROM::LoadState(const StateSave::Ptr& state)
{
    MapperBase::LoadState(state);

    state->StoreValue(_register);
}

uint8_t CNROM::CpuRead(uint16_t address)
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

void CNROM::CpuWrite(uint8_t M, uint16_t address)
{
    if (address >= 0x8000)
    {
        _register = M;
    }
}

uint8_t CNROM::PpuRead()
{
    if (_ppuAddress < 0x2000)
    {
        return _chrRom[_ppuAddress + (0x2000 * _register)];
    }
    else
    {
        return DefaultNameTableRead();
    }
}

void CNROM::PpuWrite(uint8_t M)
{
    if (_ppuAddress >= 0x2000)
    {
        DefaultNameTableWrite(M);
    }
}