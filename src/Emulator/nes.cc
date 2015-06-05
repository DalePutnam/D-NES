/*
 * nes.cc
 *
 *  Created on: Aug 28, 2014
 *      Author: Dale
 */

#include "nes.h"
#include "mappers/nrom.h"
#include <boost/algorithm/string.hpp>

NES::NES(std::string filename, IDisplay& display, bool cpuLogEnabled)
	: clock(0),
	  scanline(241),
	  stop(true),
	  pause(false),
	  nmi(false),
	  cart(Cart::Create(filename)),
	  ppu(*new PPU(*this, cart, display)),
	  cpu(*new CPU(*this, ppu, cart, cpuLogEnabled))

{
	std::vector<std::string> stringList;
	boost::algorithm::split(stringList, filename, boost::is_any_of("\\/"));
	gameName = stringList.back().substr(0, stringList.back().length() - 4);
}

unsigned int NES::GetClock()
{
	return clock;
}

unsigned int NES::GetScanline()
{
	return scanline;
}

std::string& NES::GetGameName()
{
	return gameName;
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

void NES::EnableCPULog()
{
	cpu.EnableLog();
}

void NES::DisableCPULog()
{
	cpu.DisableLog();
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
	return cpu.IsPaused();
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

	if (cpu.IsPaused())
	{
		cpu.Resume();
	}
}

void NES::Resume()
{
	cpu.Resume();
}

void NES::Pause()
{
	cpu.Pause();
}

void NES::Reset() {}

NES::~NES()
{
	delete &cpu;
	delete &ppu;
	delete &cart;
}

