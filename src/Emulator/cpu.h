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
#include <array>
#include <cstdio>
#include <cstdint>
#include <condition_variable>

#include "cart.h"
#include "state.h"

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

    State::Ptr SaveState();
    void LoadState(const State::Ptr& state);

    static constexpr uint32_t NTSC_FREQUENCY = 1789773;

private:
    PPU* Ppu;
    APU* Apu;
    Cart* Cartridge;

    uint64_t Clock;

    // CPU Main Memory
    uint8_t Memory[0x800];

    std::atomic<bool> StopFlag;

    volatile bool Paused;
    std::atomic<bool> PauseFlag;
    std::mutex PauseMutex;
    std::condition_variable PauseCv;

    bool LogEnabled;
    std::atomic<bool> EnableLogFlag;
    std::FILE* LogFile;

    // Debug Strings
    bool LogIsOfficial;
    char ProgramCounter[5];
    char InstructionStr[5];
    char OpCode[3];
    char AddressingArg1[3];
    char AddressingArg2[3];
    char Addressing[28];
    char Registers[41];

    bool ControllerStrobe;
    uint8_t ControllerOneShift;
    std::atomic<uint8_t> ControllerOneState;

    bool NmiLineStatus;
    bool NmiRaised;
    bool NmiPending;

    bool IrqRaised;
    bool IrqPending;
    bool AccumulatorFlag;

    uint8_t DmcDmaDelay;

    // Registers
    uint16_t PC; // Program Counter
    uint8_t S; // Stack Pointer
    uint8_t P; // Processor Status
    uint8_t A; // Accumulator
    uint8_t X; // X Index
    uint8_t Y; // Y Index

    enum Instruction
    {
        // Official Instructions
        ADC, AND, ASL, BCC, BCS, BEQ, BIT, BMI, BNE, BPL, BRK, BVC, BVS, CLC,
        CLD, CLI, CLV, CMP, CPX, CPY, DEC, DEX, DEY, EOR, INC, INX, INY, JMP,
        JSR, LDA, LDX, LDY, LSR, NOP, ORA, PHA, PHP, PLA, PLP, ROL, ROR, RTI,
        RTS, SBC, SEC, SED, SEI, STA, STX, STY, TAX, TAY, TSX, TXA, TXS, TYA,
        // Unofficial Instructions
        AHX, ALR, ANC, ARR, AXS, DCP, ISC, SLO, LAS, LAX, RLA, RRA, SAX, SHX,
        SHY, SRE, STP, TAS, XAA
    };

    enum AddressMode
    {
        ACCUMULATOR, RELATIVE, IMMEDIATE, IMPLIED, ZEROPAGE, ZEROPAGE_X, ZEROPAGE_Y,
        ABSOLUTE, ABSOLUTE_X, ABSOLUTE_Y, INDIRECT, INDIRECT_X, INDIRECT_Y
    };

    struct InstructionDescriptor
    {
        Instruction instruction;
        AddressMode addressMode;
        bool isReadModifyWrite;
        bool isOfficial;
    };

    static const std::array<InstructionDescriptor, 0x100> InstructionSet;

    void IncrementClock();

    uint8_t Peek(uint16_t address);
    uint8_t Read(uint16_t address, bool noDMA = false);
    void Write(uint8_t M, uint16_t address, bool noDMA = false);

    // Addressing Modes
    uint16_t Relative();
    uint16_t Accumulator();
    uint16_t Immediate();
    uint16_t ZeroPage();
    uint16_t ZeroPageX();
    uint16_t ZeroPageY();
    uint16_t Absolute(bool isJump = false);
    uint16_t AbsoluteX(bool isRMW = false);
    uint16_t AbsoluteY(bool isRMW = false);
    uint16_t Indirect();
    uint16_t IndirectX();
    uint16_t IndirectY(bool isRMW = false);

    // Instruction Set

    // Read Instructions
    void DoADC(uint16_t address); // Add with Carry
    void DoAND(uint16_t address); // Logical AND
    void DoBIT(uint16_t address); // Bit Test
    void DoCMP(uint16_t address); // Compare
    void DoCPX(uint16_t address); // Compare X Register
    void DoCPY(uint16_t address); // Compare Y Register
    void DoEOR(uint16_t address); // Exclusive OR
    void DoLDA(uint16_t address); // Load Accumulator
    void DoLDX(uint16_t address); // Load X Register
    void DoLDY(uint16_t address); // Load Y Register
    void DoORA(uint16_t address); // Logical Inclusive OR
    void DoSBC(uint16_t address); // Subtract with Carry

    // Write Instructions
    void DoSTA(uint16_t address); // Store Accumulator
    void DoSTX(uint16_t address); // Store X Register
    void DoSTY(uint16_t address); // Store Y Register

    // Read-Modify-Write Instructions
    void DoASL(uint16_t address); // Arithmetic Shift Left
    void DoDEC(uint16_t address); // Decrement Memory
    void DoINC(uint16_t address); // Increment Memory
    void DoLSR(uint16_t address); // Logical Shift Right
    void DoROL(uint16_t address); // Rotate Left
    void DoROR(uint16_t address); // Rotate Right

    // Branch Instructions
    void DoBranch(int8_t offset); // Common Branch function
    void DoBCC(uint16_t address); // Branch if Carry Clear
    void DoBCS(uint16_t address); // Branch if Carry Set
    void DoBEQ(uint16_t address); // Branch if Equal
    void DoBMI(uint16_t address); // Branch if Minus
    void DoBNE(uint16_t address); // Branch if Not Equal
    void DoBPL(uint16_t address); // Branch if Positive
    void DoBVC(uint16_t address); // Branch if Overflow Clear
    void DoBVS(uint16_t address); // Branch if Overflow Set

    // Stack Instructions
    void DoBRK(); // Force Interrupt
    void DoJSR(); // Jump to Subroutine
    void DoPHA(); // Push Accumulator
    void DoPHP(); // Push Processor Status
    void DoPLA(); // Pull Accumulator
    void DoPLP(); // Pull Processor Status
    void DoRTI(); // Return from interrupt
    void DoRTS(); // Return from subroutine

    // Implied Instructions
    void DoCLC(); // Clear Carry Flag
    void DoCLD(); // Clear Decimal Mode
    void DoCLI(); // Clear Interrupt Disable
    void DoCLV(); // Clear Overflow Flag
    void DoDEX(); // Decrement X Register
    void DoDEY(); // Decrement Y Register
    void DoINX(); // Increment X Register
    void DoINY(); // Increment Y Register
    void DoNOP(uint16_t address); // No Operation
    void DoSEC(); // Set Carry Flag
    void DoSED(); // Set Decimal Flag
    void DoSEI(); // Set Interrupt Disable
    void DoTAX(); // Transfer Accumulator to X
    void DoTAY(); // Transfer Accumulator to Y
    void DoTSX(); // Transfer Stack Pointer to X
    void DoTXA(); // Transfer X to Accumulator
    void DoTXS(); // Transfer X to Stack Pointer
    void DoTYA(); // Transfer Y to Accumulator

    // Jump
    void DoJMP(uint16_t address); // Jump

    // Unofficial instructions
    void DoAHX(uint16_t address);
    void DoALR(uint16_t address);
    void DoANC(uint16_t address);
    void DoARR(uint16_t address);
    void DoAXS(uint16_t address);
    void DoDCP(uint16_t address);
    void DoISC(uint16_t address);
    void DoLAS(uint16_t address);
    void DoLAX(uint16_t address);
    void DoRLA(uint16_t address);
    void DoRRA(uint16_t address);
    void DoSAX(uint16_t address);
    void DoSHY(uint16_t address);
    void DoSHX(uint16_t address);
    void DoSLO(uint16_t address);
    void DoSRE(uint16_t address);
    void DoTAS(uint16_t address);
    void DoXAA(uint16_t address);

    void PollNMIInput();
    void CheckNMIRaised();
    void DoNMI(); // Handle non-maskable interrupt
    void CheckIRQ();
    void DoIRQ(); // Handle standard interrupt

    void DoOamDMA(uint8_t page);
    void DoDmcDMA();

    void Step(); // Execute next instruction

    void SetControllerStrobe(bool strobe);
    uint8_t GetControllerOneShift();

    // Diagnostics
    
    bool IsLogEnabled();
    void LogProgramCounter();
    void LogRegisters();
    void LogOpcode(uint8_t opcode);
    void LogOfficial(bool isOfficial);
    void LogInstructionName(const char* name);
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
    void LogIndirectX(uint8_t pointer, uint8_t lowIndirect, uint16_t address);
    void LogIndirectY(uint8_t pointer, uint16_t initialAddress, uint16_t finalAddress);
    void PrintLog();
};
