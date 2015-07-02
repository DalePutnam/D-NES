/*
 * cpu.h
 *
 *  Created on: Mar 14, 2014
 *      Author: Dale
 */

#ifndef CPU_H_
#define CPU_H_

/*
 * 6502 CPU Simulator
 */

#include <fstream>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "ppu.h"
#include "mappers/cart.h"

class NES;

class CPU
{
    // CPU Main Memory
    unsigned char memory[0x800];

    std::atomic<bool> pauseFlag;
    std::mutex pauseMutex;
    std::condition_variable pauseCV;
    volatile bool isPaused;

    std::atomic<bool> logEnabled;
    std::ofstream* logStream;

    // Debug Strings
    char programCounter[5];
    char instruction[5];
    char opcode[3];
    char addressingArg1[3];
    char addressingArg2[3];
    char addressing[28];
    char registers[41];

    NES& nes;
    PPU& ppu;
    Cart& cart;

    bool controllerStrobe;
    unsigned char controllerOneShift;
    std::atomic<unsigned char> controllerOneState;

    // Cycles to next NMI check
    int nextNMI;

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
    char Relative();
    unsigned char Accumulator();
    unsigned char Immediate();
    unsigned short int ZeroPage();
    unsigned short int ZeroPageX();
    unsigned short int ZeroPageY();
    unsigned short int Absolute(bool isJump = false);
    unsigned short int AbsoluteX(bool isRMW = false);
    unsigned short int AbsoluteY(bool isRMW = false);
    unsigned short int Indirect();
    unsigned short int IndexedIndirect();
    unsigned short int IndirectIndexed(bool isRMW = false);

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
    void Branch(char offset); // Common Branch function
    void BCC(char offset); // Branch if Carry Clear
    void BCS(char offset); // Branch if Carry Set
    void BEQ(char offset); // Branch if Equal
    void BMI(char offset); // Branch if Minus
    void BNE(char offset); // Branch if Not Equal
    void BPL(char offset); // Branch if Positive
    void BVC(char offset); // Branch if Overflow Clear
    void BVS(char offset); // Branch if Overflow Set

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

    void HandleNMI(); // Handle non-maskable interrupt
    void HandleIRQ(); // Handle standard interrupt

    bool NextInst(); // Execute next instruction

    void SetControllerStrobe(bool strobe);
    unsigned char GetControllerOneShift();

    // Diagnostics
    unsigned char DebugRead(unsigned short int address);
    bool IsLogEnabled();
    void LogProgramCounter();
    void LogRegisters();
    void LogOpcode(unsigned char opcode);
    void LogInstructionName(std::string name);
    void LogRelative(unsigned char value);
    void LogImmediate(unsigned char value);
    void LogZeroPage(unsigned char address);
    void LogZeroPageX(unsigned char initialAddress, unsigned char finalAddress);
    void LogZeroPageY(unsigned char initialAddress, unsigned char finalAddress);
    void LogAbsolute(unsigned char lowByte, unsigned char highByte, unsigned short int address, bool isJump);
    void LogAbsoluteX(unsigned char lowByte, unsigned char highByte, unsigned short int initialAddress, unsigned short int finalAddress);
    void LogAbsoluteY(unsigned char lowByte, unsigned char highByte, unsigned short int initialAddress, unsigned short int finalAddress);
    void LogIndirect(unsigned char lowIndirect, unsigned char highIndirect, unsigned short int indirect, unsigned short int address);
    void LogIndexedIndirect(unsigned char pointer, unsigned char lowIndirect, unsigned short int address);
    void LogIndirectIndexed(unsigned char pointer, unsigned short int initialAddress, unsigned short int finalAddress);
    void PrintLog();

public:

    CPU(NES& nes, PPU& ppu, Cart& cart, bool logEnabled = false);
    void Run(); // Run CPU
    void Reset(); // Reset the CPU to starting conditions

    void SetControllerOneState(unsigned char state);
    unsigned char GetControllerOneState();

    void Pause();
    void Resume();

    bool IsPaused();

    void EnableLog();
    void DisableLog();

    ~CPU();
};


#endif /* CPU_H_ */
