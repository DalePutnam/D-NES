#include <cstring>
#include <iostream>

#include "sxrom.h"
#include "../nes.h"
#include "../cpu.h"

Cart::MirrorMode SXROM::GetMirrorMode()
{
    uint8_t mirroring = controlRegister & 0x3;

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
    bool modeSize = !!(controlRegister & 0x10);

    if (modeSize)
    {
        if (address < 0x1000)
        {
            uint8_t page = chrRegister1;
            uint32_t addr = address;

            if (chrSize == 0)
            {
                return chrRam[(addr + (0x1000 * page)) % 0x2000];
            }
            else
            {
                return chr[(addr + (0x1000 * page)) % chrSize];
            }
        }
        else
        {
            uint8_t page = chrRegister2;
            uint32_t addr = address - 0x1000;

            if (chrSize == 0)
            {
                return chrRam[(addr + (0x1000 * page)) % 0x2000];
            }
            else
            {
                return chr[(addr + (0x1000 * page)) % chrSize];
            }
        }
    }
    else
    {
        uint8_t page = chrRegister1 >> 1;
        uint32_t addr = address;

        if (chrSize == 0)
        {
            return chrRam[addr];
        }
        else
        {
            return chr[(addr + (0x2000 * page)) % chrSize];
        }
    }
}

void SXROM::ChrWrite(uint8_t M, uint16_t address)
{
    bool modeSize = !!(controlRegister & 0x10);

    if (chrSize == 0)
    {
        if (modeSize)
        {
            if (address < 0x1000)
            {
                uint8_t page = chrRegister1;
                uint32_t addr = address;
                chrRam[(addr + (0x1000 * page)) % 0x2000] = M;
            }
            else
            {
                uint8_t page = chrRegister2;
                uint32_t addr = address - 0x1000;
                chrRam[(addr + (0x1000 * page)) % 0x2000] = M;
            }
        }
        else
        {
            uint32_t addr = address;
            chrRam[addr] = M;
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
        if (!!(prgRegister & 0x10))
        {
            return 0xFF;
        }
        else
        {
            return wram[address];
        }
    }
    else
    {
        bool modeSize = !!(controlRegister & 0x08);
        bool modeSlot = !!(controlRegister & 0x04);
        uint8_t page = prgRegister & 0x0F;

        if (modeSize)
        {
            if (modeSlot)
            {
                if (address < 0x6000)
                {
                    uint32_t addr = address - 0x2000;
                    return prg[(addr + (0x4000 * page)) % prgSize];
                }
                else
                {
                    uint32_t addr = address - 0x6000;
                    return prg[(addr + (0x4000 * 0xF)) % prgSize];
                }
            }
            else
            {
                if (address < 0x6000)
                {
                    uint32_t addr = address - 0x2000;
                    return prg[addr % prgSize];

                }
                else
                {
                    uint32_t addr = address - 0x6000;
                    return prg[(addr + (0x4000 * page)) % prgSize];
                }
            }
        }
        else
        {
            page = page >> 1;
            uint32_t addr = address - 0x2000;
            return prg[(addr + (0x8000 * page)) % prgSize];
        }
    }
}

void SXROM::PrgWrite(uint8_t M, uint16_t address)
{
    if (address < 0x2000)
    {
        if (!(prgRegister & 0x10))
        {
            wram[address] = M;
        }
    }
    else
    {
        if (!!(M & 0x80))
        {
            counter = 0;
            tempRegister = 0;
            prgRegister = prgRegister | 0x0C;
            return;
        }
        else
        {
            if (cpu->GetClock() - lastWriteCycle > 6)
            {
                lastWriteCycle = cpu->GetClock();
                tempRegister = tempRegister | ((M & 0x1) << counter);
                ++counter;
            }
            else
            {
                return;
            }
        }

        if (counter == 5)
        {
            if (address >= 0x2000 && address < 0x4000)
            {
                controlRegister = tempRegister;
            }
            else if (address >= 0x4000 && address < 0x6000)
            {
                chrRegister1 = tempRegister;
            }
            else if (address >= 0x6000 && address < 0x8000)
            {
                chrRegister2 = tempRegister;
            }
            else if (address >= 0x8000 && address < 0xA000)
            {
                prgRegister = tempRegister;
            }

            counter = 0;
            tempRegister = 0;
        }
    }
}

SXROM::SXROM(const std::string& fileName, const std::string& saveDir)
    : MapperBase(fileName, saveDir)
    , lastWriteCycle(0)
    , counter(0)
    , tempRegister(0)
    , controlRegister(0x0C)
    , chrRegister1(0)
    , chrRegister2(0)
    , prgRegister(0)
{
}

SXROM::~SXROM()
{
}
