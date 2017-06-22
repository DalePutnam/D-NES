/*
 * cpu.cc
 *
 *  Created on: Mar 15, 2014
 *      Author: Dale
 */

#include <string>
#include <cstring>

#include "cpu.h"
#include "ppu.h"
#include "apu.h"

uint8_t CPU::DebugRead(uint16_t address)
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
    else if (address > 0x5FFF && address < 0x10000)
    {
        return Cartridge->PrgRead(address - 0x6000);
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

    if (PpuRendevous > 0)
    {
        PpuRendevous -= 3;
    }
}

uint8_t CPU::Read(uint16_t address)
{
    uint8_t value;

    do
    {
        IncrementClock();
    } while (IsStalled);


    // Any address less then 0x2000 is just the
    // Internal Ram mirrored every 0x800 bytes
    if (address < 0x2000)
    {
        value = Memory[address % 0x800];
    }
    else if (address >= 0x2000 && address < 0x4000)
    {
        uint16_t addr = (address - 0x2000) % 8;

        if (addr == 2)
        {
            value = Ppu->ReadPPUStatus();
        }
        else if (addr == 4)
        {
            value = Ppu->ReadOAMData();
        }
        else if (addr == 7)
        {
            value = Ppu->ReadPPUData();
        }
        else
        {
            value = 0xFF;
        }
    }
    else if (address == 0x4015)
    {
        value = Apu->ReadAPUStatus();
    }
    else if (address == 0x4016)
    {
        value = GetControllerOneShift();
    }
    else if (address > 0x5FFF && address < 0x10000)
    {
        value = Cartridge->PrgRead(address - 0x6000);
    }
    else
    {
        value = 0x00;
    }

    CheckNMI();
    CheckIRQ();

    return value;
}

void CPU::Write(uint8_t M, uint16_t address)
{
    // Note: THIS IS NOT THREAD SAFE
    IncrementClock();

    // OAM DMA
    if (address == 0x4014)
    {
        uint16_t page = M * 0x100;

        if (Clock % 6 == 0)
        {
            IncrementClock();
        }
        else
        {
            IncrementClock();
            IncrementClock();
        }

        for (int i = 0; i < 0x100; ++i)
        {
            uint8_t value = Read(page + i);
            IncrementClock();
            Ppu->WriteOAMDATA(value);
        }
    }
    // Any address less then 0x2000 is just the
    // Internal Ram mirrored every 0x800 bytes
    else if (address < 0x2000)
    {
        Memory[address % 0x800] = M;
    }
    else if (address >= 0x2000 && address < 0x4000)
    {
        uint16_t addr = (address - 0x2000) % 8;

        if (addr == 0)
        {
            Ppu->WritePPUCTRL(M);
        }
        else if (addr == 1)
        {
            Ppu->WritePPUMASK(M);
        }
        else if (addr == 3)
        {
            Ppu->WriteOAMADDR(M);
        }
        else if (addr == 4)
        {
            Ppu->WriteOAMDATA(M);
        }
        else if (addr == 5)
        {
            Ppu->WritePPUSCROLL(M);
        }
        else if (addr == 6)
        {
            Ppu->WritePPUADDR(M);
        }
        else if (addr == 7)
        {
            Ppu->WritePPUDATA(M);
        }
    }
    else if ((address >= 0x4000 && address <= 0x4015) || address == 0x4017)
    {
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
        case 0x4017: Apu->WriteAPUFrameCounter(M); break;
        default: break;
        }
    }
    else if (address == 0x4016)
    {
        SetControllerStrobe(!!(M & 0x1));
    }
    else if (address > 0x5FFF && address < 0x10000)
    {
        Cartridge->PrgWrite(M, address - 0x6000);
    }

    CheckNMI();
    CheckIRQ();
}

int8_t CPU::Relative()
{
    int8_t value = Read(PC++);

    if (IsLogEnabled())
    {
        LogRelative(value);
    }

    return value;
}

uint8_t CPU::Accumulator()
{
    Read(PC);

    if (IsLogEnabled())
    {
        LogAccumulator();
    }

    return A;
}

uint8_t CPU::Immediate()
{
    uint8_t value = Read(PC++);

    if (IsLogEnabled())
    {
        LogImmediate(value);
    }

    return value;
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
        uint16_t otherHighByte = DebugRead((highIndirect << 8) + (lowIndirect + 1));
        uint16_t otherAddress = (otherHighByte << 8) + lowByte;
        LogIndirect(static_cast<uint8_t>(lowIndirect), static_cast<uint8_t>(highIndirect), (highIndirect << 8) + lowIndirect, otherAddress);
    }

    return address;
}

// Indexed Indirect addressing reads a byte from memory
// and adds the X register to it (mod 256). This is then
// used as a zero page address and two bytes are read from
// this address and the next (with zero page wrap around)
// to make the final 16-bit address of the operand
uint16_t CPU::IndexedIndirect()
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
        LogIndexedIndirect(pointer, lowIndirect, address);
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
uint16_t CPU::IndirectIndexed(bool isRMW)
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
        LogIndirectIndexed(static_cast<uint8_t>(pointer), initialAddress, finalAddress);
    }

    return finalAddress;
}

// Add With Carry
// Adds M  and the Carry flag to the Accumulator (A)
// Sets the Carry, Negative, Overflow and Zero
// flags as necessary.
void CPU::ADC(uint8_t M)
{
    if (IsLogEnabled()) LogInstructionName("ADC");

    uint8_t C = P & 0x01; // get carry flag
    uint8_t origA = A;
    int16_t result = A + M + C;

    // If overflow occurred, set the carry flag
    (result > 0xFF) ? P = P | 0x01 : P = P & 0xFE;

    A = static_cast<uint8_t>(result);

    // if Result is 0 set zero flag
    (A == 0) ? P = P | 0x02 : P = P & 0xFD;
    // if bit seven is 1 set negative flag
    ((A & 0x80) >> 7) == 1 ? P = P | 0x80 : P = P & 0x7F;
    // if signed overflow occurred set overflow flag
    ((A >> 7) != (origA >> 7) && (A >> 7) != (M >> 7)) ? P = P | 0x40 : P = P & 0xBF;
}

// Logical And
// Performs a logical and on the Accumulator and M
// Sets the Negative and Zero flags if necessary
void CPU::AND(uint8_t M)
{
    if (IsLogEnabled()) LogInstructionName("AND");

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
uint8_t CPU::ASL(uint8_t M)
{
    if (IsLogEnabled()) LogInstructionName("ASL");

    uint8_t origM = M;
    M = M << 1;

    // Set carry flag to bit 7 of origM
    (origM >> 7) == 1 ? P = P | 0x01 : P = P & 0xFE;
    // if Result is 0 set zero flag
    (M == 0) ? P = P | 0x02 : P = P & 0xFD;
    // if bit seven is 1 set negative flag
    (M >> 7) == 1 ? P = P | 0x80 : P = P & 0x7F;

    return M;
}

// Common Branch function
// Any branch instruction that takes the branch will use this function to do so
void CPU::Branch(int8_t offset)
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
void CPU::BCC(int8_t offset)
{
    if (IsLogEnabled()) LogInstructionName("BCC");

    if ((P & 0x01) == 0)
    {
        Branch(offset);
    }
}

// Branch if Carry Set
// If the Carry flag is 1 then adjust PC by the (signed) amount
// found at the next memory location.
void CPU::BCS(int8_t offset)
{
    if (IsLogEnabled()) LogInstructionName("BCS");

    if ((P & 0x01) == 1)
    {
        Branch(offset);
    }
}

// Branch if Equal
// If the Zero flag is 1 then adjust PC by the (signed) amount
// found at the next memory location.
void CPU::BEQ(int8_t offset)
{
    if (IsLogEnabled()) LogInstructionName("BEQ");

    if (((P & 0x02) >> 1) == 1)
    {
        Branch(offset);
    }
}

// Bit Test
// Used to test if certain bits are set in M.
// The Accumulator is expected to hold a mask pattern and
// is anded with M to clear or set the Zero flag.
// The Overflow and Negative flags are also set if necessary.
void CPU::BIT(uint8_t M)
{
    if (IsLogEnabled()) LogInstructionName("BIT");

    uint8_t result = A & M;

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
void CPU::BMI(int8_t offset)
{
    if (IsLogEnabled()) LogInstructionName("BMI");

    if (((P & 0x80) >> 7) == 1)
    {
        Branch(offset);
    }
}

// Branch if Not Equal
// If the Zero flag is 0 then adjust PC by the (signed) amount
// found at the next memory location.
void CPU::BNE(int8_t offset)
{
    if (IsLogEnabled()) LogInstructionName("BNE");

    if (((P & 0x02) >> 1) == 0)
    {
        Branch(offset);
    }
}

// Branch if Positive
// If the Negative flag is 0 then adjust PC by the (signed) amount
// found at the next memory location.
void CPU::BPL(int8_t offset)
{
    if (IsLogEnabled()) LogInstructionName("BPL");

    if (((P & 0x80) >> 7) == 0)
    {
        Branch(offset);
    }
}

// Force Interrupt
// Push PC and P onto the stack and jump to the
// address stored at the interrupt vector (0xFFFE and 0xFFFF)
void CPU::BRK()
{
    if (IsLogEnabled()) LogInstructionName("BRK");

    uint8_t highPC = static_cast<uint8_t>(PC >> 8);
    uint8_t lowPC = static_cast<uint8_t>(PC & 0xFF);

    Write(highPC, 0x100 + S);
    Write(lowPC, 0x100 + (S - 1));
    Write(P | 0x30, 0x100 + (S - 2));
    S -= 3;

    P |= 0x4;

    PC = Read(0xFFFE) + (((uint16_t)Read(0xFFFF)) * 0x100);
}

// Branch if Overflow Clear
// If the Overflow flag is 0 then adjust PC by the (signed) amount
// found at the next memory location.
void CPU::BVC(int8_t offset)
{
    if (IsLogEnabled()) LogInstructionName("BVC");

    if (((P & 0x40) >> 6) == 0)
    {
        Branch(offset);
    }
}

// Branch if Overflow Set
// If the Overflow flag is 0 then adjust PC by the (signed) amount
// found at the next memory location.
void CPU::BVS(int8_t offset)
{
    if (IsLogEnabled()) LogInstructionName("BVS");

    if (((P & 0x40) >> 6) == 1)
    {
        Branch(offset);
    }
}

// Clear Carry Flag
void CPU::CLC()
{
    if (IsLogEnabled()) LogInstructionName("CLC");

    P = P & 0xFE;
}

// Clear Decimal Mode
// Note: Since this 6502 simulator is for a NES emulator
// this flag doesn't do anything as the NES's CPU didn't
// include decimal mode (and it was stupid anyway)
void CPU::CLD()
{
    if (IsLogEnabled()) LogInstructionName("CLD");

    P = P & 0xF7;
}

// Clear Interrupt Disable
void CPU::CLI()
{
    if (IsLogEnabled()) LogInstructionName("CLI");

    P = P & 0xFB;
}

// Clear Overflow flag
void CPU::CLV()
{
    if (IsLogEnabled()) LogInstructionName("CLV");

    P = P & 0xBF;
}

// Compare
// Compares the Accumulator with M by subtracting
// A from M and setting the Zero flag if they were equal
// and the Carry flag if the Accumulator was larger (or equal)
void CPU::CMP(uint8_t M)
{
    if (IsLogEnabled()) LogInstructionName("CMP");

    uint8_t result = A - M;
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
void CPU::CPX(uint8_t M)
{
    if (IsLogEnabled()) LogInstructionName("CPX");

    uint8_t result = X - M;
    // if X >= M set carry flag
    (X >= M) ? P = P | 0x01 : P = P & 0xFE;
    // if X = M set zero flag
    (X == M) ? P = P | 0x02 : P = P & 0xFD;
    // set negative flag to bit 7 of result
    ((result >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;
}

// Compare Y Register
// Compares the Y register with M by subtracting
// Y from M and setting the Zero flag if they were equal
// and the Carry flag if the Y register was larger (or equal)
void CPU::CPY(uint8_t M)
{
    if (IsLogEnabled()) LogInstructionName("CPY");

    uint8_t result = Y - M;
    // if Y >= M set carry flag
    (Y >= M) ? P = P | 0x01 : P = P & 0xFE;
    // if Y = M set zero flag
    (Y == M) ? P = P | 0x02 : P = P & 0xFD;
    // set negative flag to bit 7 of result
    ((result >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;
}

// Decrement Memory
// Subtracts one from M and then returns the new value
uint8_t CPU::DEC(uint8_t M)
{
    if (IsLogEnabled()) LogInstructionName("DEC");

    uint8_t result = M - 1;
    // if result is 0 set zero flag
    (result == 0) ? P = P | 0x02 : P = P & 0xFD;
    // set negative flag to bit 7 of result
    ((result >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;
    return result;
}

// Decrement X Register
// Subtracts one from X
void CPU::DEX()
{
    if (IsLogEnabled()) LogInstructionName("DEX");

    --X;
    // if X is 0 set zero flag
    (X == 0) ? P = P | 0x02 : P = P & 0xFD;
    // set negative flag to bit 7 of X
    ((X >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;
}

// Decrement X Register
// Subtracts one from Y
void CPU::DEY()
{
    if (IsLogEnabled()) LogInstructionName("DEY");

    --Y;
    // if Y is 0 set zero flag
    (Y == 0) ? P = P | 0x02 : P = P & 0xFD;
    // set negative flag to bit 7 of Y
    ((Y >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;
}

// Exclusive Or
// Performs and exclusive or on the Accumulator (A)
// and M. The Zero and Negative flags are set if necessary.
void CPU::EOR(uint8_t M)
{
    if (IsLogEnabled()) LogInstructionName("EOR");

    A = A ^ M;
    // if A is 0 set zero flag
    (A == 0) ? P = P | 0x02 : P = P & 0xFD;
    // set negative flag to bit 7 of A
    ((A >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;
}

// Increment Memory
// Adds one to M and then returns the new value
uint8_t CPU::INC(uint8_t M)
{
    if (IsLogEnabled()) LogInstructionName("INC");

    uint8_t result = M + 1;
    // if result is 0 set zero flag
    (result == 0) ? P = P | 0x02 : P = P & 0xFD;
    // set negative flag to bit 7 of result
    ((result >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;
    return result;
}

// Increment X Register
// Adds one to X
void CPU::INX()
{
    if (IsLogEnabled()) LogInstructionName("INX");

    ++X;
    // if X is 0 set zero flag
    (X == 0) ? P = P | 0x02 : P = P & 0xFD;
    // set negative flag to bit 7 of X
    ((X >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;
}

// Increment Y Register
// Adds one to Y
void CPU::INY()
{
    if (IsLogEnabled()) LogInstructionName("INY");

    ++Y;
    // if Y is 0 set zero flag
    (Y == 0) ? P = P | 0x02 : P = P & 0xFD;
    // set negative flag to bit 7 of Y
    ((Y >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;
}

// Jump
// Sets PC to M
void CPU::JMP(uint16_t M)
{
    if (IsLogEnabled()) LogInstructionName("JMP");

    PC = M;
}

// Jump to Subroutine
// Pushes PC onto the stack and then sets PC to M
// This implementation may produce inaccurate behaviour since
// in the actual 6502, the last operation of the address fetch doesn't
// happen until after the rest of JSR has completed, but here
// it happens before with some fudging to compensate (decrementing PC)
void CPU::JSR(uint16_t M)
{
    if (IsLogEnabled()) LogInstructionName("JSR");

    PC--;
    Read(0x100 + S); // internal operation

    uint8_t highPC = static_cast<uint8_t>(PC >> 8);
    uint8_t lowPC = static_cast<uint8_t>(PC & 0xFF);

    Write(highPC, 0x100 + S--);
    Write(lowPC, 0x100 + S--);

    PC = M;
}

// Load Accumulator
// Sets the accumulator to M
void CPU::LDA(uint8_t M)
{
    if (IsLogEnabled()) LogInstructionName("LDA");

    A = M;

    // if A is 0 set zero flag
    (A == 0) ? P = P | 0x02 : P = P & 0xFD;
    // set negative flag to bit 7 of A
    ((A >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;
}

// Load X Register
// Sets the X to M
void CPU::LDX(uint8_t M)
{
    if (IsLogEnabled()) LogInstructionName("LDX");

    X = M;

    // if X is 0 set zero flag
    (X == 0) ? P = P | 0x02 : P = P & 0xFD;
    // set negative flag to bit 7 of X
    ((X >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;
}

// Load Y Register
// Sets the Y to M
void CPU::LDY(uint8_t M)
{
    if (IsLogEnabled()) LogInstructionName("LDY");

    Y = M;

    // if Y is 0 set zero flag
    (Y == 0) ? P = P | 0x02 : P = P & 0xFD;
    // set negative flag to bit 7 of Y
    ((Y >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;
}

// Logical Shift Right
// Performs a logical left shift on M and returns the result
// The Carry flag is set to the former bit zero of M.
// The Zero and Negative flags are set like normal/
uint8_t CPU::LSR(uint8_t M)
{
    if (IsLogEnabled()) LogInstructionName("LSR");

    uint8_t result = M >> 1;

    // set carry flag to bit 0 of M
    ((M & 0x01) == 1) ? P = P | 0x01 : P = P & 0xFE;
    // if result is 0 set zero flag
    (result == 0) ? P = P | 0x02 : P = P & 0xFD;
    // set negative flag to bit 7 of result
    ((result >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;

    return result;
}

// No Operation
// Admittedly this really doesn't need its own function, but its
// a convenient place to put the logger code.
void CPU::NOP()
{
    if (IsLogEnabled()) LogInstructionName("NOP");
}

// Logical Inclusive Or
// Performs a logical inclusive or on the Accumulator (A) and M.
// The Zero and Negative flags are set if necessary
void CPU::ORA(uint8_t M)
{
    if (IsLogEnabled()) LogInstructionName("ORA");

    A = A | M;
    // if A is 0 set zero flag
    (A == 0) ? P = P | 0x02 : P = P & 0xFD;
    // set negative flag to bit 7 of A
    ((A >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;
}

// Push Accumulator
// Pushes the Accumulator (A) onto the stack
void CPU::PHA()
{
    if (IsLogEnabled()) LogInstructionName("PHA");

    Write(A, 0x100 + S--);
}

// Push Processor Status
// Pushes P onto the stack
void CPU::PHP()
{
    if (IsLogEnabled()) LogInstructionName("PHP");

    Write(P | 0x30, 0x100 + S--);
}

// Pull Accumulator
// Pulls the Accumulator (A) off the stack
// Sets Zero and Negative flags if necessary
void CPU::PLA()
{
    if (IsLogEnabled()) LogInstructionName("PLA");

    Read(0x100 + S++);

    A = Read(0x100 + S);
    // if A is 0 set zero flag
    (A == 0) ? P = P | 0x02 : P = P & 0xFD;
    // set negative flag to bit 7 of A
    ((A >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;
}

// Pull Processor Status
// Pulls P off the stack
// Sets Zero and Negative flags if necessary
void CPU::PLP()
{
    if (IsLogEnabled()) LogInstructionName("PLP");

    Read(0x100 + S++);

    P = (Read(0x100 + S) & 0xEF) | 0x20;
}

// Rotate Left
// Rotates the bits of one left. The carry flag is shifted
// into bit 0 and then set to the former bit 7. Returns the result.
// The Zero and Negative flags are set if necessary
uint8_t CPU::ROL(uint8_t M)
{
    if (IsLogEnabled()) LogInstructionName("ROL");

    uint8_t result = (M << 1) | (P & 0x01);
    // set carry flag to old bit 7
    ((M >> 7) == 1) ? P = P | 0x01 : P = P & 0xFE;

    // if result is 0 set zero flag
    (result == 0) ? P = P | 0x02 : P = P & 0xFD;
    // set negative flag to bit 7 of result
    ((result >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;

    return result;
}

// Rotate Right
// Rotates the bits of one left. The carry flag is shifted
// into bit 7 and then set to the former bit 0
// The Zero and Negative flags are set if necessary
uint8_t CPU::ROR(uint8_t M)
{
    if (IsLogEnabled()) LogInstructionName("ROR");

    uint8_t result = (M >> 1) | ((P & 0x01) << 7);
    // set carry flag to old bit 0
    ((M & 0x01) == 1) ? P = P | 0x01 : P = P & 0xFE;
    // if result is 0 set zero flag
    (result == 0) ? P = P | 0x02 : P = P & 0xFD;
    // set negative flag to bit 7 of result
    ((result >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;

    return result;
}

// Return From Interrupt
// Pulls PC and P from the stack and continues
// execution at the new PC
void CPU::RTI()
{
    if (IsLogEnabled()) LogInstructionName("RTI");

    Read(0x100 + S);

    P = Read(0x100 + (S + 1)) & 0xCF;
    uint16_t lowPC = Read(0x100 + (S + 2));
    uint16_t highPC = Read(0x100 + (S + 3));
    PC = (highPC << 8) | lowPC;
    S += 3;
}

// Return From Subroutine
// Pulls PC from the stack and continues
// execution at the new PC
void CPU::RTS()
{
    if (IsLogEnabled()) LogInstructionName("RTS");

    Read(0x100 + S++);

    uint16_t lowPC = Read(0x100 + S++);
    uint16_t highPC = Read(0x100 + S);
    PC = (highPC << 8) | lowPC;

    Read(PC++);
}

// Subtract With Carry
// Subtract A from M and the not of the Carry flag.
// Sets the Carry, Overflow, Negative and Zero flags if necessary
void CPU::SBC(uint8_t M)
{
    if (IsLogEnabled()) LogInstructionName("SBC");

    uint8_t C = P & 0x01; // get carry flag
    uint8_t origA = A;
    int16_t result = A - M - (1 - C);

    // if signed overflow occurred set overflow flag
    //(result > 127 || result < -128) ? P = P | 0x40 : P = P & 0xBF;
    // If overflow occurred, clear the carry flag
    (result < 0) ? P = P & 0xFE : P = P | 0x01;

    A = static_cast<uint8_t>(result);

    // if Result is 0 set zero flag
    (A == 0) ? P = P | 0x02 : P = P & 0xFD;
    // if bit seven is 1 set negative flag
    ((A & 0x80) >> 7) == 1 ? P = P | 0x80 : P = P & 0x7F;
    // if signed overflow occurred set overflow flag
    ((origA >> 7) != (M >> 7)) && ((origA >> 7) != (A >> 7)) ? P = P | 0x40 : P = P & 0xBF;
}

// Set Carry Flag
void CPU::SEC()
{
    if (IsLogEnabled()) LogInstructionName("SEC");

    P = P | 0x01;
}

// Set Decimal Mode
// Setting this flag has no actual effect.
// See the comment of CLD for why.
void CPU::SED()
{
    if (IsLogEnabled()) LogInstructionName("SED");

    P = P | 0x08;
}

// Set Interrupt Disable
void CPU::SEI()
{
    if (IsLogEnabled()) LogInstructionName("SEI");

    P = P | 0x04;
}

// Store Accumulator
// This simply returns A since all memory writes
// are done externally to these functions
uint8_t CPU::STA()
{
    if (IsLogEnabled()) LogInstructionName("STA");

    return A;
}

// Store X Register
// This simply returns X since all memory writes
// are done externally to these functions
uint8_t CPU::STX()
{
    if (IsLogEnabled()) LogInstructionName("STX");

    return X;
}

// Store Y Register
// This simply returns Y since all memory writes
// are done externally to these functions
uint8_t CPU::STY()
{
    if (IsLogEnabled()) LogInstructionName("STY");

    return Y;
}

// Transfer Accumulator to X
void CPU::TAX()
{
    if (IsLogEnabled()) LogInstructionName("TAX");

    X = A;

    // Set zero flag if X is 0
    (X == 0) ? P = P | 0x02 : P = P & 0xFD;
    // Set negative flag if bit 7 of X is set
    ((X >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;
}

// Transfer Accumulator to Y
void CPU::TAY()
{
    if (IsLogEnabled()) LogInstructionName("TAY");

    Y = A;

    // Set zero flag if Y is 0
    (Y == 0) ? P = P | 0x02 : P = P & 0xFD;
    // Set negative flag if bit 7 of Y is set
    ((Y >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;
}

// Transfer Stack Pointer to X
void CPU::TSX()
{
    if (IsLogEnabled()) LogInstructionName("TSX");

    X = S;

    // Set zero flag if X is 0
    (X == 0) ? P = P | 0x02 : P = P & 0xFD;
    // Set negative flag if bit 7 of X is set
    ((X >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;
}

// Transfer X to Accumulator
void CPU::TXA()
{
    if (IsLogEnabled()) LogInstructionName("TXA");

    A = X;

    // Set zero flag if A is 0
    (A == 0) ? P = P | 0x02 : P = P & 0xFD;
    // Set negative flag if bit 7 of A is set
    ((A >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;
}

// Transfer X to Stack Pointer
void CPU::TXS()
{
    if (IsLogEnabled()) LogInstructionName("TXS");

    S = X;
}

// Transfer Y to Accumulator
void CPU::TYA()
{
    if (IsLogEnabled()) LogInstructionName("TYA");

    A = Y;

    // Set zero flag if A is 0
    (A == 0) ? P = P | 0x02 : P = P & 0xFD;
    // Set negative flag if bit 7 of A is set
    ((A >> 7) == 1) ? P = P | 0x80 : P = P & 0x7F;
}

void CPU::CheckNMI()
{
    if (NmiRaised)
    {
        NmiRaised = false;
        NmiPending = true;
    }

    if (PpuRendevous <= 0 || NmiLineStatus)
    {
        Ppu->Run();

        uint64_t nmiOccuredCycle;
        bool nmiLine = Ppu->CheckNMI(nmiOccuredCycle);

        if (!NmiLineStatus && nmiLine)
        {
            if (Clock - nmiOccuredCycle >= 2)
            {
                NmiPending = true;
            }
            else
            {
                NmiRaised = true;
            }
        }

        NmiLineStatus = nmiLine;
        PpuRendevous = Ppu->ScheduleSync();
    }
}

void CPU::HandleNMI()
{
    uint8_t highPC = static_cast<uint8_t>(PC >> 8);
    uint8_t lowPC = static_cast<uint8_t>(PC & 0xFF);

    Read(PC);
    Read(PC);

    Write(highPC, 0x100 + S);
    Write(lowPC, 0x100 + (S - 1));
    Write(P | 0x20, 0x100 + (S - 2));
    S -= 3;

    P |= 0x4;

    PC = Read(0xFFFA) + (static_cast<uint16_t>(Read(0xFFFB)) * 0x100);
}

void CPU::CheckIRQ()
{
    // If IRQ line is high and interrupt inhibit flag is false
    if (Apu->CheckIRQ() && !(P & 0x4))
    {
        IrqPending = true;
    }
}

void CPU::HandleIRQ()
{
    uint8_t highPC = static_cast<uint8_t>(PC >> 8);
    uint8_t lowPC = static_cast<uint8_t>(PC & 0xFF);

    Read(PC);
    Read(PC);

    Write(highPC, 0x100 + S);
    Write(lowPC, 0x100 + (S - 1));
    Write(P | 0x20, 0x100 + (S - 2));
    S -= 3;

    P |= 0x4;

    PC = Read(0xFFFE) + (static_cast<uint16_t>(Read(0xFFFF)) * 0x100);
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
    , StartupFlag(true)
    , StopFlag(false)
    , PauseFlag(false)
    , Paused(false)
    , LogEnabled(false)
    , EnableLogFlag(false)
    , LogFile(nullptr)
    , ControllerStrobe(0)
    , ControllerOneShift(0)
    , ControllerOneState(0)
    , PpuRendevous(0)
    , NmiLineStatus(false)
    , NmiRaised(false)
    , NmiPending(false)
    , IsStalled(false)
    , IrqPending(false)
    , S(0xFD)
    , P(0x24)
    , A(0)
    , X(0)
    , Y(0)
{
    memset(Memory, 0, sizeof(uint8_t) * 0x800);

    sprintf(Addressing, "%s", "");
    sprintf(Instruction, "%s", "");
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
}

void CPU::SetStalled(bool stalled)
{
    IsStalled = stalled;
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

void CPU::SetControllerOneState(uint8_t state)
{
    ControllerOneState = state;
}

uint8_t CPU::GetControllerOneState()
{
    return ControllerOneState;
}

void CPU::Pause()
{
    if (!Paused)
    {
        PauseFlag = true;
    }
}

void CPU::Resume()
{
    std::unique_lock<std::mutex> lock(PauseMutex);
    PauseCv.notify_all();
	Ppu->ResetFrameCounter();
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

// Currently unimplemented
void CPU::Reset()
{

}

// Run the CPU
void CPU::Run()
{
    if (StartupFlag)
    {
        if (Ppu == nullptr)
        {
            throw std::runtime_error("CPU: No PPU attached");
        }
        else if (Apu == nullptr)
        {
            throw std::runtime_error("CPU: No APU attached");
        }
        else if (Cartridge == nullptr)
        {
            throw std::runtime_error("CPU: No Cartridge attached");
        }
        else
        {
            // Initialize PC to the address found at the reset vector (0xFFFC and 0xFFFD)
            PC = (static_cast<uint16_t>(DebugRead(0xFFFD)) << 8) + DebugRead(0xFFFC);
            StartupFlag = false;
        }
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
                    std::string message = "Failed to open log file: ";
                    message += logName;

                    throw std::runtime_error(message);
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

            PauseCv.wait(lock);
            Paused = false;
            PauseFlag = false;
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
        NmiPending = false;
        HandleNMI();
    }

    if (IrqPending)
    {
        IrqPending = false;
        HandleIRQ();
    }

    if (IsLogEnabled())
    {
        Ppu->Run();

        LogProgramCounter();
        LogRegisters();
    }

    uint8_t opcode = Read(PC++); // Retrieve opcode from memory
    uint16_t addr;
    uint8_t value;

    if (IsLogEnabled()) LogOpcode(opcode);

    // This switch statement executes the instruction associated with each opcode
    // With a few exceptions this involves calling one of the 9 addressing mode functions
    // and passing their result into the instruction function. Depending on the instruction
    // the result may then be written back to the same address. After this procedure is
    // complete then the required cycles for that instruction are added to the cycle counter.
    switch (opcode)
    {
        // ADC OpCodes
    case 0x69:
        ADC(Immediate());
        break;
    case 0x65:
        ADC(Read(ZeroPage()));
        break;
    case 0x75:
        ADC(Read(ZeroPageX()));
        break;
    case 0x6D:
        ADC(Read(Absolute()));
        break;
    case 0x7D:
        ADC(Read(AbsoluteX()));
        break;
    case 0x79:
        ADC(Read(AbsoluteY()));
        break;
    case 0x61:
        ADC(Read(IndexedIndirect()));
        break;
    case 0x71:
        ADC(Read(IndirectIndexed()));
        break;
        // AND OpCodes
    case 0x29:
        AND(Immediate());
        break;
    case 0x25:
        AND(Read(ZeroPage()));
        break;
    case 0x35:
        AND(Read(ZeroPageX()));
        break;
    case 0x2D:
        AND(Read(Absolute()));
        break;
    case 0x3D:
        AND(Read(AbsoluteX()));
        break;
    case 0x39:
        AND(Read(AbsoluteY()));
        break;
    case 0x21:
        AND(Read(IndexedIndirect()));
        break;
    case 0x31:
        AND(Read(IndirectIndexed()));
        break;
        // ASL OpCodes
    case 0x0A:
        A = ASL(Accumulator());
        break;
    case 0x06:
        addr = ZeroPage();
        value = Read(addr);
        Write(value, addr);
        Write(ASL(value), addr);
        break;
    case 0x16:
        addr = ZeroPageX();
        value = Read(addr);
        Write(value, addr);
        Write(ASL(value), addr);
        break;
    case 0x0E:
        addr = Absolute();
        value = Read(addr);
        Write(value, addr);
        Write(ASL(value), addr);
        break;
    case 0x1E:
        addr = AbsoluteX(true);
        value = Read(addr);
        Write(value, addr);
        Write(ASL(value), addr);
        break;
        // BCC OpCode
    case 0x90:
        BCC(Relative());
        break;
        // BCS OpCode
    case 0xB0:
        BCS(Relative());
        break;
        // BEQ OpCode
    case 0xF0:
        BEQ(Relative());
        break;
        // BIT OpCodes
    case 0x24:
        BIT(Read(ZeroPage()));
        break;
    case 0x2C:
        BIT(Read(Absolute()));
        break;
        // BMI OpCode
    case 0x30:
        BMI(Relative());
        break;
        // BNE OpCode
    case 0xD0:
        BNE(Relative());
        break;
        // BPL OpCode
    case 0x10:
        BPL(Relative());
        break;
        // BRK OpCode
    case 0x00:
        Read(PC++);
        BRK();
        break;
        // BVC OpCode
    case 0x50:
        BVC(Relative());
        break;
        // BVS OpCode
    case 0x70:
        BVS(Relative());
        break;
        // CLC OpCode
    case 0x18:
        Read(PC);
        CLC();
        break;
        // CLD OpCode
    case 0xD8:
        Read(PC);
        CLD();
        break;
        // CLI OpCode
    case 0x58:
        Read(PC);
        CLI();
        break;
        // CLV OpCode
    case 0xB8:
        Read(PC);
        CLV();
        break;
        // CMP OpCodes
    case 0xC9:
        CMP(Immediate());
        break;
    case 0xC5:
        CMP(Read(ZeroPage()));
        break;
    case 0xD5:
        CMP(Read(ZeroPageX()));
        break;
    case 0xCD:
        CMP(Read(Absolute()));
        break;
    case 0xDD:
        CMP(Read(AbsoluteX()));
        break;
    case 0xD9:
        CMP(Read(AbsoluteY()));
        break;
    case 0xC1:
        CMP(Read(IndexedIndirect()));
        break;
    case 0xD1:
        CMP(Read(IndirectIndexed()));
        break;
        // CPX OpCodes
    case 0xE0:
        CPX(Immediate());
        break;
    case 0xE4:
        CPX(Read(ZeroPage()));
        break;
    case 0xEC:
        CPX(Read(Absolute()));
        break;
        // CPY OpCodes
    case 0xC0:
        CPY(Immediate());
        break;
    case 0xC4:
        CPY(Read(ZeroPage()));
        break;
    case 0xCC:
        CPY(Read(Absolute()));
        break;
        // DEC OpCodes
    case 0xC6:
        addr = ZeroPage();
        value = Read(addr);
        Write(value, addr);
        Write(DEC(value), addr);
        break;
    case 0xD6:
        addr = ZeroPageX();
        value = Read(addr);
        Write(value, addr);
        Write(DEC(value), addr);
        break;
    case 0xCE:
        addr = Absolute();
        value = Read(addr);
        Write(value, addr);
        Write(DEC(value), addr);
        break;
    case 0xDE:
        addr = AbsoluteX(true);
        value = Read(addr);
        Write(value, addr);
        Write(DEC(value), addr);
        break;
        // DEX Opcode
    case 0xCA:
        Read(PC);
        DEX();
        break;
        // DEX Opcode
    case 0x88:
        Read(PC);
        DEY();
        break;
        // EOR OpCodes
    case 0x49:
        EOR(Immediate());
        break;
    case 0x45:
        EOR(Read(ZeroPage()));
        break;
    case 0x55:
        EOR(Read(ZeroPageX()));
        break;
    case 0x4D:
        EOR(Read(Absolute()));
        break;
    case 0x5D:
        EOR(Read(AbsoluteX()));
        break;
    case 0x59:
        EOR(Read(AbsoluteY()));
        break;
    case 0x41:
        EOR(Read(IndexedIndirect()));
        break;
    case 0x51:
        EOR(Read(IndirectIndexed()));
        break;
        // INC OpCodes
    case 0xE6:
        addr = ZeroPage();
        value = Read(addr);
        Write(value, addr);
        Write(INC(value), addr);
        break;
    case 0xF6:
        addr = ZeroPageX();
        value = Read(addr);
        Write(value, addr);
        Write(INC(value), addr);
        break;
    case 0xEE:
        addr = Absolute();
        value = Read(addr);
        Write(value, addr);
        Write(INC(value), addr);
        break;
    case 0xFE:
        addr = AbsoluteX(true);
        value = Read(addr);
        Write(value, addr);
        Write(INC(value), addr);
        break;
        // INX OpCode
    case 0xE8:
        Read(PC);
        INX();
        break;
        // INY OpCode
    case 0xC8:
        Read(PC);
        INY();
        break;
        // JMP OpCodes
    case 0x4C:
        JMP(Absolute(true));
        break;
    case 0x6C:
        JMP(Indirect());
        break;
        // JSR OpCode
    case 0x20:
        JSR(Absolute(true));
        break;
        // LDA OpCodes
    case 0xA9:
        LDA(Immediate());
        break;
    case 0xA5:
        LDA(Read(ZeroPage()));
        break;
    case 0xB5:
        LDA(Read(ZeroPageX()));
        break;
    case 0xAD:
        LDA(Read(Absolute()));
        break;
    case 0xBD:
        LDA(Read(AbsoluteX()));
        break;
    case 0xB9:
        LDA(Read(AbsoluteY()));
        break;
    case 0xA1:
        LDA(Read(IndexedIndirect()));
        break;
    case 0xB1:
        LDA(Read(IndirectIndexed()));
        break;
        // LDX OpCodes
    case 0xA2:
        LDX(Immediate());
        break;
    case 0xA6:
        LDX(Read(ZeroPage()));
        break;
    case 0xB6:
        LDX(Read(ZeroPageY()));
        break;
    case 0xAE:
        LDX(Read(Absolute()));
        break;
    case 0xBE:
        LDX(Read(AbsoluteY()));
        break;
        // LDY OpCodes
    case 0xA0:
        LDY(Immediate());
        break;
    case 0xA4:
        LDY(Read(ZeroPage()));
        break;
    case 0xB4:
        LDY(Read(ZeroPageX()));
        break;
    case 0xAC:
        LDY(Read(Absolute()));
        break;
    case 0xBC:
        LDY(Read(AbsoluteX()));
        break;
        // LSR OpCodes
    case 0x4A:
        A = LSR(Accumulator());
        break;
    case 0x46:
        addr = ZeroPage();
        value = Read(addr);
        Write(value, addr);
        Write(LSR(value), addr);
        break;
    case 0x56:
        addr = ZeroPageX();
        value = Read(addr);
        Write(value, addr);
        Write(LSR(value), addr);
        break;
    case 0x4E:
        addr = Absolute();
        value = Read(addr);
        Write(value, addr);
        Write(LSR(value), addr);
        break;
    case 0x5E:
        addr = AbsoluteX(true);
        value = Read(addr);
        Write(value, addr);
        Write(LSR(value), addr);
        break;
        // NOP OPCode
    case 0xEA:
        Read(PC);
        NOP();
        break;
        // ORA OpCodes
    case 0x09:
        ORA(Immediate());
        break;
    case 0x05:
        ORA(Read(ZeroPage()));
        break;
    case 0x15:
        ORA(Read(ZeroPageX()));
        break;
    case 0x0D:
        ORA(Read(Absolute()));
        break;
    case 0x1D:
        ORA(Read(AbsoluteX()));
        break;
    case 0x19:
        ORA(Read(AbsoluteY()));
        break;
    case 0x01:
        ORA(Read(IndexedIndirect()));
        break;
    case 0x11:
        ORA(Read(IndirectIndexed()));
        break;
        // PHA OpCode
    case 0x48:
        Read(PC);
        PHA();
        break;
        // PHP OpCode
    case 0x08:
        Read(PC);
        PHP();
        break;
        // PLA OpCode
    case 0x68:
        Read(PC);
        PLA();
        break;
        // PLP OpCode
    case 0x28:
        Read(PC);
        PLP();
        break;
        // ROL OpCodes
    case 0x2A:
        A = ROL(Accumulator());
        break;
    case 0x26:
        addr = ZeroPage();
        value = Read(addr);
        Write(value, addr);
        Write(ROL(value), addr);
        break;
    case 0x36:
        addr = ZeroPageX();
        value = Read(addr);
        Write(value, addr);
        Write(ROL(value), addr);
        break;
    case 0x2E:
        addr = Absolute();
        value = Read(addr);
        Write(value, addr);
        Write(ROL(value), addr);
        break;
    case 0x3E:
        addr = AbsoluteX(true);
        value = Read(addr);
        Write(value, addr);
        Write(ROL(value), addr);
        break;
        // ROR OpCodes
    case 0x6A:
        A = ROR(Accumulator());
        break;
    case 0x66:
        addr = ZeroPage();
        value = Read(addr);
        Write(value, addr);
        Write(ROR(value), addr);
        break;
    case 0x76:
        addr = ZeroPageX();
        value = Read(addr);
        Write(value, addr);
        Write(ROR(value), addr);
        break;
    case 0x6E:
        addr = Absolute();
        value = Read(addr);
        Write(value, addr);
        Write(ROR(value), addr);
        break;
    case 0x7E:
        addr = AbsoluteX(true);
        value = Read(addr);
        Write(value, addr);
        Write(ROR(value), addr);
        break;
        // RTI OpCode
    case 0x40:
        Read(PC);
        RTI();
        break;
        // RTS OpCode
    case 0x60:
        Read(PC);
        RTS();
        break;
        // SBC OpCodes
    case 0xE9:
        SBC(Immediate());
        break;
    case 0xE5:
        SBC(Read(ZeroPage()));
        break;
    case 0xF5:
        SBC(Read(ZeroPageX()));
        break;
    case 0xED:
        SBC(Read(Absolute()));
        break;
    case 0xFD:
        SBC(Read(AbsoluteX()));
        break;
    case 0xF9:
        SBC(Read(AbsoluteY()));
        break;
    case 0xE1:
        SBC(Read(IndexedIndirect()));
        break;
    case 0xF1:
        SBC(Read(IndirectIndexed()));
        break;
        // SEC OpCode
    case 0x38:
        Read(PC);
        SEC();
        break;
        // SED OpCode
    case 0xF8:
        Read(PC);
        SED();
        break;
        // SEI OpCode
    case 0x78:
        Read(PC);
        SEI();
        break;
        // STA OpCodes
    case 0x85:
        Write(STA(), ZeroPage());
        break;
    case 0x95:
        Write(STA(), ZeroPageX());
        break;
    case 0x8D:
        Write(STA(), Absolute());
        break;
    case 0x9D:
        Write(STA(), AbsoluteX(true));
        break;
    case 0x99:
        Write(STA(), AbsoluteY(true));
        break;
    case 0x81:
        Write(STA(), IndexedIndirect());
        break;
    case 0x91:
        Write(STA(), IndirectIndexed(true));
        break;
        // STX OpCodes
    case 0x86:
        Write(STX(), ZeroPage());
        break;
    case 0x96:
        Write(STX(), ZeroPageY());
        break;
    case 0x8E:
        Write(STX(), Absolute());
        break;
        // STY OpCodes
    case 0x84:
        Write(STY(), ZeroPage());
        break;
    case 0x94:
        Write(STY(), ZeroPageX());
        break;
    case 0x8C:
        Write(STY(), Absolute());
        break;
        // TAX OpCode
    case 0xAA:
        Read(PC);
        TAX();
        break;
        // TAY OpCode
    case 0xA8:
        Read(PC);
        TAY();
        break;
        // TSX OpCode
    case 0xBA:
        Read(PC);
        TSX();
        break;
        // TXA OpCode
    case 0x8A:
        Read(PC);
        TXA();
        break;
        // TXS OpCode
    case 0x9A:
        Read(PC);
        TXS();
        break;
        // TYA OpCode
    case 0x98:
        Read(PC);
        TYA();
        break;
    default: // Otherwise illegal OpCode
        throw std::runtime_error("CPU tried to execute invalid opcode!");
    }

    if (IsLogEnabled()) PrintLog();
}

void CPU::LogProgramCounter()
{
    sprintf(ProgramCounter, "%04X", PC);
}

void CPU::LogRegisters()
{
    int32_t scanline = Ppu->GetCurrentScanline();

    if (scanline == 261)
    {
        scanline = -1;
    }

    sprintf(Registers, "A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%3u SL:%d", A, X, Y, P, S, Ppu->GetCurrentDot(), scanline);
}

void CPU::LogOpcode(uint8_t opcode)
{
    sprintf(OpCode, "%02X", opcode);
}

void CPU::LogInstructionName(std::string name)
{
    sprintf(Instruction, "%s", name.c_str());
}

void CPU::LogAccumulator()
{
    sprintf(Addressing, "A");
}

void CPU::LogRelative(uint8_t value)
{

    sprintf(Addressing, "$%04X", PC + static_cast<int8_t>(value));
    sprintf(AddressingArg1, "%02X", static_cast<uint8_t>(value));
}

void CPU::LogImmediate(uint8_t arg1)
{
    sprintf(Addressing, "#$%02X", arg1);
    sprintf(AddressingArg1, "%02X", arg1);
}

void CPU::LogZeroPage(uint8_t address)
{
    sprintf(Addressing, "$%02X = %02X", address, DebugRead(address));
    sprintf(AddressingArg1, "%02X", address);
}


void CPU::LogZeroPageX(uint8_t initialAddress, uint8_t finalAddress)
{
    sprintf(Addressing, "$%02X,X @ %02X = %02X", initialAddress, finalAddress, DebugRead(finalAddress));
    sprintf(AddressingArg1, "%02X", initialAddress);
}

void CPU::LogZeroPageY(uint8_t initialAddress, uint8_t finalAddress)
{
    sprintf(Addressing, "$%02X,Y @ %02X = %02X", initialAddress, finalAddress, DebugRead(finalAddress));
    sprintf(AddressingArg1, "%02X", initialAddress);
}

void CPU::LogAbsolute(uint8_t lowByte, uint8_t highByte, uint16_t address, bool isJump)
{
    if (!isJump)
    {
        sprintf(Addressing, "$%04X = %02X", address, DebugRead(address));
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
    sprintf(Addressing, "$%04X,X @ %04X = %02X", initialAddress, finalAddress, DebugRead(finalAddress));
    sprintf(AddressingArg1, "%02X", lowByte);
    sprintf(AddressingArg2, "%02X", highByte);
}

void CPU::LogAbsoluteY(uint8_t lowByte, uint8_t highByte, uint16_t initialAddress, uint16_t finalAddress)
{
    sprintf(Addressing, "$%04X,Y @ %04X = %02X", initialAddress, finalAddress, DebugRead(finalAddress));
    sprintf(AddressingArg1, "%02X", lowByte);
    sprintf(AddressingArg2, "%02X", highByte);
}

void CPU::LogIndirect(uint8_t lowIndirect, uint8_t highIndirect, uint16_t indirect, uint16_t address)
{
    sprintf(Addressing, "($%04X) = %04X", indirect, address);
    sprintf(AddressingArg1, "%02X", lowIndirect);
    sprintf(AddressingArg2, "%02X", highIndirect);
}

void CPU::LogIndexedIndirect(uint8_t pointer, uint8_t lowIndirect, uint16_t address)
{
    sprintf(Addressing, "($%02X,X) @ %02X = %04X = %02X", pointer, lowIndirect, address, DebugRead(address));
    sprintf(AddressingArg1, "%02X", pointer);
}

void CPU::LogIndirectIndexed(uint8_t pointer, uint16_t initialAddress, uint16_t finalAddress)
{
    sprintf(Addressing, "($%02X),Y = %04X @ %04X = %02X", pointer, initialAddress, finalAddress, DebugRead(finalAddress));
    sprintf(AddressingArg1, "%02X", pointer);
}

void CPU::PrintLog()
{
    using namespace std;

    if (LogFile != nullptr)
    {
        fprintf(LogFile, "%-6s%-3s%-3s%-4s%-4s%-28s%s\n", ProgramCounter, OpCode, AddressingArg1, AddressingArg2, Instruction, Addressing, Registers);
    }

    sprintf(Registers, "%s", "");
    sprintf(Addressing, "%s", "");
    sprintf(Instruction, "%s", "");
    sprintf(ProgramCounter, "%s", "");
    sprintf(AddressingArg1, "%s", "");
    sprintf(AddressingArg1, "%s", "");
}
