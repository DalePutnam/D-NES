/*
 * cart.h
 *
 *  Created on: Jul 8, 2014
 *      Author: Dale
 */

#ifndef CART_H_
#define CART_H_

#include <fstream>
#include <string>
#include "mappers/mapper.h"
#include "mappers/nrom.h"

class Cart
{
	Mapper* mapper;

public:
	unsigned char PrgRead(unsigned short int address);
	void PrgWrite(unsigned char M, unsigned short int address);

	unsigned char ChrRead(unsigned short int address);
	void ChrWrite(unsigned char M, unsigned short int address);

	Cart(std::string filename);
	~Cart();
};


#endif /* CARTRIDGE_H_ */
