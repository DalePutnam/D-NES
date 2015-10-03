/*
 * ppu.h
 *
 *  Created on: Aug 7, 2014
 *      Author: Dale
 */

#ifndef PPU_H_
#define PPU_H_

#include <queue>
#include <boost/cstdint.hpp>
#include <boost/chrono/chrono.hpp>

#include "clock.h"
#include "mappers/cart.h"
#include "Interfaces/idisplay.h"


class NES;

class PPU
{
    Clock& clock;
    NES& nes;
    Cart& cart;
    IDisplay& display;

    static const uint32_t rgbLookupTable[64];

    boost::chrono::high_resolution_clock::time_point intervalStart;

    uint32_t ppuClock;
    int16_t dot;
    int16_t line;
    bool even;
    bool reset;

    // Main Registers

    // Controller Register Fields
    bool ppuAddressIncrement;
    uint16_t baseSpriteTableAddress;
    uint16_t baseBackgroundTableAddress;
    bool spriteSize;
    bool nmiEnabled;

    // Mask Register Fields
    bool grayscale;
    bool showBackgroundLeft;
    bool showSpritesLeft;
    bool showBackground;
    bool showSprites;
    bool intenseRed;
    bool intenseGreen;
    bool intenseBlue;

    // Status Register Fields
    uint8_t lowerBits;
    bool spriteOverflow;
    bool sprite0Hit;
    bool inVBLANK;
    bool nmiOccured;
    int sprite0SecondaryOAM;

    uint8_t oamAddress;
    uint16_t ppuAddress;
    uint16_t ppuTempAddress;
    uint8_t fineXScroll;

    bool addressLatch;

    // Main Register Buffers
    std::queue<std::pair<uint64_t, uint8_t>> ctrlBuffer;
    std::queue<std::pair<uint64_t, uint8_t>> maskBuffer;
    std::queue<std::pair<uint64_t, uint8_t>> scrollBuffer;
    std::queue<std::pair<uint64_t, uint8_t>> oamAddrBuffer;
    std::queue<std::pair<uint64_t, uint8_t>> ppuAddrBuffer;
    std::queue<std::pair<uint64_t, uint8_t>> oamDataBuffer;
    std::queue<std::pair<uint64_t, uint8_t>> ppuDataBuffer;

    // PPU Data Read Buffer
    uint8_t dataBuffer;

    // Primary Object Attribute Memory (OAM)
    uint8_t primaryOAM[0x100];

    // Secondary Object Attribute Memory (OAM)
    uint8_t secondaryOAM[0x20];

    // Name Table RAM
    uint8_t nameTable0[0x400];
    uint8_t nameTable1[0x400];

    // Palette RAM
    uint8_t palettes[0x20];

    // Temporary Values
    uint8_t nameTableByte;
    uint8_t attributeByte;
    uint8_t tileBitmapLow;
    uint8_t tileBitmapHigh;

    // Shift Registers, Latches and Counters
    uint16_t backgroundShift0;
    uint16_t backgroundShift1;
    uint8_t backgroundAttributeShift0;
    uint8_t backgroundAttributeShift1;
    uint8_t backgroundAttribute;

    uint8_t spriteCount;
    uint8_t glitchCount;
    uint8_t spriteShift0[8];
    uint8_t spriteShift1[8];
    uint8_t spriteAttribute[8];
    uint8_t spriteCounter[8];

    void Tick();

    void UpdateState();
    void SpriteEvaluation();
    void Render();

    void IncrementXScroll();
    void IncrementYScroll();
    void IncrementClock();

    uint8_t Read(uint16_t address);
    void Write(uint16_t address, uint8_t value);
    uint8_t ReadNameTable(uint16_t address);
    void WriteNameTable(uint16_t address, uint8_t value);

public:
    PPU(Clock& clock, NES& nes, Cart& cart, IDisplay& display);

    uint8_t ReadPPUStatus();
    uint8_t ReadOAMData();
    uint8_t ReadPPUData();

    void WritePPUCTRL(uint8_t M);
    void WritePPUMASK(uint8_t M);
    void WriteOAMADDR(uint8_t M);
    void WriteOAMDATA(uint8_t M);
    void WritePPUSCROLL(uint8_t M);
    void WritePPUADDR(uint8_t M);
    void WritePPUDATA(uint8_t M);

    int ScheduleSync();
    void Sync();
    void GetNameTable(int table, uint8_t* pixels);
    void GetPatternTable(int table, int palette, uint8_t* pixels);
    void GetPalette(int palette, uint8_t* pixels);
    void GetPrimaryOAM(int sprite, uint8_t* pixels);
    void GetSecondaryOAM(int sprite, uint8_t* pixels);

    ~PPU();
};

#endif /* PPU_H_ */
