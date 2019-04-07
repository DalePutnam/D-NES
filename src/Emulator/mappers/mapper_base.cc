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
        _mirroring = Cart::MirrorMode::VERTICAL;
    }
    else
    {
        _mirroring = Cart::MirrorMode::HORIZONTAL;
    }
}

MapperBase::~MapperBase()
{
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

Cart::MirrorMode MapperBase::GetMirrorMode()
{
	return _mirroring;
}

bool MapperBase::CheckIRQ()
{
    return false;
}