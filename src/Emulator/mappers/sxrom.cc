#include <cstring>
#include <iostream>

#include "sxrom.h"
#include "../nes.h"
#include "../cpu.h"

int SXROM::GetStateSize()
{
    return MapperBase::GetStateSize()+sizeof(uint64_t)+(sizeof(uint8_t)*5)+(sizeof(uint8_t*)*4)+sizeof(char);
}

void SXROM::SaveState(char* state)
{
    memcpy(state, &LastWriteCycle, sizeof(uint64_t));
    state += sizeof(uint64_t);

    memcpy(state, &Counter, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(state, &TempRegister, sizeof(uint8_t));
    state += sizeof(uint8_t);

	memcpy(state, &PrgPage0Pointer, sizeof(uint8_t*));
	state += sizeof(uint8_t*);

	memcpy(state, &PrgPage1Pointer, sizeof(uint8_t*));
	state += sizeof(uint8_t*);

	memcpy(state, &ChrPage0Pointer, sizeof(uint8_t*));
	state += sizeof(uint8_t*);

	memcpy(state, &ChrPage1Pointer, sizeof(uint8_t*));
	state += sizeof(uint8_t*);

	memcpy(state, &PrgPage, sizeof(uint8_t));
	state += sizeof(uint8_t);

	memcpy(state, &ChrPage0, sizeof(uint8_t));
	state += sizeof(uint8_t);

	memcpy(state, &ChrPage1, sizeof(uint8_t));
	state += sizeof(uint8_t);

	char packedBool = 0;
	packedBool |= WramDisable << 3;
	packedBool |= PrgLastPageFixed << 2;
	packedBool |= PrgPageSize16K << 1;
	packedBool |= ChrPageSize4K << 0;

	memcpy(state, &packedBool, sizeof(char));
	state += sizeof(char);

    MapperBase::SaveState(state);
}

void SXROM::LoadState(const char* state)
{
    memcpy(&LastWriteCycle, state, sizeof(uint64_t));
    state += sizeof(uint64_t);

    memcpy(&Counter, state, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(&TempRegister, state, sizeof(uint8_t));
    state += sizeof(uint8_t);

	memcpy(&PrgPage0Pointer, state, sizeof(uint8_t*));
	state += sizeof(uint8_t*);

	memcpy(&PrgPage1Pointer, state, sizeof(uint8_t*));
	state += sizeof(uint8_t*);

	memcpy(&ChrPage0Pointer, state, sizeof(uint8_t*));
	state += sizeof(uint8_t*);

	memcpy(&ChrPage1Pointer, state, sizeof(uint8_t*));
	state += sizeof(uint8_t*);

	memcpy(&PrgPage, state, sizeof(uint8_t));
	state += sizeof(uint8_t);

	memcpy(&ChrPage0, state, sizeof(uint8_t));
	state += sizeof(uint8_t);

	memcpy(&ChrPage1, state, sizeof(uint8_t));
	state += sizeof(uint8_t);

	char packedBool;
	memcpy(&packedBool, state, sizeof(char));
	state += sizeof(char);

	WramDisable = !!(packedBool & 0x8);
	PrgLastPageFixed = !!(packedBool & 0x4);
	PrgPageSize16K = !!(packedBool & 0x2);
	ChrPageSize4K = !!(packedBool & 0x1);

    MapperBase::LoadState(state);
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
    if (ChrSize == 0)
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
            return Wram[address];
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
    if (address < 0x2000)
    {
        if (!WramDisable)
        {
            Wram[address] = M;
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
            if (Cpu->GetClock() - LastWriteCycle > 6)
            {
                LastWriteCycle = Cpu->GetClock();
                TempRegister = TempRegister | ((M & 0x1) << Counter);
                ++Counter;
            }
            else
            {
                return;
            }
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
					Mirroring = Cart::MirrorMode::SINGLE_SCREEN_A;
					break;
				case 1:
					Mirroring = Cart::MirrorMode::SINGLE_SCREEN_B;
					break;
				case 2:
					Mirroring = Cart::MirrorMode::VERTICAL;
					break;
				case 3:
					Mirroring = Cart::MirrorMode::HORIZONTAL;
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
		prgOffset = (PrgPage * 0x4000) % PrgSize;

		if (PrgLastPageFixed)
		{
			PrgPage0Pointer = Prg + prgOffset;
			PrgPage1Pointer = Prg + ((0x4000 * 0xf) % PrgSize);
		}
		else
		{
			PrgPage0Pointer = Prg;
			PrgPage1Pointer = Prg + prgOffset;
		}
	}
	else
	{
		prgOffset = ((PrgPage >> 1) * 0x8000) % PrgSize;
		PrgPage0Pointer = Prg + prgOffset;
	}

	if (ChrPageSize4K)
	{
		chrOffset0 = (ChrPage0 * 0x1000) % (ChrSize == 0 ? 0x2000 : ChrSize);
		chrOffset1 = (ChrPage1 * 0x1000) % (ChrSize == 0 ? 0x2000 : ChrSize);

		ChrPage0Pointer = Chr + chrOffset0;
		ChrPage1Pointer = Chr + chrOffset1;
	}
	else
	{
		chrOffset0 = ((ChrPage0 >> 1) * 0x2000) % (ChrSize == 0 ? 0x2000 : ChrSize);
		ChrPage0Pointer = Chr + chrOffset0;
	}
	
}

SXROM::SXROM(const std::string& fileName, const std::string& saveDir)
    : MapperBase(fileName, saveDir)
    , LastWriteCycle(0)
    , Counter(0)
    , TempRegister(0)
	, PrgPage0Pointer(Prg)
	, PrgPage1Pointer(Prg + ((0x4000 * 0xf) % PrgSize))
	, ChrPage0Pointer(Chr)
	, ChrPage1Pointer(Chr)
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
