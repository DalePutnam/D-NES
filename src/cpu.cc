/*
 * cpu.cc
 *
 *  Created on: Mar 15, 2014
 *      Author: Dale
 */
#include <iostream>
#include "cpu.h"

#ifdef DEBUG
unsigned char CPU::SoftRead(unsigned short int address)
{
	// Any address less then 0x2000 is just the
	// Internal Ram mirrored every 0x800 bytes
	if (address < 0x2000)
	{
		return memory[address % 0x800];
	}
	else if (address > 0x5FFF && address < 0x10000)
	{
		return cart->PrgRead(address - 0x6000);
	}
	else
	{
		return 0x00;
	}
}

#endif

unsigned char CPU::Read(unsigned short int address)
{
	//cycles += 3;
	// Any address less then 0x2000 is just the
	// Internal Ram mirrored every 0x800 bytes
	if (address < 0x2000)
	{
		return memory[address % 0x800];
	}
	else if (address > 0x5FFF && address < 0x10000)
	{
		return cart->PrgRead(address - 0x6000);
	}
	else
	{
		return 0x00;
	}
}

void CPU::Write(unsigned char M, unsigned short int address)
{
	//cycles += 3;
	// Any address less then 0x2000 is just the
	// Internal Ram mirrored every 0x800 bytes
	if (address < 0x2000)
	{
		memory[address % 0x800] = M;
	}
	else if (address > 0x5FFF && address < 0x10000)
	{
		return cart->PrgWrite(M, address - 0x6000);
	}
	else
	{

	}
}

// ZeroPage Addressing takes the the next byte of memory
// as the location of the instruction operand
unsigned short int CPU::ZeroPage()
{
#ifdef DEBUG
	logger.logAddressingArgs(SoftRead(PC));
	logger.logAddressingMode("ZeroPage", SoftRead(PC));
	logger.logValue(SoftRead(PC));
#endif
	return Read(PC++);
}

// ZeroPage,X Addressing takes the the next byte of memory
// with the value of the X register added (mod 256)
// as the location of the instruction operand
unsigned short int CPU::ZeroPageX()
{
#ifdef DEBUG
	logger.logAddressingArgs(SoftRead(PC));
	logger.logAddressingMode("ZeroPageX", SoftRead(PC));
	logger.logValue((SoftRead(PC) + X) % 0x100);
#endif
	return (Read(PC++) + X) % 0x100;
}

// ZeroPage,Y Addressing takes the the next byte of memory
// with the value of the Y register added (mod 256)
// as the location of the instruction operand
unsigned short int CPU::ZeroPageY()
{
#ifdef DEBUG
	logger.logAddressingArgs(SoftRead(PC));
	logger.logAddressingMode("ZeroPageY", SoftRead(PC));
	logger.logValue((SoftRead(PC) + Y) % 0x100);
#endif
	return (Read(PC++) + Y) % 0x100;
}

// Absolute Addressing reads two bytes from memory and
// combines them into the full 16-bit address of the operand
// Note: The JMP and JSR instructions uses the literal value returned by
// this mode as its operand
unsigned short int CPU::Absolute()
{
#ifdef DEBUG
	int addr = SoftRead(PC) + (((unsigned short int) SoftRead(PC + 1)) * 0x100);
	logger.logAddressingArgs(SoftRead(PC), SoftRead(PC + 1));
	logger.logAddressingMode("Absolute", addr);
	logger.logValue(SoftRead(addr));
#endif
	unsigned short int address = Read(PC) + (((unsigned short int) Read(PC + 1)) * 0x100);
	PC += 2;
	return address;
}

// Absolute,X Addressing reads two bytes from memory and
// combines them into a 16-bit address. X is then added
// to this address to get the final location of the operand
// Should this result in the address crossing a page of memory
// (ie: from 0x00F8 to 0x0105) then an addition cycle is added
// to the instruction.
unsigned short int CPU::AbsoluteX()
{
#ifdef DEBUG
	int addr = SoftRead(PC) + (((unsigned short int) SoftRead(PC + 1)) * 0x100);
	logger.logAddressingArgs(SoftRead(PC), SoftRead(PC + 1));
	logger.logAddressingMode("AbsoluteX", addr);
	logger.logValue(SoftRead(addr + X));
#endif
	unsigned short int arg = (Read(PC) + (((unsigned short int) Read(PC + 1)) * 0x100));
	unsigned short int address = arg + X;

	if ((address % 0x100) <= (arg % 0x100))
	{
		oops = 1; // Additional cycle for crossing a page
	}

	PC += 2;
	return address;
}

// Absolute,Y Addressing reads two bytes from memory and
// combines them into a 16-bit address. Y is then added
// to this address to get the final location of the operand
// Should this result in the address crossing a page of memory
// (ie: from 0x00F8 to 0x0105) then an addition cycle is added
// to the instruction.
unsigned short int CPU::AbsoluteY()
{
#ifdef DEBUG
	int addr = SoftRead(PC) + (((unsigned short int) SoftRead(PC + 1)) * 0x100);
	logger.logAddressingArgs(SoftRead(PC), SoftRead(PC + 1));
	logger.logAddressingMode("AbsoluteY", addr);
	logger.logValue(SoftRead(addr + X));
#endif
	unsigned short int arg = (Read(PC) + (((unsigned short int) Read(PC + 1)) * 0x100));
	unsigned short int address = arg + Y;

	if ((address % 0x100) < (arg % 0x100))
	{
		oops = 1; // Additional cycle for crossing a page
	}

	PC += 2;
	return address;
}

// Indirect Addressing reads two bytes from memory, then
// combines them into a 16-bit address. It then reads bytes
// from this address and the address immediately following it.
// These two new bytes are then combined into the final 16-bit
// address of the operand.
// Notes:
// 1. Due to a glitch in the original hardware this mode never crosses
// pages. If the original address that was read was 0x01FF, then the two
// bytes of the final address will be read from 0x01FF and 0x0100.
// 2. JMP is the only instruction that uses this mode and like Absolute
// it treats the final result as its operand rather than the address of
// the operand
unsigned short int CPU::Indirect()
{
#ifdef DEBUG
	int addr = SoftRead(PC) + (((unsigned short int) SoftRead(PC + 1)) * 0x100);
	logger.logAddressingArgs(SoftRead(PC), SoftRead(PC + 1));
	logger.logAddressingMode("Indirect", addr);
	logger.logValue(SoftRead(addr) + (((unsigned short int) SoftRead(addr + 1)) * 0x100));
#endif
	unsigned short int address = Read(PC) + (((unsigned short int) Read(PC + 1)) * 0x100); // Address where true address will be fetched from
	PC += 2;
	unsigned char lowbyte = (unsigned char) address + 1; // Least significant byte of address incremented (but the MSB is ALWAYS unaffected)
	unsigned short int highaddr = (address & 0xFF00) + lowbyte; // Calculation of the MSB address
	return Read(address) + (((unsigned short int) Read(highaddr)) * 0x100);
}

// Indexed Indirect addressing reads a byte from memory
// and adds the X register to it (mod 256). This is then
// used as a zero page address and two bytes are read from
// this address and the next (with zero page wrap around)
// to make the final 16-bit address of the operand
unsigned short int CPU::IndexedIndirect()
{
#ifdef DEBUG
	int argument = SoftRead(PC);
	int addrfin = SoftRead((unsigned char) (argument + X)) + (((unsigned short int) SoftRead((unsigned char) (argument + X + 1))) * 0x100);
	logger.logAddressingArgs(SoftRead(PC));
	logger.logAddressingMode("IndexedIndirect", SoftRead(PC));
	logger.logFinalAddress(addrfin);
	logger.logValue(SoftRead(addrfin));
#endif
	unsigned char arg = Read(PC++);
	return Read((unsigned char) (arg + X)) + (((unsigned short int) Read((unsigned char) (arg + X + 1))) * 0x100);
}

// Indirect Indexed addressing reads a byte from memory
// and uses it as a zero page address. It then reads a byte
// from that address and the next one. (no zero page wrap this time)
// These two bytes are combined into a 16-bit address which the
// Y register is added to to create the final address of the operand.
// Should this result in page being crossed, then an additional cycle
// is added.
unsigned short int CPU::IndirectIndexed()
{
#ifdef DEBUG
	int argument = SoftRead(PC);
	int addrfin = SoftRead(argument) + (((unsigned short int) SoftRead((unsigned char) (argument + 1))) * 0x100);
	logger.logAddressingArgs(SoftRead(PC));
	logger.logAddressingMode("IndirectIndexed", SoftRead(PC));
	logger.logFinalAddress(addrfin);
	logger.logValue(SoftRead((unsigned short int) (addrfin + Y)));
#endif
	unsigned char arg = Read(PC++);
	unsigned short int address = Read(arg) + (((unsigned short int) Read((unsigned char) (arg + 1))) * 0x100);

	if (((address + Y) % 0x100) < (address % 0x100))
	{
		oops = 1;
	}

	return address + Y; // Additional cycle for crossing a page
}

// Add With Carry
// Adds M  and the Carry flag to the Accumulator (A)
// Sets the Carry, Negative, Overflow and Zero
// flags as necessary.
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

// Logical And
// Performs a logical and on the Accumulator and M
// Sets the Negative and Zero flags if necessary
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

// Arithmetic Shift Left
// Performs a left shift on M and sets
// the Carry, Zero and Negative flags as necessary.
// In this case Carry gets the former bit 7 of M.
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

// Branch if Carry Clear
// If the Carry flag is 0 then adjust PC by the (signed) amount
// found at the next memory location.
void CPU::BCC()
{
#ifdef DEBUG
	logger.logOpName("BCC");
	logger.logAddressingArgs(SoftRead(PC));
#endif
	if ((P & 0x01) == 0)
	{
		unsigned char origPC = PC >> 2;
		// Signed addition of the next byte to PC, then convert back to unsigned
		PC = ((unsigned short int) (((short int) PC) + ((short int) Read(PC))));
		oops = 1; // One cycle added for successful branch

		if (origPC != (PC >> 2))
		{
			oops = 2; // Two cycles added if page crossed
		}
	}
	PC++;
}

// Branch if Carry Set
// If the Carry flag is 1 then adjust PC by the (signed) amount
// found at the next memory location.
void CPU::BCS()
{
#ifdef DEBUG
	logger.logOpName("BCS");
	logger.logAddressingArgs(SoftRead(PC));
#endif
	if ((P & 0x01) == 1)
	{
		unsigned char origPC = PC >> 2;
		// Signed addition of the next byte to PC, then convert back to unsigned
		PC = ((unsigned short int) (((short int) PC) + ((short int) Read(PC))));
		oops = 1; // One cycle added for successful branch

		if (origPC != (PC >> 2))
		{
			oops = 2; // Two cycles added if page crossed
		}
	}
	PC++;
}

// Branch if Equal
// If the Zero flag is 1 then adjust PC by the (signed) amount
// found at the next memory location.
void CPU::BEQ()
{
#ifdef DEBUG
	logger.logOpName("BEQ");
	logger.logAddressingArgs(SoftRead(PC));
#endif
	if (((P & 0x02) >> 1) == 1)
	{
		unsigned char origPC = PC >> 2;
		// Signed addition of the next byte to PC, then convert back to unsigned
		PC = ((unsigned short int) (((short int) PC) + ((short int) Read(PC))));
		oops = 1; // One cycle added for successful branch

		if (origPC != (PC >> 2))
		{
			oops = 2; // Two cycles added if page crossed
		}
	}
	PC++;
}

// Bit Test
// Used to test if certain bits are set in M.
// The Accumulator is expected to hold a mask pattern and
// is anded with M to clear or set the Zero flag.
// The Overflow and Negative flags are also set if necessary.
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

// Branch if Minus
// If the Negative flag is 1 then adjust PC by the (signed) amount
// found at the next memory location.
void CPU::BMI()
{
#ifdef DEBUG
	logger.logOpName("BMI");
	logger.logAddressingArgs(SoftRead(PC));
#endif
	if (((P & 0x80) >> 7) == 1)
	{
		unsigned char origPC = PC >> 2;
		// Signed addition of the next byte to PC, then convert back to unsigned
		PC = ((unsigned short int) (((short int) PC) + ((short int) Read(PC))));
		oops = 1; // One cycle added for successful branch

		if (origPC != (PC >> 2))
		{
			oops = 2; // Two cycles added if page crossed
		}
	}
	PC++;
}

// Branch if Not Equal
// If the Zero flag is 0 then adjust PC by the (signed) amount
// found at the next memory location.
void CPU::BNE()
{
#ifdef DEBUG
	logger.logOpName("BNE");
	logger.logAddressingArgs(SoftRead(PC));
#endif
	if (((P & 0x02) >> 1) == 0)
	{
		unsigned char origPC = PC >> 2;
		// Signed addition of the next byte to PC, then convert back to unsigned
		PC = ((unsigned short int) (((short int) PC) + ((short int) Read(PC))));
		oops = 1; // One cycle added for successful branch

		if (origPC != (PC >> 2))
		{
			oops = 2; // Two cycles added if page crossed
		}
	}
	PC++;
}

// Branch if Positive
// If the Negative flag is 0 then adjust PC by the (signed) amount
// found at the next memory location.
void CPU::BPL()
{
#ifdef DEBUG
	logger.logOpName("BPL");
	logger.logAddressingArgs(SoftRead(PC));
#endif
	if (((P & 0x80) >> 7) == 0)
	{
		unsigned char origPC = PC >> 2;
		// Signed addition of the next byte to PC, then convert back to unsigned
		PC = ((unsigned short int) (((short int) PC) + ((short int) Read(PC))));
		oops = 1; // One cycle added for successful branch

		if (origPC != (PC >> 2))
		{
			oops = 2; // Two cycles added if page crossed
		}
	}
	PC++;
}

// Force Interrupt
// Push PC and P onto the stack and jump to the
// address stored at the interrupt vector (0xFFFE and 0xFFFF)
void CPU::BRK()
{
#ifdef DEBUG
	logger.logOpName("BRK");
#endif
	Write(PC >> 4, 0x100 + S);
	Write(PC, 0x100 + (S - 1));
	Write(P | 0x30, 0x100 + (S - 2));
	S -= 3;

	PC = Read(0xFFFE) + (((unsigned short int) Read(0xFFFF)) * 0x100);
}

// Branch if Overflow Clear
// If the Overflow flag is 0 then adjust PC by the (signed) amount
// found at the next memory location.
void CPU::BVC()
{
#ifdef DEBUG
	logger.logOpName("BVC");
	logger.logAddressingArgs(SoftRead(PC));
#endif
	if (((P & 0x40) >> 6) == 0)
	{
		unsigned char origPC = PC >> 2;
		// Signed addition of the next byte to PC, then convert back to unsigned
		PC = ((unsigned short int) (((short int) PC) + ((short int) Read(PC))));
		oops = 1; // One cycle added for successful branch

		if (origPC != (PC >> 2))
		{
			oops = 2; // Two cycles added if page crossed
		}
	}
	PC++;
}

// Branch if Overflow Set
// If the Overflow flag is 0 then adjust PC by the (signed) amount
// found at the next memory location.
void CPU::BVS()
{
#ifdef DEBUG
	logger.logOpName("BVS");
	logger.logAddressingArgs(SoftRead(PC));
#endif
	if (((P & 0x40) >> 6) == 1)
	{
		unsigned char origPC = PC >> 2;
		// Signed addition of the next byte to PC, then convert back to unsigned
		PC = ((unsigned short int) (((short int) PC) + ((short int) Read(PC))));
		oops = 1; // One cycle added for successful branch

		if (origPC != (PC >> 2))
		{
			oops = 2; // Two cycles added if page crossed
		}
	}
	PC++;
}

// Clear Carry Flag
void CPU::CLC()
{
#ifdef DEBUG
	logger.logOpName("CLC");
#endif
	P = P & 0xFE;
}

// Clear Decimal Mode
// Note: Since this 6502 simulator is for a NES emulator
// this flag doesn't do anything as the NES's CPU didn't
// include decimal mode (and it was stupid anyway)
void CPU::CLD()
{
#ifdef DEBUG
	logger.logOpName("CLD");
#endif
	P = P & 0xF7;
}

// Clear Interrupt Disable
void CPU::CLI()
{
#ifdef DEBUG
	logger.logOpName("CLI");
#endif
	P = P & 0xFB;
}

// Clear Overflow flag
void CPU::CLV()
{
#ifdef DEBUG
	logger.logOpName("CLV");
#endif
	P = P & 0xBF;
}

// Compare
// Compares the Accumulator with M by subtracting
// A from M and setting the Zero flag if they were equal
// and the Carry flag if the Accumulator was larger (or equal)
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

// Compare X Register
// Compares the X register with M by subtracting
// X from M and setting the Zero flag if they were equal
// and the Carry flag if the X register was larger (or equal)
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

// Compare Y Register
// Compares the Y register with M by subtracting
// Y from M and setting the Zero flag if they were equal
// and the Carry flag if the Y register was larger (or equal)
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

// Decrement Memory
// Subtracts one from M and then returns the new value
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

// Decrement X Register
// Subtracts one from X
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

// Decrement X Register
// Subtracts one from Y
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

// Exclusive Or
// Performs and exclusive or on the Accumulator (A)
// and M. The Zero and Negative flags are set if necessary.
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

// Increment Memory
// Adds one to M and then returns the new value
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

// Increment X Register
// Adds one to X
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

// Increment Y Register
// Adds one to Y
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

// Jump
// Sets PC to M
void CPU::JMP(unsigned short int M)
{
#ifdef DEBUG
	logger.logOpName("JMP");
#endif
	PC = M;
}

// Jump to Subroutine
// Pushes PC onto the stack and then sets PC to M
void CPU::JSR(unsigned short int M)
{
#ifdef DEBUG
	logger.logOpName("JSR");
#endif
	PC--;
	Write(PC >> 8, 0x100 + S);
	Write(PC, 0x100 + (S - 1));
	S -= 2;

	PC = M;
}

// Load Accumulator
// Sets the accumulator to M
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

// Load X Register
// Sets the X to M
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

// Load Y Register
// Sets the Y to M
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

// Logical Shift Right
// Performs a logical left shift on M and returns the result
// The Carry flag is set to the former bit zero of M.
// The Zero and Negative flags are set like normal/
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

// No Operation
// Admittedly this really doesn't need its own function, but its
// a convenient place to put the logger code.
void CPU::NOP()
{
#ifdef DEBUG
	logger.logOpName("NOP");
#endif
}

// Logical Inclusive Or
// Performs a logical inclusive or on the Accumulator (A) and M.
// The Zero and Negative flags are set if necessary
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

// Push Accumulator
// Pushes the Accumulator (A) onto the stack
void CPU::PHA()
{
#ifdef DEBUG
	logger.logOpName("PHA");
#endif
	Write(A, 0x100 + S);
	S--;
}

// Push Processor Status
// Pushes P onto the stack
void CPU::PHP()
{
#ifdef DEBUG
	logger.logOpName("PHP");
#endif
	Write(P | 0x30, 0x100 + S);
	S--;
}

// Pull Accumulator
// Pulls the Accumulator (A) off the stack
// Sets Zero and Negative flags if necessary
void CPU::PLA()
{
#ifdef DEBUG
	logger.logOpName("PLA");
#endif
	A = Read(0x100 + ++S);
	// if A is 0 set zero flag
	(A == 0) ? P = P | 0x02 : P = P & 0xFD;
	// set negative flag to bit 7 of A
	((A >> 7) == 1)? P = P | 0x80 : P = P & 0x7F;
}

// Pull Processor Status
// Pulls P off the stack
// Sets Zero and Negative flags if necessary
void CPU::PLP()
{
#ifdef DEBUG
	logger.logOpName("PLP");
#endif
	P = (Read(0x100 + ++S) & 0xEF) | 0x20;
}

// Rotate Left
// Rotates the bits of one left. The carry flag is shifted
// into bit 0 and then set to the former bit 7. Returns the result.
// The Zero and Negative flags are set if necessary
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

// Rotate Right
// Rotates the bits of one left. The carry flag is shifted
// into bit 7 and then set to the former bit 0
// The Zero and Negative flags are set if necessary
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

// Return From Interrupt
// Pulls PC and P from the stack and continues
// execution at the new PC
void CPU::RTI()
{
#ifdef DEBUG
	logger.logOpName("RTI");
#endif
	P = (Read(0x100 + (S + 1)) & 0xEF) | 0x20;
	PC = Read(0x100 + (S + 2)) + (((unsigned short int) Read(0x100 + (S + 3))) * 0x100);
	S += 3;
}

// Return From Subroutine
// Pulls PC from the stack and continues
// execution at the new PC
void CPU::RTS()
{
#ifdef DEBUG
	logger.logOpName("RTS");
#endif
	PC = (Read(0x100 + (S + 1)) + (((unsigned short int) Read(0x100 + (S + 2))) * 0x100));
	PC++;
	S += 2;
}

// Subtract With Carry
// Subtract A from M and the not of the Carry flag.
// Sets the Carry, Overflow, Negative and Zero flags if necessary
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

// Set Carry Flag
void CPU::SEC()
{
#ifdef DEBUG
	logger.logOpName("SEC");
#endif
	P = P | 0x01;
}

// Set Decimal Mode
// Setting this flag has no actual effect.
// See the comment of CLD for why.
void CPU::SED()
{
#ifdef DEBUG
	logger.logOpName("SED");
#endif
	P = P | 0x08;
}

// Set Interrupt Disable
void CPU::SEI()
{
#ifdef DEBUG
	logger.logOpName("SEI");
#endif
	P = P | 0x04;
}

// Store Accumulator
// This simply returns A since all memory writes
// are done externally to these functions
unsigned char CPU::STA()
{
#ifdef DEBUG
	logger.logOpName("STA");
#endif
	return A;
}

// Store X Register
// This simply returns X since all memory writes
// are done externally to these functions
unsigned char CPU::STX()
{
#ifdef DEBUG
	logger.logOpName("STX");
#endif
	return X;
}

// Store Y Register
// This simply returns Y since all memory writes
// are done externally to these functions
unsigned char CPU::STY()
{
#ifdef DEBUG
	logger.logOpName("STY");
#endif
	return Y;
}

// Transfer Accumulator to X
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

// Transfer Accumulator to Y
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

// Transfer Stack Pointer to X
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

// Transfer X to Accumulator
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

// Transfer X to Stack Pointer
void CPU::TXS()
{
#ifdef DEBUG
	logger.logOpName("TXS");
#endif
	S = X;
}

// Transfer Y to Accumulator
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

CPU::CPU(Cart* cart):
	memory(new unsigned char[0x800]),
	cart(cart),
	cycles(0),
	oops(false),
	S(0xFD),
	P(0x24),
	A(0),
	X(0),
	Y(0)
{
	for (int i = 0; i < 0x800; i++)
	{
		memory[i] = 0xFF;
	}

	// Initialize PC to the address found at the reset vector (0xFFFC and 0xFFFD)
	PC = Read(0xFFFC) + (((unsigned short int) Read(0xFFFD)) * 0x100);
}

// Currently unimplemented
void CPU::Reset()
{
}

// Run the CPU for the specified number of cycles
// Since the instructions all take varying numbers
// of cycles the CPU will like run a few cycles past cyc.
// If cyc is -1 then the CPU will simply run until it encounters
// and illegal opcode
int CPU::Run(int cyc)
{
	cycles = 0;

	if (cyc == -1)
	{
		for(;;) // Run until illegal opcode
		{
			if(!NextOP()) break;
		}
	}
	else
	{
		while (cyc > cycles) // Run until cycles overtakes cyc
		{
			if(!NextOP()) break;
		}
	}

	return 0;
}

// Execute the next instruction at PC and return true
// or return false if the next value is not an opcode
bool CPU::NextOP()
{
#ifdef DEBUG
	logger.logProgramCounter(PC);
	logger.logOpCode(SoftRead(PC));
	logger.logRegisters(A, X, Y, P, S);
	logger.logCycles(cycles);
#endif
	unsigned char opcode = Read(PC++); 	// Retrieve opcode from memory
	unsigned short int addr;

	oops = 0; // Reset oops cycles (some addressing modes take extra cycles in certain conditions)

	// This switch statement executes the instruction associated with each opcode
	// With a few exceptions this involves calling on of the 9 addressing mode functions
	// and passing their result into the instruction function. Depending on the instruction
	// the result may then be written back to the same address. After this procedure is
	// complete then the required cycles for that instruction are added to the cycle counter.
	// Exceptions:
	// * Immediate Addressing: This mode simply takes the value immediately following the
	// opcode as the operand.
	// * Accumulator Mode: This mode has the instruction operate directly on the accumulator
	// rather than a byte of memory.
	// * Relative addressing: The Branch instructions use this mode. It uses the next value in memory
	// as a relative offset to be added to PC if the branch condition is true. This is currently
	// implemented within the branch instructions.
	// * Implied Addressing: This refers to all the instructions that take no input at all.
	// All of these except Implied Addressing have their logging code included in the switch statement (sorry).
	switch (opcode)
	{
	// ADC OpCodes
	case 0x69:
#ifdef DEBUG
		logger.logAddressingArgs(SoftRead(PC));
		logger.logAddressingMode("Immediate");
#endif
		ADC(Read(PC++));
		cycles += 2;
		break;
	case 0x65:
		ADC(Read(ZeroPage()));
		cycles += 3;
		break;
	case 0x75:
		ADC(Read(ZeroPageX()));
		cycles += 4;
		break;
	case 0x6D:
		ADC(Read(Absolute()));
		cycles += 4;
		break;
	case 0x7D:
		ADC(Read(AbsoluteX()));
		cycles += 4 + oops;
		break;
	case 0x79:
		ADC(Read(AbsoluteY()));
		cycles += 4 + oops;
		break;
	case 0x61:
		ADC(Read(IndexedIndirect()));
		cycles += 6;
		break;
	case 0x71:
		ADC(Read(IndirectIndexed()));
		cycles += 5 + oops;
		break;
	// AND OpCodes
	case 0x29:
#ifdef DEBUG
		logger.logAddressingArgs(SoftRead(PC));
		logger.logAddressingMode("Immediate");
#endif
		AND(Read(PC++));
		cycles += 2;
		break;
	case 0x25:
		AND(Read(ZeroPage()));
		cycles += 3;
		break;
	case 0x35:
		AND(Read(ZeroPageX()));
		cycles += 4;
		break;
	case 0x2D:
		AND(Read(Absolute()));
		cycles += 4;
		break;
	case 0x3D:
		AND(Read(AbsoluteX()));
		cycles += 4 + oops;
		break;
	case 0x39:
		AND(Read(AbsoluteY()));
		cycles += 4 + oops;
		break;
	case 0x21:
		AND(Read(IndexedIndirect()));
		cycles += 6;
		break;
	case 0x31:
		AND(Read(IndirectIndexed()));
		cycles += 5 + oops;
		break;
	// ASL OpCodes
	case 0x0A:
#ifdef DEBUG
		logger.logAddressingMode("Accumulator");
#endif
		A = ASL(A);
		cycles += 2;
		break;
	case 0x06:
		addr = ZeroPage();
		Write(ASL(Read(addr)), addr);
		cycles += 5;
		break;
	case 0x16:
		addr = ZeroPageX();
		Write(ASL(Read(addr)), addr);
		cycles += 6;
		break;
	case 0x0E:
		addr = Absolute();
		Write(ASL(Read(addr)), addr);
		cycles += 6;
		break;
	case 0x1E:
		addr = AbsoluteX();
		Write(ASL(Read(addr)), addr);
		cycles += 7;
		break;
	// BCC OpCode
	case 0x90:
#ifdef DEBUG
		logger.logAddressingMode("Relative", SoftRead(PC));
#endif
		BCC();
		cycles += 2 + oops;
		break;
	// BCS OpCode
	case 0xB0:
#ifdef DEBUG
		logger.logAddressingMode("Relative", SoftRead(PC));
#endif
		BCS();
		cycles += 2 + oops;
		break;
	// BEQ OpCode
	case 0xF0:
#ifdef DEBUG
		logger.logAddressingMode("Relative", SoftRead(PC));
#endif
		BEQ();
		cycles += 2 + oops;
		break;
	// BIT OpCodes
	case 0x24:
		BIT(Read(ZeroPage()));
		cycles += 3;
		break;
	case 0x2C:
		BIT(Read(Absolute()));
		cycles += 4;
		break;
	// BMI OpCode
	case 0x30:
#ifdef DEBUG
		logger.logAddressingMode("Relative", SoftRead(PC));
#endif
		BMI();
		cycles += 2 + oops;
		break;
	// BNE OpCode
	case 0xD0:
#ifdef DEBUG
		logger.logAddressingMode("Relative", SoftRead(PC));
#endif
		BNE();
		cycles += 2 + oops;
		break;
	// BPL OpCode
	case 0x10:
#ifdef DEBUG
		logger.logAddressingMode("Relative", SoftRead(PC));
#endif
		BPL();
		cycles += 2 + oops;
		break;
	// BRK OpCode
	case 0x00:
		BRK();
		cycles += 7;
		break;
	// BVC OpCode
	case 0x50:
#ifdef DEBUG
		logger.logAddressingMode("Relative", SoftRead(PC));
#endif
		BVC();
		cycles += 2 + oops;
		break;
	// BVS OpCode
	case 0x70:
#ifdef DEBUG
		logger.logAddressingMode("Relative", SoftRead(PC));
#endif
		BVS();
		cycles += 2 + oops;
		break;
	// CLC OpCode
	case 0x18:
		CLC();
		cycles += 2;
		break;
	// CLD OpCode
	case 0xD8:
		CLD();
		cycles += 2;
		break;
	// CLI OpCode
	case 0x58:
		CLI();
		cycles += 2;
		break;
	// CLV OpCode
	case 0xB8:
		CLV();
		cycles += 2;
		break;
	// CMP OpCodes
	case 0xC9:
#ifdef DEBUG
		logger.logAddressingArgs(SoftRead(PC));
		logger.logAddressingMode("Immediate");
#endif
		CMP(Read(PC++));
		cycles += 2;
		break;
	case 0xC5:
		CMP(Read(ZeroPage()));
		cycles += 3;
		break;
	case 0xD5:
		CMP(Read(ZeroPageX()));
		cycles += 4;
		break;
	case 0xCD:
		CMP(Read(Absolute()));
		cycles += 4;
		break;
	case 0xDD:
		CMP(Read(AbsoluteX()));
		cycles += 4 + oops;
		break;
	case 0xD9:
		CMP(Read(AbsoluteY()));
		cycles += 4 + oops;
		break;
	case 0xC1:
		CMP(Read(IndexedIndirect()));
		cycles += 6;
		break;
	case 0xD1:
		CMP(Read(IndirectIndexed()));
		cycles += 5 + oops;
		break;
	// CPX OpCodes
	case 0xE0:
#ifdef DEBUG
		logger.logAddressingArgs(SoftRead(PC));
		logger.logAddressingMode("Immediate");
#endif
		CPX(Read(PC++));
		cycles += 2;
		break;
	case 0xE4:
		CPX(Read(ZeroPage()));
		cycles += 3;
		break;
	case 0xEC:
		CPX(Read(Absolute()));
		cycles += 4;
		break;
	// CPY OpCodes
	case 0xC0:
#ifdef DEBUG
		logger.logAddressingArgs(SoftRead(PC));
		logger.logAddressingMode("Immediate");
#endif
		CPY(Read(PC++));
		cycles += 2;
		break;
	case 0xC4:
		CPY(Read(ZeroPage()));
		cycles += 3;
		break;
	case 0xCC:
		CPY(Read(Absolute()));
		cycles += 4;
		break;
	// DEC OpCodes
	case 0xC6:
		addr = ZeroPage();
		Write(DEC(Read(addr)), addr);
		cycles += 5;
		break;
	case 0xD6:
		addr = ZeroPageX();
		Write(DEC(Read(addr)), addr);
		cycles += 6;
		break;
	case 0xCE:
		addr = Absolute();
		Write(DEC(Read(addr)), addr);
		cycles += 6;
		break;
	case 0xDE:
		addr = AbsoluteX();
		Write(DEC(Read(addr)), addr);
		cycles += 7;
		break;
	// DEX Opcode
	case 0xCA:
		DEX();
		cycles += 2;
		break;
	// DEX Opcode
	case 0x88:
		DEY();
		cycles += 2;
		break;
	// EOR OpCodes
	case 0x49:
#ifdef DEBUG
		logger.logAddressingArgs(SoftRead(PC));
		logger.logAddressingMode("Immediate");
#endif
		EOR(Read(PC++));
		cycles += 2;
		break;
	case 0x45:
		EOR(Read(ZeroPage()));
		cycles += 3;
		break;
	case 0x55:
		EOR(Read(ZeroPageX()));
		cycles += 4;
		break;
	case 0x4D:
		EOR(Read(Absolute()));
		cycles += 4;
		break;
	case 0x5D:
		EOR(Read(AbsoluteX()));
		cycles += 4 + oops;
		break;
	case 0x59:
		EOR(Read(AbsoluteY()));
		cycles += 4 + oops;
		break;
	case 0x41:
		EOR(Read(IndexedIndirect()));
		cycles += 6;
		break;
	case 0x51:
		EOR(Read(IndirectIndexed()));
		cycles += 5 + oops;
		break;
	// INC OpCodes
	case 0xE6:
		addr = ZeroPage();
		Write(INC(Read(addr)), addr);
		cycles += 5;
		break;
	case 0xF6:
		addr = ZeroPageX();
		Write(INC(Read(addr)), addr);
		cycles += 6;
		break;
	case 0xEE:
		addr = Absolute();
		Write(INC(Read(addr)), addr);
		cycles += 6;
		break;
	case 0xFE:
		addr = AbsoluteX();
		Write(INC(Read(addr)), addr);
		cycles += 7;
		break;
	// INX OpCode
	case 0xE8:
		INX();
		cycles += 2;
		break;
	// INY OpCode
	case 0xC8:
		INY();
		cycles += 2;
		break;
	// JMP OpCodes
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
	// JSR OpCode
	case 0x20:
#ifdef DEBUG
		logger.setSpecial();
#endif
		JSR(Absolute());
		cycles += 6;
		break;
	// LDA OpCodes
	case 0xA9:
#ifdef DEBUG
		logger.logAddressingArgs(SoftRead(PC));
		logger.logAddressingMode("Immediate");
#endif
		LDA(Read(PC++));
		cycles += 2;
		break;
	case 0xA5:
		LDA(Read(ZeroPage()));
		cycles += 3;
		break;
	case 0xB5:
		LDA(Read(ZeroPageX()));
		cycles += 4;
		break;
	case 0xAD:
		LDA(Read(Absolute()));
		cycles += 4;
		break;
	case 0xBD:
		LDA(Read(AbsoluteX()));
		cycles += 4 + oops;
		break;
	case 0xB9:
		LDA(Read(AbsoluteY()));
		cycles += 4 + oops;
		break;
	case 0xA1:
		LDA(Read(IndexedIndirect()));
		cycles += 6;
		break;
	case 0xB1:
		LDA(Read(IndirectIndexed()));
		cycles += 5 + oops;
		break;
	// LDX OpCodes
	case 0xA2:
#ifdef DEBUG
		logger.logAddressingArgs(SoftRead(PC));
		logger.logAddressingMode("Immediate");
#endif
		LDX(Read(PC++));
		cycles += 2;
		break;
	case 0xA6:
		LDX(Read(ZeroPage()));
		cycles += 3;
		break;
	case 0xB6:
		LDX(Read(ZeroPageY()));
		cycles += 4;
		break;
	case 0xAE:
		LDX(Read(Absolute()));
		cycles += 4;
		break;
	case 0xBE:
		LDX(Read(AbsoluteY()));
		cycles += 4 + oops;
		break;
	// LDY OpCodes
	case 0xA0:
#ifdef DEBUG
		logger.logAddressingArgs(SoftRead(PC));
		logger.logAddressingMode("Immediate");
#endif
		LDY(Read(PC++));
		cycles += 2;
		break;
	case 0xA4:
		LDY(Read(ZeroPage()));
		cycles += 3;
		break;
	case 0xB4:
		LDY(Read(ZeroPageX()));
		cycles += 4;
		break;
	case 0xAC:
		LDY(Read(Absolute()));
		cycles += 4;
		break;
	case 0xBC:
		LDY(Read(AbsoluteX()));
		cycles += 4 + oops;
		break;
	// LSR OpCodes
	case 0x4A:
#ifdef DEBUG
		logger.logAddressingMode("Accumulator");
#endif
		A = LSR(A);
		cycles += 2;
		break;
	case 0x46:
		addr = ZeroPage();
		Write(LSR(Read(addr)), addr);
		cycles += 5;
		break;
	case 0x56:
		addr = ZeroPageX();
		Write(LSR(Read(addr)), addr);
		cycles += 6;
		break;
	case 0x4E:
		addr = Absolute();
		Write(LSR(Read(addr)), addr);
		cycles += 6;
		break;
	case 0x5E:
		addr = AbsoluteX();
		Write(LSR(Read(addr)), addr);
		cycles += 7;
		break;
	// NOP OPCode
	case 0xEA:
		NOP();
		cycles += 2;
		break;
	// ORA OpCodes
	case 0x09:
#ifdef DEBUG
		logger.logAddressingArgs(SoftRead(PC));
		logger.logAddressingMode("Immediate");
#endif
		ORA(Read(PC++));
		cycles += 2;
		break;
	case 0x05:
		ORA(Read(ZeroPage()));
		cycles += 3;
		break;
	case 0x15:
		ORA(Read(ZeroPageX()));
		cycles += 4;
		break;
	case 0x0D:
		ORA(Read(Absolute()));
		cycles += 4;
		break;
	case 0x1D:
		ORA(Read(AbsoluteX()));
		cycles += 4 + oops;
		break;
	case 0x19:
		ORA(Read(AbsoluteY()));
		cycles += 4 + oops;
		break;
	case 0x01:
		ORA(Read(IndexedIndirect()));
		cycles += 6;
		break;
	case 0x11:
		ORA(Read(IndirectIndexed()));
		cycles += 5 + oops;
		break;
	// PHA OpCode
	case 0x48:
		PHA();
		cycles += 3;
		break;
	// PHP OpCode
	case 0x08:
		PHP();
		cycles += 3;
		break;
	// PLA OpCode
	case 0x68:
		PLA();
		cycles += 3;
		break;
	// PLP OpCode
	case 0x28:
		PLP();
		cycles += 3;
		break;
	// ROL OpCodes
	case 0x2A:
#ifdef DEBUG
		logger.logAddressingMode("Accumulator");
#endif
		A = ROL(A);
		cycles += 2;
		break;
	case 0x26:
		addr = ZeroPage();
		Write(ROL(Read(addr)), addr);
		cycles += 5;
		break;
	case 0x36:
		addr = ZeroPageX();
		Write(ROL(Read(addr)), addr);
		cycles += 6;
		break;
	case 0x2E:
		addr = Absolute();
		Write(ROL(Read(addr)), addr);
		cycles += 6;
		break;
	case 0x3E:
		addr = AbsoluteX();
		Write(ROL(Read(addr)), addr);
		cycles += 7;
		break;
	// ROR OpCodes
	case 0x6A:
#ifdef DEBUG
		logger.logAddressingMode("Accumulator");
#endif
		A = ROR(A);
		cycles += 2;
		break;
	case 0x66:
		addr = ZeroPage();
		Write(ROR(Read(addr)), addr);
		cycles += 5;
		break;
	case 0x76:
		addr = ZeroPageX();
		Write(ROR(Read(addr)), addr);
		cycles += 6;
		break;
	case 0x6E:
		addr = Absolute();
		Write(ROR(Read(addr)), addr);
		cycles += 6;
		break;
	case 0x7E:
		addr = AbsoluteX();
		Write(ROR(Read(addr)), addr);
		cycles += 7;
		break;
	// RTI OpCode
	case 0x40:
		RTI();
		cycles += 6;
		break;
	// RTS OpCode
	case 0x60:
		RTS();
		cycles += 6;
		break;
	// SBC OpCodes
	case 0xE9:
#ifdef DEBUG
		logger.logAddressingArgs(SoftRead(PC));
		logger.logAddressingMode("Immediate");
#endif
		SBC(Read(PC++));
		cycles += 2;
		break;
	case 0xE5:
		SBC(Read(ZeroPage()));
		cycles += 3;
		break;
	case 0xF5:
		SBC(Read(ZeroPageX()));
		cycles += 4;
		break;
	case 0xED:
		SBC(Read(Absolute()));
		cycles += 4;
		break;
	case 0xFD:
		SBC(Read(AbsoluteX()));
		cycles += 4 + oops;
		break;
	case 0xF9:
		SBC(Read(AbsoluteY()));
		cycles += 4 + oops;
		break;
	case 0xE1:
		SBC(Read(IndexedIndirect()));
		cycles += 6;
		break;
	case 0xF1:
		SBC(Read(IndirectIndexed()));
		cycles += 5 + oops;
		break;
	// SEC OpCode
	case 0x38:
		SEC();
		cycles += 2;
		break;
	// SED OpCode
	case 0xF8:
		SED();
		cycles += 2;
		break;
	// SEI OpCode
	case 0x78:
		SEI();
		cycles += 2;
		break;
	// STA OpCodes
	case 0x85:
		Write(STA(), ZeroPage());
		cycles += 3;
		break;
	case 0x95:
		Write(STA(), ZeroPageX());
		cycles += 4;
		break;
	case 0x8D:
		Write(STA(), Absolute());
		cycles += 4;
		break;
	case 0x9D:
		Write(STA(), AbsoluteX());
		cycles += 5;
		break;
	case 0x99:
		Write(STA(), AbsoluteY());
		cycles += 5;
		break;
	case 0x81:
		Write(STA(), IndexedIndirect());
		cycles += 6;
		break;
	case 0x91:
		Write(STA(), IndirectIndexed());
		cycles += 6;
		break;
	// STX OpCodes
	case 0x86:
		Write(STX(), ZeroPage());
		cycles += 3;
		break;
	case 0x96:
		Write(STX(), ZeroPageY());
		cycles += 4;
		break;
	case 0x8E:
		Write(STX(), Absolute());
		cycles += 4;
		break;
	// STY OpCodes
	case 0x84:
		Write(STY(), ZeroPage());
		cycles += 3;
		break;
	case 0x94:
		Write(STY(), ZeroPageX());
		cycles += 4;
		break;
	case 0x8C:
		Write(STY(), Absolute());
		cycles += 4;
		break;
	// TAX OpCode
	case 0xAA:
		TAX();
		cycles += 2;
		break;
	// TAY OpCode
	case 0xA8:
		TAY();
		cycles += 2;
		break;
	// TSX OpCode
	case 0xBA:
		TSX();
		cycles += 2;
		break;
	// TXA OpCode
	case 0x8A:
		TXA();
		cycles += 2;
		break;
	// TXS OpCode
	case 0x9A:
		TXS();
		cycles += 2;
		break;
	// TYA OpCode
	case 0x98:
		TYA();
		cycles += 2;
		break;
	default: // Otherwise illegal OpCode
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
