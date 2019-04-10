#include <fstream>
#include <exception>
#include <cstring>

#include "cpu.h"
#include "ppu.h"
#include "file.h"
#include "mapper_base.h"
#include "nes_exception.h"

using namespace std;

MapperBase::MapperBase(iNesFile& file)
    : _cpu(nullptr)
    , _ppu(nullptr)
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

    if (file.GetMirroring() == iNesFile::Mirroring::Vertical)
    {
        _mirroringVertical = true;
    }
    else
    {
        _mirroringVertical = false;
    }
}

void MapperBase::AttachCPU(CPU* cpu)
{
    _cpu = cpu;
}

void MapperBase::AttachPPU(PPU* ppu)
{
    _ppu = ppu;
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

uint8_t MapperBase::PpuRead()
{
    if (_ppuAddress < 0x2000)
    {
        return ChrRead(_ppuAddress);
    }
    else if (_ppuAddress >= 0x2000 && _ppuAddress < 0x3F00)
    {
        return NameTableRead(_ppuAddress);
    }
    else
    {
        return 0x00;
    }
}

void MapperBase::PpuWrite(uint8_t M)
{
    if (_ppuAddress < 0x2000)
    {
        ChrWrite(M, _ppuAddress);
    }
    else if (_ppuAddress < 0x3F00)
    {
        NameTableWrite(M, _ppuAddress);
    }
}

bool MapperBase::CheckIRQ()
{
    return false;
}

uint8_t MapperBase::NameTableRead(uint16_t address)
{
    address = (address - 0x2000) % 0x1000;

    if (_mirroringVertical)
    {
        if (address < 0x400 || (address >= 0x800 && address < 0xC00))
        {
            return _ppu->ReadNameTable0(address % 0x400);
        }
        else
        {
            return _ppu->ReadNameTable1(address % 0x400);
        }
    }
    else
    {
        if (address < 0x800)
        {
            return _ppu->ReadNameTable0(address % 0x400);
        }
        else
        {
            return _ppu->ReadNameTable1(address % 0x400);
        }
    }
}

void MapperBase::NameTableWrite(uint8_t M, uint16_t address)
{
    address = (address - 0x2000) % 0x1000;

    if (_mirroringVertical)
    {
        if (address < 0x400 || (address >= 0x800 && address < 0xC00))
        {
            _ppu->WriteNameTable0(M, address % 0x400);
        }
        else
        {
            _ppu->WriteNameTable1(M, address % 0x400);
        }
    }
    else
    {

        if (address < 0x800)
        {
            _ppu->WriteNameTable0(M, address % 0x400);
        }
        else
        {
            _ppu->WriteNameTable1(M, address % 0x400);
        }
    }
}