/*
 * cart.cc
 *
 *  Created on: Jul 8, 2014
 *      Author: Dale
 */

#include <iostream>
#include "cart.h"

unsigned char Cart::PrgRead(unsigned short int address)
{
	return mapper->PrgRead(address);
}

void Cart::PrgWrite(unsigned char M, unsigned short int address)
{
	mapper->PrgWrite(M, address);
}

unsigned char Cart::ChrRead(unsigned short int address)
{
	return mapper->ChrRead(address);
}

void Cart::ChrWrite(unsigned char M, unsigned short int address)
{
	mapper->ChrWrite(M, address);
}

Cart::Cart(std::string filename)
{
	// Open file stream to ROM file
	std::ifstream rom(filename.c_str(), std::ifstream::in);

	if (!rom.fail())
	{
		// Read Bytes 6 and 7 to determine the mapper number
		rom.seekg(0x06, rom.beg);
		unsigned char flags6 = rom.get();
		unsigned char flags7 = rom.get();
		unsigned char mapper_number = (flags7 & 0xF0) | (flags6 >> 4);

		rom.seekg(0x00, rom.beg);

		switch (mapper_number)
		{
		case 0x00:
			mapper = new NROM(rom);
			break;
		default:
			rom.close();
			break;
		}
	}
}

Cart::~Cart()
{
	delete mapper;
}
