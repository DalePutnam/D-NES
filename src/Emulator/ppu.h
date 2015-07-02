/*
 * ppu.h
 *
 *  Created on: Aug 7, 2014
 *      Author: Dale
 */

#ifndef PPU_H_
#define PPU_H_

#include <queue>
//#include <chrono>
#include <ctime>
#include <boost\chrono\chrono.hpp>

#include "Interfaces/idisplay.h"
#include "mappers/cart.h"

class NES;

class PPU
{
    NES& nes;
    Cart& cart;
    IDisplay& display;

    static const unsigned int rgbLookupTable[64];

    boost::chrono::high_resolution_clock::time_point intervalStart;

    unsigned int ppuClock;
    short int dot;
    short int line;
    bool even;
    bool reset;

    // Main Registers

    // Controller Register Fields
    bool ppuAddressIncrement;
    unsigned short int baseSpriteTableAddress;
    unsigned short int baseBackgroundTableAddress;
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
    unsigned char lowerBits;
    bool spriteOverflow;
    bool sprite0Hit;
    bool inVBLANK;
    bool nmiOccured;
    int sprite0SecondaryOAM;

    unsigned char oamAddress;
    unsigned short int ppuAddress;
    unsigned short int ppuTempAddress;
    unsigned char fineXScroll;

    bool addressLatch;

    // Main Register Buffers
    std::queue<std::pair<unsigned int, unsigned char>> ctrlBuffer;
    std::queue<std::pair<unsigned int, unsigned char>> maskBuffer;
    std::queue<std::pair<unsigned int, unsigned char>> scrollBuffer;
    std::queue<std::pair<unsigned int, unsigned char>> oamAddrBuffer;
    std::queue<std::pair<unsigned int, unsigned char>> ppuAddrBuffer;
    std::queue<std::pair<unsigned int, unsigned char>> oamDataBuffer;
    std::queue<std::pair<unsigned int, unsigned char>> ppuDataBuffer;

    // PPU Data Read Buffer
    unsigned char dataBuffer;

    // Primary Object Attribute Memory (OAM)
    unsigned char primaryOAM[0x100];

    // Secondary Object Attribute Memory (OAM)
    unsigned char secondaryOAM[0x20];

    // Name Table RAM
    unsigned char nameTable0[0x400];
    unsigned char nameTable1[0x400];

    // Palette RAM
    unsigned char palettes[0x20];

    // Temporary Values
    unsigned char nameTableByte;
    unsigned char attributeByte;
    unsigned char tileBitmapLow;
    unsigned char tileBitmapHigh;

    // Shift Registers, Latches and Counters
    unsigned short int backgroundShift0;
    unsigned short int backgroundShift1;
    unsigned char backgroundAttributeShift0;
    unsigned char backgroundAttributeShift1;
    unsigned char backgroundAttribute;

    unsigned char spriteCount;
    unsigned char glitchCount;
    unsigned char spriteShift0[8];
    unsigned char spriteShift1[8];
    unsigned char spriteAttribute[8];
    unsigned char spriteCounter[8];

    void Tick();

    void UpdateState();
    void SpriteEvaluation();
    void Render();

    void IncrementXScroll();
    void IncrementYScroll();
    void IncrementClock();

    unsigned char Read(unsigned short int address);
    void Write(unsigned short int address, unsigned char value);
    unsigned char ReadNameTable(unsigned short int address);
    void WriteNameTable(unsigned short int address, unsigned char value);

public:
    PPU(NES& nes, Cart& cart, IDisplay& display);

    unsigned char ReadPPUStatus();
    unsigned char ReadOAMData();
    unsigned char ReadPPUData();

    void WritePPUCTRL(unsigned char M);
    void WritePPUMASK(unsigned char M);
    void WriteOAMADDR(unsigned char M);
    void WriteOAMDATA(unsigned char M);
    void WritePPUSCROLL(unsigned char M);
    void WritePPUADDR(unsigned char M);
    void WritePPUDATA(unsigned char M);

    int ScheduleSync();
    void Sync();
    void GetNameTable(int table, unsigned char* pixels);
    void GetPatternTable(int table, int palette, unsigned char* pixels);
    void GetPalette(int palette, unsigned char* pixels);
    void GetPrimaryOAM(int sprite, unsigned char* pixels);
    void GetSecondaryOAM(int sprite, unsigned char* pixels);

    ~PPU();
};

#endif /* PPU_H_ */
