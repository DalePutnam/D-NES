/*
 * main.cc
 *
 *  Created on: Mar 15, 2014
 *      Author: Dale
 */

#include <ctime>
#include <string>
#include <iostream>

#include "cpu.h"
#include "memory.h"

using namespace std;


int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		cout << "No ROM specified" << endl;
	}
	else
	{
		string filename(argv[1]);
		VRAM* vram = new VRAM(filename);
		PPU* ppu = new PPU(vram);
		RAM* ram = new RAM(filename);
		CPU* cpu = new CPU(ram);
		ram->attachPPU(ppu);


		clock_t t1, t2, diff;
		t1 = clock();

		cpu->Run(-1);

		t2 = clock();
		diff = t2 - t1;

		float milliseconds = diff / (CLOCKS_PER_SEC / 1000);

		cout << "CPU ran for " << milliseconds << "ms" << endl;

		delete cpu;
		delete ram;
		delete ppu;
		delete vram;
	}

	return 0;
}


