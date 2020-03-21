/*
 * ppu.h
 *
 *  Created on: Aug 7, 2014
 *      Author: Dale
 */

#pragma once

#include <cstdint>

#include "nes.h"
#include "state_save.h"

class CPU;
class Cart;
class VideoBackend;

struct PPUState;

class PPU
{
public:
    PPU(VideoBackend* vb, NESCallback* callback);
    ~PPU();

    void AttachCPU(CPU* cpu);
    void AttachCart(Cart* cart);

    int32_t GetCurrentDot();
    int32_t GetCurrentScanline();
    uint64_t GetClock();

    void Step();
    bool GetNMIActive();
    
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
    Cart* Cartridge;
    VideoBackend* VideoOut;
    NESCallback* Callback;

    PPUState* ppuState;
};
