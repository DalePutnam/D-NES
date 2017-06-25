#include <cstring>
#include <iostream>

#include "sxrom.h"
#include "../nes.h"
#include "../cpu.h"

int SXROM::GetStateSize()
{
    return MapperBase::GetStateSize() + sizeof(uint64_t) + (sizeof(uint8_t) * 6);
}

void SXROM::SaveState(char* state)
{
    memcpy(state, &LastWriteCycle, sizeof(uint64_t));
    state += sizeof(uint64_t);

    memcpy(state, &Counter, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(state, &TempRegister, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(state, &ControlRegister, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(state, &ChrRegister1, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(state, &ChrRegister2, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(state, &PrgRegister, sizeof(uint8_t));
    state += sizeof(uint8_t);

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

    memcpy(&ControlRegister, state, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(&ChrRegister1, state, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(&ChrRegister2, state, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(&PrgRegister, state, sizeof(uint8_t));
    state += sizeof(uint8_t);

    MapperBase::LoadState(state);
}

Cart::MirrorMode SXROM::GetMirrorMode()
{
    uint8_t mirroring = ControlRegister & 0x3;

    switch (mirroring)
    {
    case 0:
        return Cart::MirrorMode::SINGLE_SCREEN_A;
    case 1:
        return Cart::MirrorMode::SINGLE_SCREEN_B;
    case 2:
        return Cart::MirrorMode::VERTICAL;
    case 3:
        return Cart::MirrorMode::HORIZONTAL;
    default:
        throw std::runtime_error("If you're seeing this something has gone horribly wrong.");
    }
}

uint8_t SXROM::ChrRead(uint16_t address)
{
    bool modeSize = !!(ControlRegister & 0x10);

    if (modeSize)
    {
        if (address < 0x1000)
        {
            uint8_t page = ChrRegister1;
            uint32_t addr = address;

            if (ChrSize == 0)
            {
                return Chr[(addr + (0x1000 * page)) % 0x2000];
            }
            else
            {
                return Chr[(addr + (0x1000 * page)) % ChrSize];
            }
        }
        else
        {
            uint8_t page = ChrRegister2;
            uint32_t addr = address - 0x1000;

            if (ChrSize == 0)
            {
                return Chr[(addr + (0x1000 * page)) % 0x2000];
            }
            else
            {
                return Chr[(addr + (0x1000 * page)) % ChrSize];
            }
        }
    }
    else
    {
        uint8_t page = ChrRegister1 >> 1;
        uint32_t addr = address;

        if (ChrSize == 0)
        {
            return Chr[addr];
        }
        else
        {
            return Chr[(addr + (0x2000 * page)) % ChrSize];
        }
    }
}

void SXROM::ChrWrite(uint8_t M, uint16_t address)
{
    bool modeSize = !!(ControlRegister & 0x10);

    if (ChrSize == 0)
    {
        if (modeSize)
        {
            if (address < 0x1000)
            {
                uint8_t page = ChrRegister1;
                uint32_t addr = address;
                Chr[(addr + (0x1000 * page)) % 0x2000] = M;
            }
            else
            {
                uint8_t page = ChrRegister2;
                uint32_t addr = address - 0x1000;
                Chr[(addr + (0x1000 * page)) % 0x2000] = M;
            }
        }
        else
        {
            uint32_t addr = address;
            Chr[addr] = M;
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
        if (!!(PrgRegister & 0x10))
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
        bool modeSize = !!(ControlRegister & 0x08);
        bool modeSlot = !!(ControlRegister & 0x04);
        uint8_t page = PrgRegister & 0x0F;

        if (modeSize)
        {
            if (modeSlot)
            {
                if (address < 0x6000)
                {
                    uint32_t addr = address - 0x2000;
                    return Prg[(addr + (0x4000 * page)) % PrgSize];
                }
                else
                {
                    uint32_t addr = address - 0x6000;
                    return Prg[(addr + (0x4000 * 0xF)) % PrgSize];
                }
            }
            else
            {
                if (address < 0x6000)
                {
                    uint32_t addr = address - 0x2000;
                    return Prg[addr % PrgSize];

                }
                else
                {
                    uint32_t addr = address - 0x6000;
                    return Prg[(addr + (0x4000 * page)) % PrgSize];
                }
            }
        }
        else
        {
            page = page >> 1;
            uint32_t addr = address - 0x2000;
            return Prg[(addr + (0x8000 * page)) % PrgSize];
        }
    }
}

void SXROM::PrgWrite(uint8_t M, uint16_t address)
{
    if (address < 0x2000)
    {
        if (!(PrgRegister & 0x10))
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
            PrgRegister = PrgRegister | 0x0C;
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
                ControlRegister = TempRegister;
            }
            else if (address >= 0x4000 && address < 0x6000)
            {
                ChrRegister1 = TempRegister;
            }
            else if (address >= 0x6000 && address < 0x8000)
            {
                ChrRegister2 = TempRegister;
            }
            else if (address >= 0x8000 && address < 0xA000)
            {
                PrgRegister = TempRegister;
            }

            Counter = 0;
            TempRegister = 0;
        }
    }
}

SXROM::SXROM(const std::string& fileName, const std::string& saveDir)
    : MapperBase(fileName, saveDir)
    , LastWriteCycle(0)
    , Counter(0)
    , TempRegister(0)
    , ControlRegister(0x0C)
    , ChrRegister1(0)
    , ChrRegister2(0)
    , PrgRegister(0)
{
}

SXROM::~SXROM()
{
}
