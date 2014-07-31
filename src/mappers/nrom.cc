/*
 * nrom.cc
 *
 *  Created on: Jul 8, 2014
 *      Author: Dale
 */

#include <iostream>
#include "nrom.h"

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
	else
	{
		prg[address - 0x2000] = M;
	}
}

unsigned char NROM::ChrRead(unsigned short int address) { return 0; }

void NROM::ChrWrite(unsigned char M, unsigned short int address) {}

NROM::NROM(std::ifstream& rom) : mirroring(0)
{
	if (!rom.fail())
	{
		// Read Byte 4 from the file to get the program size
		// For the type of ROM the emulator currently supports
		// this will be either 1 or 2 representing 0x4000 or 0x8000 bytes
		rom.seekg(0x04, rom.beg);
		prgSize = rom.get() * 0x4000;
		chrSize = rom.get() * 0x2000;

		prg = new char[prgSize];

		// Read entire program to memory
		rom.seekg(0x10, rom.beg);
		rom.read(prg, prgSize);

		// initialize chr RAM or read chr to memory
		if (chrSize == 0)
		{
			chr = new char[0x2000];
			for (int i = 0; i < 0x2000; i++)
			{
				chr[i] = 0;
			}
		}
		else
		{
			chr = new char[chrSize];
			rom.read(chr, chrSize);
		}
	}
	else
	{
		std::cout << "Open Failed!!" << std::endl;
	}
	rom.close();
}

NROM::~NROM()
{
	delete [] prg;
	delete [] chr;
}

