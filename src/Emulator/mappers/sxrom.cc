#include <cstring>
#include <iostream>

#include "sxrom.h"
#include "cpu.h"
#include "ppu.h"

void SXROM::SaveState(StateSave::Ptr& state)
{
    MapperBase::SaveState(state);
    
    state->StoreValue(LastWriteCycle);
    state->StoreValue(Counter);
    state->StoreValue(TempRegister);
    state->StoreValue(PrgPage);
    state->StoreValue(ChrPage0);
    state->StoreValue(ChrPage1);

    state->StorePackedValues(
        WramDisable,
        PrgLastPageFixed,
        PrgPageSize16K,
        ChrPageSize4K
    );
}

void SXROM::LoadState(const StateSave::Ptr& state)
{
    MapperBase::LoadState(state);

    state->ExtractValue(LastWriteCycle);
    state->ExtractValue(Counter);
    state->ExtractValue(TempRegister);
    state->ExtractValue(PrgPage);
    state->ExtractValue(ChrPage0);
    state->ExtractValue(ChrPage1);

    state->ExtractPackedValues(
        WramDisable,
        PrgLastPageFixed,
        PrgPageSize16K,
        ChrPageSize4K
    );
	
	UpdatePageOffsets();
}

uint8_t SXROM::ChrRead(uint16_t address)
{
    if (ChrPageSize4K)
    {
        if (address < 0x1000)
        {
            uint32_t addr = address;
			return ChrPage0Pointer[addr];
        }
        else
        {
            uint32_t addr = address - 0x1000;
			return ChrPage1Pointer[addr];
        }
    }
    else
    {
        uint32_t addr = address;
		return ChrPage0Pointer[addr];
    }
}

void SXROM::ChrWrite(uint8_t M, uint16_t address)
{
    if (_chrRomSize == 0)
    {
        if (ChrPageSize4K)
        {
            if (address < 0x1000)
            {
                uint32_t addr = address;
				ChrPage0Pointer[addr] = M;
            }
            else
            {
                uint32_t addr = address - 0x1000;
				ChrPage1Pointer[addr] = M;
            }
        }
        else
        {
            uint32_t addr = address;
			ChrPage0Pointer[addr] = M;
        }
    }
    else
    {
        return;
    }
}

uint8_t SXROM::PrgRead(uint16_t address)
{
    // Battery backed memory
    if (address >= 0x0000 && address < 0x2000)
    {
        if (WramDisable)
        {
            return 0xFF;
        }
        else
        {
            return _prgRam[address];
        }
    }
    else
    {
        if (PrgPageSize16K)
        {
			if (address < 0x6000)
			{
				uint32_t addr = address - 0x2000;
				return PrgPage0Pointer[addr];

			}
			else
			{
				uint32_t addr = address - 0x6000;
				return PrgPage1Pointer[addr];
			}
        }
        else
        {
            uint32_t addr = address - 0x2000;
            return PrgPage0Pointer[addr];
        }
    }
}

void SXROM::PrgWrite(uint8_t M, uint16_t address)
{
    if (Cpu->GetClock() - LastWriteCycle <= 6)
    {
        return;
    }

    LastWriteCycle = Cpu->GetClock();

    if (address < 0x2000)
    {
        if (!WramDisable)
        {
            _prgRam[address] = M;
        }
    }
    else
    {
        if (!!(M & 0x80))
        {
            Counter = 0;
            TempRegister = 0;
			PrgPageSize16K = true;
			PrgLastPageFixed = true;

			UpdatePageOffsets();

            return;
        }
        else 
        {
            TempRegister = TempRegister | ((M & 0x1) << Counter);
            ++Counter;
        }

        if (Counter == 5)
        {
            if (address >= 0x2000 && address < 0x4000)
            {
                ChrPageSize4K = !!(TempRegister & 0x10);
				PrgPageSize16K = !!(TempRegister & 0x8);
				PrgLastPageFixed = !!(TempRegister & 0x4);

				uint8_t mirroring = TempRegister & 0x3;
				switch (mirroring)
				{
				case 0:
					_mirroring = Cart::MirrorMode::SINGLE_SCREEN_A;
					break;
				case 1:
					_mirroring = Cart::MirrorMode::SINGLE_SCREEN_B;
					break;
				case 2:
					_mirroring = Cart::MirrorMode::VERTICAL;
					break;
				case 3:
					_mirroring = Cart::MirrorMode::HORIZONTAL;
					break;
				}
            }
            else if (address >= 0x4000 && address < 0x6000)
            {
				ChrPage0 = TempRegister;
            }
            else if (address >= 0x6000 && address < 0x8000)
            {
				ChrPage1 = TempRegister;
            }
            else if (address >= 0x8000 && address < 0xA000)
            {
				PrgPage = TempRegister & 0xF;
				WramDisable = !!(TempRegister & 0x10);
            }

			UpdatePageOffsets();

            Counter = 0;
            TempRegister = 0;
        }
    }
}

void SXROM::UpdatePageOffsets()
{
	uint32_t prgOffset, chrOffset0, chrOffset1;

	if (PrgPageSize16K)
	{
		prgOffset = (PrgPage * 0x4000) % _prgRomSize;

		if (PrgLastPageFixed)
		{
			PrgPage0Pointer = _prgRom.get() + prgOffset;
			PrgPage1Pointer = _prgRom.get() + ((0x4000 * 0xf) % _prgRomSize);
		}
		else
		{
			PrgPage0Pointer = _prgRom.get();
			PrgPage1Pointer = _prgRom.get() + prgOffset;
		}
	}
	else
	{
		prgOffset = ((PrgPage >> 1) * 0x8000) % _prgRomSize;
		PrgPage0Pointer = _prgRom.get() + prgOffset;
	}

	if (ChrPageSize4K)
	{
		chrOffset0 = (ChrPage0 * 0x1000) % (_chrRomSize == 0 ? _chrRamSize : _chrRomSize);
		chrOffset1 = (ChrPage1 * 0x1000) % (_chrRomSize == 0 ? _chrRamSize : _chrRomSize);

		ChrPage0Pointer = (_chrRomSize == 0 ? _chrRam.get() : _chrRom.get()) + chrOffset0;
		ChrPage1Pointer = (_chrRomSize == 0 ? _chrRam.get() : _chrRom.get()) + chrOffset1;
	}
	else
	{
		chrOffset0 = ((ChrPage0 >> 1) * 0x2000) % (_chrRomSize == 0 ? _chrRamSize : _chrRomSize);
		ChrPage0Pointer = (_chrRomSize == 0 ? _chrRam.get() : _chrRom.get()) + chrOffset0;
	}
	
}

SXROM::SXROM(iNesFile& file)
    : MapperBase(file)
    , LastWriteCycle(0)
    , Counter(0)
    , TempRegister(0)
	, PrgPage0Pointer(_prgRom.get())
	, PrgPage1Pointer(_prgRom.get() + ((0x4000 * 0xf) % _prgRomSize))
	, ChrPage0Pointer(_chrRomSize == 0 ? _chrRam.get() : _chrRom.get())
	, ChrPage1Pointer(_chrRomSize == 0 ? _chrRam.get() : _chrRom.get())
	, PrgPage(0)
	, ChrPage0(0)
	, ChrPage1(0)
	, WramDisable(true)
	, PrgLastPageFixed(true)
	, PrgPageSize16K(true)
	, ChrPageSize4K(false)
{
}

SXROM::~SXROM()
{
}
