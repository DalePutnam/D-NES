/*
 * cpu.h
 *
 *  Created on: Mar 14, 2014
 *      Author: Dale
 */

#pragma once

 /*
  * 6502 CPU Simulator
  */

#include <mutex>
#include <atomic>
#include <cstdio>
#include <cstdint>
#include <condition_variable>

#include "cart.h"

class PPU;
class APU;

class CPU
{
public:
    CPU();
    ~CPU();

    void AttachPPU(PPU* ppu);
    void AttachAPU(APU* apu);
    void AttachCart(Cart* cart);

    void SetStalled(bool stalled);

    uint64_t GetClock();

    void SetControllerOneState(uint8_t state);
    uint8_t GetControllerOneState();

    void Run(); // Run CPU
    void Stop();

    void Reset(); // Reset the CPU to starting conditions
    void Pause();
    void Resume();

    bool IsPaused();

    void SetLogEnabled(bool enabled);

    void SaveState(char* state);
    void LoadState(const char* state);

    static int STATE_SIZE;

private:
    PPU* Ppu;
    APU* Apu;
    Cart* Cartridge;

    uint64_t Clock;

    // CPU Main Memory
    uint8_t Memory[0x800];

    bool StartupFlag;

    std::atomic<bool> StopFlag;

    volatile bool Paused;
    std::atomic<bool> PauseFlag;
    std::mutex PauseMutex;
    std::condition_variable PauseCv;

    bool LogEnabled;
    std::atomic<bool> EnableLogFlag;
    std::FILE* LogFile;

    // Debug Strings
    char ProgramCounter[5];
    char Instruction[5];
    char OpCode[3];
    char AddressingArg1[3];
    char AddressingArg2[3];
    char Addressing[28];
    char Registers[41];

    bool ControllerStrobe;
    uint8_t ControllerOneShift;
    std::atomic<uint8_t> ControllerOneState;

    // Cycles to next NMI check
    int PpuRendevous;
    bool NmiLineStatus;
    bool NmiRaised;
    bool NmiPending;

    bool IsStalled;
    bool IrqPending;

    // Registers
    uint16_t PC; // Program Counter
    uint8_t S; // Stack Pointer
    uint8_t P; // Processor Status
    uint8_t A; // Accumulator
    uint8_t X; // X Index
    uint8_t Y; // Y Index

    void IncrementClock();

    uint8_t Read(uint16_t address);
    void Write(uint8_t M, uint16_t address);

    // Addressing Modes
    int8_t Relative();
    uint8_t Accumulator();
    uint8_t Immediate();
    uint16_t ZeroPage();
    uint16_t ZeroPageX();
    uint16_t ZeroPageY();
    uint16_t Absolute(bool isJump = false);
    uint16_t AbsoluteX(bool isRMW = false);
    uint16_t AbsoluteY(bool isRMW = false);
    uint16_t Indirect();
    uint16_t IndexedIndirect();
    uint16_t IndirectIndexed(bool isRMW = false);

    // Instruction Set

    // Read Instructions
    void ADC(uint8_t M); // Add with Carry
    void AND(uint8_t M); // Logical AND
    void BIT(uint8_t M); // Bit Test
    void CMP(uint8_t M); // Compare
    void CPX(uint8_t M); // Compare X Register
    void CPY(uint8_t M); // Compare Y Register
    void EOR(uint8_t M); // Exclusive OR
    void LDA(uint8_t M); // Load Accumulator
    void LDX(uint8_t M); // Load X Register
    void LDY(uint8_t M); // Load Y Register
    void ORA(uint8_t M); // Logical Inclusive OR
    void SBC(uint8_t M); // Subtract with Carry

    // Write Instructions
    uint8_t STA(); // Store Accumulator
    uint8_t STX(); // Store X Register
    uint8_t STY(); // Store Y Register

    // Read-Modify-Write Instructions
    uint8_t ASL(uint8_t M); // Arithmetic Shift Left
    uint8_t DEC(uint8_t M); // Decrement Memory
    uint8_t INC(uint8_t M); // Increment Memory
    uint8_t LSR(uint8_t M); // Logical Shift Right
    uint8_t ROL(uint8_t M); // Rotate Left
    uint8_t ROR(uint8_t M); // Rotate Right

    // Branch Instructions
    void Branch(int8_t offset); // Common Branch function
    void BCC(int8_t offset); // Branch if Carry Clear
    void BCS(int8_t offset); // Branch if Carry Set
    void BEQ(int8_t offset); // Branch if Equal
    void BMI(int8_t offset); // Branch if Minus
    void BNE(int8_t offset); // Branch if Not Equal
    void BPL(int8_t offset); // Branch if Positive
    void BVC(int8_t offset); // Branch if Overflow Clear
    void BVS(int8_t offset); // Branch if Overflow Set

    // Stack Instructions
    void BRK(); // Force Interrupt
    void JSR(uint16_t M); // Jump to Subroutine
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
    void JMP(uint16_t M); // Jump

    void CheckNMI();
    void HandleNMI(); // Handle non-maskable interrupt
    void CheckIRQ();
    void HandleIRQ(); // Handle standard interrupt

    void Step(); // Execute next instruction

    void SetControllerStrobe(bool strobe);
    uint8_t GetControllerOneShift();

    // Diagnostics
    uint8_t DebugRead(uint16_t address);
    bool IsLogEnabled();
    void LogProgramCounter();
    void LogRegisters();
    void LogOpcode(uint8_t opcode);
    void LogInstructionName(std::string name);
    void LogAccumulator();
    void LogRelative(uint8_t value);
    void LogImmediate(uint8_t value);
    void LogZeroPage(uint8_t address);
    void LogZeroPageX(uint8_t initialAddress, uint8_t finalAddress);
    void LogZeroPageY(uint8_t initialAddress, uint8_t finalAddress);
    void LogAbsolute(uint8_t lowByte, uint8_t highByte, uint16_t address, bool isJump);
    void LogAbsoluteX(uint8_t lowByte, uint8_t highByte, uint16_t initialAddress, uint16_t finalAddress);
    void LogAbsoluteY(uint8_t lowByte, uint8_t highByte, uint16_t initialAddress, uint16_t finalAddress);
    void LogIndirect(uint8_t lowIndirect, uint8_t highIndirect, uint16_t indirect, uint16_t address);
    void LogIndexedIndirect(uint8_t pointer, uint8_t lowIndirect, uint16_t address);
    void LogIndirectIndexed(uint8_t pointer, uint16_t initialAddress, uint16_t finalAddress);
    void PrintLog();
};
