/*
 * vram.h
 *
 *  Created on: Mar 21, 2014
 *      Author: Dale
 */

#ifndef VRAM_H_
#define VRAM_H_

#include <string>

class VRAM
{
	int mirrorMode;

	char* chr;
	char* nameTable0;
	char* nameTable1;
	char* paletteRam;

public:
	VRAM(std::string filename);
	unsigned char read(unsigned short int address);
	void write(unsigned char M, unsigned short int address);
	~VRAM();
};



#endif /* VRAM_H_ */
