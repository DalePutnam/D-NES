/*
 * nrom.h
 *
 *  Created on: Jul 8, 2014
 *      Author: Dale
 */

#ifndef NROM_H_
#define NROM_H_

#include <fstream>
#include <string>
#include "cart.h"

class NROM : public Cart
{
	int mirroring;
	int chrSize;
	int prgSize;

	char* prg;
	char* chr;

public:
	unsigned char PrgRead(unsigned short int address);
	void PrgWrite(unsigned char M, unsigned short int address);

	unsigned char ChrRead(unsigned short int address);
	void ChrWrite(unsigned char M, unsigned short int address);

	NROM(std::ifstream& rom);
	~NROM();
};



#endif /* NROM_H_ */
