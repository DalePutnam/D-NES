#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "sxrom.h"
#include "../nes.h"

Cart::MirrorMode SXROM::GetMirrorMode()
{
    unsigned char mirroring = controlRegister & 0x3;

    switch (mirroring)
    {
    case 0:
        return MirrorMode::SINGLE_SCREEN_A;
    case 1:
        return MirrorMode::SINGLE_SCREEN_B;
    case 2:
        return MirrorMode::VERTICAL;
    case 3:
        return MirrorMode::HORIZONTAL;
    default:
        throw "WTF!";
    }
}

unsigned char SXROM::ChrRead(unsigned short int address)
{
    bool modeSize = !!(controlRegister & 0x10);
    
    if (modeSize)
    {
        if (address < 0x1000)
        {
            unsigned char page = chrRegister1;
            unsigned int addr = address;

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
            unsigned char page = chrRegister2;
            unsigned int addr = address - 0x1000;

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
        unsigned char page = chrRegister1 >> 1;
        unsigned int addr = address;

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

void SXROM::ChrWrite(unsigned char M, unsigned short int address)
{
    bool modeSize = !!(controlRegister & 0x10);

    if (chrSize == 0)
    {
        if (modeSize)
        {
            if (address < 0x1000)
            {
                unsigned char page = chrRegister1;
                unsigned int addr = address;
                chrRam[(addr + (0x1000 * page)) % 0x2000] = M;
            }
            else
            {
                unsigned char page = chrRegister2;
                unsigned int addr = address - 0x1000;
                chrRam[(addr + (0x1000 * page)) % 0x2000] = M;
            }
        }
        else
        {
            unsigned char page = chrRegister1 >> 1;
            unsigned int addr = address;
            chrRam[addr] = M;
        }
    }
    else
    {
        return;
    }
}

unsigned char SXROM::PrgRead(unsigned short int address)
{
    // Battery backed memory, not implemented
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
        unsigned char page = prgRegister & 0x0F;

        if (modeSize)
        {
            if (modeSlot)
            {
                if (address < 0x6000)
                {
                    unsigned int addr = address - 0x2000;
                    return prg[(addr + (0x4000 * page)) % prgSize];
                }
                else
                {
                    unsigned int addr = address - 0x6000;
                    return prg[(addr + (0x4000 * 0xF)) % prgSize];
                }
            }
            else
            {
                if (address < 0x6000)
                {
                    unsigned int addr = address - 0x2000;
                    return prg[addr % prgSize];
                    
                }
                else
                {
                    unsigned int addr = address - 0x6000;
                    return prg[(addr + (0x4000 * page)) % prgSize];
                }
            }
        }
        else
        {
            page = page >> 1;
            unsigned int addr = address - 0x2000;
            return prg[(addr + (0x8000 * page)) % prgSize];
        }
    }
}

void SXROM::PrgWrite(unsigned char M, unsigned short int address)
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
            if (clock.GetClock() - lastWriteCycle > 6)
            {
                lastWriteCycle = clock.GetClock();
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

SXROM::SXROM(std::string& filename, Clock& clock, NES& nes) :
    file(*new boost::iostreams::mapped_file_source(filename)),
    save(0),
    clock(clock),
    nes(nes),
    lastWriteCycle(0),
    counter(0),
    tempRegister(0),
    controlRegister(0x0C),
    chrRegister1(0),
    chrRegister2(0),
    prgRegister(0)
{
    if (file.is_open())
    {
        // Read Byte 4 from the file to get the program size
        // For the type of ROM the emulator currently supports
        // this will be either 1 or 2 representing 0x4000 or 0x8000 bytes
        prgSize = file.data()[4] * 0x4000;
        chrSize = file.data()[5] * 0x2000;

        char flags6 = file.data()[6];

        if (flags6 & 0x2)
        {
            std::vector<std::string> stringList;
            boost::algorithm::split(stringList, filename, boost::is_any_of("\\/"));
            std::string gameName = stringList.back().substr(0, stringList.back().length() - 4);

            boost::filesystem::path gameDir = "saves";
            
            if (!boost::filesystem::exists(gameDir))
            {
                boost::filesystem::create_directory(gameDir);
            }

            boost::filesystem::path path = gameDir.string() + "/" + gameName + ".sav";

            if (boost::filesystem::exists(path))
            {
                save = new boost::iostreams::mapped_file(path.string(), boost::iostreams::mapped_file::mapmode::readwrite);
            }
            else
            {
                boost::iostreams::mapped_file_params params(path.string());
                params.new_file_size = 0x2000;
                params.mode = boost::iostreams::mapped_file::mapmode::readwrite;

                save = new boost::iostreams::mapped_file(params);
            }

            if (save->is_open())
            {
                wram = save->data();
            }
            else
            {
                throw "Open Save File Failed";
            }
        }
        else
        {
            wram = new char[0x2000];
        }

        prg = file.data() + 16; // Data pointer plus the size of the file header

        // initialize chr RAM or read chr to memory
        if (chrSize == 0)
        {
            chrRam = new char[0x2000];
            for (int i = 0; i < 0x2000; i++)
            {
                chrRam[i] = 0;
            }
        }
        else
        {
            chr = file.data() + prgSize + 16;
        }
    }
    else
    {
        std::cout << "Open Failed!!" << std::endl;
    }
}

SXROM::~SXROM()
{
    file.close();
    delete &file;

    if (save)
    {
        save->close();
        delete save;
    }
    else
    {
        delete[] wram;
    }

    if (chrSize == 0)
    {
        delete[] chrRam;
    }
}
