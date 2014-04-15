/*
 * memory
 *
 *  Created on: Mar 15, 2014
 *      Author: Dale
 */

#ifndef MEMORY_
#define MEMORY_

#include <string>
#include "ppu.h"

class RAM
{
	PPU* ppu;

	char* iram;
	char* apuioreg;
	char* cartspace;

public:
	RAM(std::string filename);
	void attachPPU(PPU* ppu);
	unsigned char read(unsigned short int address);
	void write(unsigned char M, unsigned short int address);
	~RAM();
};


#endif /* MEMORY_ */
