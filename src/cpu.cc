/*
 * cpu.cc
 *
 *  Created on: Mar 15, 2014
 *      Author: Dale
 */
#include <iostream>
#include "cpu.h"

unsigned short int CPU::ZeroPage()
{
#ifdef DEBUG
	logger.logAddressingArgs(memory->read(PC));
	logger.logAddressingMode("ZeroPage", memory->read(PC));
#endif
	return memory->read(PC++);
}

unsigned short int CPU::ZeroPageX()
{
#ifdef DEBUG
	logger.logAddressingArgs(memory->read(PC));
	logger.logAddressingMode("ZeroPageX", memory->read(PC));
#endif
	return (memory->read(PC++) + X) % 0x100;
}

unsigned short int CPU::ZeroPageY()
{
#ifdef DEBUG
	logger.logAddressingArgs(memory->read(PC));
	logger.logAddressingMode("ZeroPageY", memory->read(PC));
#endif
	return (memory->read(PC++) + Y) % 0x100;
}

unsigned short int CPU::Absolute()
{
#ifdef DEBUG
	logger.logAddressingArgs(memory->read(PC), memory->read(PC + 1));
	logger.logAddressingMode("Absolute", memory->read(PC) + (((unsigned short int) memory->read(PC + 1)) * 0x100));
#endif
	unsigned short int address = memory->read(PC) + (((unsigned short int) memory->read(PC + 1)) * 0x100);
	PC += 2;
	return address;
}

unsigned short int CPU::AbsoluteX()
{
#ifdef DEBUG
	logger.logAddressingArgs(memory->read(PC), memory->read(PC + 1));
	logger.logAddressingMode("AbsoluteX", (memory->read(PC) + (((unsigned short int) memory->read(PC + 1)) * 0x100)));
#endif
	unsigned short int arg = (memory->read(PC) + (((unsigned short int) memory->read(PC + 1)) * 0x100));
	unsigned short int address = arg + X;

	if ((address % 0x100) <= (arg % 0x100))
	{
		oops = 1;
	}

	PC += 2;
	return address;
}

unsigned short int CPU::AbsoluteY()
{
#ifdef DEBUG
	logger.logAddressingArgs(memory->read(PC), memory->read(PC + 1));
	logger.logAddressingMode("AbsoluteY", (memory->read(PC) + (((unsigned short int) memory->read(PC + 1)) * 0x100)));
#endif
	unsigned short int arg = (memory->read(PC) + (((unsigned short int) memory->read(PC + 1)) * 0x100));
	unsigned short int address = arg + Y;

	if ((address % 0x100) < (arg % 0x100))
	{
		oops = 1;
	}

	PC += 2;
	return address;
}

unsigned short int CPU::Indirect()
{
#ifdef DEBUG
	logger.logAddressingArgs(memory->read(PC), memory->read(PC + 1));
	logger.logAddressingMode("Indirect", memory->read(PC) + (((unsigned short int) memory->read(PC + 1)) * 0x100));
#endif
	unsigned short int addr = memory->read(PC) + (((unsigned short int) memory->read(PC + 1)) * 0x100);
	PC += 2;
	unsigned char lowbyte = (unsigned char) addr + 1;
	unsigned short int highaddr = (addr & 0xFF00) + lowbyte;
	return memory->read(addr) + (((unsigned short int) memory->read(highaddr)) * 0x100);
}

unsigned short int CPU::IndexedIndirect()
{
#ifdef DEBUG
	logger.logAddressingArgs(memory->read(PC));
	logger.logAddressingMode("IndexedIndirect", memory->read(PC));
#endif
	unsigned char arg = memory->read(PC++);
	return memory->read((unsigned char) (arg + X)) + (((unsigned short int) memory->read((unsigned char) (arg + X + 1))) * 0x100);
}

unsigned short int CPU::IndirectIndexed()
{
#ifdef DEBUG
	logger.logAddressingArgs(memory->read(PC));
	logger.logAddressingMode("IndirectIndexed", memory->read(PC));
#endif
	unsigned char arg = memory->read(PC++);
	//unsigned short int address = ((unsigned short int) memory->read(arg)) + memory->read((unsigned char) (arg + 1));
	unsigned short int address = memory->read(arg) + (((unsigned short int) memory->read((unsigned char) (arg + 1))) * 0x100);

	if (((address + Y) % 0x100) < (address % 0x100))
	{
		oops = 1;
	}

	return address + Y;
}

void CPU::ADC(unsigned char M)
{
#ifdef DEBUG
	logger.logOpName("ADC");
#endif
	unsigned char C = P & 0x01; // get carry flag
	unsigned char origA = A;
	short int result = A + M + C;

	// If overflow occurred, set the carry flag
	(result > 0xFF) ? P = P | 0x01 : P = P & 0xFE;

	A = (unsigned char) result;

	// if Result is 0 set zero flag
	(A == 0) ? P = P | 0x02 : P = P & 0xFD;
	// if bit seven is 1 set negative flag
	((A & 0x80) >> 7) == 1 ? P = P | 0x80 : P = P & 0x7F;
	// if signed overflow occurred set overflow flag
	((A >> 7) != ((origA & 0x80) >> 7) && ((A & 0x80) >> 7) != ((M & 0x80) >> 7)) ? P = P | 0x40 : P = P & 0xBF;
}

void CPU::AND(unsigned char M)
{
#ifdef DEBUG
	logger.logOpName("AND");
#endif
	A = A & M;

	// if Result is 0 set zero flag
	(A == 0) ? P = P | 0x02 : P = P & 0xFD;
	// if bit seven is 1 set negative flag
	(A >> 7) == 1 ? P = P | 0x80 : P = P & 0x7F;
}

unsigned char CPU::ASL(unsigned char M)
{
#ifdef DEBUG
	logger.logOpName("ASL");
#endif
	unsigned char origM = M;
	M = M << 1;

	// Set carry flag to bit 7 of origM
	(origM >> 7) == 1 ? P = P | 0x01 : P = P & 0xFE;
	// if Result is 0 set zero flag
	(M == 0) ? P = P | 0x02 : P = P & 0xFD;
	// if bit seven is 1 set negative flag
	(M >> 7) == 1 ? P = P | 0x80 : P = P & 0x7F;

	return M;
}

void CPU::BCC()
{
#ifdef DEBUG
	logger.logOpName("BCC");
	logger.logAddressingArgs(memory->read(PC));
#endif
	if ((P & 0x01) == 0)
	{
		unsigned char origPC = PC >> 2;
		PC = ((unsigned short int) (((short int) PC) + ((short int) memory->read(PC))));
		oops = 1;

		if (origPC != (PC >> 2))
		{
			oops = 2;
		}
	}
	PC++;
}

void CPU::BCS()
{
#ifdef DEBUG
	logger.logOpName("BCS");
	logger.logAddressingArgs(memory->read(PC));
#endif
	if ((P & 0x01) == 1)
	{
		unsigned char origPC = PC >> 2;
		PC = ((unsigned short int) (((short int) PC) + ((short int) memory->read(PC))));
		oops = 1;

		if (origPC != (PC >> 2))
		{
			oops = 2;
		}
	}
	PC++;
}

void CPU::BEQ()
{
#ifdef DEBUG
	logger.logOpName("BEQ");
	logger.logAddressingArgs(memory->read(PC));
#endif
	if (((P & 0x02) >> 1) == 1)
	{
		unsigned char origPC = PC >> 2;
		PC = ((unsigned short int) (((short int) PC) + ((short int) memory->read(PC))));
		oops = 1;

		if (origPC != (PC >> 2))
		{
			oops = 2;
		}
	}
	PC++;
}

void CPU::BIT(unsigned char M)
{
#ifdef DEBUG
	logger.logOpName("BIT");
#endif
	unsigned char result = A & M;

	// if Result is 0 set zero flag
	(result == 0) ? P = P | 0x02 : P = P & 0xFD;
	// Set overflow flag to bit 6 of M
	(((M & 0x40) >> 6) == 1) ? P = P | 0x40 : P = P & 0xBF;
	// Set negative flag to bit 7 of M
	(((M & 0x80) >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;
}

void CPU::BMI()
{
#ifdef DEBUG
	logger.logOpName("BMI");
	logger.logAddressingArgs(memory->read(PC));
#endif
	if (((P & 0x80) >> 7) == 1)
	{
		unsigned char origPC = PC >> 2;
		PC = ((unsigned short int) (((short int) PC) + ((short int) memory->read(PC))));
		oops = 1;

		if (origPC != (PC >> 2))
		{
			oops = 2;
		}
	}
	PC++;
}

void CPU::BNE()
{
#ifdef DEBUG
	logger.logOpName("BNE");
	logger.logAddressingArgs(memory->read(PC));
#endif
	if (((P & 0x02) >> 1) == 0)
	{
		unsigned char origPC = PC >> 2;
		PC = ((unsigned short int) (((short int) PC) + ((short int) memory->read(PC))));
		oops = 1;

		if (origPC != (PC >> 2))
		{
			oops = 2;
		}
	}
	PC++;
}

void CPU::BPL()
{
#ifdef DEBUG
	logger.logOpName("BPL");
	logger.logAddressingArgs(memory->read(PC));
#endif
	if (((P & 0x80) >> 7) == 0)
	{
		unsigned char origPC = PC >> 2;
		PC = ((unsigned short int) (((short int) PC) + ((short int) memory->read(PC))));
		oops = 1;

		if (origPC != (PC >> 2))
		{
			oops = 2;
		}
	}
	PC++;
}

void CPU::BRK()
{
#ifdef DEBUG
	logger.logOpName("BRK");
#endif
	memory->write(PC >> 4, 0x100 + S);
	memory->write(PC, 0x100 + (S - 1));
	memory->write(P | 0x30, 0x100 + (S - 2));
	S -= 3;

	PC = memory->read(0xFFFE) + (((unsigned short int) memory->read(0xFFFF)) * 0x100);
}

void CPU::BVC()
{
#ifdef DEBUG
	logger.logOpName("BVC");
	logger.logAddressingArgs(memory->read(PC));
#endif
	if (((P & 0x40) >> 6) == 0)
	{
		unsigned char origPC = PC >> 2;
		PC = ((unsigned short int) (((short int) PC) + ((short int) memory->read(PC))));
		oops = 1;

		if (origPC != (PC >> 2))
		{
			oops = 2;
		}
	}
	PC++;
}

void CPU::BVS()
{
#ifdef DEBUG
	logger.logOpName("BVS");
	logger.logAddressingArgs(memory->read(PC));
#endif
	if (((P & 0x40) >> 6) == 1)
	{
		unsigned char origPC = PC >> 2;
		PC = ((unsigned short int) (((short int) PC) + ((short int) memory->read(PC))));
		oops = 1;

		if (origPC != (PC >> 2))
		{
			oops = 2;
		}
	}
	PC++;
}

void CPU::CLC()
{
#ifdef DEBUG
	logger.logOpName("CLC");
#endif
	P = P & 0xFE;
}

void CPU::CLD()
{
#ifdef DEBUG
	logger.logOpName("CLD");
#endif
	P = P & 0xF7;
}

void CPU::CLI()
{
#ifdef DEBUG
	logger.logOpName("CLI");
#endif
	P = P & 0xFB;
}

void CPU::CLV()
{
#ifdef DEBUG
	logger.logOpName("CLV");
#endif
	P = P & 0xBF;
}

void CPU::CMP(unsigned char M)
{
#ifdef DEBUG
	logger.logOpName("CMP");
#endif
	unsigned char result = A - M;
	// if A >= M set carry flag
	(A >= M) ? P = P | 0x01 : P = P & 0xFE;
	// if A = M set zero flag
	(A == M) ? P = P | 0x02 : P = P & 0xFD;
	// set negative flag to bit 7 of result
	((result >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;
}

void CPU::CPX(unsigned char M)
{
#ifdef DEBUG
	logger.logOpName("CPX");
#endif
	unsigned char result = X - M;
	// if X >= M set carry flag
	(X >= M) ? P = P | 0x01 : P = P & 0xFE;
	// if X = M set zero flag
	(X == M) ? P = P | 0x02 : P = P & 0xFD;
	// set negative flag to bit 7 of result
	((result >> 7) == 1)? P = P | 0x80 : P = P & 0x7F;
}

void CPU::CPY(unsigned char M)
{
#ifdef DEBUG
	logger.logOpName("CPY");
#endif
	unsigned char result = Y - M;
	// if Y >= M set carry flag
	(Y >= M) ? P = P | 0x01 : P = P & 0xFE;
	// if Y = M set zero flag
	(Y == M) ? P = P | 0x02 : P = P & 0xFD;
	// set negative flag to bit 7 of result
	((result >> 7) == 1)? P = P | 0x80 : P = P & 0x7F;
}

unsigned char CPU::DEC(unsigned char M)
{
#ifdef DEBUG
	logger.logOpName("DEC");
#endif
	unsigned char result = M - 1;
	// if result is 0 set zero flag
	(result == 0) ? P = P | 0x02 : P = P & 0xFD;
	// set negative flag to bit 7 of result
	((result >> 7) == 1)? P = P | 0x80 : P = P & 0x7F;
	return result;
}

void CPU::DEX()
{
#ifdef DEBUG
	logger.logOpName("DEX");
#endif
	--X;
	// if X is 0 set zero flag
	(X == 0) ? P = P | 0x02 : P = P & 0xFD;
	// set negative flag to bit 7 of X
	((X >> 7) == 1)? P = P | 0x80 : P = P & 0x7F;
}

void CPU::DEY()
{
#ifdef DEBUG
	logger.logOpName("DEY");
#endif
	--Y;
	// if Y is 0 set zero flag
	(Y == 0) ? P = P | 0x02 : P = P & 0xFD;
	// set negative flag to bit 7 of Y
	((Y >> 7) == 1)? P = P | 0x80 : P = P & 0x7F;
}

void CPU::EOR(unsigned char M)
{
#ifdef DEBUG
	logger.logOpName("EOR");
#endif
	A = A ^ M;
	// if A is 0 set zero flag
	(A == 0) ? P = P | 0x02 : P = P & 0xFD;
	// set negative flag to bit 7 of A
	((A >> 7) == 1)? P = P | 0x80 : P = P & 0x7F;
}

unsigned char CPU::INC(unsigned char M)
{
#ifdef DEBUG
	logger.logOpName("INC");
#endif
	unsigned char result = M + 1;
	// if result is 0 set zero flag
	(result == 0) ? P = P | 0x02 : P = P & 0xFD;
	// set negative flag to bit 7 of result
	((result >> 7) == 1)? P = P | 0x80 : P = P & 0x7F;
	return result;
}

void CPU::INX()
{
#ifdef DEBUG
	logger.logOpName("INX");
#endif
	++X;
	// if X is 0 set zero flag
	(X == 0) ? P = P | 0x02 : P = P & 0xFD;
	// set negative flag to bit 7 of X
	((X >> 7) == 1)? P = P | 0x80 : P = P & 0x7F;
}

void CPU::INY()
{
#ifdef DEBUG
	logger.logOpName("INY");
#endif
	++Y;
	// if Y is 0 set zero flag
	(Y == 0) ? P = P | 0x02 : P = P & 0xFD;
	// set negative flag to bit 7 of Y
	((Y >> 7) == 1)? P = P | 0x80 : P = P & 0x7F;
}

void CPU::JMP(unsigned short int M)
{
#ifdef DEBUG
	logger.logOpName("JMP");
#endif
	PC = M;
}

void CPU::JSR(unsigned short int M)
{
#ifdef DEBUG
	logger.logOpName("JSR");
#endif
	PC--;
	memory->write(PC >> 8, 0x100 + S);
	memory->write(PC, 0x100 + (S - 1));
	S -= 2;

	PC = M;
}

void CPU::LDA(unsigned char M)
{
#ifdef DEBUG
	logger.logOpName("LDA");
#endif
	A = M;

	// if A is 0 set zero flag
	(A == 0) ? P = P | 0x02 : P = P & 0xFD;
	// set negative flag to bit 7 of A
	((A >> 7) == 1)? P = P | 0x80 : P = P & 0x7F;
}

void CPU::LDX(unsigned char M)
{
#ifdef DEBUG
	logger.logOpName("LDX");
#endif
	X = M;

	// if X is 0 set zero flag
	(X == 0) ? P = P | 0x02 : P = P & 0xFD;
	// set negative flag to bit 7 of X
	((X >> 7) == 1)? P = P | 0x80 : P = P & 0x7F;
}

void CPU::LDY(unsigned char M)
{
#ifdef DEBUG
	logger.logOpName("LDY");
#endif
	Y = M;

	// if Y is 0 set zero flag
	(Y == 0) ? P = P | 0x02 : P = P & 0xFD;
	// set negative flag to bit 7 of Y
	((Y >> 7) == 1)? P = P | 0x80 : P = P & 0x7F;
}

unsigned char CPU::LSR(unsigned char M)
{
#ifdef DEBUG
	logger.logOpName("LSR");
#endif
	unsigned char result = M >> 1;

	// set carry flag to bit 0 of M
	((M & 0x01) == 1) ? P = P | 0x01 : P = P & 0xFE;
	// if result is 0 set zero flag
	(result == 0) ? P = P | 0x02 : P = P & 0xFD;
	// set negative flag to bit 7 of result
	((result >> 7) == 1)? P = P | 0x80 : P = P & 0x7F;

	return result;
}

void CPU::NOP()
{
#ifdef DEBUG
	logger.logOpName("NOP");
#endif
}

void CPU::ORA(unsigned char M)
{
#ifdef DEBUG
	logger.logOpName("ORA");
#endif
	A = A | M;
	// if A is 0 set zero flag
	(A == 0) ? P = P | 0x02 : P = P & 0xFD;
	// set negative flag to bit 7 of A
	((A >> 7) == 1)? P = P | 0x80 : P = P & 0x7F;
}

void CPU::PHA()
{
#ifdef DEBUG
	logger.logOpName("PHA");
#endif
	memory->write(A, 0x100 + S);
	S--;
}

void CPU::PHP()
{
#ifdef DEBUG
	logger.logOpName("PHP");
#endif
	memory->write(P | 0x30, 0x100 + S);
	S--;
}

void CPU::PLA()
{
#ifdef DEBUG
	logger.logOpName("PLA");
#endif
	A = memory->read(0x100 + ++S);
	// if A is 0 set zero flag
	(A == 0) ? P = P | 0x02 : P = P & 0xFD;
	// set negative flag to bit 7 of A
	((A >> 7) == 1)? P = P | 0x80 : P = P & 0x7F;
}

void CPU::PLP()
{
#ifdef DEBUG
	logger.logOpName("PLP");
#endif
	P = (memory->read(0x100 + ++S) & 0xEF) | 0x20;
}

unsigned char CPU::ROL(unsigned char M)
{
#ifdef DEBUG
	logger.logOpName("ROL");
#endif
	unsigned char result = (M << 1) | (P & 0x01);
	// set carry flag to old bit 7
	((M >> 7) == 1) ? P = P | 0x01 : P = P & 0xFE;

	// if result is 0 set zero flag
	(result == 0) ? P = P | 0x02 : P = P & 0xFD;
	// set negative flag to bit 7 of result
	((result >> 7) == 1)? P = P | 0x80 : P = P & 0x7F;

	return result;
}

unsigned char CPU::ROR(unsigned char M)
{
#ifdef DEBUG
	logger.logOpName("ROR");
#endif
	unsigned char result = (M >> 1) | ((P & 0x01) << 7);
	// set carry flag to old bit 0
	((M & 0x01) == 1) ? P = P | 0x01 : P = P & 0xFE;
	// if result is 0 set zero flag
	(result == 0) ? P = P | 0x02 : P = P & 0xFD;
	// set negative flag to bit 7 of result
	((result >> 7) == 1)? P = P | 0x80 : P = P & 0x7F;

	return result;
}

void CPU::RTI()
{
#ifdef DEBUG
	logger.logOpName("RTI");
#endif
	P = (memory->read(0x100 + (S + 1)) & 0xEF) | 0x20;
	PC = memory->read(0x100 + (S + 2)) + (((unsigned short int) memory->read(0x100 + (S + 3))) * 0x100);
	S += 3;
}

void CPU::RTS()
{
#ifdef DEBUG
	logger.logOpName("RTS");
#endif
	PC = (memory->read(0x100 + (S + 1)) + (((unsigned short int) memory->read(0x100 + (S + 2))) * 0x100));
	PC++;
	S += 2;
}

void CPU::SBC(unsigned char M)
{
#ifdef DEBUG
	logger.logOpName("SBC");
#endif
	char C = P & 0x01; // get carry flag
	char signA = (char) A;
	char signM = (char) M;
	int result = signA - signM - (1 - C);

	// if signed overflow occurred set overflow flag
	(result > 127 || result < -128) ? P = P | 0x40 : P = P & 0xBF;
	// If overflow occurred, clear the carry flag
	(((char) result) >= 0) ? P = P | 0x01 : P = P & 0xFE;

	A = (unsigned char) result;

	// if Result is 0 set zero flag
	(A == 0) ? P = P | 0x02 : P = P & 0xFD;
	// if bit seven is 1 set negative flag
	((A & 0x80) >> 7) == 1 ? P = P | 0x80 : P = P & 0x7F;
}

void CPU::SEC()
{
#ifdef DEBUG
	logger.logOpName("SEC");
#endif
	P = P | 0x01;
}

void CPU::SED()
{
#ifdef DEBUG
	logger.logOpName("SED");
#endif
	P = P | 0x08;
}

void CPU::SEI()
{
#ifdef DEBUG
	logger.logOpName("SEI");
#endif
	P = P | 0x04;
}

unsigned char CPU::STA()
{
#ifdef DEBUG
	logger.logOpName("STA");
#endif
	return A;
}

unsigned char CPU::STX()
{
#ifdef DEBUG
	logger.logOpName("STX");
#endif
	return X;
}

unsigned char CPU::STY()
{
#ifdef DEBUG
	logger.logOpName("STY");
#endif
	return Y;
}

void CPU::TAX()
{
#ifdef DEBUG
	logger.logOpName("TAX");
#endif
	X = A;

	// Set zero flag if X is 0
	(X == 0) ? P = P | 0x02 : P = P & 0xFD;
	// Set negative flag if bit 7 of X is set
	((X >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;
}

void CPU::TAY()
{
#ifdef DEBUG
	logger.logOpName("TAY");
#endif
	Y = A;

	// Set zero flag if Y is 0
	(Y == 0) ? P = P | 0x02 : P = P & 0xFD;
	// Set negative flag if bit 7 of Y is set
	((Y >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;
}

void CPU::TSX()
{
#ifdef DEBUG
	logger.logOpName("TSX");
#endif
	X = S;

	// Set zero flag if X is 0
	(X == 0) ? P = P | 0x02 : P = P & 0xFD;
	// Set negative flag if bit 7 of X is set
	((X >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;
}

void CPU::TXA()
{
#ifdef DEBUG
	logger.logOpName("TXA");
#endif
	A = X;

	// Set zero flag if A is 0
	(A == 0) ? P = P | 0x02 : P = P & 0xFD;
	// Set negative flag if bit 7 of A is set
	((A >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;
}

void CPU::TXS()
{
#ifdef DEBUG
	logger.logOpName("TXS");
#endif
	S = X;
}

void CPU::TYA()
{
#ifdef DEBUG
	logger.logOpName("TYA");
#endif
	A = Y;

	// Set zero flag if A is 0
	(A == 0) ? P = P | 0x02 : P = P & 0xFD;
	// Set negative flag if bit 7 of A is set
	((A >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;
}

CPU::CPU(RAM* memory):
	memory(memory),
	cycles(0),
	oops(0),
	S(0xFD),
	P(0x24),
	A(0),
	X(0),
	Y(0)
{
#ifdef DEBUG
	logger.attachMemory(memory);
#endif
	PC = memory->read(0xFFFC) + (((unsigned short int) memory->read(0xFFFD)) * 0x100);
}

void CPU::Reset()
{
}

int CPU::Run(int cyc)
{
	cycles = 0;

	if (cyc == -1)
	{
		for(;;)
		{
			if(!NextOP()) break;
		}
	}
	else
	{
		while (cyc > cycles)
		{
			if(!NextOP()) break;
		}
	}

	return 0;
}

bool CPU::NextOP()
{
#ifdef DEBUG
	logger.logProgramCounter(PC);
	logger.logOpCode(memory->read(PC));
	logger.logRegisters(A, X, Y, P, S);
	logger.logCycles(cycles);
#endif
	unsigned char opcode = memory->read(PC++);
	unsigned short int addr;

	oops = 0;

	switch (opcode)
	{
	case 0x69:
#ifdef DEBUG
		logger.logAddressingArgs(memory->read(PC));
		logger.logAddressingMode("Immediate");
#endif
		ADC(memory->read(PC++));
		cycles += 2;
		break;
	case 0x65:
		ADC(memory->read(ZeroPage()));
		cycles += 3;
		break;
	case 0x75:
		ADC(memory->read(ZeroPageX()));
		cycles += 4;
		break;
	case 0x6D:
		ADC(memory->read(Absolute()));
		cycles += 4;
		break;
	case 0x7D:
		ADC(memory->read(AbsoluteX()));
		cycles += 4 + oops;
		break;
	case 0x79:
		ADC(memory->read(AbsoluteY()));
		cycles += 4 + oops;
		break;
	case 0x61:
		ADC(memory->read(IndexedIndirect()));
		cycles += 6;
		break;
	case 0x71:
		ADC(memory->read(IndirectIndexed()));
		cycles += 5 + oops;
		break;
	case 0x29:
#ifdef DEBUG
		logger.logAddressingArgs(memory->read(PC));
		logger.logAddressingMode("Immediate");
#endif
		AND(memory->read(PC++));
		cycles += 2;
		break;
	case 0x25:
		AND(memory->read(ZeroPage()));
		cycles += 3;
		break;
	case 0x35:
		AND(memory->read(ZeroPageX()));
		cycles += 4;
		break;
	case 0x2D:
		AND(memory->read(Absolute()));
		cycles += 4;
		break;
	case 0x3D:
		AND(memory->read(AbsoluteX()));
		cycles += 4 + oops;
		break;
	case 0x39:
		AND(memory->read(AbsoluteY()));
		cycles += 4 + oops;
		break;
	case 0x21:
		AND(memory->read(IndexedIndirect()));
		cycles += 6;
		break;
	case 0x31:
		AND(memory->read(IndirectIndexed()));
		cycles += 5 + oops;
		break;
	case 0x0A:
#ifdef DEBUG
		logger.logAddressingMode("Accumulator");
#endif
		A = ASL(A);
		cycles += 2;
		break;
	case 0x06:
		addr = ZeroPage();
		memory->write(ASL(memory->read(addr)), addr);
		cycles += 5;
		break;
	case 0x16:
		addr = ZeroPageX();
		memory->write(ASL(memory->read(addr)), addr);
		cycles += 6;
		break;
	case 0x0E:
		addr = Absolute();
		memory->write(ASL(memory->read(addr)), addr);
		cycles += 6;
		break;
	case 0x1E:
		addr = AbsoluteX();
		memory->write(ASL(memory->read(addr)), addr);
		cycles += 7;
		break;
	case 0x90:
#ifdef DEBUG
		logger.logAddressingMode("Relative", memory->read(PC));
#endif
		BCC();
		cycles += 2 + oops;
		break;
	case 0xB0:
#ifdef DEBUG
		logger.logAddressingMode("Relative", memory->read(PC));
#endif
		BCS();
		cycles += 2 + oops;
		break;
	case 0xF0:
#ifdef DEBUG
		logger.logAddressingMode("Relative", memory->read(PC));
#endif
		BEQ();
		cycles += 2 + oops;
		break;
	case 0x24:
		BIT(memory->read(ZeroPage()));
		cycles += 3;
		break;
	case 0x2C:
		BIT(memory->read(Absolute()));
		cycles += 4;
		break;
	case 0x30:
#ifdef DEBUG
		logger.logAddressingMode("Relative", memory->read(PC));
#endif
		BMI();
		cycles += 2 + oops;
		break;
	case 0xD0:
#ifdef DEBUG
		logger.logAddressingMode("Relative", memory->read(PC));
#endif
		BNE();
		cycles += 2 + oops;
		break;
	case 0x10:
#ifdef DEBUG
		logger.logAddressingMode("Relative", memory->read(PC));
#endif
		BPL();
		cycles += 2 + oops;
		break;
	case 0x00:
		BRK();
		cycles += 7;
		break;
	case 0x50:
#ifdef DEBUG
		logger.logAddressingMode("Relative", memory->read(PC));
#endif
		BVC();
		cycles += 2 + oops;
		break;
	case 0x70:
#ifdef DEBUG
		logger.logAddressingMode("Relative", memory->read(PC));
#endif
		BVS();
		cycles += 2 + oops;
		break;
	case 0x18:
		CLC();
		cycles += 2;
		break;
	case 0xD8:
		CLD();
		cycles += 2;
		break;
	case 0x58:
		CLI();
		cycles += 2;
		break;
	case 0xB8:
		CLV();
		cycles += 2;
		break;
	case 0xC9:
#ifdef DEBUG
		logger.logAddressingArgs(memory->read(PC));
		logger.logAddressingMode("Immediate");
#endif
		CMP(memory->read(PC++));
		cycles += 2;
		break;
	case 0xC5:
		CMP(memory->read(ZeroPage()));
		cycles += 3;
		break;
	case 0xD5:
		CMP(memory->read(ZeroPageX()));
		cycles += 4;
		break;
	case 0xCD:
		CMP(memory->read(Absolute()));
		cycles += 4;
		break;
	case 0xDD:
		CMP(memory->read(AbsoluteX()));
		cycles += 4 + oops;
		break;
	case 0xD9:
		CMP(memory->read(AbsoluteY()));
		cycles += 4 + oops;
		break;
	case 0xC1:
		CMP(memory->read(IndexedIndirect()));
		cycles += 6;
		break;
	case 0xD1:
		CMP(memory->read(IndirectIndexed()));
		cycles += 5 + oops;
		break;
	case 0xE0:
#ifdef DEBUG
		logger.logAddressingArgs(memory->read(PC));
		logger.logAddressingMode("Immediate");
#endif
		CPX(memory->read(PC++));
		cycles += 2;
		break;
	case 0xE4:
		CPX(memory->read(ZeroPage()));
		cycles += 3;
		break;
	case 0xEC:
		CPX(memory->read(Absolute()));
		cycles += 4;
		break;
	case 0xC0:
#ifdef DEBUG
		logger.logAddressingArgs(memory->read(PC));
		logger.logAddressingMode("Immediate");
#endif
		CPY(memory->read(PC++));
		cycles += 2;
		break;
	case 0xC4:
		CPY(memory->read(ZeroPage()));
		cycles += 3;
		break;
	case 0xCC:
		CPY(memory->read(Absolute()));
		cycles += 4;
		break;
	case 0xC6:
		addr = ZeroPage();
		memory->write(DEC(memory->read(addr)), addr);
		cycles += 5;
		break;
	case 0xD6:
		addr = ZeroPageX();
		memory->write(DEC(memory->read(addr)), addr);
		cycles += 6;
		break;
	case 0xCE:
		addr = Absolute();
		memory->write(DEC(memory->read(addr)), addr);
		cycles += 6;
		break;
	case 0xDE:
		addr = AbsoluteX();
		memory->write(DEC(memory->read(addr)), addr);
		cycles += 7;
		break;
	case 0xCA:
		DEX();
		cycles += 2;
		break;
	case 0x88:
		DEY();
		cycles += 2;
		break;
	case 0x49:
#ifdef DEBUG
		logger.logAddressingArgs(memory->read(PC));
		logger.logAddressingMode("Immediate");
#endif
		EOR(memory->read(PC++));
		cycles += 2;
		break;
	case 0x45:
		EOR(memory->read(ZeroPage()));
		cycles += 3;
		break;
	case 0x55:
		EOR(memory->read(ZeroPageX()));
		cycles += 4;
		break;
	case 0x4D:
		EOR(memory->read(Absolute()));
		cycles += 4;
		break;
	case 0x5D:
		EOR(memory->read(AbsoluteX()));
		cycles += 4 + oops;
		break;
	case 0x59:
		EOR(memory->read(AbsoluteY()));
		cycles += 4 + oops;
		break;
	case 0x41:
		EOR(memory->read(IndexedIndirect()));
		cycles += 6;
		break;
	case 0x51:
		EOR(memory->read(IndirectIndexed()));
		cycles += 5 + oops;
		break;
	case 0xE6:
		addr = ZeroPage();
		memory->write(INC(memory->read(addr)), addr);
		cycles += 5;
		break;
	case 0xF6:
		addr = ZeroPageX();
		memory->write(INC(memory->read(addr)), addr);
		cycles += 6;
		break;
	case 0xEE:
		addr = Absolute();
		memory->write(INC(memory->read(addr)), addr);
		cycles += 6;
		break;
	case 0xFE:
		addr = AbsoluteX();
		memory->write(INC(memory->read(addr)), addr);
		cycles += 7;
		break;
	case 0xE8:
		INX();
		cycles += 2;
		break;
	case 0xC8:
		INY();
		cycles += 2;
		break;
	case 0x4C:
#ifdef DEBUG
		logger.setSpecial();
#endif
		JMP(Absolute());
		cycles += 3;
		break;
	case 0x6C:
#ifdef DEBUG
		logger.setSpecial();
#endif
		JMP(Indirect());
		cycles += 5;
		break;
	case 0x20:
#ifdef DEBUG
		logger.setSpecial();
#endif
		JSR(Absolute());
		cycles += 6;
		break;
	case 0xA9:
#ifdef DEBUG
		logger.logAddressingArgs(memory->read(PC));
		logger.logAddressingMode("Immediate");
#endif
		LDA(memory->read(PC++));
		cycles += 2;
		break;
	case 0xA5:
		LDA(memory->read(ZeroPage()));
		cycles += 3;
		break;
	case 0xB5:
		LDA(memory->read(ZeroPageX()));
		cycles += 4;
		break;
	case 0xAD:
		LDA(memory->read(Absolute()));
		cycles += 4;
		break;
	case 0xBD:
		LDA(memory->read(AbsoluteX()));
		cycles += 4 + oops;
		break;
	case 0xB9:
		LDA(memory->read(AbsoluteY()));
		cycles += 4 + oops;
		break;
	case 0xA1:
		LDA(memory->read(IndexedIndirect()));
		cycles += 6;
		break;
	case 0xB1:
		LDA(memory->read(IndirectIndexed()));
		cycles += 5 + oops;
		break;
	case 0xA2:
#ifdef DEBUG
		logger.logAddressingArgs(memory->read(PC));
		logger.logAddressingMode("Immediate");
#endif
		LDX(memory->read(PC++));
		cycles += 2;
		break;
	case 0xA6:
		LDX(memory->read(ZeroPage()));
		cycles += 3;
		break;
	case 0xB6:
		LDX(memory->read(ZeroPageY()));
		cycles += 4;
		break;
	case 0xAE:
		LDX(memory->read(Absolute()));
		cycles += 4;
		break;
	case 0xBE:
		LDX(memory->read(AbsoluteY()));
		cycles += 4 + oops;
		break;
	case 0xA0:
#ifdef DEBUG
		logger.logAddressingArgs(memory->read(PC));
		logger.logAddressingMode("Immediate");
#endif
		LDY(memory->read(PC++));
		cycles += 2;
		break;
	case 0xA4:
		LDY(memory->read(ZeroPage()));
		cycles += 3;
		break;
	case 0xB4:
		LDY(memory->read(ZeroPageX()));
		cycles += 4;
		break;
	case 0xAC:
		LDY(memory->read(Absolute()));
		cycles += 4;
		break;
	case 0xBC:
		LDY(memory->read(AbsoluteX()));
		cycles += 4 + oops;
		break;
	case 0x4A:
#ifdef DEBUG
		logger.logAddressingMode("Accumulator");
#endif
		A = LSR(A);
		cycles += 2;
		break;
	case 0x46:
		addr = ZeroPage();
		memory->write(LSR(memory->read(addr)), addr);
		cycles += 5;
		break;
	case 0x56:
		addr = ZeroPageX();
		memory->write(LSR(memory->read(addr)), addr);
		cycles += 6;
		break;
	case 0x4E:
		addr = Absolute();
		memory->write(LSR(memory->read(addr)), addr);
		cycles += 6;
		break;
	case 0x5E:
		addr = AbsoluteX();
		memory->write(LSR(memory->read(addr)), addr);
		cycles += 7;
		break;
	case 0xEA:
		NOP();
		cycles += 2;
		break;
	case 0x09:
#ifdef DEBUG
		logger.logAddressingArgs(memory->read(PC));
		logger.logAddressingMode("Immediate");
#endif
		ORA(memory->read(PC++));
		cycles += 2;
		break;
	case 0x05:
		ORA(memory->read(ZeroPage()));
		cycles += 3;
		break;
	case 0x15:
		ORA(memory->read(ZeroPageX()));
		cycles += 4;
		break;
	case 0x0D:
		ORA(memory->read(Absolute()));
		cycles += 4;
		break;
	case 0x1D:
		ORA(memory->read(AbsoluteX()));
		cycles += 4 + oops;
		break;
	case 0x19:
		ORA(memory->read(AbsoluteY()));
		cycles += 4 + oops;
		break;
	case 0x01:
		ORA(memory->read(IndexedIndirect()));
		cycles += 6;
		break;
	case 0x11:
		ORA(memory->read(IndirectIndexed()));
		cycles += 5 + oops;
		break;
	case 0x48:
		PHA();
		cycles += 3;
		break;
	case 0x08:
		PHP();
		cycles += 3;
		break;
	case 0x68:
		PLA();
		cycles += 3;
		break;
	case 0x28:
		PLP();
		cycles += 3;
		break;
	case 0x2A:
#ifdef DEBUG
		logger.logAddressingMode("Accumulator");
#endif
		A = ROL(A);
		cycles += 2;
		break;
	case 0x26:
		addr = ZeroPage();
		memory->write(ROL(memory->read(addr)), addr);
		cycles += 5;
		break;
	case 0x36:
		addr = ZeroPageX();
		memory->write(ROL(memory->read(addr)), addr);
		cycles += 6;
		break;
	case 0x2E:
		addr = Absolute();
		memory->write(ROL(memory->read(addr)), addr);
		cycles += 6;
		break;
	case 0x3E:
		addr = AbsoluteX();
		memory->write(ROL(memory->read(addr)), addr);
		cycles += 7;
		break;
	case 0x6A:
#ifdef DEBUG
		logger.logAddressingMode("Accumulator");
#endif
		A = ROR(A);
		cycles += 2;
		break;
	case 0x66:
		addr = ZeroPage();
		memory->write(ROR(memory->read(addr)), addr);
		cycles += 5;
		break;
	case 0x76:
		addr = ZeroPageX();
		memory->write(ROR(memory->read(addr)), addr);
		cycles += 6;
		break;
	case 0x6E:
		addr = Absolute();
		memory->write(ROR(memory->read(addr)), addr);
		cycles += 6;
		break;
	case 0x7E:
		addr = AbsoluteX();
		memory->write(ROR(memory->read(addr)), addr);
		cycles += 7;
		break;
	case 0x40:
		RTI();
		cycles += 6;
		break;
	case 0x60:
		RTS();
		cycles += 6;
		break;
	case 0xE9:
#ifdef DEBUG
		logger.logAddressingArgs(memory->read(PC));
		logger.logAddressingMode("Immediate");
#endif
		SBC(memory->read(PC++));
		cycles += 2;
		break;
	case 0xE5:
		SBC(memory->read(ZeroPage()));
		cycles += 3;
		break;
	case 0xF5:
		SBC(memory->read(ZeroPageX()));
		cycles += 4;
		break;
	case 0xED:
		SBC(memory->read(Absolute()));
		cycles += 4;
		break;
	case 0xFD:
		SBC(memory->read(AbsoluteX()));
		cycles += 4 + oops;
		break;
	case 0xF9:
		SBC(memory->read(AbsoluteY()));
		cycles += 4 + oops;
		break;
	case 0xE1:
		SBC(memory->read(IndexedIndirect()));
		cycles += 6;
		break;
	case 0xF1:
		SBC(memory->read(IndirectIndexed()));
		cycles += 5 + oops;
		break;
	case 0x38:
		SEC();
		cycles += 2;
		break;
	case 0xF8:
		SED();
		cycles += 2;
		break;
	case 0x78:
		SEI();
		cycles += 2;
		break;
	case 0x85:
		memory->write(STA(), ZeroPage());
		cycles += 3;
		break;
	case 0x95:
		memory->write(STA(), ZeroPageX());
		cycles += 4;
		break;
	case 0x8D:
		memory->write(STA(), Absolute());
		cycles += 4;
		break;
	case 0x9D:
		memory->write(STA(), AbsoluteX());
		cycles += 5;
		break;
	case 0x99:
		memory->write(STA(), AbsoluteY());
		cycles += 5;
		break;
	case 0x81:
		memory->write(STA(), IndexedIndirect());
		cycles += 6;
		break;
	case 0x91:
		memory->write(STA(), IndirectIndexed());
		cycles += 6;
		break;
	case 0x86:
		memory->write(STX(), ZeroPage());
		cycles += 3;
		break;
	case 0x96:
		memory->write(STX(), ZeroPageY());
		cycles += 4;
		break;
	case 0x8E:
		memory->write(STX(), Absolute());
		cycles += 4;
		break;
	case 0x84:
		memory->write(STY(), ZeroPage());
		cycles += 3;
		break;
	case 0x94:
		memory->write(STY(), ZeroPageX());
		cycles += 4;
		break;
	case 0x8C:
		memory->write(STY(), Absolute());
		cycles += 4;
		break;
	case 0xAA:
		TAX();
		cycles += 2;
		break;
	case 0xA8:
		TAY();
		cycles += 2;
		break;
	case 0xBA:
		TSX();
		cycles += 2;
		break;
	case 0x8A:
		TXA();
		cycles += 2;
		break;
	case 0x9A:
		TXS();
		cycles += 2;
		break;
	case 0x98:
		TYA();
		cycles += 2;
		break;
	default:
		return false;
	}
#ifdef DEBUG
	logger.printLog();
#endif
	return true;
}

CPU::~CPU()
{
}



