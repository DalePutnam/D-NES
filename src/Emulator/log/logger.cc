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

Logger::Logger() :
	outstream(&cout),
	opcode(0),
	opname(""),
	addrArg1(-1),
	addrArg2(-1),
	finalAddr(0),
	value(0),
	addrMode(""),
	cycles(0),
	scanlines(0),
	PC(0),
	A(0),
	X(0),
	Y(0),
	P(0),
	S(0),
	special(false)
{
}

void Logger::setLogStream(std::ostream& out)
{
	outstream = &out;
}

// Sets the current Opcode
void Logger::logOpCode(int opcode)
{
	this->opcode = opcode;
}

// Sets the current instruction name
void Logger::logOpName(const std::string& opname)
{
	this->opname = opname;
}

// Logs the current addressing modes in their specific formats
// Immediate:       #$10
// Relative:        $1000
// Accumulator:     A
// ZeroPage:        $00 = FF
// ZeroPageX:       $00,X @ 44 = FF
// ZeroPageY:       $00,Y @ 55 = FF
// Absolute:        $2000 = FF or $2000 if special is true
// AbsoluteX:       $2000,X @ 2044 = FF
// AbsoluteY:       $2000,Y @ 2055 = FF
// Indirect:        ($0200) = DB7E
// IndexedIndirect: ($80,X) @ 82 = 0300 = 5B
// IndirectIndexed: ($89),Y = 0300 @ 0300 = 89
void Logger::logAddressingMode(const std::string& mode, unsigned short int addr)
{
	ostringstream oss;
	if (mode.compare("Immediate") == 0)
	{
		oss << hex << uppercase << setfill('0') << setw(2) <<  addrArg1;
		addrMode = "#$" + oss.str();
	}
	else if (mode.compare("Relative") == 0)
	{
		unsigned short int val = (unsigned short int) (((short int) PC) + ((char) addr) + 2); // Get value
		oss << hex << uppercase << setfill('0') << setw(4) << val; // Format value
		addrMode = "$" + oss.str();
	}
	else if (mode.compare("Accumulator") == 0)
	{
		addrMode = "A";
	}
	else if (mode.compare("ZeroPage") == 0)
	{
		int val = value; // Get Value
		oss << hex << uppercase << setfill('0') << setw(2) <<  addrArg1; // Format address
		string lvalue = oss.str(); // Save formated address
		oss.str(""); // Blank string stream
		oss << hex << uppercase << setfill('0') << setw(2) << val; // Format value
		string rvalue = oss.str(); // Save formated value
		addrMode = "$" + lvalue + " = " + rvalue; // Combine into final string
	}
	else if (mode.compare("ZeroPageX") == 0)
	{
		int val = value; // Get Value
		oss << hex << uppercase << setfill('0') << setw(2) <<  addrArg1 << ",X @ " << setfill('0') << setw(2) <<  ((int) ((addrArg1 + X) % 0x100)); // Format address
		string lvalue = oss.str(); // Save formated address
		oss.str(""); // Blank string stream
		oss << hex << uppercase << setfill('0') << setw(2) << val; // Format value
		string rvalue = oss.str(); // Save formated value
		addrMode = "$" + lvalue + " = " + rvalue; // Combine into final string
	}
	else if (mode.compare("ZeroPageY") == 0)
	{
		int val = value; // Get Value
		oss << hex << uppercase << setfill('0') << setw(2) <<  addrArg1 << ",Y @ " << setfill('0') << setw(2) <<  ((int) ((addrArg1 + Y) % 0x100)); // Format address
		string lvalue = oss.str(); // Save formated address
		oss.str(""); // Blank string stream
		oss << hex << uppercase << setfill('0') << setw(2) << val; // Format value
		string rvalue = oss.str(); // Save formated value
		addrMode = "$" + lvalue + " = " + rvalue; // Combine into final string
	}
	else if (mode.compare("Absolute") == 0 && !special)
	{
		int val = value; // Get Value
		oss << hex << uppercase << setfill('0') << setw(4) << addr; // Format address
		string lvalue = oss.str(); // Save formated address
		oss.str(""); // Blank string stream
		oss << hex << uppercase << setfill('0') << setw(2) << val; // Format value
		string rvalue = oss.str(); // Save formated value
		addrMode = "$" + lvalue + " = " + rvalue; // Combine into final string
	}
	else if (mode.compare("Absolute") == 0 && special)
	{
		oss << hex << uppercase << setfill('0') << setw(4) << addr; // Format address
		string lvalue = oss.str();
		addrMode = "$" + lvalue; // Combine into final string
	}
	else if (mode.compare("AbsoluteX") == 0)
	{
		int val = value; // Get Value
		oss << hex << uppercase << setfill('0') << setw(4) << addr << ",X @ " << setfill('0') << setw(4) << (unsigned short int) (addr + X); // Format address
		string lvalue = oss.str(); // Save formated address
		oss.str(""); // Blank string stream
		oss << hex << uppercase << setfill('0') << setw(2) << val; // Format value
		string rvalue = oss.str(); // Save formated value
		addrMode = "$" + lvalue + " = " + rvalue; // Combine into final string
	}
	else if (mode.compare("AbsoluteY") == 0)
	{
		int val = value; // Get Value
		oss << hex << uppercase << setfill('0') << setw(4) << addr << ",Y @ " << setfill('0') << setw(4) << (unsigned short int) (addr + Y); // Format address
		string lvalue = oss.str(); // Save formated address
		oss.str(""); // Blank string stream
		oss << hex << uppercase << setfill('0') << setw(2) << val; // Format value
		string rvalue = oss.str(); // Save formated value
		addrMode = "$" + lvalue + " = " + rvalue; // Combine into final string
	}
	else if (mode.compare("IndexedIndirect") == 0)
	{
		unsigned char arg = (unsigned char) addr;
		unsigned short int addrfin = finalAddr; // get address
		int val = value; // Get Value
		oss << hex << uppercase << setfill('0') << setw(2) <<  addrArg1 << ",X) @ " << setfill('0') << setw(2) << ((int) ((arg + X) % 0x100)); // Format original address
		string lvalue = oss.str(); // Store original address
		oss.str(""); // Blank string stream
		oss << hex << uppercase << setfill('0') << setw(4) << addrfin; // Format final address
		string mvalue = oss.str(); // Store final address
		oss.str(""); // Blank string stream
		oss << hex << uppercase << setfill('0') << setw(2) << val; // Format value
		string rvalue = oss.str(); // Store value
		addrMode = "($" + lvalue + " = " + mvalue + " = " + rvalue; // Combine into final string
	}
	else if (mode.compare("IndirectIndexed") == 0)
	{
		unsigned short int addrfin = finalAddr; // get address
		int val = value; // Get Value
		oss << hex << uppercase << setfill('0') << setw(2) <<  addrArg1 << "),Y = " << setfill('0') << setw(4) << addrfin; // Format original address
		string lvalue = oss.str();  // Store original address
		oss.str(""); // Blank string stream
		oss << hex << uppercase << setfill('0') << setw(4) << (unsigned short int) (addrfin + Y); // Format final address
		string mvalue = oss.str(); // Store final address
		oss.str(""); // Blank string stream
		oss << hex << uppercase << setfill('0') << setw(2) << val; // Format value
		string rvalue = oss.str(); // Store value
		addrMode = "($" + lvalue + " @ " + mvalue + " = " + rvalue;  // Combine into final string
	}
	else if (mode.compare("Indirect") == 0)
	{
		unsigned short int val = value; // Get Value
		oss << hex << uppercase << setfill('0') << setw(4) <<  addr << ") = " << setfill('0') << setw(4) << val; // Format Value
		addrMode = "($" + oss.str();  // Combine into final string
	}
}

// Set Special flag
void Logger::setSpecial()
{
	special = true;
}

// Log the addressing mode arguments
void Logger::logAddressingArgs(int arg1, int arg2)
{
	this->addrArg1 = arg1;
	this->addrArg2 = arg2;
}

// Log the addressing mode arguments
void Logger::logAddressingArgs(int arg1)
{
	this->addrArg1 = arg1;
	this->addrArg2 = -1;
}

void Logger::logFinalAddress(int addr)
{
	this->finalAddr = addr;
}

void Logger::logValue(int val)
{
	this->value = val;
}

// Logs current cycle count
void Logger::logCycles(int cycles)
{
	this->cycles = cycles;
}

// Logs current cycle count
void Logger::logScanlines(int lines)
{
	scanlines = lines;
}

// Log Program Counter
void Logger::logProgramCounter(int counter)
{
	this->PC = counter;
}

// Log register values
void Logger::logRegisters(int A, int X, int Y, int P, int S)
{
	this->A = A;
	this->X = X;
	this->Y = Y;
	this->P = P;
	this->S = S;
}

// Print out all current log information
void Logger::printLog()
{
	std::ostream& out = *outstream;

	out << hex << uppercase << setfill('0') << setw(4) << PC << "  "; // Output Program counter followed by a space
	out << setfill('0') << setw(2) << opcode << " "; // Output opcode followed by a space

	if (addrArg1 == -1)
	{
		out << "   "; // if there are no arguments then output 3 spaces
	}
	else
	{
		out << setfill('0') << setw(2) << addrArg1 << " "; // otherwise output the first argument followed by a space
	}

	if (addrArg2 == -1)
	{
		out << "  "; // if there is only one argument output 2 spaces
	}
	else
	{
		out << setfill('0') << setw(2) << addrArg2; // otherwise output the second argument
	}

	out << "  " << opname << " ";// << "A:"; // output instruction name
	out << addrMode << setfill(' ') << setw(30 - addrMode.length()) << "A:"; // output addressing mode log, followed by filler spaces
	out << setfill('0') << setw(2) << A << " X:"; // Output Accumulator
	out << setfill('0') << setw(2) << X << " Y:"; // Output X register
	out << setfill('0') << setw(2) << Y << " P:"; // Output Y Register
	out << setfill('0') << setw(2) << P << " SP:"; // Output Processor Status
	out << setfill('0') << setw(2) << S << " CYC:"; // Output Stack Pointer
	out << dec << setfill(' ') << setw(3) << cycles << " SL:";
	out << scanlines << endl;
	// Reset logged values
	opcode = 0;
	opname = "";
	addrArg1 = -1;
	addrArg2 = -1;
	addrMode = "";
	finalAddr = 0;
	value = 0;
	PC = 0;
	A = 0;
	X = 0;
	Y = 0;
	P = 0;
	S = 0;
	special = false;
}


