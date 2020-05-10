#include <cstring>
#include <iostream>

#include "mmc1.h"
#include "cpu.h"
#include "ppu.h"

void MMC1::SaveState(StateSave::Ptr& state)
{
    MapperBase::SaveState(state);
    
    state->StoreValue(_lastWriteCycle);
    state->StoreValue(_cycleCounter);
    state->StoreValue(_tempRegister);
    state->StoreValue(_prgPage);
    state->StoreValue(_chrPage0);
    state->StoreValue(_chrPage1);

    state->StorePackedValues(
        _wramDisable,
        _prgLastPageFixed,
        _prgPageSize16K,
        _chrPageSize4K
    );
}

void MMC1::LoadState(const StateSave::Ptr& state)
{
    MapperBase::LoadState(state);

    state->ExtractValue(_lastWriteCycle);
    state->ExtractValue(_cycleCounter);
    state->ExtractValue(_tempRegister);
    state->ExtractValue(_prgPage);
    state->ExtractValue(_chrPage0);
    state->ExtractValue(_chrPage1);

    state->ExtractPackedValues(
        _wramDisable,
        _prgLastPageFixed,
        _prgPageSize16K,
        _chrPageSize4K
    );
	
	UpdatePageOffsets();
}

uint8_t MMC1::CpuRead(uint16_t address)
{
    // Battery backed memory
    if (address >= 0x6000 && address < 0x8000)
    {
        if (_wramDisable)
        {
            return 0xFF;
        }
        else
        {
            return _prgRam[address - 0x6000];
        }
    }
    else if (address >= 0x8000)
    {
        if (_prgPageSize16K)
        {
			if (address < 0xC000)
			{
				uint32_t addr = address - 0x8000;
				return _prgPage0Pointer[addr];
			}
			else
			{
				uint32_t addr = address - 0xC000;
				return _prgPage1Pointer[addr];
			}
        }
        else
        {
            uint32_t addr = address - 0x8000;
            return _prgPage0Pointer[addr];
        }
    }
    else
    {
        return 0x00;
    }
}

void MMC1::CpuWrite(uint8_t M, uint16_t address)
{
    if (_nes.GetCpu().GetClock() - _lastWriteCycle <= 6)
    {
        return;
    }

    _lastWriteCycle = _nes.GetCpu().GetClock();

    if (address >= 0x6000 && address < 0x8000)
    {
        if (!_wramDisable)
        {
            _prgRam[address - 0x6000] = M;
        }
    }
    else
    {
        if (!!(M & 0x80))
        {
            _cycleCounter = 0;
            _tempRegister = 0;
			_prgPageSize16K = true;
			_prgLastPageFixed = true;

			UpdatePageOffsets();

            return;
        }
        else 
        {
            _tempRegister = _tempRegister | ((M & 0x1) << _cycleCounter);
            ++_cycleCounter;
        }

        if (_cycleCounter == 5)
        {
            if (address >= 0x8000 && address < 0xA000)
            {
                _chrPageSize4K = !!(_tempRegister & 0x10);
				_prgPageSize16K = !!(_tempRegister & 0x8);
				_prgLastPageFixed = !!(_tempRegister & 0x4);
				_mirroring = _tempRegister & 0x3;
            }
            else if (address >= 0xA000 && address < 0xC000)
            {
				_chrPage0 = _tempRegister;
            }
            else if (address >= 0xC000 && address < 0xE000)
            {
				_chrPage1 = _tempRegister;
            }
            else if (address >= 0xE000 && address < 0x10000)
            {
				_prgPage = _tempRegister & 0xF;
				_wramDisable = !!(_tempRegister & 0x10);
            }

			UpdatePageOffsets();

            _cycleCounter = 0;
            _tempRegister = 0;
        }
    }
}

uint8_t MMC1::NameTableRead(uint16_t address)
{
    address = (address - 0x2000) % 0x1000;

    switch (_mirroring)
    {
    case 0:
        return _vram[address % 0x400];
        break;
    case 1:
        return _vram[(address % 0x400) + 0x400];
        break;
    case 2:
        if (address < 0x400 || (address >= 0x800 && address < 0xC00))
        {
            return _vram[address % 0x400];
        }
        else
        {
            return _vram[(address % 0x400) + 0x400];
        }
        break;
    case 3:
        if (address < 0x800)
        {
            return _vram[address % 0x400];
        }
        else
        {
            return _vram[(address % 0x400) + 0x400];
        }
        break;
    default:
        assert(false); // Unreachable
        return 0;
    }
}

void MMC1::NameTableWrite(uint8_t M, uint16_t address)
{
    address = (address - 0x2000) % 0x1000;

    switch (_mirroring)
    {
    case 0:
        _vram[address % 0x400] = M;
        break;
    case 1:
        _vram[(address % 0x400) + 0x400] = M;
        break;
    case 2:
        if (address < 0x400 || (address >= 0x800 && address < 0xC00))
        {
            _vram[address % 0x400] = M;
        }
        else
        {
            _vram[(address % 0x400) + 0x400] = M;
        }
        break;
    case 3:
        if (address < 0x800)
        {
            _vram[address % 0x400] = M;
        }
        else
        {
            _vram[(address % 0x400) + 0x400] = M;
        }
        break;
    default:
        assert(false); // Unreachable
    }
}

uint8_t MMC1::PpuRead()
{
    return MMC1::PpuPeek(_ppuAddress);
}

void MMC1::PpuWrite(uint8_t M)
{
    if (_ppuAddress < 0x2000)
    {
        ChrWrite(M, _ppuAddress);
    }
    else
    {
        NameTableWrite(M, _ppuAddress);
    }
}

uint8_t MMC1::PpuPeek(uint16_t address)
{
    if (address < 0x2000)
    {
        return ChrRead(address);
    }
    else
    {
        return NameTableRead(address);
    }
}

uint8_t MMC1::ChrRead(uint16_t address)
{
    if (_chrPageSize4K)
    {
        if (address < 0x1000)
        {
            uint32_t addr = address;
			return _chrPage0Pointer[addr];
        }
        else
        {
            uint32_t addr = address - 0x1000;
			return _chrPage1Pointer[addr];
        }
    }
    else
    {
        uint32_t addr = address;
		return _chrPage0Pointer[addr];
    }
}

void MMC1::ChrWrite(uint8_t M, uint16_t address)
{
    if (_chrRomSize == 0)
    {
        if (_chrPageSize4K)
        {
            if (address < 0x1000)
            {
                uint32_t addr = address;
				_chrPage0Pointer[addr] = M;
            }
            else
            {
                uint32_t addr = address - 0x1000;
				_chrPage1Pointer[addr] = M;
            }
        }
        else
        {
            uint32_t addr = address;
			_chrPage0Pointer[addr] = M;
        }
    }
    else
    {
        return;
    }
}

void MMC1::UpdatePageOffsets()
{
	uint32_t prgOffset, chrOffset0, chrOffset1;

	if (_prgPageSize16K)
	{
		prgOffset = (_prgPage * 0x4000) % _prgRomSize;

		if (_prgLastPageFixed)
		{
			_prgPage0Pointer = _prgRom.get() + prgOffset;
			_prgPage1Pointer = _prgRom.get() + ((0x4000 * 0xf) % _prgRomSize);
		}
		else
		{
			_prgPage0Pointer = _prgRom.get();
			_prgPage1Pointer = _prgRom.get() + prgOffset;
		}
	}
	else
	{
		prgOffset = ((_prgPage >> 1) * 0x8000) % _prgRomSize;
		_prgPage0Pointer = _prgRom.get() + prgOffset;
	}

	if (_chrPageSize4K)
	{
		chrOffset0 = (_chrPage0 * 0x1000) % _chrSize;
		chrOffset1 = (_chrPage1 * 0x1000) % _chrSize;

		_chrPage0Pointer = _chr + chrOffset0;
		_chrPage1Pointer = _chr + chrOffset1;
	}
	else
	{
		chrOffset0 = ((_chrPage0 >> 1) * 0x2000) % _chrSize;
		_chrPage0Pointer = _chr + chrOffset0;
	}
	
}

MMC1::MMC1(NES& nes, iNesFile& file)
    : MapperBase(nes, file)
    , _lastWriteCycle(0)
    , _cycleCounter(0)
    , _tempRegister(0)
    , _chr(_chrRomSize == 0 ? _chrRam.get() : _chrRom.get())
    , _chrSize(_chrRomSize == 0 ? _chrRamSize : _chrRomSize)
    , _mirroring(0)
	, _prgPage0Pointer(_prgRom.get())
	, _prgPage1Pointer(_prgRom.get() + ((0x4000 * 0xf) % _prgRomSize))
	, _chrPage0Pointer(_chrRomSize == 0 ? _chrRam.get() : _chrRom.get())
	, _chrPage1Pointer(_chrRomSize == 0 ? _chrRam.get() : _chrRom.get())
	, _prgPage(0)
	, _chrPage0(0)
	, _chrPage1(0)
	, _wramDisable(true)
	, _prgLastPageFixed(true)
	, _prgPageSize16K(true)
	, _chrPageSize4K(false)
{
}
