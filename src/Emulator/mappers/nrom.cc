/*
 * nrom.cc
 *
 *  Created on: Jul 8, 2014
 *      Author: Dale
 */

#include <fstream>
#include <iostream>

#include "nrom.h"

Cart::MirrorMode NROM::GetMirrorMode()
{
    return mirroring;
}

unsigned char NROM::PrgRead(unsigned short int address)
{
    // Battery backed memory, not implemented
    if (address >= 0x0000 && address < 0x2000)
    {
        return 0x00;
    }
    else
    {
        if (prgSize == 0x4000)
        {
            return prg[(address - 0x2000) % 0x4000];
        }
        else
        {
            return prg[address - 0x2000];
        }
    }
}

void NROM::PrgWrite(unsigned char M, unsigned short int address)
{
    if (address >= 0x0000 && address < 0x2000)
    {

    }
}

unsigned char NROM::ChrRead(unsigned short int address)
{
    if (chrSize != 0)
    {
        return chr[address];
    }
    else
    {
        return chrRam[address];
    }
}

void NROM::ChrWrite(unsigned char M, unsigned short int address)
{
    if (chrSize == 0)
    {
        chrRam[address] = M;
    }
}

NROM::NROM(std::string& filename)
    : file(*new boost::iostreams::mapped_file_source(filename)),
    mirroring(Cart::MirrorMode::HORIZONTAL)
{
    if (file.is_open())
    {
        // Read Byte 4 from the file to get the program size
        // For the type of ROM the emulator currently supports
        // this will be either 1 or 2 representing 0x4000 or 0x8000 bytes
        prgSize = file.data()[4] * 0x4000;
        chrSize = file.data()[5] * 0x2000;

        char flags6 = file.data()[6];
        mirroring = static_cast<Cart::MirrorMode>(flags6 & 0x01);

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

NROM::~NROM()
{
    file.close();
    delete &file;

    if (chrSize == 0)
    {
        delete[] chrRam;
    }
}

