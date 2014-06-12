/*
 * memory
 *
 *  Created on: Mar 15, 2014
 *      Author: Dale
 */

#ifndef MEMORY_
#define MEMORY_

#include <string>

class RAM
{
	char* iram; // Internal RAM
	char* apuioreg; // Audio processor registers, can be written to but currently do nothing
	char* cartspace; // Space where the ROM file is written to

public:
	RAM(std::string filename);
	unsigned char read(unsigned short int address); // Read from specified address
	void write(unsigned char M, unsigned short int address); // Write to specified address
	~RAM();
};


#endif /* MEMORY_ */
