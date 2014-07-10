/*
 * cpu.h
 *
 *  Created on: Mar 14, 2014
 *      Author: Dale
 */

#ifndef CPU_H_
#define CPU_H_

#ifdef DEBUG
#include "log/logger.h"
#endif

/*
 * 6502 CPU Simulator
 */

#include "cart.h"

class CPU
{
#ifdef DEBUG
	// Logger
	Logger logger;
#endif

	// CPU Main Memory
	//RAM* memory;
	unsigned char* memory;

	Cart* cart;

	// Cycle Counters
	int cycles;

	// Extra read flag
	bool oops;

	// Registers
	unsigned short int PC; // Program Counter
	unsigned char S; // Stack Pointer
	unsigned char P; // Processor Status
	unsigned char A; // Accumulator
	unsigned char X; // X Index
	unsigned char Y; // Y Index

	unsigned char Read(unsigned short int address);
	void Write(unsigned char M, unsigned short int address);

	// Addressing Modes
	unsigned char Accumulator();
	unsigned char Immediate();
	unsigned char Relative();
	unsigned short int ZeroPage();
	unsigned short int ZeroPageX();
	unsigned short int ZeroPageY();
	unsigned short int Absolute();
	unsigned short int AbsoluteX();
	unsigned short int AbsoluteY();
	unsigned short int Indirect();
	unsigned short int IndexedIndirect();
	unsigned short int IndirectIndexed();

	// Instruction Set

	// Read Instructions
	void ADC(unsigned char M); // Add with Carry
	void AND(unsigned char M); // Logical AND
	void BIT(unsigned char M); // Bit Test
	void CMP(unsigned char M); // Compare
	void CPX(unsigned char M); // Compare X Register
	void CPY(unsigned char M); // Compare Y Register
	void EOR(unsigned char M); // Exclusive OR
	void LDA(unsigned char M); // Load Accumulator
	void LDX(unsigned char M); // Load X Register
	void LDY(unsigned char M); // Load Y Register
	void ORA(unsigned char M); // Logical Inclusive OR
	void SBC(unsigned char M); // Subtract with Carry

	// Write Instructions
	unsigned char STA(); // Store Accumulator
	unsigned char STX(); // Store X Register
	unsigned char STY(); // Store Y Register

	// Read-Modify-Write Instructions
	unsigned char ASL(unsigned char M); // Arithmetic Shift Left
	unsigned char DEC(unsigned char M); // Decrement Memory
	unsigned char INC(unsigned char M); // Increment Memory
	unsigned char LSR(unsigned char M); // Logical Shift Right
	unsigned char ROL(unsigned char M); // Rotate Left
	unsigned char ROR(unsigned char M); // Rotate Right

	// Branch Instructions
	void BCC(); // Branch if Carry Clear
	void BCS(); // Branch if Carry Set
	void BEQ(); // Branch if Equal
	void BMI(); // Branch if Minus
	void BNE(); // Branch if Not Equal
	void BPL(); // Branch if Positive
	void BVC(); // Branch if Overflow Clear
	void BVS(); // Branch if Overflow Set

	// Stack Instructions
	void BRK(); // Force Interrupt
	void JSR(unsigned short int M); // Jump to Subroutine
	void PHA(); // Push Accumulator
	void PHP(); // Push Processor Status
	void PLA(); // Pull Accumulator
	void PLP(); // Pull Processor Status
	void RTI(); // Return from interrupt
	void RTS(); // Return from subroutine

	// Implied Instructions
	void CLC(); // Clear Carry Flag
	void CLD(); // Clear Decimal Mode
	void CLI(); // Clear Interrupt Disable
	void CLV(); // Clear Overflow Flag
	void DEX(); // Decrement X Register
	void DEY(); // Decrement Y Register
	void INX(); // Increment X Register
	void INY(); // Increment Y Register
	void NOP(); // No Operation
	void SEC(); // Set Carry Flag
	void SED(); // Set Decimal Flag
	void SEI(); // Set Interrupt Disable
	void TAX(); // Transfer Accumulator to X
	void TAY(); // Transfer Accumulator to Y
	void TSX(); // Transfer Stack Pointer to X
	void TXA(); // Transfer X to Accumulator
	void TXS(); // Transfer X to Stack Pointer
	void TYA(); // Transfer Y to Accumulator

	// Jump
	void JMP(unsigned short int M); // Jump

	bool NextOP(); // Execute next instruction

public:

#ifdef DEBUG
	unsigned char SoftRead(unsigned short int address);
#endif


	CPU(Cart* cart);
	int Run(int cyc); // Run CPU for the specified number of cycles
	void Reset(); // Reset the CPU to starting conditions
	~CPU();
};


#endif /* CPU_H_ */
