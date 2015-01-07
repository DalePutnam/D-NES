/*
 * nes.cc
 *
 *  Created on: Aug 28, 2014
 *      Author: Dale
 */

#include "nes.h"
#include "mappers/nrom.h"

#ifdef DEBUG
void NES::setLogStream(std::ostream& out)
{
	cpu.setLogStream(out);
}
#endif

NES::NES(std::string filename, IDisplay& display)
	: clock(0),
	  scanline(241),
	  stop(true),
	  pause(false),
	  nmi(false),
	  cart(Cart::Create(filename)),
	  ppu(*new PPU(*this, cart, display)),
	  cpu(*new CPU(*this, ppu, cart))

{}

unsigned int NES::GetClock()
{
	return clock;
}

unsigned int NES::GetScanline()
{
	return scanline;
}

void NES::IncrementClock(int increment)
{
	unsigned int old = clock;
	clock += increment;

	if (clock % 341 < old % 341)
	{
		if (scanline == 260)
		{
			scanline = -1;
		}
		else
		{
			scanline++;
		}
	}
}

void NES::RaiseNMI()
{
	nmi = true;
}

bool NES::NMIRaised()
{
	bool value = nmi;
	nmi = false;
	return value;
}

void NES::GetNameTable(int table, unsigned char* pixels)
{
	ppu.GetNameTable(table, pixels);
}

void NES::GetPatternTable(int table, int palette, unsigned char* pixels)
{
	ppu.GetPatternTable(table, palette, pixels);
}

void NES::GetPalette(int palette, unsigned char* pixels)
{
	ppu.GetPalette(palette, pixels);
}

void NES::GetPrimaryOAM(int sprite, unsigned char* pixels)
{
	ppu.GetPrimaryOAM(sprite, pixels);
}

void NES::GetSecondaryOAM(int sprite, unsigned char* pixels)
{
	ppu.GetSecondaryOAM(sprite, pixels);
}

bool NES::IsStopped()
{
	bool isStopped;
	stopMutex.lock();
	isStopped = stop;
	stopMutex.unlock();
	return isStopped;
}

bool NES::IsPaused()
{
	bool isPaused;
	pauseMutex.lock();
	isPaused = pause;
	pauseMutex.unlock();
	return isPaused;
}

void NES::Start()
{
	stopMutex.lock();
	stop = false;
	stopMutex.unlock();

	cpu.Run();

	stopMutex.lock();
	stop = true;
	stopMutex.unlock();
}

void NES::Stop()
{
	stopMutex.lock();
	stop = true;
	stopMutex.unlock();
}

void NES::Resume()
{
	pauseMutex.lock();
	pause = false;
	pauseMutex.unlock();
}

void NES::Pause()
{
	pauseMutex.lock();
	pause = true;
	pauseMutex.unlock();
}

void NES::Reset() {}

NES::~NES()
{
	delete &cpu;
	delete &ppu;
	delete &cart;
}

