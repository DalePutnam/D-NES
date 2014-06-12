/*
 * cpu.h
 *
 *  Created on: Mar 14, 2014
 *      Author: Dale
 */

#ifndef CPU_H_
#define CPU_H_

#include "ram.h"
#ifdef DEBUG
#include "log/logger.h"
#endif

/*
 * 6502 CPU Simulator
 */

class CPU
{
#ifdef DEBUG
	// Logger
	Logger logger;
#endif

	// CPU Main Memory
	RAM* memory;

	// Cycle Counters
	int cycles;
	int oops;

	// Registers
	unsigned short int PC; // Program Counter
	unsigned char S; // Stack Pointer
	unsigned char P; // Processor Status
	unsigned char A; // Accumulator
	unsigned char X; // X Index
	unsigned char Y; // Y Index

	// Addressing Modes
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
	void ADC(unsigned char M); // Add with Carry
	void AND(unsigned char M); // Logical AND
	unsigned char ASL(unsigned char M); // Arithmetic Shift Left
	void BCC(); // Branch if Carry Clear
	void BCS(); // Branch if Carry Set
	void BEQ(); // Branch if Equal
	void BIT(unsigned char M); // Bit Test
	void BMI(); // Branch if Minus
	void BNE(); // Branch if Not Equal
	void BPL(); // Branch if Positive
	void BRK(); // Force Interrupt
	void BVC(); // Branch if Overflow Clear
	void BVS(); // Branch if Overflow Set
	void CLC(); // Clear Carry Flag
	void CLD(); // Clear Decimal Mode
	void CLI(); // Clear Interrupt Disable
	void CLV(); // Clear Overflow Flag
	void CMP(unsigned char M); // Compare
	void CPX(unsigned char M); // Compare X Register
	void CPY(unsigned char M); // Compare Y Register
	unsigned char DEC(unsigned char M); // Decrement Memory
	void DEX(); // Decrement X Register
	void DEY(); // Decrement Y Register
	void EOR(unsigned char M); // Exclusive OR
	unsigned char INC(unsigned char M); // Increment Memory
	void INX(); // Increment X Register
	void INY(); // Increment Y Register
	void JMP(unsigned short int M); // Jump
	void JSR(unsigned short int M); // Jump to Subroutine
	void LDA(unsigned char M); // Load Accumulator
	void LDX(unsigned char M); // Load X Register
	void LDY(unsigned char M); // Load Y Register
	unsigned char LSR(unsigned char M); // Logical Shift Right
	void NOP(); // No Operation
	void ORA(unsigned char M); // Logical Inclusive OR
	void PHA(); // Push Accumulator
	void PHP(); // Push Processor Status
	void PLA(); // Pull Accumulator
	void PLP(); // Pull Processor Status
	unsigned char ROL(unsigned char M); // Rotate Left
	unsigned char ROR(unsigned char M); // Rotate Right
	void RTI(); // Return from interrupt
	void RTS(); // Return from subroutine
	void SBC(unsigned char M); // Subtract with Carry
	void SEC(); // Set Carry Flag
	void SED(); // Set Decimal Flag
	void SEI(); // Set Interrupt Disable
	unsigned char STA(); // Store Accumulator
	unsigned char STX(); // Store X Register
	unsigned char STY(); // Store Y Register
	void TAX(); // Transfer Accumulator to X
	void TAY(); // Transfer Accumulator to Y
	void TSX(); // Transfer Stack Pointer to X
	void TXA(); // Transfer X to Accumulator
	void TXS(); // Transfer X to Stack Pointer
	void TYA(); // Transfer Y to Accumulator

	bool NextOP(); // Execute next instruction

public:

	CPU(RAM* ram);
	int Run(int cyc); // Run CPU for the specified number of cycles
	void Reset(); // Reset the CPU to starting conditions
	~CPU();
};


#endif /* CPU_H_ */
