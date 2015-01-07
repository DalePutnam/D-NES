/*
 * nrom.h
 *
 *  Created on: Jul 8, 2014
 *      Author: Dale
 */

#ifndef NROM_H_
#define NROM_H_

#include "cart.h"
#include "boost/iostreams/device/mapped_file.hpp"

class NROM : public Cart
{
	boost::iostreams::mapped_file_source& file;

	MirrorMode mirroring;
	int chrSize;
	int prgSize;

	const char* prg;
	const char* chr;
	char* chrRam;

public:
	MirrorMode GetMirrorMode();

	unsigned char PrgRead(unsigned short int address);
	void PrgWrite(unsigned char M, unsigned short int address);

	unsigned char ChrRead(unsigned short int address);
	void ChrWrite(unsigned char M, unsigned short int address);

	NROM(std::string& filename);
	~NROM();
};



#endif /* NROM_H_ */
