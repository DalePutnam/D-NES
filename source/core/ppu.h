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
class CPU;
class Cart;

struct PPUState;

class PPU
{
public:
    PPU(NES& nes);

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

    bool EndOfFrame();

private:
    NES& _nes;

    Cart* _cart;
    std::unique_ptr<PPUState> _ppuState;
    std::unique_ptr<uint32_t[]> _frameBuffer;

    bool _frameEnd{false};
};
