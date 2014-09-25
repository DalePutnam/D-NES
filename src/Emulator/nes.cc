/*
 * nes.cc
 *
 *  Created on: Aug 28, 2014
 *      Author: Dale
 */

#include <sstream>

#include "nes.h"
#include "mappers/nrom.h"

#ifdef DEBUG
void NES::setLogStream(std::ostream& out)
{
	cpu->setLogStream(out);
}
#endif

NES::NES(std::string filename) : clock(0), stop(true)
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
			cart = new NROM(rom);
			break;
		default:
			rom.close();
			std::ostringstream oss;
			oss << "Mapper " << (int) mapper_number << " specified by " << filename << " does not exist.";
			throw oss.str();
			break;
		}

		cpu = new CPU(*this, cart, &clock);
		//ppu = new PPU(cart, &clock);
	}
}

bool NES::isStopped()
{
	bool isStopped;
	mtx.lock();
	isStopped = stop;
	mtx.unlock();
	return isStopped;
}

void NES::Start()
{
	mtx.lock();
	stop = false;
	mtx.unlock();

	cpu->Run(-1);

	mtx.lock();
	stop = true;
	mtx.unlock();
}

void NES::Stop()
{
	mtx.lock();
	stop = true;
	mtx.unlock();
}

void NES::Pause() {}
void NES::Reset() {}

NES::~NES()
{
	delete cpu;
	//delete ppu;
	delete cart;
}

