/*
 * mapper.h
 *
 *  Created on: Jul 8, 2014
 *      Author: Dale
 */

#ifndef MAPPER_H_
#define MAPPER_H_

#include <string>

class Cart
{
public:
	virtual unsigned char PrgRead(unsigned short int address) = 0;
	virtual void PrgWrite(unsigned char M, unsigned short int address) = 0;

	virtual unsigned char ChrRead(unsigned short int address) = 0;
	virtual void ChrWrite(unsigned char M, unsigned short int address) = 0;

	virtual ~Cart();
};




#endif /* MAPPER_H_ */
