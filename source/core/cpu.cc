/*
 * cpu.cc
 *
 *  Created on: Mar 15, 2014
 *      Author: Dale
 */

#include <string>
#include <cstring>
#include <iostream>

#include "nes.h"

#include "cpu.h"
#include "ppu.h"
#include "apu.h"

// One of the headers on windows defines OVERFLOW
#ifdef OVERFLOW
#undef OVERFLOW
#endif

// Processor status flags
#define NEGATIVE 0x80
#define OVERFLOW 0x40
#define INTERRUPT 0x20
#define BREAK 0x10
#define DECIMAL 0x8
#define IRQ_INHIBIT 0x4
#define ZERO 0x2
#define CARRY 0x1

// Flag operations
#define SET_FLAG(P, FLAG) ((P) |= (FLAG))
#define CLEAR_FLAG(P, FLAG) ((P) &= ~(FLAG))
#define TEST_FLAG(P, FLAG) (((P) & (FLAG)) != 0)
#define SET_OR_CLEAR_FLAG(P, FLAG, TEST) ((TEST) ? SET_FLAG(P, FLAG) : CLEAR_FLAG(P, FLAG))

// Some common conditions
#define IS_NEGATIVE(A) (((A) & NEGATIVE) != 0)
#define IS_ZERO(A) ((A) == 0)

#define STACK_BASE 0x100

uint8_t CPU::Peek(uint16_t address)
{
    // Any address less then 0x2000 is just the
    // Internal Ram mirrored every 0x800 bytes
    if (address < 0x2000)
    {
        return Memory[address % 0x800];
    }
    else if (address >= 0x2000 && address < 0x4000)
    {
        return 0xFF; // We ignore any reads to the PPU registers
    }
    else if (address >= 0x4020 && address < 0x10000)
    {
        return Cartridge->CpuRead(address);
    }
    else
    {
        return 0xFF;
    }
}

void CPU::IncrementClock()
{
    Clock += 3;

    Apu->Step();

    if (Apu->CheckDmaRequest())
    {
        DmcDmaDelay = 4;
    }
}

uint8_t CPU::Read(uint16_t address, bool noDMA)
{
    if (AccumulatorFlag)
    {
        return A;
    }

    IrqPending = IrqRaised;

    uint8_t value;

    IncrementClock();

    while (DmcDmaDelay > 0 && !noDMA)
    {
        DmcDmaDelay--;

        Read(PC, true);

        if (DmcDmaDelay == 0)
        {
            DoDmcDMA();
        }
    }

    CheckNMIRaised();
    Ppu->Step();
    Ppu->Step();

    if (address < 0x2000)
    {
        // Internal RAM
        value = Memory[address % 0x800];
    }
    else if (address >= 0x2000 && address < 0x4000)
    {
        // PPU Registers
        switch ((address - 0x2000) % 8)
        {
        case 2:  value = Ppu->ReadPPUStatus(); break;
        case 4:  value = Ppu->ReadOAMData(); break;
        case 7:  value = Ppu->ReadPPUData(); break;
        default: value = 0x00; break;
        }
    }
    else if (address >= 0x4000 && address < 0x4018)
    {
        // APU/IO Registers
        switch (address)
        {
        case 0x4015: value = Apu->ReadAPUStatus(); break;
        case 0x4016: value = GetControllerOneShift(); break;
        default:     value = 0x00; break;
        }
    }
    else if (address >= 0x4020 && address < 0x10000)
    {
        // Cartridge Space
        value = Cartridge->CpuRead(address);
    }
    else
    {
        // All other addresses unassigned
        value = 0x00;
    }

    Ppu->Step();
    PollNMIInput();

    CheckIRQ();

    return value;
}

void CPU::Write(uint8_t M, uint16_t address, bool noDMA)
{
    if (AccumulatorFlag)
    {
        A = M;
        return;
    }

    IrqPending = IrqRaised;

    IncrementClock();

    if (DmcDmaDelay > 0 && !noDMA)
    {
        DmcDmaDelay--;
    }

    CheckNMIRaised();
    Ppu->Step();

    // OAM DMA
    if (address == 0x4014 && !noDMA)
    {
        // If DMC DMA was requested this cycle, delay 2 cycles execute the DMA
        if (DmcDmaDelay > 0)
        {
            Read(PC, true);
            Read(PC, true);
            DoDmcDMA();
            DmcDmaDelay = 0;
        }

        DoOamDMA(M);
    }
    else if (address < 0x2000)
    {
        // Internal RAM
        Memory[address % 0x800] = M;
    }
    else if (address >= 0x2000 && address < 0x4000)
    {
        // PPU Registers
        switch ((address - 0x2000) % 8)
        {
        case 0: Ppu->WritePPUCTRL(M); break;
        case 1: Ppu->WritePPUMASK(M); break;
        case 3: Ppu->WriteOAMADDR(M); break;
        case 4: Ppu->WriteOAMDATA(M); break;
        case 5: Ppu->WritePPUSCROLL(M); break;
        case 6: Ppu->WritePPUADDR(M); break;
        case 7: Ppu->WritePPUDATA(M); break;
        default: break;
        }
    }
    else if (address >= 0x4000 && address < 0x4018)
    {
        // APU/IO Registers
        switch (address)
        {
        case 0x4000: Apu->WritePulseOneRegister(0, M); break;
        case 0x4001: Apu->WritePulseOneRegister(1, M); break;
        case 0x4002: Apu->WritePulseOneRegister(2, M); break;
        case 0x4003: Apu->WritePulseOneRegister(3, M); break;
        case 0x4004: Apu->WritePulseTwoRegister(0, M); break;
        case 0x4005: Apu->WritePulseTwoRegister(1, M); break;
        case 0x4006: Apu->WritePulseTwoRegister(2, M); break;
        case 0x4007: Apu->WritePulseTwoRegister(3, M); break;
        case 0x4008: Apu->WriteTriangleRegister(0, M); break;
        case 0x400A: Apu->WriteTriangleRegister(1, M); break;
        case 0x400B: Apu->WriteTriangleRegister(2, M); break;
        case 0x400C: Apu->WriteNoiseRegister(0, M); break;
        case 0x400E: Apu->WriteNoiseRegister(1, M); break;
        case 0x400F: Apu->WriteNoiseRegister(2, M); break;
        case 0x4010: Apu->WriteDmcRegister(0, M); break;
        case 0x4011: Apu->WriteDmcRegister(1, M); break;
        case 0x4012: Apu->WriteDmcRegister(2, M); break;
        case 0x4013: Apu->WriteDmcRegister(3, M); break;
        case 0x4015: Apu->WriteAPUStatus(M); break;
        case 0x4016: SetControllerStrobe(!!(M & 0x1)); break;
        case 0x4017: Apu->WriteAPUFrameCounter(M); break;
        default: break;
        }
    }
    else if (address >= 0x4020 && address < 0x10000)
    {
        // Cartridge Space
        Cartridge->CpuWrite(M, address);
    }

    Ppu->Step();
    Ppu->Step();
    PollNMIInput();

    CheckIRQ();
}

uint16_t CPU::Relative()
{
    if (IsLogEnabled())
    {
        LogRelative(Peek(PC));
    }

    return PC++;
}

uint16_t CPU::Accumulator()
{
    if (IsLogEnabled())
    {
        LogAccumulator();
    }

    Read(PC);
    AccumulatorFlag = true;

    return PC;
}

uint16_t CPU::Immediate()
{
    if (IsLogEnabled())
    {
        LogImmediate(Peek(PC));
    }

    return PC++;
}

// ZeroPage Addressing takes the the next byte of memory
// as the location of the instruction operand
uint16_t CPU::ZeroPage()
{
    uint8_t address = Read(PC++);

    if (IsLogEnabled())
    {
        LogZeroPage(address);
    }

    return address;
}

// ZeroPage,X Addressing takes the the next byte of memory
// with the value of the X register added (mod 256)
// as the location of the instruction operand
uint16_t CPU::ZeroPageX()
{
    uint8_t initialAddress = Read(PC++);
    uint8_t finalAddress = initialAddress + X;
    Read(initialAddress);

    if (IsLogEnabled())
    {
        LogZeroPageX(initialAddress, finalAddress);
    }

    return finalAddress;
}

// ZeroPage,Y Addressing takes the the next byte of memory
// with the value of the Y register added (mod 256)
// as the location of the instruction operand
uint16_t CPU::ZeroPageY()
{
    uint8_t initialAddress = Read(PC++);
    uint8_t finalAddress = initialAddress + Y;
    Read(initialAddress);

    if (IsLogEnabled())
    {
        LogZeroPageY(initialAddress, finalAddress);
    }

    return finalAddress;
}

// Absolute Addressing reads two bytes from memory and
// combines them into the full 16-bit address of the operand
// Note: The JMP and JSR instructions uses the literal value returned by
// this mode as their operand
uint16_t CPU::Absolute(bool isJump)
{
    uint16_t lowByte = Read(PC++);
    uint16_t highByte = Read(PC++);
    uint16_t address = (highByte << 8) + lowByte;

    if (IsLogEnabled())
    {
        LogAbsolute(static_cast<uint8_t>(lowByte), static_cast<uint8_t>(highByte), address, isJump);
    }

    return address;
}

// Absolute,X Addressing reads two bytes from memory and
// combines them into a 16-bit address. X is then added
// to this address to get the final location of the operand
// Should this result in the address crossing a page of memory
// (ie: from 0x00F8 to 0x0105) then an addition cycle is added
// to the instruction.
uint16_t CPU::AbsoluteX(bool isRMW)
{
    // Fetch High and Low Bytes
    uint16_t lowByte = Read(PC++);
    uint16_t highByte = Read(PC++);
    uint16_t initialAddress = (highByte << 8) + lowByte;
    uint16_t finalAddress = initialAddress + X;

    if ((finalAddress & 0xFF00) != (initialAddress & 0xFF00) || isRMW)
    {
        // This extra read is done at the address that is calculated by adding
        // the X register to the initial address, but disregarding page crossing
        Read((initialAddress & 0xFF00) | (finalAddress & 0x00FF));
    }

    if (IsLogEnabled())
    {
        LogAbsoluteX(static_cast<uint8_t>(lowByte), static_cast<uint8_t>(highByte), initialAddress, finalAddress);
    }

    return finalAddress;
}

// Absolute,Y Addressing reads two bytes from memory and
// combines them into a 16-bit address. Y is then added
// to this address to get the final location of the operand
// Should this result in the address crossing a page of memory
// (ie: from 0x00F8 to 0x0105) then an addition cycle is added
// to the instruction.
uint16_t CPU::AbsoluteY(bool isRMW)
{
    // Fetch High and Low Bytes
    uint16_t lowByte = Read(PC++);
    uint16_t highByte = Read(PC++);
    uint16_t initialAddress = (highByte << 8) + lowByte;
    uint16_t finalAddress = initialAddress + Y;

    if ((finalAddress & 0xFF00) != (initialAddress & 0xFF00) || isRMW)
    {
        // This extra read is done at the address that is calculated by adding
        // the X register to the initial address, but disregarding page crossing
        Read((initialAddress & 0xFF00) | (finalAddress & 0x00FF));
    }

    if (IsLogEnabled())
    {
        LogAbsoluteY(static_cast<uint8_t>(lowByte), static_cast<uint8_t>(highByte), initialAddress, finalAddress);
    }

    return finalAddress;
}

// Indirect Addressing reads two bytes from memory, then
// combines them into a 16-bit address. It then reads bytes
// from this address and the address immediately following it.
// These two new bytes are then combined into the final 16-bit
// address of the operand.
// Notes:
// 1. Due to a glitch in the original hardware this mode never crosses
// pages. For example, if the original address that was read was 0x01FF, then the two
// bytes of the final address will be read from 0x01FF and 0x0100.
// 2. JMP is the only instruction that uses this mode and like Absolute
// it treats the final result as its operand rather than the address of
// the operand
uint16_t CPU::Indirect()
{
    uint16_t lowIndirect = Read(PC++);
    uint16_t highIndirect = Read(PC++);

    uint16_t lowByte = Read((highIndirect << 8) + lowIndirect);
    uint16_t highByte = Read((highIndirect << 8) + ((lowIndirect + 1) & 0xFF));

    uint16_t address = (highByte << 8) + lowByte;

    if (IsLogEnabled())
    {
        LogIndirect(static_cast<uint8_t>(lowIndirect), static_cast<uint8_t>(highIndirect), (highIndirect << 8) + lowIndirect, address);
    }

    return address;
}

// Indexed Indirect addressing reads a byte from memory
// and adds the X register to it (mod 256). This is then
// used as a zero page address and two bytes are read from
// this address and the next (with zero page wrap around)
// to make the final 16-bit address of the operand
uint16_t CPU::IndirectX()
{
    uint8_t pointer = Read(PC++); // Get pointer
    uint8_t lowIndirect = pointer + X;
    uint8_t highIndirect = pointer + X + 1;

    Read(pointer); // Read from pointer

    // Fetch high and low bytes
    uint16_t lowByte = Read(lowIndirect);
    uint16_t highByte = Read(highIndirect);
    uint16_t address = (highByte << 8) + lowByte; // Construct address

    if (IsLogEnabled())
    {
        LogIndirectX(pointer, lowIndirect, address);
    }

    return address;
}

// Indirect Indexed addressing reads a byte from memory
// and uses it as a zero page address. It then reads a byte
// from that address and the next one. (no zero page wrap this time)
// These two bytes are combined into a 16-bit address which the
// Y register is added to to create the final address of the operand.
// Should this result in page being crossed, then an additional cycle
// is added.
uint16_t CPU::IndirectY(bool isRMW)
{
    uint16_t pointer = Read(PC++); // Fetch pointer

    // Fetch high and low bytes
    uint16_t lowByte = Read(pointer);
    uint16_t highByte = Read((pointer + 1) % 0x100);
    uint16_t initialAddress = (highByte << 8) + lowByte;
    uint16_t finalAddress = initialAddress + Y;

    if ((finalAddress & 0xFF00) != (initialAddress & 0xFF00) || isRMW)
    {
        Read((initialAddress & 0xFF00) | (finalAddress & 0x00FF));
    }

    if (IsLogEnabled())
    {
        LogIndirectY(static_cast<uint8_t>(pointer), initialAddress, finalAddress);
    }

    return finalAddress;
}

// Add With Carry
// Adds M  and the Carry flag to the Accumulator (A)
// Sets the Carry, Negative, Overflow and Zero
// flags as necessary.
void CPU::DoADC(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("ADC");

    uint8_t M = Read(address);

    uint16_t wideResult = A + M + TEST_FLAG(P, CARRY);
    uint8_t result = static_cast<uint8_t>(wideResult);

    // If overflow occurred, set the carry flag
    SET_OR_CLEAR_FLAG(P, CARRY, (wideResult > 0xFF));
    // if Result is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(result));
    // if bit seven is 1 set negative flag
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(result));
    // if signed overflow occurred set overflow flag
    SET_OR_CLEAR_FLAG(P, OVERFLOW, (result >> 7) != (A >> 7) && (result >> 7) != (M >> 7));
    
    A = result;
}

// Logical And
// Performs a logical and on the Accumulator and M
// Sets the Negative and Zero flags if necessary
void CPU::DoAND(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("AND");

    uint8_t M = Read(address);

    A = A & M;

    // if Result is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(A));
    // if bit seven is 1 set negative flag
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(A));
}

// Arithmetic Shift Left
// Performs a left shift on M and sets
// the Carry, Zero and Negative flags as necessary.
// In this case Carry gets the former bit 7 of M.
void CPU::DoASL(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("ASL");

    uint8_t M = Read(address);
    Write(M, address);

    uint8_t result = M << 1;

    // Set carry flag to bit 7 of M
    SET_OR_CLEAR_FLAG(P, CARRY, (M & 0x80) != 0);
    // if Result is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(result));
    // if bit seven is 1 set negative flag
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(result));

    Write(result, address);
}

// Common Branch function
// Any branch instruction that takes the branch will use this function to do so
void CPU::DoBranch(int8_t offset)
{
    Read(PC);

    uint16_t newPC = PC + offset;

    if ((PC & 0xFF00) != (newPC & 0xFF00))
    {
        Read((PC & 0xFF00) | (newPC & 0x00FF));
    }

    PC = newPC;
}

// Branch if Carry Clear
// If the Carry flag is 0 then adjust PC by the (signed) amount
// found at the next memory location.
void CPU::DoBCC(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("BCC");

    int8_t offset = Read(address);

    if (!TEST_FLAG(P, CARRY))
    {
        DoBranch(offset);
    }
}

// Branch if Carry Set
// If the Carry flag is 1 then adjust PC by the (signed) amount
// found at the next memory location.
void CPU::DoBCS(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("BCS");

    int8_t offset = Read(address);

    if (TEST_FLAG(P, CARRY))
    {
        DoBranch(offset);
    }
}

// Branch if Equal
// If the Zero flag is 1 then adjust PC by the (signed) amount
// found at the next memory location.
void CPU::DoBEQ(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("BEQ");

    int8_t offset = Read(address);

    if (TEST_FLAG(P, ZERO))
    {
        DoBranch(offset);
    }
}

// Bit Test
// Used to test if certain bits are set in M.
// The Accumulator is expected to hold a mask pattern and
// is anded with M to clear or set the Zero flag.
// The Overflow and Negative flags are also set if necessary.
void CPU::DoBIT(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("BIT");

    uint8_t M = Read(address);

    uint8_t result = A & M;

    // if Result is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(result));
    // Set negative flag to bit 7 of M
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(M));
    // Set overflow flag to bit 6 of M
    SET_OR_CLEAR_FLAG(P, OVERFLOW, (M & 0x40) != 0);
}

// Branch if Minus
// If the Negative flag is 1 then adjust PC by the (signed) amount
// found at the next memory location.
void CPU::DoBMI(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("BMI");

    int8_t offset = Read(address);

    if (TEST_FLAG(P, NEGATIVE))
    {
        DoBranch(offset);
    }
}

// Branch if Not Equal
// If the Zero flag is 0 then adjust PC by the (signed) amount
// found at the next memory location.
void CPU::DoBNE(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("BNE");

    int8_t offset = Read(address);

    if (!TEST_FLAG(P, ZERO))
    {
        DoBranch(offset);
    }
}

// Branch if Positive
// If the Negative flag is 0 then adjust PC by the (signed) amount
// found at the next memory location.
void CPU::DoBPL(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("BPL");

    int8_t offset = Read(address);

    if (!TEST_FLAG(P, NEGATIVE))
    {
        DoBranch(offset);
    }
}

// Force Interrupt
// Push PC and P onto the stack and jump to the
// address stored at the interrupt vector (0xFFFE and 0xFFFF)
void CPU::DoBRK()
{
    uint8_t value = Read(PC++);

    if (IsLogEnabled()) LogInstructionName("BRK");

    uint8_t highPC = static_cast<uint8_t>(PC >> 8);
    uint8_t lowPC = static_cast<uint8_t>(PC & 0xFF);

    Write(highPC, STACK_BASE + S--);
    Write(lowPC, STACK_BASE + S--);
    Write(P | INTERRUPT | BREAK, STACK_BASE + S--);

    SET_FLAG(P, IRQ_INHIBIT);

    uint16_t newLowPC = Read(0xFFFE);
    uint16_t newHighPC = Read(0xFFFF);

    PC = (newHighPC << 8) | newLowPC;
}

// Branch if Overflow Clear
// If the Overflow flag is 0 then adjust PC by the (signed) amount
// found at the next memory location.
void CPU::DoBVC(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("BVC");

    int8_t offset = Read(address);

    if (!TEST_FLAG(P, OVERFLOW))
    {
        DoBranch(offset);
    }
}

// Branch if Overflow Set
// If the Overflow flag is 0 then adjust PC by the (signed) amount
// found at the next memory location.
void CPU::DoBVS(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("BVS");

    int8_t offset = Read(address);

    if (TEST_FLAG(P, OVERFLOW))
    {
        DoBranch(offset);
    }
}

// Clear Carry Flag
void CPU::DoCLC()
{
    if (IsLogEnabled()) LogInstructionName("CLC");

    Read(PC);

    CLEAR_FLAG(P, CARRY);
}

// Clear Decimal Mode
// Note: Since this 6502 simulator is for a NES emulator
// this flag doesn't do anything as the NES's CPU didn't
// include decimal mode (and it was stupid anyway)
void CPU::DoCLD()
{
    if (IsLogEnabled()) LogInstructionName("CLD");

    Read(PC);

    CLEAR_FLAG(P, DECIMAL);
}

// Clear Interrupt Disable
void CPU::DoCLI()
{
    if (IsLogEnabled()) LogInstructionName("CLI");

    Read(PC);

    CLEAR_FLAG(P, IRQ_INHIBIT);
}

// Clear Overflow flag
void CPU::DoCLV()
{
    if (IsLogEnabled()) LogInstructionName("CLV");

    Read(PC);

    CLEAR_FLAG(P, OVERFLOW);
}

// Compare
// Compares the Accumulator with M by subtracting
// A from M and setting the Zero flag if they were equal
// and the Carry flag if the Accumulator was larger (or equal)
void CPU::DoCMP(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("CMP");

    uint8_t M = Read(address);

    uint8_t result = A - M;

    // if A >= M set carry flag
    SET_OR_CLEAR_FLAG(P, CARRY, (A >= M));
    // if A = M set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, (A == M));
    // set negative flag to bit 7 of result
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(result));
}

// Compare X Register
// Compares the X register with M by subtracting
// X from M and setting the Zero flag if they were equal
// and the Carry flag if the X register was larger (or equal)
void CPU::DoCPX(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("CPX");

    uint8_t M = Read(address);

    uint8_t result = X - M;

    // if X >= M set carry flag
    SET_OR_CLEAR_FLAG(P, CARRY, (X >= M));
    // if X = M set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, (X == M));
    // set negative flag to bit 7 of result
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(result));
}

// Compare Y Register
// Compares the Y register with M by subtracting
// Y from M and setting the Zero flag if they were equal
// and the Carry flag if the Y register was larger (or equal)
void CPU::DoCPY(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("CPY");

    uint8_t M = Read(address);

    uint8_t result = Y - M;

    // if Y >= M set carry flag
    SET_OR_CLEAR_FLAG(P, CARRY, (Y >= M));
    // if Y = M set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, (Y == M));
    // set negative flag to bit 7 of result
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(result));
}

// Decrement Memory
// Subtracts one from M and then returns the new value
void CPU::DoDEC(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("DEC");

    uint8_t M = Read(address);
    Write(M, address);

    uint8_t result = M - 1;

    // if result is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(result));
    // set negative flag to bit 7 of result
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(result));
    
    Write(result, address);
}

// Decrement X Register
// Subtracts one from X
void CPU::DoDEX()
{
    if (IsLogEnabled()) LogInstructionName("DEX");

    Read(PC);

    --X;

    // if X is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(X));
    // set negative flag to bit 7 of X
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(X));
}

// Decrement X Register
// Subtracts one from Y
void CPU::DoDEY()
{
    if (IsLogEnabled()) LogInstructionName("DEY");

    Read(PC);

    --Y;

    // if Y is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(Y));
    // set negative flag to bit 7 of Y
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(Y));
}

// Exclusive Or
// Performs and exclusive or on the Accumulator (A)
// and M. The Zero and Negative flags are set if necessary.
void CPU::DoEOR(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("EOR");

    uint8_t M = Read(address);

    A = A ^ M;

    // if A is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(A));
    // set negative flag to bit 7 of A
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(A));
}

// Increment Memory
// Adds one to M and then returns the new value
void CPU::DoINC(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("INC");

    uint8_t M = Read(address);
    Write(M, address);

    uint8_t result = M + 1;

    // if result is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(result));
    // set negative flag to bit 7 of result
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(result));
    
    Write(result, address);
}

// Increment X Register
// Adds one to X
void CPU::DoINX()
{
    if (IsLogEnabled()) LogInstructionName("INX");

    Read(PC);

    ++X;

    // if X is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(X));
    // set negative flag to bit 7 of X
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(X));
}

// Increment Y Register
// Adds one to Y
void CPU::DoINY()
{
    if (IsLogEnabled()) LogInstructionName("INY");

    Read(PC);

    ++Y;

    // if Y is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(Y));
    // set negative flag to bit 7 of Y
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(Y));
}

// Jump
// Sets PC to M
void CPU::DoJMP(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("JMP");

    PC = address;
}

// Jump to Subroutine
// Pushes PC onto the stack and then sets PC to location specified
// by next two bytes after the opcode. This instruction does the
// memory accesses in a different order than the normal absolute
// addressing mode so these accesses are done hear rather than
// in their own addressing function.
void CPU::DoJSR()
{
    uint8_t newLowPC = Read(PC++);
    uint8_t highPC = static_cast<uint8_t>(PC >> 8);
    uint8_t lowPC = static_cast<uint8_t>(PC & 0xFF);

    Read(STACK_BASE + S); // internal operation
    Write(highPC, STACK_BASE + S--);
    Write(lowPC, STACK_BASE + S--);

    uint8_t newHighPC = Read(PC);

    PC = newHighPC;
    PC = (PC << 8) | newLowPC;

    if (IsLogEnabled())
    {
        LogInstructionName("JSR");
        LogAbsolute(newLowPC, newHighPC, PC, true);
    }
}

// Load Accumulator
// Sets the accumulator to M
void CPU::DoLDA(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("LDA");

    A = Read(address);

    // if A is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(A));
    // set negative flag to bit 7 of A
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(A));
}

// Load X Register
// Sets the X to M
void CPU::DoLDX(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("LDX");

    X = Read(address);

    // if X is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(X));
    // set negative flag to bit 7 of X
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(X));
}

// Load Y Register
// Sets the Y to M
void CPU::DoLDY(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("LDY");

    Y = Read(address);

    // if Y is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(Y));
    // set negative flag to bit 7 of Y
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(Y));
}

// Logical Shift Right
// Performs a logical left shift on M and returns the result
// The Carry flag is set to the former bit zero of M.
// The Zero and Negative flags are set like normal/
void CPU::DoLSR(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("LSR");

    uint8_t M = Read(address);
    Write(M, address);

    uint8_t result = M >> 1;

    // set carry flag to bit 0 of M
    SET_OR_CLEAR_FLAG(P, CARRY, (M & 0x01) != 0);
    // if result is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(result));
    // set negative flag to bit 7 of result
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(result));

    Write(result, address);
}

// No Operation
void CPU::DoNOP(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("NOP");

    Read(address);
}

// Logical Inclusive Or
// Performs a logical inclusive or on the Accumulator (A) and M.
// The Zero and Negative flags are set if necessary
void CPU::DoORA(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("ORA");

    uint8_t M = Read(address);

    A = A | M;

    // if A is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(A));
    // set negative flag to bit 7 of A
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(A));
}

// Push Accumulator
// Pushes the Accumulator (A) onto the stack
void CPU::DoPHA()
{
    if (IsLogEnabled()) LogInstructionName("PHA");

    Read(PC);
    Write(A, STACK_BASE + S--);
}

// Push Processor Status
// Pushes P onto the stack
void CPU::DoPHP()
{
    if (IsLogEnabled()) LogInstructionName("PHP");

    Read(PC);
    Write(P | INTERRUPT | BREAK, 0x100 + S--);
}

// Pull Accumulator
// Pulls the Accumulator (A) off the stack
// Sets Zero and Negative flags if necessary
void CPU::DoPLA()
{
    if (IsLogEnabled()) LogInstructionName("PLA");

    Read(PC);
    Read(STACK_BASE + S++);

    A = Read(STACK_BASE + S);

    // if A is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(A));
    // set negative flag to bit 7 of A
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(A));
}

// Pull Processor Status
// Pulls P off the stack
void CPU::DoPLP()
{
    if (IsLogEnabled()) LogInstructionName("PLP");

    Read(PC);
    Read(STACK_BASE + S++);

    P = Read(STACK_BASE + S);

    CLEAR_FLAG(P, BREAK);
    CLEAR_FLAG(P, INTERRUPT);
}

// Rotate Left
// Rotates the bits of one left. The carry flag is shifted
// into bit 0 and then set to the former bit 7. Returns the result.
// The Zero and Negative flags are set if necessary
void CPU::DoROL(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("ROL");

    uint8_t M = Read(address);
    Write(M, address);

    uint8_t result = (M << 1) | TEST_FLAG(P, CARRY);

    // set carry flag to old bit 7
    SET_OR_CLEAR_FLAG(P, CARRY, (M & 0x80) != 0);
    // if result is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(result));
    // set negative flag to bit 7 of result
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(result));

    Write(result, address);
}

// Rotate Right
// Rotates the bits of one left. The carry flag is shifted
// into bit 7 and then set to the former bit 0
// The Zero and Negative flags are set if necessary
void CPU::DoROR(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("ROR");

    uint8_t M = Read(address);
    Write(M, address);

    uint8_t result = (M >> 1) | (TEST_FLAG(P, CARRY) << 7);

    // set carry flag to old bit 0
    SET_OR_CLEAR_FLAG(P, CARRY, (M & 0x01) != 0);
    // if result is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(result));
    // set negative flag to bit 7 of result
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(result));

    Write(result, address);
}

// Return From Interrupt
// Pulls PC and P from the stack and continues
// execution at the new PC
void CPU::DoRTI()
{
    if (IsLogEnabled()) LogInstructionName("RTI");

    Read(PC);
    Read(STACK_BASE + S++);

    P = Read(STACK_BASE + S++);
    CLEAR_FLAG(P, BREAK);
    CLEAR_FLAG(P, INTERRUPT);

    uint16_t lowPC = Read(STACK_BASE + S++);
    uint16_t highPC = Read(STACK_BASE + S);

    PC = (highPC << 8) | lowPC;
}

// Return From Subroutine
// Pulls PC from the stack and continues
// execution at the new PC
void CPU::DoRTS()
{
    if (IsLogEnabled()) LogInstructionName("RTS");

    Read(PC);
    Read(STACK_BASE + S++);

    uint16_t lowPC = Read(STACK_BASE + S++);
    uint16_t highPC = Read(STACK_BASE + S);

    PC = (highPC << 8) | lowPC;

    Read(PC++);
}

// Subtract With Carry
// Subtract A from M and the not of the Carry flag.
// Sets the Carry, Overflow, Negative and Zero flags if necessary
void CPU::DoSBC(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("SBC");

    uint8_t M = Read(address);

    uint16_t wideResult = A - M - (1 - TEST_FLAG(P, CARRY));
    uint8_t result = static_cast<uint8_t>(wideResult);

    // if underflow occured clear carry flag
    SET_OR_CLEAR_FLAG(P, CARRY, !(wideResult > 0x7FFF));
    // if Result is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(result));
    // if bit seven is 1 set negative flag
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(result));
    // if signed overflow occurred set overflow flag
    SET_OR_CLEAR_FLAG(P, OVERFLOW, ((A >> 7) != (M >> 7)) && ((A >> 7) != (result >> 7)));

    A = result;
}

// Set Carry Flag
void CPU::DoSEC()
{
    if (IsLogEnabled()) LogInstructionName("SEC");

    Read(PC);

    SET_FLAG(P, CARRY);
}

// Set Decimal Mode
// Setting this flag has no actual effect.
// See the comment of CLD for why.
void CPU::DoSED()
{
    if (IsLogEnabled()) LogInstructionName("SED");

    Read(PC);

    SET_FLAG(P, DECIMAL);
}

// Set Interrupt Disable
void CPU::DoSEI()
{
    if (IsLogEnabled()) LogInstructionName("SEI");

    Read(PC);

    SET_FLAG(P, IRQ_INHIBIT);
}

// Store Accumulator
// This simply returns A since all memory writes
// are done externally to these functions
void CPU::DoSTA(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("STA");

    Write(A, address);
}

// Store X Register
// This simply returns X since all memory writes
// are done externally to these functions
void CPU::DoSTX(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("STX");

    Write(X, address);
}

// Store Y Register
// This simply returns Y since all memory writes
// are done externally to these functions
void CPU::DoSTY(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("STY");

    Write(Y, address);
}

// Transfer Accumulator to X
void CPU::DoTAX()
{
    if (IsLogEnabled()) LogInstructionName("TAX");

    Read(PC);

    X = A;

    // Set zero flag if X is 0
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(X));
    // Set negative flag if bit 7 of X is set
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(X));
}

// Transfer Accumulator to Y
void CPU::DoTAY()
{
    if (IsLogEnabled()) LogInstructionName("TAY");

    Read(PC);

    Y = A;

    // Set zero flag if Y is 0
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(Y));
    // Set negative flag if bit 7 of Y is set
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(Y));
}

// Transfer Stack Pointer to X
void CPU::DoTSX()
{
    if (IsLogEnabled()) LogInstructionName("TSX");

    Read(PC);

    X = S;

    // Set zero flag if X is 0
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(X));
    // Set negative flag if bit 7 of X is set
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(X));
}

// Transfer X to Accumulator
void CPU::DoTXA()
{
    if (IsLogEnabled()) LogInstructionName("TXA");

    Read(PC);

    A = X;

    // Set zero flag if A is 0
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(A));
    // Set negative flag if bit 7 of A is set
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(A));
}

// Transfer X to Stack Pointer
void CPU::DoTXS()
{
    if (IsLogEnabled()) LogInstructionName("TXS");

    Read(PC);

    S = X;
}

// Transfer Y to Accumulator
void CPU::DoTYA()
{
    if (IsLogEnabled()) LogInstructionName("TYA");

    Read(PC);

    A = Y;

    // Set zero flag if A is 0
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(A));
    // Set negative flag if bit 7 of A is set
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(A));
}

void CPU::DoAHX(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("AHX");

    uint8_t addressHigh = address >> 8;
    uint8_t result = (A & X & (addressHigh + 1));

    if ((address - Y) < (address & 0xFF00))
    {
        address = (address & 0x00FF) | (static_cast<uint16_t>(result) << 8);
    }

    Write(result, address);
}

void CPU::DoALR(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("ALR");

    uint8_t M = Read(address);

    A = (A & M);

    // Set carry flag to old bit 0
    SET_OR_CLEAR_FLAG(P, CARRY, (A & 0x1) != 0);
    
    A >>= 1;

    // if Result is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(A));
    // if bit seven is 1 set negative flag
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(A));
}

void CPU::DoANC(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("AXS");

    uint8_t M = Read(address);

    A = A & M;
    
    // if Result is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(A));
    // if bit seven is 1 set negative flag
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(A));
    // Copy negative flag to carry flag
    SET_OR_CLEAR_FLAG(P, CARRY, TEST_FLAG(P, NEGATIVE));
}

void CPU::DoARR(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("ARR");

    uint8_t M = Read(address);

    A = (A & M);
    A = (A >> 1) | (TEST_FLAG(P, CARRY) << 7);

    // if Result is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(A));
    // if bit seven is 1 set negative flag
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(A));
    // Set carry flag to bit 6
    SET_OR_CLEAR_FLAG(P, CARRY, (A & 0x40) != 0);
    // Set overflow flag to bit 6 xor bit 5
    SET_OR_CLEAR_FLAG(P, OVERFLOW, ((A & 0x40) ^ ((A & 0x20) << 1)) != 0);
}

void CPU::DoAXS(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("AXS");

    uint8_t M = Read(address);
    
    uint8_t AX = (A & X);
    X = AX - M;

    // if (A & X) >= M set carry flag
    SET_OR_CLEAR_FLAG(P, CARRY, (AX >= M));
    // if (A & X) = M set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, (AX == M));
    // Set negative flag if X is negative
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(X));
}

void CPU::DoDCP(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("DCP");

    uint8_t M = Read(address);
    Write(M, address);

    uint8_t result = M - 1;
    
    SET_OR_CLEAR_FLAG(P, CARRY, (A >= result));
    SET_OR_CLEAR_FLAG(P, ZERO, (A == result));
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(A - result));

    Write(result, address);
}

void CPU::DoISC(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("ISC");

    uint8_t M = Read(address);
    Write(M, address);

    M = M + 1;
    uint16_t wideResult = A - M - (1 - TEST_FLAG(P, CARRY));
    uint8_t result = static_cast<uint8_t>(wideResult);

    // If overflow occurred, clear the carry flag
    SET_OR_CLEAR_FLAG(P, CARRY, !(wideResult >= 0x7FFF));
    // if signed overflow occurred set overflow flag
    SET_OR_CLEAR_FLAG(P, OVERFLOW, ((A >> 7) != (result >> 7)) && ((A >> 7) != (M >> 7)));
    // if Result is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(result));
    // if bit seven is 1 set negative flag
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(result));
    
    A = result;
    Write(M, address);
}

void CPU::DoLAS(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("LAS");

    uint8_t M = Read(address);
    
    A = (S & M);

    // if Result is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(A));
    // if bit seven is 1 set negative flag
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(A));
}

void CPU::DoLAX(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("LAX");

    uint8_t M = Read(address);

    A = X = M;

    // if Result is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(A));
    // if bit seven is 1 set negative flag
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(A));
}

void CPU::DoRLA(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("RLA");

    uint8_t M = Read(address);
    Write(M, address);

    uint8_t result = (M << 1) | TEST_FLAG(P, CARRY);
    A = A & result;

    SET_OR_CLEAR_FLAG(P, CARRY, (M & 0x80) != 0);
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(A));
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(A));

    Write(result, address);
}

void CPU::DoRRA(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("RRA");

    uint8_t M = Read(address);
    Write(M, address);

    uint8_t result = (M >> 1) | (TEST_FLAG(P, CARRY) << 7);

    Write(result, address);

    SET_OR_CLEAR_FLAG(P, CARRY, (M & 0x1) != 0);

    M = result;
    uint16_t wideResult = A + M + TEST_FLAG(P, CARRY);
    result = static_cast<uint8_t>(wideResult);

    // If overflow occurred, set the carry flag
    SET_OR_CLEAR_FLAG(P, CARRY, (wideResult > 0xFF));
    // if signed overflow occurred set overflow flag
    SET_OR_CLEAR_FLAG(P, OVERFLOW, ((A >> 7) != (result >> 7)) && ((result >> 7) != (M >> 7)));
    // if Result is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(result));
    // if bit seven is 1 set negative flag
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(result));
    
    A = result;
}

void CPU::DoSAX(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("SAX");

    Write(A & X, address);
}

void CPU::DoSHY(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("SHY");

    uint8_t addressHigh = address >> 8;
    uint8_t result = (Y & (addressHigh + 1));

    if ((address - X) < (address & 0xFF00))
    {
        address = (address & 0x00FF) | (static_cast<uint16_t>(result) << 8);
    }

    Write(result, address);
}

void CPU::DoSHX(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("SHX");

    uint8_t addressHigh = address >> 8;
    uint8_t result = (X & (addressHigh + 1));

    if ((address - Y) < (address & 0xFF00))
    {
        address = (address & 0x00FF) | (static_cast<uint16_t>(result) << 8);
    }

    Write(result, address);
}

void CPU::DoSLO(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("SLO");

    uint8_t M = Read(address);
    Write(M, address);

    uint8_t result = M << 1;
    A = A | result;

    SET_OR_CLEAR_FLAG(P, CARRY, (M & 0x80) != 0);
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(A));
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(A));

    Write(result, address);
}

void CPU::DoSRE(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("SRE");

    uint8_t M = Read(address);
    Write(M, address);

    uint8_t result = M >> 1;
    A = A ^ result;

    SET_OR_CLEAR_FLAG(P, CARRY, (M & 0x1) != 0);
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(A));
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(A));

    Write(result, address);
}

void CPU::DoTAS(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("TAS");
    
    uint8_t addressHigh = address >> 8;
    uint8_t result = (A & X & (addressHigh + 1));

    if ((address - Y) < (address & 0xFF00))
    {
        address = (address & 0x00FF) | (static_cast<uint16_t>(result) << 8);
    }

    S = (A & X);

    Write(result, address);
}

void CPU::DoXAA(uint16_t address)
{
    if (IsLogEnabled()) LogInstructionName("XAA");

    uint8_t M = Read(address);
    A = (A | 0xEE) & X & M;

    // if Result is 0 set zero flag
    SET_OR_CLEAR_FLAG(P, ZERO, IS_ZERO(A));
    // if bit seven is 1 set negative flag
    SET_OR_CLEAR_FLAG(P, NEGATIVE, IS_NEGATIVE(A));
}

void CPU::PollNMIInput()
{
    bool nmiLine = Ppu->GetNMIActive();

    // False to true transition detected
    if (!NmiLineStatus && nmiLine)
    {
        NmiRaised = true;
    }

    NmiLineStatus = nmiLine;
}

void CPU::CheckNMIRaised()
{
    if (NmiRaised)
    {
        NmiRaised = false;
        NmiPending = true;
    }
}

void CPU::DoNMI()
{
    uint8_t highPC = static_cast<uint8_t>(PC >> 8);
    uint8_t lowPC = static_cast<uint8_t>(PC & 0xFF);

    Read(PC);
    Read(PC);

    Write(highPC, STACK_BASE + S--);
    Write(lowPC, STACK_BASE + S--);
    Write(P | INTERRUPT, STACK_BASE + S--);

    SET_FLAG(P, IRQ_INHIBIT);

    uint16_t newLowPC = Read(0xFFFA);
    uint16_t newHighPC = Read(0xFFFB);

    PC = (newHighPC << 8) | newLowPC;
}

void CPU::CheckIRQ()
{
    bool irq = Apu->CheckIRQ() || Cartridge->CheckIRQ();

    // If IRQ line is high and interrupt inhibit flag is false
    if (irq && !TEST_FLAG(P, IRQ_INHIBIT))
    {
        IrqRaised = true;
    }
    else
    {
        IrqRaised = false;
    }
}

void CPU::DoIRQ()
{
    uint8_t highPC = static_cast<uint8_t>(PC >> 8);
    uint8_t lowPC = static_cast<uint8_t>(PC & 0xFF);

    Read(PC);
    Read(PC);

    Write(highPC, STACK_BASE + S--);
    Write(lowPC, STACK_BASE + S--);
    Write(P | INTERRUPT, STACK_BASE + S--);

    SET_FLAG(P, IRQ_INHIBIT);

    uint16_t newLowPC = Read(0xFFFE);
    uint16_t newHighPC = Read(0xFFFF);

    PC = (newHighPC << 8) | newLowPC;
}

void CPU::DoOamDMA(uint8_t page)
{
    uint16_t address = page * 0x100;

    if (Clock % 6 == 0)
    {
        Read(PC, true);

        if (DmcDmaDelay > 0)
        {
            Read(PC, true);
            Read(PC, true);
            DoDmcDMA();
            DmcDmaDelay = 0;
        }
    }

    Read(PC, true);

    if (DmcDmaDelay > 0)
    {
        Read(PC, true);
        Read(PC, true);
        DoDmcDMA();
        DmcDmaDelay = 0;
    }

    for (int i = 0; i < 0x100; ++i)
    {
        Write(Read(address + i, true), 0x2004, true);

        if (DmcDmaDelay > 0)
        {
            if (i == 0xFE)
            {
                Read(PC, true);
            }
            else if (i == 0xFF)
            {
                Read(PC, true);
                Read(PC, true);
                Read(PC, true);
            }
            else
            {

                Read(PC, true);
                Read(PC, true);
            }
            
            DoDmcDMA();
            DmcDmaDelay = 0;
        }
    }
}

void CPU::DoDmcDMA()
{
    Apu->WriteDmaByte(Cartridge->CpuRead(Apu->GetDmaAddress() - 0x6000));
}

void CPU::SetControllerStrobe(bool strobe)
{
    ControllerStrobe = strobe;
    ControllerOneShift = ControllerOneState.load();
}

uint8_t CPU::GetControllerOneShift()
{
    if (ControllerStrobe)
    {
        ControllerOneShift = ControllerOneState.load();
    }

    uint8_t result = ControllerOneShift & 0x1;
    ControllerOneShift >>= 1;

    return result;
}

CPU::CPU()
    : Ppu(nullptr)
    , Apu(nullptr)
    , Cartridge(nullptr)
    , Clock(0)
    , StopFlag(false)
    , Paused(false)
    , PauseFlag(false)
    , LogEnabled(false)
    , EnableLogFlag(false)
    , LogFile(nullptr)
    , ControllerStrobe(0)
    , ControllerOneShift(0)
    , ControllerOneState(0)
    , NmiLineStatus(false)
    , NmiRaised(false)
    , NmiPending(false)
    , IrqRaised(false)
    , IrqPending(false)
    , AccumulatorFlag(false)
    , DmcDmaDelay(0)
    , PC(0)
    , S(0xFD)
    , P(0x24)
    , A(0)
    , X(0)
    , Y(0)
{
    memset(Memory, 0, sizeof(uint8_t) * 0x800);

    sprintf(Addressing, "%s", "");
    sprintf(InstructionStr, "%s", "");
    sprintf(ProgramCounter, "%s", "");
    sprintf(AddressingArg1, "%s", "");
    sprintf(AddressingArg2, "%s", "");
}

uint64_t CPU::GetClock()
{
    return Clock;
}

void CPU::AttachPPU(PPU* ppu)
{
    Ppu = ppu;
}

void CPU::AttachAPU(APU* apu)
{
    Apu = apu;
}

void CPU::AttachCart(Cart* cart)
{
    Cartridge = cart;
    PC = (static_cast<uint16_t>(Peek(0xFFFD)) << 8) + Peek(0xFFFC);
}

bool CPU::IsLogEnabled()
{
    return LogEnabled;
}

CPU::~CPU()
{
    if (LogEnabled && LogFile != nullptr)
    {
        fclose(LogFile);
    }
}

void CPU::SetControllerOneState(uint8_t statePtr)
{
    ControllerOneState = statePtr;
}

uint8_t CPU::GetControllerOneState()
{
    return ControllerOneState;
}

void CPU::Pause()
{
    std::unique_lock<std::mutex> lock(PauseMutex);

    if (!Paused)
    {
        PauseFlag = true;
        PauseCv.wait(lock);
    }
}

void CPU::Resume()
{
    std::unique_lock<std::mutex> lock(PauseMutex);
    PauseCv.notify_all();
}

bool CPU::IsPaused()
{
    std::unique_lock<std::mutex> lock(PauseMutex);
    return Paused;
}

void CPU::SetLogEnabled(bool enabled)
{
    EnableLogFlag = enabled;
}

StateSave::Ptr CPU::SaveState()
{
    StateSave::Ptr state = StateSave::New();
    state->StoreValue(Clock);
    state->StoreValue(Memory);
    state->StoreValue(ControllerOneShift);
    state->StoreValue(PC);
    state->StoreValue(S);
    state->StoreValue(P);
    state->StoreValue(A);
    state->StoreValue(X);
    state->StoreValue(Y);
    state->StorePackedValues(ControllerStrobe, NmiLineStatus, NmiRaised, NmiPending, IrqPending);

    return state;
}

void CPU::LoadState(const StateSave::Ptr& state)
{
    state->ExtractValue(Clock);
    state->ExtractValue(Memory);
    state->ExtractValue(ControllerOneShift);
    state->ExtractValue(PC);
    state->ExtractValue(S);
    state->ExtractValue(P);
    state->ExtractValue(A);
    state->ExtractValue(X);
    state->ExtractValue(Y);
    state->ExtractPackedValues(ControllerStrobe, NmiLineStatus, NmiRaised, NmiPending, IrqPending);
}

// Currently unimplemented
void CPU::Reset()
{

}

// Run the CPU
void CPU::Run()
{
    if (Ppu == nullptr)
    {
        throw NesException("CPU", "Failed to start, no PPU attached");
    }
    else if (Apu == nullptr)
    {
        throw NesException("CPU", "Failed to start, no APU attached");
    }
    else if (Cartridge == nullptr)
    {
        throw NesException("CPU", "Failed to start, no Cartidge attached");
    }
    else
    {
        // Initialize PC to the address found at the reset vector (0xFFFC and 0xFFFD)
        PC = (static_cast<uint16_t>(Peek(0xFFFD)) << 8) + Peek(0xFFFC);
    }

    while (!StopFlag) // Run stop command issued
    {
        if (EnableLogFlag != LogEnabled)
        {
            LogEnabled = EnableLogFlag;

            if (LogEnabled)
            {
                long long time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                std::string logName = Cartridge->GetGameName() + "_" + std::to_string(time) + ".log";
                LogFile = fopen(logName.c_str(), "w");

                if (LogFile == nullptr)
                {
                    std::string message = "Failed to open log file. ";
                    message += logName;

                    throw NesException("CPU", message);
                }
            }
            else
            {
                fclose(LogFile);
                LogFile = nullptr;
            }

        }

        Step();

        if (PauseFlag)
        {
            std::unique_lock<std::mutex> lock(PauseMutex);
            Paused = true;
            PauseFlag = false;

            PauseCv.notify_all();
            PauseCv.wait(lock);

            Paused = false;
        }
    }
}

void CPU::Stop()
{
    StopFlag = true;

    // If pause, resume so that run exits
    if (Paused)
    {
        Resume();
    }
}

// Execute the next instruction at PC and return true
// or return false if the next value is not an opcode
void CPU::Step()
{
    if (NmiPending)
    {
        DoNMI();
        NmiPending = false;
    }
    else if (IrqPending)
    {
        DoIRQ();
        IrqPending = false;
    }

    if (IsLogEnabled())
    {
        LogProgramCounter();
        LogRegisters();
    }

    uint8_t opcode = Read(PC++); // Retrieve opcode from memory
    
    if (IsLogEnabled()) LogOpcode(opcode);

    // Decode opcode
    InstructionDescriptor desc = InstructionSet[opcode];

    if (IsLogEnabled()) LogOfficial(desc.isOfficial);

    // Execute corresponding addressing mode

    uint16_t address = PC;
    switch (desc.addressMode)
    {
    case ABSOLUTE:
        address = Absolute(desc.instruction == JMP);
        break;
    case ABSOLUTE_X:
        address = AbsoluteX(desc.isReadModifyWrite);
        break;
    case ABSOLUTE_Y:
        address = AbsoluteY(desc.isReadModifyWrite);
        break;
    case ACCUMULATOR:
        address = Accumulator();
        break;
    case IMMEDIATE:
        address = Immediate();
        break;
    case INDIRECT:
        address = Indirect();
        break;
    case INDIRECT_X:
        address = IndirectX();
        break;
    case INDIRECT_Y:
        address = IndirectY(desc.isReadModifyWrite);
        break;
    case RELATIVE:
        address = Relative();
        break;
    case ZEROPAGE:
        address = ZeroPage();
        break;
    case ZEROPAGE_X:
        address = ZeroPageX();
        break;
    case ZEROPAGE_Y:
        address = ZeroPageY();
        break;
    case IMPLIED:
        address = PC;
        break;
    }

    // Execute instruction
    switch (desc.instruction)
    {
    case ADC:
        DoADC(address);
        break;
    case AND:
        DoAND(address);
        break;
    case ASL:
        DoASL(address);
        break;
    case BCC:
        DoBCC(address);
        break;
    case BCS:
        DoBCS(address);
        break;
    case BEQ:
        DoBEQ(address);
        break;
    case BIT:
        DoBIT(address);
        break;
    case BMI:
        DoBMI(address);
        break;
    case BNE:
        DoBNE(address);
        break;
    case BPL:
        DoBPL(address);
        break;
    case BRK:
        DoBRK();
        break;
    case BVC:
        DoBVC(address);
        break;
    case BVS:
        DoBVS(address);
        break;
    case CLC:
        DoCLC();
        break;
    case CLD:
        DoCLD();
        break;
    case CLI:
        DoCLI();
        break;
    case CLV:
        DoCLV();
        break;
    case CMP:
        DoCMP(address);
        break;
    case CPX:
        DoCPX(address);
        break;
    case CPY:
        DoCPY(address);
        break;
    case DEC:
        DoDEC(address);
        break;
    case DEX:
        DoDEX();
        break;
    case DEY:
        DoDEY();
        break;
    case EOR:
        DoEOR(address);
        break;
    case INC:
        DoINC(address);
        break;
    case INX:
        DoINX();
        break;
    case INY:
        DoINY();
        break;
    case JMP:
        DoJMP(address);
        break;
    case JSR:
        DoJSR();
        break;
    case LDA:
        DoLDA(address);
        break;
    case LDX:
        DoLDX(address);
        break;
    case LDY:
        DoLDY(address);
        break;
    case LSR:
        DoLSR(address);
        break;
    case NOP:
        DoNOP(address);
        break;
    case ORA:
        DoORA(address);
        break;
    case PHA:
        DoPHA();
        break;
    case PHP:
        DoPHP();
        break;
    case PLA:
        DoPLA();
        break;
    case PLP:
        DoPLP();
        break;
    case ROL:
        DoROL(address);
        break;
    case ROR:
        DoROR(address);
        break;
    case RTI:
        DoRTI();
        break;
    case RTS:
        DoRTS();
        break;
    case SBC:
        DoSBC(address);
        break;
    case SEC:
        DoSEC();
        break;
    case SED:
        DoSED();
        break;
    case SEI:
        DoSEI();
        break;
    case STA:
        DoSTA(address);
        break;
    case STX:
        DoSTX(address);
        break;
    case STY:
        DoSTY(address);
        break;
    case TAX:
        DoTAX();
        break;
    case TAY:
        DoTAY();
        break;
    case TSX:
        DoTSX();
        break;
    case TXA:
        DoTXA();
        break;
    case TXS:
        DoTXS();
        break;
    case TYA:
        DoTYA();
        break;
    // Unofficial Instructions
    case AHX:
        DoAHX(address);
        break;
    case ALR:
        DoALR(address);
        break;
    case ANC:
        DoANC(address);
        break;
    case ARR:
        DoARR(address);
        break;
    case AXS:
        DoAXS(address);
        break;
    case DCP:
        DoDCP(address);
        break;
    case ISC:
        DoISC(address);
        break;
    case LAS:
        DoLAS(address);
        break;
    case LAX:
        DoLAX(address);
        break;
    case RLA:
        DoRLA(address);
        break;
    case RRA:
        DoRRA(address);
        break;
    case SAX:
        DoSAX(address);
        break;
    case SHX:
        DoSHX(address);
        break;
    case SHY:
        DoSHY(address);
        break;
    case SLO:
        DoSLO(address);
        break;
    case SRE:
        DoSRE(address);
        break;
    case TAS:
        DoTAS(address);
        break;
    case XAA:
        DoXAA(address);
        break;
    case STP:
        throw NesException("CPU", "Executed STP instruction, halting");
    }

    AccumulatorFlag = false;

    if (IsLogEnabled()) PrintLog();
}

void CPU::LogProgramCounter()
{
    sprintf(ProgramCounter, "%04X", PC);
}

void CPU::LogRegisters()
{
    sprintf(Registers, "A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%3u SL:%d", A, X, Y, P, S, Ppu->GetCurrentDot(), Ppu->GetCurrentScanline());
}

void CPU::LogOpcode(uint8_t opcode)
{
    sprintf(OpCode, "%02X", opcode);
}

void CPU::LogOfficial(bool isOfficial)
{
    LogIsOfficial = isOfficial;
}

void CPU::LogInstructionName(const char* name)
{
    if (LogIsOfficial) 
    {
        sprintf(InstructionStr, " %s", name);
    }
    else
    {
        sprintf(InstructionStr, "*%s", name);
    }
    
}

void CPU::LogAccumulator()
{
    sprintf(Addressing, "A");
}

void CPU::LogRelative(uint8_t value)
{
    sprintf(Addressing, "$%04X", (PC + static_cast<int8_t>(value)) + 1);
    sprintf(AddressingArg1, "%02X", static_cast<uint8_t>(value));
}

void CPU::LogImmediate(uint8_t arg1)
{
    sprintf(Addressing, "#$%02X", arg1);
    sprintf(AddressingArg1, "%02X", arg1);
}

void CPU::LogZeroPage(uint8_t address)
{
    sprintf(Addressing, "$%02X = %02X", address, Peek(address));
    sprintf(AddressingArg1, "%02X", address);
}


void CPU::LogZeroPageX(uint8_t initialAddress, uint8_t finalAddress)
{
    sprintf(Addressing, "$%02X,X @ %02X = %02X", initialAddress, finalAddress, Peek(finalAddress));
    sprintf(AddressingArg1, "%02X", initialAddress);
}

void CPU::LogZeroPageY(uint8_t initialAddress, uint8_t finalAddress)
{
    sprintf(Addressing, "$%02X,Y @ %02X = %02X", initialAddress, finalAddress, Peek(finalAddress));
    sprintf(AddressingArg1, "%02X", initialAddress);
}

void CPU::LogAbsolute(uint8_t lowByte, uint8_t highByte, uint16_t address, bool isJump)
{
    if (!isJump)
    {
        sprintf(Addressing, "$%04X = %02X", address, Peek(address));
    }
    else
    {
        sprintf(Addressing, "$%04X", address);
    }

    sprintf(AddressingArg1, "%02X", lowByte);
    sprintf(AddressingArg2, "%02X", highByte);
}

void CPU::LogAbsoluteX(uint8_t lowByte, uint8_t highByte, uint16_t initialAddress, uint16_t finalAddress)
{
    sprintf(Addressing, "$%04X,X @ %04X = %02X", initialAddress, finalAddress, Peek(finalAddress));
    sprintf(AddressingArg1, "%02X", lowByte);
    sprintf(AddressingArg2, "%02X", highByte);
}

void CPU::LogAbsoluteY(uint8_t lowByte, uint8_t highByte, uint16_t initialAddress, uint16_t finalAddress)
{
    sprintf(Addressing, "$%04X,Y @ %04X = %02X", initialAddress, finalAddress, Peek(finalAddress));
    sprintf(AddressingArg1, "%02X", lowByte);
    sprintf(AddressingArg2, "%02X", highByte);
}

void CPU::LogIndirect(uint8_t lowIndirect, uint8_t highIndirect, uint16_t indirect, uint16_t address)
{
    sprintf(Addressing, "($%04X) = %04X", indirect, address);
    sprintf(AddressingArg1, "%02X", lowIndirect);
    sprintf(AddressingArg2, "%02X", highIndirect);
}

void CPU::LogIndirectX(uint8_t pointer, uint8_t lowIndirect, uint16_t address)
{
    sprintf(Addressing, "($%02X,X) @ %02X = %04X = %02X", pointer, lowIndirect, address, Peek(address));
    sprintf(AddressingArg1, "%02X", pointer);
}

void CPU::LogIndirectY(uint8_t pointer, uint16_t initialAddress, uint16_t finalAddress)
{
    sprintf(Addressing, "($%02X),Y = %04X @ %04X = %02X", pointer, initialAddress, finalAddress, Peek(finalAddress));
    sprintf(AddressingArg1, "%02X", pointer);
}

void CPU::PrintLog()
{
    using namespace std;

    if (LogFile != nullptr)
    {
        fprintf(LogFile, "%-6s%-3s%-3s%-3s%-5s%-28s%s\n", ProgramCounter, OpCode, AddressingArg1, AddressingArg2, InstructionStr, Addressing, Registers);
    }

    sprintf(OpCode, "%s", "");
    sprintf(Registers, "%s", "");
    sprintf(Addressing, "%s", "");
    sprintf(InstructionStr, "%s", "");
    sprintf(ProgramCounter, "%s", "");
    sprintf(AddressingArg1, "%s", "");
    sprintf(AddressingArg2, "%s", "");
}

const std::array<CPU::InstructionDescriptor, 0x100> CPU::InstructionSet
{{
    { BRK, IMPLIED,     false, true  }, // 0x00
    { ORA, INDIRECT_X,  false, true  }, // 0x01
    { STP, IMPLIED,     false, false }, // 0x02
    { SLO, INDIRECT_X,  true,  false }, // 0x03
    { NOP, ZEROPAGE,    false, false }, // 0x04
    { ORA, ZEROPAGE,    false, true  }, // 0x05
    { ASL, ZEROPAGE,    true,  true  }, // 0x06
    { SLO, ZEROPAGE,    true,  false }, // 0x07
    { PHP, IMPLIED,     false, true  }, // 0x08
    { ORA, IMMEDIATE,   false, true  }, // 0x09
    { ASL, ACCUMULATOR, true,  true  }, // 0x0A
    { ANC, IMMEDIATE,   false, false }, // 0x0B
    { NOP, ABSOLUTE,    false, false }, // 0x0C
    { ORA, ABSOLUTE,    false, true  }, // 0x0D
    { ASL, ABSOLUTE,    true,  true  }, // 0x0E
    { SLO, ABSOLUTE,    true,  false }, // 0x0F
    { BPL, RELATIVE,    false, true  }, // 0x10
    { ORA, INDIRECT_Y,  false, true  }, // 0x11
    { STP, IMPLIED,     false, false }, // 0x12
    { SLO, INDIRECT_Y,  true,  false }, // 0x13
    { NOP, ZEROPAGE_X,  false, false }, // 0x14
    { ORA, ZEROPAGE_X,  false, true  }, // 0x15
    { ASL, ZEROPAGE_X,  true,  true  }, // 0x16
    { SLO, ZEROPAGE_X,  true,  false }, // 0x17
    { CLC, IMPLIED,     false, true  }, // 0x18
    { ORA, ABSOLUTE_Y,  false, true  }, // 0x19
    { NOP, IMPLIED,     false, false }, // 0x1A
    { SLO, ABSOLUTE_Y,  true,  false }, // 0x1B
    { NOP, ABSOLUTE_X,  false, false }, // 0x1C
    { ORA, ABSOLUTE_X,  false, true  }, // 0x1D
    { ASL, ABSOLUTE_X,  true,  true  }, // 0x1E
    { SLO, ABSOLUTE_X,  true,  false }, // 0x1F
    { JSR, IMPLIED,     false, true  }, // 0x20
    { AND, INDIRECT_X,  false, true  }, // 0x21
    { STP, IMPLIED,     false, false }, // 0x22
    { RLA, INDIRECT_X,  true,  false }, // 0x23
    { BIT, ZEROPAGE,    false, true  }, // 0x24
    { AND, ZEROPAGE,    false, true  }, // 0x25
    { ROL, ZEROPAGE,    true,  true  }, // 0x26
    { RLA, ZEROPAGE,    true,  false }, // 0x27
    { PLP, IMPLIED,     false, true  }, // 0x28
    { AND, IMMEDIATE,   false, true  }, // 0x29
    { ROL, ACCUMULATOR, true,  true  }, // 0x2A
    { ANC, IMMEDIATE,   false, false }, // 0x2B
    { BIT, ABSOLUTE,    false, true  }, // 0x2C
    { AND, ABSOLUTE,    false, true  }, // 0x2D
    { ROL, ABSOLUTE,    true,  true  }, // 0x2E
    { RLA, ABSOLUTE,    true,  false }, // 0x2F
    { BMI, RELATIVE,    false, true  }, // 0x30
    { AND, INDIRECT_Y,  false, true  }, // 0x31
    { STP, IMPLIED,     false, false }, // 0x32
    { RLA, INDIRECT_Y,  true,  false }, // 0x33
    { NOP, ZEROPAGE_X,  false, false }, // 0x34
    { AND, ZEROPAGE_X,  false, true  }, // 0x35
    { ROL, ZEROPAGE_X,  false, true  }, // 0x36
    { RLA, ZEROPAGE_X,  true,  false }, // 0x37
    { SEC, IMPLIED,     false, true  }, // 0x38
    { AND, ABSOLUTE_Y,  false, true  }, // 0x39
    { NOP, IMPLIED,     false, false }, // 0x3A
    { RLA, ABSOLUTE_Y,  true,  false }, // 0x3B
    { NOP, ABSOLUTE_X,  false, false }, // 0x3C
    { AND, ABSOLUTE_X,  false, true  }, // 0x3D
    { ROL, ABSOLUTE_X,  true,  true  }, // 0x3E
    { RLA, ABSOLUTE_X,  true,  false }, // 0x3F
    { RTI, IMPLIED,     false, true  }, // 0x40
    { EOR, INDIRECT_X,  false, true  }, // 0x41
    { STP, IMPLIED,     false, false }, // 0x42
    { SRE, INDIRECT_X,  true,  false }, // 0x43
    { NOP, ZEROPAGE,    false, false }, // 0x44
    { EOR, ZEROPAGE,    false, true  }, // 0x45
    { LSR, ZEROPAGE,    true,  true  }, // 0x46
    { SRE, ZEROPAGE,    true,  false }, // 0x47
    { PHA, IMPLIED,     false, true  }, // 0x48
    { EOR, IMMEDIATE,   false, true  }, // 0x49
    { LSR, ACCUMULATOR, true,  true  }, // 0x4A
    { ALR, IMMEDIATE,   false, false }, // 0x4B
    { JMP, ABSOLUTE,    false, true  }, // 0x4C
    { EOR, ABSOLUTE,    false, true  }, // 0x4D
    { LSR, ABSOLUTE,    true,  true  }, // 0x4E
    { SRE, ABSOLUTE,    true,  false }, // 0x4F
    { BVC, RELATIVE,    false, true  }, // 0x50
    { EOR, INDIRECT_Y,  false, true  }, // 0x51
    { STP, IMPLIED,     false, false }, // 0x52
    { SRE, INDIRECT_Y,  true,  false }, // 0x53
    { NOP, ZEROPAGE_X,  false, false }, // 0x54
    { EOR, ZEROPAGE_X,  false, true  }, // 0x55
    { LSR, ZEROPAGE_X,  true,  true  }, // 0x56
    { SRE, ZEROPAGE_X,  true,  false }, // 0x57
    { CLI, IMPLIED,     false, true  }, // 0x58
    { EOR, ABSOLUTE_Y,  false, true  }, // 0x59
    { NOP, IMPLIED,     false, false }, // 0x5A
    { SRE, ABSOLUTE_Y,  true,  false }, // 0x5B
    { NOP, ABSOLUTE_X,  false, false }, // 0x5C
    { EOR, ABSOLUTE_X,  false, true  }, // 0x5D
    { LSR, ABSOLUTE_X,  true,  true  }, // 0x5E
    { SRE, ABSOLUTE_X,  true,  false }, // 0x5F
    { RTS, IMPLIED,     false, true  }, // 0x60
    { ADC, INDIRECT_X,  false, true  }, // 0x61
    { STP, IMPLIED,     false, false }, // 0x62
    { RRA, INDIRECT_X,  true,  false }, // 0x63
    { NOP, ZEROPAGE,    false, false }, // 0x64
    { ADC, ZEROPAGE,    false, true  }, // 0x65
    { ROR, ZEROPAGE,    true,  true  }, // 0x66
    { RRA, ZEROPAGE,    true,  false }, // 0x67
    { PLA, IMPLIED,     false, true  }, // 0x68
    { ADC, IMMEDIATE,   false, true  }, // 0x69
    { ROR, ACCUMULATOR, true,  true  }, // 0x6A
    { ARR, IMMEDIATE,   false, false }, // 0x6B
    { JMP, INDIRECT,    false, true  }, // 0x6C
    { ADC, ABSOLUTE,    false, true  }, // 0x6D
    { ROR, ABSOLUTE,    true,  true  }, // 0x6E
    { RRA, ABSOLUTE,    true,  false }, // 0x6F
    { BVS, RELATIVE,    false, true  }, // 0x70
    { ADC, INDIRECT_Y,  false, true  }, // 0x71
    { STP, IMPLIED,     false, false }, // 0x72
    { RRA, INDIRECT_Y,  true,  false }, // 0x73
    { NOP, ZEROPAGE_X,  false, false }, // 0x74
    { ADC, ZEROPAGE_X,  false, true  }, // 0x75
    { ROR, ZEROPAGE_X,  true,  true  }, // 0x76
    { RRA, ZEROPAGE_X,  true,  false }, // 0x77
    { SEI, IMPLIED,     false, true  }, // 0x78
    { ADC, ABSOLUTE_Y,  false, true  }, // 0x79
    { NOP, IMPLIED,     false, false }, // 0x7A
    { RRA, ABSOLUTE_Y,  true,  false }, // 0x7B
    { NOP, ABSOLUTE_X,  false, false }, // 0x7C
    { ADC, ABSOLUTE_X,  false, true  }, // 0x7D
    { ROR, ABSOLUTE_X,  true,  true  }, // 0x7E
    { RRA, ABSOLUTE_X,  true,  false }, // 0x7F
    { NOP, IMMEDIATE,   false, false }, // 0x80
    { STA, INDIRECT_X,  true,  true  }, // 0x81
    { NOP, IMMEDIATE,   false, false }, // 0x82
    { SAX, INDIRECT_X,  true,  false }, // 0x83
    { STY, ZEROPAGE,    true,  true  }, // 0x84
    { STA, ZEROPAGE,    true,  true  }, // 0x85
    { STX, ZEROPAGE,    true,  true  }, // 0x86
    { SAX, ZEROPAGE,    true,  false }, // 0x87
    { DEY, IMPLIED,     false, true  }, // 0x88
    { NOP, IMMEDIATE,   false, false }, // 0x89
    { TXA, IMPLIED,     false, true  }, // 0x8A
    { XAA, IMMEDIATE,   false, false }, // 0x8B
    { STY, ABSOLUTE,    true,  true  }, // 0x8C
    { STA, ABSOLUTE,    true,  true  }, // 0x8D
    { STX, ABSOLUTE,    true,  true  }, // 0x8E
    { SAX, ABSOLUTE,    true,  false }, // 0x8F
    { BCC, RELATIVE,    false, true  }, // 0x90
    { STA, INDIRECT_Y,  true,  true  }, // 0x91
    { STP, IMPLIED,     false, false }, // 0x92
    { AHX, INDIRECT_Y,  true,  false }, // 0x93
    { STY, ZEROPAGE_X,  true,  true  }, // 0x94
    { STA, ZEROPAGE_X,  true,  true  }, // 0x95
    { STX, ZEROPAGE_Y,  true,  true  }, // 0x96
    { SAX, ZEROPAGE_Y,  true,  false }, // 0x97
    { TYA, IMPLIED,     false, true  }, // 0x98
    { STA, ABSOLUTE_Y,  true,  true  }, // 0x99
    { TXS, IMPLIED,     false, true  }, // 0x9A
    { TAS, ABSOLUTE_Y,  true,  false }, // 0x9B
    { SHY, ABSOLUTE_X,  true,  false }, // 0x9C
    { STA, ABSOLUTE_X,  true,  true  }, // 0x9D
    { SHX, ABSOLUTE_Y,  true,  false }, // 0x9E
    { AHX, ABSOLUTE_Y,  true,  false }, // 0x9F
    { LDY, IMMEDIATE,   false, true  }, // 0xA0
    { LDA, INDIRECT_X,  false, true  }, // 0xA1
    { LDX, IMMEDIATE,   false, true  }, // 0xA2
    { LAX, INDIRECT_X,  false, false }, // 0xA3
    { LDY, ZEROPAGE,    false, true  }, // 0xA4
    { LDA, ZEROPAGE,    false, true  }, // 0xA5
    { LDX, ZEROPAGE,    false, true  }, // 0xA6
    { LAX, ZEROPAGE,    false, false }, // 0xA7
    { TAY, IMPLIED,     false, true  }, // 0xA8
    { LDA, IMMEDIATE,   false, true  }, // 0xA9
    { TAX, IMPLIED,     false, true  }, // 0xAA
    { LAX, IMMEDIATE,   false, false }, // 0xAB
    { LDY, ABSOLUTE,    false, true  }, // 0xAC
    { LDA, ABSOLUTE,    false, true  }, // 0xAD
    { LDX, ABSOLUTE,    false, true  }, // 0xAE
    { LAX, ABSOLUTE,    false, false }, // 0xAF
    { BCS, RELATIVE,    false, true  }, // 0xB0
    { LDA, INDIRECT_Y,  false, true  }, // 0xB1
    { STP, IMPLIED,     false, false }, // 0xB2
    { LAX, INDIRECT_Y,  false, false }, // 0xB3
    { LDY, ZEROPAGE_X,  false, true  }, // 0xB4
    { LDA, ZEROPAGE_X,  false, true  }, // 0xB5
    { LDX, ZEROPAGE_Y,  false, true  }, // 0xB6
    { LAX, ZEROPAGE_Y,  false, false }, // 0xB7
    { CLV, IMPLIED,     false, true  }, // 0xB8
    { LDA, ABSOLUTE_Y,  false, true  }, // 0xB9
    { TSX, IMPLIED,     false, true  }, // 0xBA
    { LAS, ABSOLUTE_Y,  false, false }, // 0xBB
    { LDY, ABSOLUTE_X,  false, true  }, // 0xBC
    { LDA, ABSOLUTE_X,  false, true  }, // 0xBD
    { LDX, ABSOLUTE_Y,  false, true  }, // 0xBE
    { LAX, ABSOLUTE_Y,  false, false }, // 0xBF
    { CPY, IMMEDIATE,   false, true  }, // 0xC0
    { CMP, INDIRECT_X,  false, true  }, // 0xC1
    { NOP, IMMEDIATE,   false, false }, // 0xC2
    { DCP, INDIRECT_X,  true,  false }, // 0xC3
    { CPY, ZEROPAGE,    false, true  }, // 0xC4
    { CMP, ZEROPAGE,    false, true  }, // 0xC5
    { DEC, ZEROPAGE,    true,  true  }, // 0xC6
    { DCP, ZEROPAGE,    true,  false }, // 0xC7
    { INY, IMPLIED,     false, true  }, // 0xC8
    { CMP, IMMEDIATE,   false, true  }, // 0xC9
    { DEX, IMPLIED,     false, true  }, // 0xCA
    { AXS, IMMEDIATE,   false, false }, // 0xCB
    { CPY, ABSOLUTE,    false, true  }, // 0xCC
    { CMP, ABSOLUTE,    false, true  }, // 0xCD
    { DEC, ABSOLUTE,    true,  true  }, // 0xCE
    { DCP, ABSOLUTE,    true,  false }, // 0xCF
    { BNE, RELATIVE,    false, true  }, // 0xD0
    { CMP, INDIRECT_Y,  false, true  }, // 0xD1
    { STP, IMPLIED,     false, false }, // 0xD2
    { DCP, INDIRECT_Y,  true,  false }, // 0xD3
    { NOP, ZEROPAGE_X,  false, false }, // 0xD4
    { CMP, ZEROPAGE_X,  false, true  }, // 0xD5
    { DEC, ZEROPAGE_X,  true,  true  }, // 0xD6
    { DCP, ZEROPAGE_X,  true,  false }, // 0xD7
    { CLD, IMPLIED,     false, true  }, // 0xD8
    { CMP, ABSOLUTE_Y,  false, true  }, // 0xD9
    { NOP, IMPLIED,     false, false }, // 0xDA
    { DCP, ABSOLUTE_Y,  true,  false }, // 0xDB
    { NOP, ABSOLUTE_X,  false, false }, // 0xDC
    { CMP, ABSOLUTE_X,  false, true  }, // 0xDD
    { DEC, ABSOLUTE_X,  true,  true  }, // 0xDE
    { DCP, ABSOLUTE_X,  true,  false }, // 0xDF
    { CPX, IMMEDIATE,   false, true  }, // 0xE0
    { SBC, INDIRECT_X,  false, true  }, // 0xE1
    { NOP, IMMEDIATE,   false, false }, // 0xE2
    { ISC, INDIRECT_X,  true,  false }, // 0xE3
    { CPX, ZEROPAGE,    false, true  }, // 0xE4
    { SBC, ZEROPAGE,    false, true  }, // 0xE5
    { INC, ZEROPAGE,    true,  true  }, // 0xE6
    { ISC, ZEROPAGE,    true,  false }, // 0xE7
    { INX, IMPLIED,     false, true  }, // 0xE8
    { SBC, IMMEDIATE,   false, true  }, // 0xE9
    { NOP, IMPLIED,     false, true  }, // 0xEA
    { SBC, IMMEDIATE,   false, false }, // 0xEB
    { CPX, ABSOLUTE,    false, true  }, // 0xEC
    { SBC, ABSOLUTE,    false, true  }, // 0xED
    { INC, ABSOLUTE,    true,  true  }, // 0xEE
    { ISC, ABSOLUTE,    true,  false }, // 0xEF
    { BEQ, RELATIVE,    false, true  }, // 0xF0
    { SBC, INDIRECT_Y,  false, true  }, // 0xF1
    { STP, IMPLIED,     false, false }, // 0xF2
    { ISC, INDIRECT_Y,  true,  false }, // 0xF3
    { NOP, ZEROPAGE_X,  false, false }, // 0xF4
    { SBC, ZEROPAGE_X,  false, true  }, // 0xF5
    { INC, ZEROPAGE_X,  true,  true  }, // 0xF6
    { ISC, ZEROPAGE_X,  true,  false }, // 0xF7
    { SED, IMPLIED,     false, true  }, // 0xF8
    { SBC, ABSOLUTE_Y,  false, true  }, // 0xF9
    { NOP, IMPLIED,     false, false }, // 0xFA
    { ISC, ABSOLUTE_Y,  true,  false }, // 0xFB
    { NOP, ABSOLUTE_X,  false, false }, // 0xFC
    { SBC, ABSOLUTE_X,  false, true  }, // 0xFD
    { INC, ABSOLUTE_X,  true,  true  }, // 0xFE
    { ISC, ABSOLUTE_X,  true,  false }  // 0xFF
}};