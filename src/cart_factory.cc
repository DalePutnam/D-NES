/*
 * cart_factory.cc
 *
 *  Created on: Aug 7, 2014
 *      Author: Dale
 */

#include "cart_factory.h"
#include "mappers/nrom.h"

Cart* CartFactory::getCartridge(std::string filename)
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
			return new NROM(rom);
		default:
			rom.close();
			return 0;
			break;
		}
	}

	return 0;
}


