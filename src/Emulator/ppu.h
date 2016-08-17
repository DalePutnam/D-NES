/*
 * ppu.h
 *
 *  Created on: Aug 7, 2014
 *      Author: Dale
 */

#ifndef PPU_H_
#define PPU_H_

#include <queue>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>

#include "mappers/cart.h"

class NES;
class CPU;

class PPU
{
    NES& nes;
    CPU* cpu;
    Cart* cart;

    uint8_t frameBuffer[256 * 240 * 3];

    enum Register { PPUCTRL, PPUMASK, PPUSTATUS, OAMADDR, OAMDATA, PPUSCROLL, PPUADDR, PPUDATA };

    static const uint32_t rgbLookupTable[64];
    static constexpr uint32_t resetDelay = 88974;

	std::chrono::high_resolution_clock::time_point intervalStart;
    std::atomic<bool> limitTo60FPS;

    uint64_t clock;
    uint16_t dot;
    uint16_t line;
    bool even;
    bool suppressNMI;
    bool interruptActive;
    uint64_t nmiOccuredCycle;

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
    bool sprite0SecondaryOAM;

    uint8_t oamAddress;
    uint16_t ppuAddress;
    uint16_t ppuTempAddress;
    uint8_t fineXScroll;

    bool addressLatch;

    // Register Buffer
    std::queue<std::tuple<uint64_t, uint8_t, Register>> registerBuffer;

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
    uint8_t spriteShift0[8];
    uint8_t spriteShift1[8];
    uint8_t spriteAttribute[8];
    uint8_t spriteCounter[8];

    bool ntscMode;
    std::atomic<bool> requestNtscMode;
    float signalLevels[256 * 8];

    void RenderNtscPixel(int pixel);
    void RenderNtscLine();

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

    std::function<void(uint8_t*)> OnFrameComplete;

public:
    PPU(NES& nes);
    ~PPU();

    void AttachCPU(CPU& cpu);
    void AttachCart(Cart& cart);

    void BindFrameCompleteCallback(void(*Fn)(uint8_t*))
    {
        OnFrameComplete = Fn;
    }

    template<class T>
    void BindFrameCompleteCallback(void(T::*Fn)(uint8_t*), T* Obj)
    {
        OnFrameComplete = std::bind(Fn, Obj, std::placeholders::_1);
    }

    uint16_t GetCurrentDot();
    uint16_t GetCurrentScanline();

    void Run();
    int ScheduleSync();
    bool CheckNMI(uint64_t& occuredCycle);

    void SetFrameLimitEnabled(bool enabled);
    void SetNtscDecodingEnabled(bool enabled);

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

    void GetNameTable(int table, uint8_t* pixels);
    void GetPatternTable(int table, int palette, uint8_t* pixels);
    void GetPalette(int palette, uint8_t* pixels);
    void GetPrimaryOAM(int sprite, uint8_t* pixels);
};

#endif /* PPU_H_ */
