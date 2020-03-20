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

#include "nes.h"

#include "cart.h"
#include "state_save.h"

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

    int32_t GetCurrentDot();
    int32_t GetCurrentScanline();
    uint64_t GetClock();

    void Step();
    bool GetNMIActive();

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

    StateSave::Ptr SaveState();
    void LoadState(const StateSave::Ptr& state);

    uint8_t ReadNameTable0(uint16_t address);
    uint8_t ReadNameTable1(uint16_t address);
    void WriteNameTable0(uint8_t M, uint16_t address);
    void WriteNameTable1(uint8_t M, uint16_t address);

private:
    CPU* Cpu;
    APU* Apu;
    Cart* Cartridge;
    VideoBackend* VideoOut;
    NESCallback* Callback;

    uint64_t Clock;
    int32_t Dot;
    int32_t Line;
    bool Even;
    bool SuppressNmi;
    bool InterruptActive;

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
    bool SpriteZeroOnNextLine;
    bool SpriteZeroOnCurrentLine;

    uint16_t OamAddress;
    uint16_t PpuAddress;
    uint16_t PpuTempAddress;
    uint8_t FineXScroll;

    bool AddressLatch;

    bool RenderingEnabled;
    bool RenderStateDelaySlot;

    // PPU Data Read Buffer
    uint8_t DataBuffer;

    // Oam Data Bus
    uint8_t OamData;
    uint8_t SecondaryOamIndex;

    // Object Attribute Memory (OAM)
    uint8_t Oam[0x120];

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

    uint16_t PpuBusAddress;

    uint8_t SpriteEvaluationCopyCycles;
    bool SpriteEvaluationRunning;
    bool SpriteEvaluationSpriteZero;

    void StepSpriteEvaluation();
    void RenderPixel();
    void RenderPixelIdle();
    void DecodePixel(uint16_t colour);

    void IncrementXScroll();
    void IncrementYScroll();
    void IncrementClock();
    void LoadBackgroundShiftRegisters();

    void SetNameTableAddress();
    void DoNameTableFetch();

    void SetBackgroundAttributeAddress();
    void DoBackgroundAttributeFetch();

    void SetBackgroundLowByteAddress();
    void DoBackgroundLowByteFetch();

    void SetBackgroundHighByteAddress();
    void DoBackgroundHighByteFetch();

    void DoSpriteAttributeFetch();
    void DoSpriteXCoordinateFetch();

    void SetSpriteLowByteAddress();
    void DoSpriteLowByteFetch();

    void SetSpriteHighByteAddress();
    void DoSpriteHighByteFetch();

    void SetBusAddress(uint16_t address);
    uint8_t Read();
    void Write(uint8_t value);
    uint8_t ReadPalette(uint16_t address);
    void WritePalette(uint8_t value, uint16_t address);

    uint8_t Peek(uint16_t address);
};
