/*
 * memory.cc
 *
 *  Created on: Mar 15, 2014
 *      Author: Dale
 */

#include <iostream>
#include <iomanip>
#include <fstream>
#include "ram.h"

using namespace std;

RAM::RAM(string filename):
		iram(new char[0x800]),
		apuioreg(new char[0x20]),
		cartspace(new char[0xBFE0])
{
	// Initialize all internal ram to 0xFF
	for (int i = 0; i < 0x800; i++)
	{
		iram[i] = 0xFF;
	}

	// Initialize apu registers to 0
	for (int i = 0; i < 0x20; i++)
	{
		apuioreg[i] = 0;
	}

	// Open file stream to ROM file
	ifstream rom(filename.c_str(), ifstream::in);

	if (!rom.fail())
	{
		// Read Byte 4 from the file to get the program size
		// For the type of ROM the emulator currently supports
		// this will be either 1 or 2 representing 0x4000 or 0x8000 bytes
		rom.seekg(0x04, rom.beg);
		int prgSize = rom.get() * 0x4000;

		// Initialize read buffer
		char * buffer = new char[prgSize];

		// Read entire program to buffer
		rom.seekg(0x10, rom.beg);
		rom.read(buffer, prgSize);
		rom.close();

		// This region of memory is normally used for battery backed saves
		// but that is currently unsupported so it is simply initialized to 0 here
		for (int i = 0x1FE0; i < 0x3FE0; i++)
		{
			cartspace[i] = 0;
		}

		// If the program size is 0x8000 bytes then the program
		// is simply written into memory from 0x3FE0 to 0xBFE0
		if (prgSize == 0x8000)
		{
			int i = 0x3FE0;
			while (i < 0xBFE0)
			{
				cartspace[i] = buffer[i - 0x3FE0];
				i++;
			}
		}

		// If the program size is 0x4000 bytes then the program
		// is written into memory from 0x3FE0 to 0x7FE0 and then mirrored
		// from 0x7FE0 to 0xBFE0
		if (prgSize == 0x4000)
		{
			int i = 0x3FE0;
			while (i < 0x7FE0)
			{
				cartspace[i] = cartspace[i + 0x4000] = buffer[i - 0x3FE0];
				i++;
			}
		}

		delete[] buffer; // Delete read buffer

	}
	else
	{
		cout << "Open Failed!!" << endl;
	}
}

// Reads a byte from the specified address
// Since in the actual NES addresses are incompletely decoded
// sections of memory are often mirrored several times
unsigned char RAM::read(unsigned short int address)
{
	// Any address less then 0x2000 is just the
	// Internal Ram mirrored every 0x800 bytes
	if (address < 0x2000)
	{
		return iram[address % 0x800];
	}

	// This region is reserved for the GPU registers
	// once the GPU is implemented
	if (address >= 0x2000 && address < 0x4000)
	{
		return 0x0000;
	}

	// This region is reserver for audio processor registers
	if (address >= 0x4000 && address < 0x4020)
	{
		address -= 0x4000;
		return apuioreg[address];
	}

	// This region is the where the ROM file is placed
	if (address >= 0x4020)
	{
		address -= 0x4020;
		return cartspace[address];
	}
	return 0;
}

// Writes a byte to the specified address
void RAM::write(unsigned char M, unsigned short int address)
{
	if (address < 0x2000)
	{
		iram[address % 0x800] = M;
	}

	if (address >= 0x2000 && address < 0x4000)
	{
		// Writes to GPU registers are currently ignored
	}

	if (address >= 0x4000 && address < 0x4020)
	{
		address -= 0x4000;
		apuioreg[address] = M;
	}

	if (address >= 0x4020)
	{
		address -= 0x4020;
		cartspace[address] = M;
	}
}

RAM::~RAM()
{
	// Deallocate all memory arrays
	delete[] iram;
	delete[] apuioreg;
	delete[] cartspace;
}


