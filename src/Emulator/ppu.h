/*
 * ppu.h
 *
 *  Created on: Aug 7, 2014
 *      Author: Dale
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>

#include "cart.h"
#include "nes_callback.h"

class CPU;
class APU;
class VideoBackend;

class PPU
{
public:
    PPU(VideoBackend* vb, NESCallback* callback);
    ~PPU();

    void AttachCPU(CPU* cpu);
    void AttachAPU(APU* apu);
    void AttachCart(Cart* cart);

    uint16_t GetCurrentDot();
    uint16_t GetCurrentScanline();

    void Run();
    void Step();
    bool CheckNMI(uint64_t& occuredCycle);

    void SetTurboModeEnabled(bool enabled);
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

    int GetFrameRate();
    void GetNameTable(int table, uint8_t* pixels);
    void GetPatternTable(int table, int palette, uint8_t* pixels);
    void GetPalette(int palette, uint8_t* pixels);
    void GetPrimaryOAM(int sprite, uint8_t* pixels);

    static int STATE_SIZE;
    void SaveState(char* state);
    void LoadState(const char* state);

private:
    CPU* Cpu;
    APU* Apu;
    Cart* Cartridge;
    VideoBackend* VideoOut;
    NESCallback* Callback;

    int FpsCounter;
    std::atomic<int> CurrentFps;
    std::chrono::steady_clock::time_point FrameCountStart;

    std::atomic<bool> RequestTurboMode;
    bool TurboModeEnabled;
    int TurboFrameSkip;

    uint64_t Clock;
    uint16_t Dot;
    uint16_t Line;
    bool Even;
    bool SuppressNmi;
    bool InterruptActive;
    uint64_t NmiOccuredCycle;

    // Main Registers

    // Controller Register Fields
    bool PpuAddressIncrement;
    uint16_t BaseSpriteTableAddress;
    uint16_t BaseBackgroundTableAddress;
    bool SpriteSizeSwitch;
    bool NmiEnabled;

    // Mask Register Fields
    bool GrayScaleEnabled;
    bool ShowBackgroundLeft;
    bool ShowSpritesLeft;
    bool ShowBackground;
    bool ShowSprites;
    bool IntenseRed;
    bool IntenseGreen;
    bool IntenseBlue;

    // Status Register Fields
    uint8_t LowerBits;
    bool SpriteOverflowFlag;
    bool SpriteZeroHitFlag;
    bool NmiOccuredFlag;
    bool SpriteZeroSecondaryOamFlag;

    uint8_t OamAddress;
    uint16_t PpuAddress;
    uint16_t PpuTempAddress;
    uint8_t FineXScroll;

    bool AddressLatch;

    // PPU Data Read Buffer
    uint8_t DataBuffer;

    // Primary Object Attribute Memory (OAM)
    uint8_t PrimaryOam[0x100];

    // Secondary Object Attribute Memory (OAM)
    uint8_t SecondaryOam[0x20];

    // Name Table RAM
    uint8_t NameTable0[0x400];
    uint8_t NameTable1[0x400];

    // Palette RAM
    uint8_t PaletteTable[0x20];

    // Temporary Values
    uint8_t NameTableByte;
    uint8_t AttributeByte;
    uint8_t TileBitmapLow;
    uint8_t TileBitmapHigh;

    // Shift Registers, Latches and Counters
    uint16_t BackgroundShift0;
    uint16_t BackgroundShift1;
    uint16_t BackgroundAttributeShift0;
    uint16_t BackgroundAttributeShift1;
    uint8_t BackgroundAttribute;

    uint8_t SpriteCount;
    uint8_t SpriteShift0[8];
    uint8_t SpriteShift1[8];
    uint8_t SpriteAttribute[8];
    int16_t SpriteCounter[8];

	uint32_t FrameBufferIndex;
	uint32_t FrameBuffer[256 * 240];

    bool NtscMode;
    std::atomic<bool> RequestNtscMode;
    float SignalLevels[256 * 8];

    void RenderNtscPixel(int pixel);
    void RenderNtscLine();

    void SpriteEvaluation();
    void RenderPixel();
    void RenderPixelIdle();
    void DecodePixel(uint16_t colour);

    void MaybeChangeModes();
    void IncrementXScroll();
    void IncrementYScroll();
    void IncrementClock();
    void LoadBackgroundShiftRegisters();
    void NameTableFetch();
    void BackgroundAttributeFetch();
    void BackgroundLowByteFetch();
    void BackgroundHighByteFetch();
    void SpriteAttributeFetch();
    void SpriteXCoordinateFetch();
    void SpriteLowByteFetch();
    void SpriteHighByteFetch();

    uint8_t Read(uint16_t address);
    void Write(uint16_t address, uint8_t value);
    uint8_t ReadNameTable(uint16_t address);
    void WriteNameTable(uint16_t address, uint8_t value);

    void UpdateFrameRate();
    void UpdateFrameSkipCounters();
};
