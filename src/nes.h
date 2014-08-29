/*
 * nes.h
 *
 *  Created on: Aug 8, 2014
 *      Author: Dale
 */

#ifndef NES_H_
#define NES_H_

#include <string>

#include "cpu.h"
#include "ppu.h"
#include "mappers/cart.h"

class NES
{
	long int clock;

	Cart* cart;
	CPU* cpu;
	//PPU* ppu;

public:
	NES(std::string filename);

	void Start();
	void Pause();
	void Reset();

	~NES();
};


#endif /* NES_H_ */
