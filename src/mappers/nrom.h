/*
 * nrom.h
 *
 *  Created on: Jul 8, 2014
 *      Author: Dale
 */

#ifndef NROM_H_
#define NROM_H_

#include <fstream>
#include "mapper.h"

class NROM : public Mapper
{
	int mirroring;
	int chrSize;
	int prgSize;

	char* prg;
	char* chr;

public:
	unsigned char Read(unsigned short int address);
	void Write(unsigned char M, unsigned short int address);

	NROM(std::ifstream& rom);
	~NROM();
};



#endif /* NROM_H_ */
