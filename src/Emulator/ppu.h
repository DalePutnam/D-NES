/*
 * ppu.h
 *
 *  Created on: Aug 7, 2014
 *      Author: Dale
 */

#ifndef PPU_H_
#define PPU_H_

#include <queue>
#include <vector>

class PPU
{
	Cart* cart;

	// Main Registers
	unsigned char ppuctrl;
	unsigned char ppumask;
	unsigned char ppustatus;
	unsigned char ppuscroll;
	unsigned char oamaddr;
	unsigned char ppuaddr;

	bool addrlatch;

	// Main Register Buffers
	std::queue<std::vector<int, unsigned char>> ctrlbuffer;
	std::queue<std::vector<int, unsigned char>> maskbuffer;
	std::queue<std::vector<int, unsigned char>> scrollbuffer;
	std::queue<std::vector<int, unsigned char>> oamaddrbuffer;
	std::queue<std::vector<int, unsigned char>> ppuaddrbuffer;
	std::queue<std::vector<int, unsigned char>> oamdatabuffer;
	std::queue<std::vector<int, unsigned char>> ppudatabuffer;

	// PPU Data Read Buffer
	unsigned char databuffer;

	// Primary Object Attribute Memory (OAM)
	unsigned char primaryoam[0x100];

	// Secondary Object Attribute Memory (OAM)
	unsigned char secondaryoam[0x20];

	// Internal VRAM
	unsigned char vram[0x800];

	// Shift Registers, Latches and Counters
	unsigned short int bshift0;
	unsigned short int bshift1;
	unsigned char attribute0;
	unsigned char attribute1;

	unsigned char spshift0;
	unsigned char spshift1;
	unsigned char spshift2;
	unsigned char spshift3;
	unsigned char spshift4;
	unsigned char spshift5;
	unsigned char spshift6;
	unsigned char spshift7;

	unsigned char splatch0;
	unsigned char splatch1;
	unsigned char splatch2;
	unsigned char splatch3;
	unsigned char splatch4;
	unsigned char splatch5;
	unsigned char splatch6;
	unsigned char splatch7;

	unsigned char spcounter0;
	unsigned char spcounter1;
	unsigned char spcounter2;
	unsigned char spcounter3;
	unsigned char spcounter4;
	unsigned char spcounter5;
	unsigned char spcounter6;
	unsigned char spcounter7;

public:
	PPU(Cart* cart, long int* clock);

	unsigned char ReadPPUStatus();
	unsigned char ReadOAMData();
	unsigned char ReadPPUData();

	void WritePPUCTRL(unsigned char M);
	void WritePPUMASK(unsigned char M);
	void WriteOAMADDR(unsigned char M);
	void WriteOAMDATA(unsigned char M);
	void WritePPUSCROLL(unsigned char M);
	void WritePPUADDR(unsigned char M);
	void WritePPUDATA(unsigned char M);

	void SyncPPU();
	void RunVB();
	void Run(int cyc);

	~PPU();
};

#endif /* PPU_H_ */
