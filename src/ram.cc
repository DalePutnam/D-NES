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
		ppu(0),
		iram(new char[0x800]),
		apuioreg(new char[0x20]),
		cartspace(new char[0xBFE0])
{
	for (int i = 0; i < 0x800; i++)
	{
		iram[i] = 0xFF;
	}

	for (int i = 0; i < 0x20; i++)
	{
		apuioreg[i] = 0;
	}

	ifstream rom(filename.c_str(), ifstream::in);

	if (!rom.fail())
	{
		rom.seekg(0x04, rom.beg);
		int prgSize = rom.get() * 0x4000;

		char * buffer = new char[prgSize];

		rom.seekg(0x10, rom.beg);
		rom.read(buffer, prgSize);
		rom.close();

		for (int i = 0x1FE0; i < 0x3FE0; i++)
		{
			cartspace[i] = 0;
		}

		if (prgSize == 0x8000)
		{
			int i = 0x3FE0;
			while (i < 0xBFE0)
			{
				cartspace[i] = buffer[i - 0x3FE0];
				i++;
			}
		}

		if (prgSize == 0x4000)
		{
			int i = 0x3FE0;
			while (i < 0x7FE0)
			{
				cartspace[i] = cartspace[i + 0x4000] = buffer[i - 0x3FE0];
				i++;
			}
		}

		delete[] buffer;

	} else {
		cout << "Open Failed!!" << endl;
	}
}

void RAM::attachPPU(PPU* ppu)
{
	this->ppu = ppu;
}

unsigned char RAM::read(unsigned short int address)
{
	if (address < 0x2000)
	{
		return iram[address % 0x800];
	}

	if (address >= 0x2000 && address < 0x4000)
	{
		address -= 0x2000;
		address = address % 0x8;

		switch (address)
		{
		case 0x00:
			return ppu->getPPUCTRL();
		case 0x01:
			return ppu->getPPUMASK();
		case 0x02:
			return ppu->getPPUSTATUS();
		case 0x03:
			return ppu->getOAMADDR();
		case 0x04:
			return ppu->getOAMDATA();
		case 0x05:
			return ppu->getPPUSCROLL();
		case 0x06:
			return ppu->getPPUADDR();
		case 0x07:
			return ppu->getPPUDATA();
		default:
			return 0xFF;
		}
	}

	if (address >= 0x4000 && address < 0x4020)
	{
		address -= 0x4000;
		return apuioreg[address];
	}

	if (address >= 0x4020)
	{
		address -= 0x4020;
		return cartspace[address];
	}
	return 0;
}

void RAM::write(unsigned char M, unsigned short int address)
{
	if (address < 0x2000)
	{
		iram[address % 0x800] = M;
	}

	if (address >= 0x2000 && address < 0x4000)
	{
		address -= 0x2000;
		address = address % 0x8;

		switch (address)
		{
		case 0x00:
			ppu->setPPUCTRL(M);
			break;
		case 0x01:
			ppu->setPPUMASK(M);
			break;
		case 0x02:
			break;
		case 0x03:
			ppu->setOAMADDR(M);
			break;
		case 0x04:
			ppu->setOAMDATA(M);
			break;
		case 0x05:
			ppu->setPPUSCROLL(M);
			break;
		case 0x06:
			ppu->setPPUADDR(M);
			break;
		case 0x07:
			ppu->setPPUDATA(M);
			break;
		}
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
	delete[] iram;
	delete[] apuioreg;
	delete[] cartspace;
}


