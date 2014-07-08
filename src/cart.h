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
	unsigned char Read(unsigned short int address);
	void Write(unsigned char M, unsigned short int address);

	Cart(std::string filename);
	~Cart();
};


#endif /* CARTRIDGE_H_ */
