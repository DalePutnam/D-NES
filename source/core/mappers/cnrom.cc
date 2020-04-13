#include "cnrom.h"

CNROM::CNROM(NES& nes, iNesFile& file)
    : MapperBase(nes, file)
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
    return CNROM::PpuPeek(_ppuAddress);
}

void CNROM::PpuWrite(uint8_t M)
{
    if (_ppuAddress >= 0x2000)
    {
        DefaultNameTableWrite(M, _ppuAddress);
    }
}

uint8_t CNROM::PpuPeek(uint16_t address)
{
    if (address < 0x2000)
    {
        return _chrRom[address + (0x2000 * _register)];
    }
    else
    {
        return DefaultNameTableRead(address);
    }
}