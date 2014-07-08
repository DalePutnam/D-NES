/*
 * mapper.h
 *
 *  Created on: Jul 8, 2014
 *      Author: Dale
 */

#ifndef MAPPER_H_
#define MAPPER_H_

#include <string>

class Mapper
{
public:
	virtual unsigned char Read(unsigned short int address) = 0;
	virtual void Write(unsigned char M, unsigned short int address) = 0;
	virtual ~Mapper() = 0;
};




#endif /* MAPPER_H_ */
