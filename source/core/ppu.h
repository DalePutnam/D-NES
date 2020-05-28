/*
 * ppu.h
 *
 *  Created on: Aug 7, 2014
 *      Author: Dale
 */

#pragma once

#include <cstdint>
#include <memory>

#include "state_save.h"

class NES;
struct PPUState;

class PPU
{
public:
    PPU(NES& nes);
    ~PPU();

    // User facing interface (via the NES object)
    bool EndOfFrame();

    int GetFrameRate();

    void GetNameTable(uint32_t tableIndex, uint8_t* imageBuffer);
    void GetPatternTable(uint32_t tableIndex, uint32_t paletteIndex, uint8_t* imageBuffer);
    void GetPalette(uint32_t paletteIndex, uint8_t* imageBuffer);
    void GetSprite(uint32_t spriteIndex, uint8_t* imageBuffer);

    StateSave::Ptr SaveState();
    void LoadState(const StateSave::Ptr& state);


    // Internal operations interface
    void Step();

    bool GetNMIActive();
    int32_t GetCurrentDot();
    int32_t GetCurrentScanline();
    uint64_t GetClock();
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

private:
    NES& _nes;

    std::unique_ptr<PPUState> _ppuState;
    std::unique_ptr<uint32_t[]> _frameBuffer;

    bool _frameEnd{false};
};
