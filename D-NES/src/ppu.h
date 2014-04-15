/*
 * ppu.h
 *
 *  Created on: Mar 21, 2014
 *      Author: Dale
 */

#ifndef PPU_H_
#define PPU_H_

#include "vram.h"

class PPU
{
	// RAM
	VRAM* vram;

	// Registers
	char ppuctrl;
	char ppumask;
	char ppustatus;
	char oamaddr;
	char ppuscroll;
	short int ppuaddr;
	short int tempaddr;
	char finex;
	char addrlatch;

	// OAM
	char* primary;
	char* secondaryOAM;
	int OAMCounter;

	// counters
	bool even;
	int scanline;
	int dot;

	// Shift Registers
	short int bgBitmapLow;
	short int bgBitmapHigh;

	char* spBitmapLow;
	char* spBitmapHigh;
	char* spAttribute;
	char* spXPos;
	int spriteCounter;

	// Latches and Counters

	short int patternTableAddr;
	char activeAttribute;
	char tileBitmapLow;
	char tileBitmapHigh;




	char* buffer;

	void render();

public:
	PPU(VRAM* vram);
	int Run(int cyc);
	void Reset();

	unsigned char getPPUCTRL();
	unsigned char getPPUMASK();
	unsigned char getPPUSTATUS();
	unsigned char getOAMADDR();
	unsigned char getOAMDATA();
	unsigned char getPPUSCROLL();
	unsigned char getPPUADDR();
	unsigned char getPPUDATA();

	void setPPUCTRL(unsigned char M);
	void setPPUMASK(unsigned char M);
	void setOAMADDR(unsigned char M);
	void setOAMDATA(unsigned char M);
	void setPPUSCROLL(unsigned char M);
	void setPPUADDR(unsigned char M);
	void setPPUDATA(unsigned char M);

	~PPU();
};


#endif /* PPU_H_ */
