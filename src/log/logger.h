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
	RAM* memory; // Pointer to RAM object
	int opcode; // Current OpCode
	std::string opname; // Current instruction name
	int addrArg1; // First address argument
	int addrArg2; // Second address argument
	std::string addrMode; // Current address mode name
	int cycles; // Current CPU Cycles
	int PC; // Current Program Counter
	int A; // Current Accumulator
	int X; // Current X Register
	int Y; // Current Y Register
	int P; // Current Processor Status
	int S; // Current Stack pointer

	bool special; // True if the next opcode treats the address provided by the addressing mode as the operand

public:
	Logger();
	void setSpecial(); // Set Special flag
	void attachMemory(RAM* memory); // Set memory pointer
	void logOpCode(int opcode); // Set OpCode
	void logOpName(std::string opname); // Set Instruction Name
	void logAddressingMode(std::string mode, unsigned short int addr = 0); // Set addressing mode name
	void logAddressingArgs(int arg1, int arg2); // Set both addressing arguments
	void logAddressingArgs(int arg1); // Set only the first addressing argumentss
	void logCycles(int cycles); // Set Cycles
	void logProgramCounter(int counter); // Set Program Counter
	void logRegisters(int A, int X, int Y, int P, int S); // Set all registers
	void printLog(); // Print out all current information
};

#endif /* LOGGER_H_ */
