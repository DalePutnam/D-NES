/*
 * vram.cc
 *
 *  Created on: Mar 21, 2014
 *      Author: Dale
 */
#include <iostream>
#include <iomanip>
#include <fstream>
#include "vram.h"

using namespace std;

VRAM::VRAM(string filename):
		mirrorMode(0),
		chr(new char[0x2000]),
		nameTable0(new char[0x400]),
		nameTable1(new char[0x400]),
		paletteRam(new char[0x20])
{
	for (int i = 0; i < 0x400; i++)
	{
		nameTable0[i] = 0x00;
	}

	for (int i = 0; i < 0x400; i++)
	{
		nameTable1[i] = 0;
	}

	for (int i = 0; i < 0x20; i++)
	{
		paletteRam[i] = 0;
	}

	ifstream rom(filename.c_str(), ifstream::in);

	if (!rom.fail())
	{
		rom.seekg(0x04, rom.beg);
		int prgSize = rom.get() * 0x4000;

		rom.seekg(0x10 + prgSize, rom.beg);
		rom.read(chr, 0x2000);
		rom.close();
	}
	else
	{
		cout << "Open Failed!!" << endl;
	}
}

unsigned char VRAM::read(unsigned short int address) {return 0;}
void VRAM::write(unsigned char M, unsigned short int address) {}

VRAM::~VRAM()
{
	delete[] chr;
	delete[] nameTable0;
	delete[] nameTable1;
	delete[] paletteRam;
}




