/*
 * nes.h
 *
 *  Created on: Aug 8, 2014
 *      Author: Dale
 */

#ifndef NES_H_
#define NES_H_

#include <string>
#include <iostream>
#include <mutex>

#include "cpu.h"
#include "ppu.h"
#include "mappers/cart.h"

class NES
{
	long int clock;
	bool stop;
	std::mutex mtx;

	Cart* cart;
	CPU* cpu;
	//PPU* ppu;

public:
#ifdef DEBUG
	void setLogStream(std::ostream& out);
#endif

	NES(std::string filename);

	bool isStopped();

	void Start();
	void Stop();
	void Pause();
	void Reset();

	~NES();
};


#endif /* NES_H_ */
