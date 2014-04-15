/*
 * logger.h
 *
 *  Created on: Mar 17, 2014
 *      Author: Dale
 */

#ifndef LOGGER_H_
#define LOGGER_H_
#include <string>
#include "../ram.h"

class Logger
{
	RAM* memory;
	int opcode;
	std::string opname;
	int addrArg1;
	int addrArg2;
	std::string addrMode;
	int cycles;
	int PC;
	int A;
	int X;
	int Y;
	int P;
	int S;

	bool special;

public:
	Logger();
	void setSpecial();
	void attachMemory(RAM* memory);
	void logOpCode(int opcode);
	void logOpName(std::string opname);
	void logAddressingMode(std::string mode, unsigned short int addr = 0);
	void logAddressingArgs(int arg1, int arg2);
	void logAddressingArgs(int arg1);
	void logCycles(int cycles);
	void logProgramCounter(int counter);
	void logRegisters(int A, int X, int Y, int P, int S);
	void printLog();
};

#endif /* LOGGER_H_ */
