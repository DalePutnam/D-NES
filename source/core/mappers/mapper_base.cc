#include <fstream>
#include <exception>
#include <cstring>

#include "cpu.h"
#include "ppu.h"
#include "mapper_base.h"

using namespace std;

MapperBase::MapperBase(NES& nes, iNesFile& file)
    : _nes(nes)
{
    _prgRom = file.GetPrgRom();
    _prgRomSize = file.GetPrgRomSize();
    
    _chrRom = file.GetChrRom();
    _chrRomSize = file.GetChrRomSize();

    _miscRom = file.GetMiscRom();
    _miscRomSize = file.GetMiscRomSize();

    _prgRamSize = file.GetPrgRamSize();
    _prgNvRamSize = file.GetPrgNvRamSize();
    _chrRamSize = file.GetChrRamSize();
    _chrNvRamSize = file.GetChrNvRamSize();

    _prgRam = std::make_unique<uint8_t[]>(_prgRamSize + _prgNvRamSize);
    _chrRam = std::make_unique<uint8_t[]>(_chrRamSize + _chrNvRamSize);

    std::fill(_prgRam.get(), _prgRam.get() + _prgRamSize + _prgNvRamSize, 0);
    std::fill(_chrRam.get(), _chrRam.get() + _chrRamSize + _chrNvRamSize, 0);

    _hasNonVolatileMemory = file.HasNonVolatileMemory();

    _mirroring = file.GetMirroring();

    if (_mirroring == iNesFile::Mirroring::FourScreen)
    {
        _vram = std::make_unique<uint8_t[]>(0x400 * 4);
    }
    else
    {
        _vram = std::make_unique<uint8_t[]>(0x400 * 2);
    }
}

void MapperBase::SaveNativeSave(std::ofstream& stream)
{
    if (_hasNonVolatileMemory)
    {
        stream.write(reinterpret_cast<char*>(_prgRam.get() + _prgRamSize), _prgNvRamSize);
        stream.write(reinterpret_cast<char*>(_chrRam.get() + _chrRamSize), _chrNvRamSize);
    }
}

void MapperBase::LoadNativeSave(std::ifstream& stream)
{
    if (_hasNonVolatileMemory)
    {
        stream.read(reinterpret_cast<char*>(_prgRam.get() + _prgRamSize), _prgNvRamSize);
        stream.read(reinterpret_cast<char*>(_chrRam.get() + _chrRamSize), _chrNvRamSize);
    }
}

void MapperBase::SaveState(StateSave::Ptr& state)
{
    state->StoreBuffer(_prgRam.get(), _prgRamSize + _prgNvRamSize);
    state->StoreBuffer(_chrRam.get(), _chrRamSize + _chrNvRamSize);
}

void MapperBase::LoadState(const StateSave::Ptr& state)
{
    state->ExtractBuffer(_prgRam.get(), _prgRamSize + _prgNvRamSize);
    state->ExtractBuffer(_chrRam.get(), _chrRamSize + _chrNvRamSize);
}

void MapperBase::SetPpuAddress(uint16_t address)
{
    _ppuAddress = address;
}

bool MapperBase::CheckIRQ()
{
    return false;
}

uint8_t MapperBase::DefaultNameTableRead(uint16_t address)
{
    address = (address - 0x2000) % 0x1000;

    if (_mirroring == iNesFile::Mirroring::Vertical)
    {
        if (address < 0x400 || (address >= 0x800 && address < 0xC00))
        {
            return _vram[address % 0x400];
        }
        else
        {
            return _vram[(address % 0x400) + 0x400];
        }
    }
    else if (_mirroring == iNesFile::Mirroring::Horizontal)
    {
        if (address < 0x800)
        {
            return _vram[address % 0x400];
        }
        else
        {
            return _vram[(address % 0x400) + 0x400];
        }
    }
    else // _mirroring == iNesFile::Mirroring::FourScreen
    {
        return _vram[address % 0x1000];
    }
}

void MapperBase::DefaultNameTableWrite(uint8_t M, uint16_t address)
{
    address = (address - 0x2000) % 0x1000;

    if (_mirroring == iNesFile::Mirroring::Vertical)
    {
        if (address < 0x400 || (address >= 0x800 && address < 0xC00))
        {
            _vram[address % 0x400] = M;
        }
        else
        {
            _vram[(address % 0x400) + 0x400] = M;
        }
    }
    else if (_mirroring == iNesFile::Mirroring::Horizontal)
    {
        if (address < 0x800)
        {
            _vram[address % 0x400] = M;
        }
        else
        {
            _vram[(address % 0x400) + 0x400] = M;
        }
    }
    else // _mirroring == iNesFile::Mirroring::FourScreen
    {
        _vram[address % 0x1000];
    }
}