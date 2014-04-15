/*
 * logger.cc
 *
 *  Created on: Mar 17, 2014
 *      Author: Dale
 */
#include <iostream>
#include <iomanip>
#include <sstream>
#include "logger.h"

using namespace std;

Logger::Logger():
	memory(0),
	opcode(0),
	opname(""),
	addrArg1(-1),
	addrArg2(-1),
	addrMode(""),
	cycles(0),
	PC(0),
	A(0),
	X(0),
	Y(0),
	P(0),
	S(0),
	special(false)
{
}

void Logger::attachMemory(RAM* memory)
{
	this->memory = memory;
}

void Logger::logOpCode(int opcode)
{
	this->opcode = opcode;
}

void Logger::logOpName(string opname)
{
	this->opname = opname;
}

void Logger::logAddressingMode(string mode, unsigned short int addr)
{
	ostringstream oss;
	if (mode.compare("Immediate") == 0)
	{
		oss << hex << uppercase << setfill('0') << setw(2) <<  addrArg1;
		addrMode = "#$" + oss.str();
	}
	else if (mode.compare("Relative") == 0)
	{
		unsigned short int val = (unsigned short int) (((short int) PC) + ((char) addr) + 2) ;
		oss << hex << uppercase << setfill('0') << setw(4) << val;
		addrMode = "$" + oss.str();
	}
	else if (mode.compare("Accumulator") == 0)
	{
		addrMode = "A";
	}
	else if (mode.compare("ZeroPage") == 0)
	{
		int val = memory->read(addr);
		oss << hex << uppercase << setfill('0') << setw(2) <<  addrArg1;
		string lvalue = oss.str();
		oss.str("");
		oss << hex << uppercase << setfill('0') << setw(2) << val;
		string rvalue = oss.str();
		addrMode = "$" + lvalue + " = " + rvalue;
	}
	else if (mode.compare("ZeroPageX") == 0)
	{
		int val = memory->read((addr + X) % 0x100);
		oss << hex << uppercase << setfill('0') << setw(2) <<  addrArg1 << ",X @ " << setfill('0') << setw(2) <<  ((int) ((addrArg1 + X) % 0x100));
		string lvalue = oss.str();
		oss.str("");
		oss << hex << uppercase << setfill('0') << setw(2) << val;
		string rvalue = oss.str();
		addrMode = "$" + lvalue + " = " + rvalue;
	}
	else if (mode.compare("ZeroPageY") == 0)
	{
		int val = memory->read((addr + Y) % 0x100);
		oss << hex << uppercase << setfill('0') << setw(2) <<  addrArg1 << ",Y @ " << setfill('0') << setw(2) <<  ((int) ((addrArg1 + Y) % 0x100));
		string lvalue = oss.str();
		oss.str("");
		oss << hex << uppercase << setfill('0') << setw(2) << val;
		string rvalue = oss.str();
		addrMode = "$" + lvalue + " = " + rvalue;
	}
	else if (mode.compare("Absolute") == 0 && !special)
	{
		int val = memory->read(addr);
		oss << hex << uppercase << setfill('0') << setw(4) << addr;
		string lvalue = oss.str();
		oss.str("");
		oss << hex << uppercase << setfill('0') << setw(2) << val;
		string rvalue = oss.str();
		addrMode = "$" + lvalue + " = " + rvalue;
	}
	else if (mode.compare("Absolute") == 0 && special)
	{
		oss << hex << uppercase << setfill('0') << setw(4) << addr;
		string lvalue = oss.str();
		addrMode = "$" + lvalue;
	}
	else if (mode.compare("AbsoluteX") == 0)
	{
		int val = memory->read(addr + X);
		oss << hex << uppercase << setfill('0') << setw(4) << addr << ",X @ " << setfill('0') << setw(4) << (unsigned short int) (addr + X);
		string lvalue = oss.str();
		oss.str("");
		oss << hex << uppercase << setfill('0') << setw(2) << val;
		string rvalue = oss.str();
		addrMode = "$" + lvalue + " = " + rvalue;
	}
	else if (mode.compare("AbsoluteY") == 0)
	{
		int val = memory->read(addr + Y);
		oss << hex << uppercase << setfill('0') << setw(4) << addr << ",Y @ " << setfill('0') << setw(4) << (unsigned short int) (addr + Y);
		string lvalue = oss.str();
		oss.str("");
		oss << hex << uppercase << setfill('0') << setw(2) << val;
		string rvalue = oss.str();
		addrMode = "$" + lvalue + " = " + rvalue;
	}
	else if (mode.compare("IndexedIndirect") == 0)
	{
		unsigned char arg = (unsigned char) addr;
		unsigned short int addrfin = memory->read((unsigned char) (arg + X)) + (((unsigned short int) memory->read((unsigned char) (arg + X + 1))) * 0x100);
		int val = memory->read(addrfin);
		oss << hex << uppercase << setfill('0') << setw(2) <<  addrArg1 << ",X) @ " << setfill('0') << setw(2) << ((int) ((arg + X) % 0x100));
		string lvalue = oss.str();
		oss.str("");
		oss << hex << uppercase << setfill('0') << setw(4) << addrfin;
		string mvalue = oss.str();
		oss.str("");
		oss << hex << uppercase << setfill('0') << setw(2) << val;
		string rvalue = oss.str();
		addrMode = "($" + lvalue + " = " + mvalue + " = " + rvalue;
	}
	else if (mode.compare("IndirectIndexed") == 0)
	{
		unsigned char arg = (unsigned char) addr;
		unsigned short int addrfin = memory->read(arg) + (((unsigned short int) memory->read((unsigned char) (arg + 1))) * 0x100);
		int val = memory->read((unsigned short int) (addrfin + Y));
		oss << hex << uppercase << setfill('0') << setw(2) <<  addrArg1 << "),Y = " << setfill('0') << setw(4) << addrfin;
		string lvalue = oss.str();
		oss.str("");
		oss << hex << uppercase << setfill('0') << setw(4) << (unsigned short int) (addrfin + Y);
		string mvalue = oss.str();
		oss.str("");
		oss << hex << uppercase << setfill('0') << setw(2) << val;
		string rvalue = oss.str();
		addrMode = "($" + lvalue + " @ " + mvalue + " = " + rvalue;
	}
	else if (mode.compare("Indirect") == 0)
	{
		unsigned short int val = memory->read(addr) + (((unsigned short int) memory->read(addr + 1)) * 0x100);
		oss << hex << uppercase << setfill('0') << setw(4) <<  addr << ") = " << setfill('0') << setw(4) << val;
		addrMode = "($" + oss.str();
	}
}

void Logger::setSpecial()
{
	special = true;
}

void Logger::logAddressingArgs(int arg1, int arg2)
{
	this->addrArg1 = arg1;
	this->addrArg2 = arg2;
}

void Logger::logAddressingArgs(int arg1)
{
	this->addrArg1 = arg1;
	this->addrArg2 = -1;
}

void Logger::logCycles(int cycles)
{
	this->cycles = cycles;
}

void Logger::logProgramCounter(int counter)
{
	this->PC = counter;
}

void Logger::logRegisters(int A, int X, int Y, int P, int S)
{
	this->A = A;
	this->X = X;
	this->Y = Y;
	this->P = P;
	this->S = S;
}

void Logger::printLog()
{
	cout << hex << uppercase << setfill('0') << setw(4) << PC << "  ";
	cout << setfill('0') << setw(2) << opcode << " ";
	//cout << setfill('0') << setw(2) << addrArg1 << " ";

	if (addrArg1 == -1)
	{
		cout << "   ";
	}
	else
	{
		cout << setfill('0') << setw(2) << addrArg1 << " ";
	}

	if (addrArg2 == -1)
	{
		cout << "  ";
	}
	else
	{
		cout << setfill('0') << setw(2) << addrArg2;
	}

	cout << "  " << opname << " ";// << "A:";
	cout << addrMode << setfill(' ') << setw(30 - addrMode.length()) << "A:";
	cout << setfill('0') << setw(2) << A << " X:";
	cout << setfill('0') << setw(2) << X << " Y:";
	cout << setfill('0') << setw(2) << Y << " P:";
	cout << setfill('0') << setw(2) << P << " SP:";
	cout << setfill('0') << setw(2) << S << endl;/*" CYC:";
	cout << dec << cycles << endl;*/

	opcode = 0;
	opname = "";
	addrArg1 = -1;
	addrArg2 = -1;
	addrMode = "";
	PC = 0;
	A = 0;
	X = 0;
	Y = 0;
	P = 0;
	S = 0;
	special = false;
}


