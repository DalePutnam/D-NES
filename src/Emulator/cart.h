/*
 * mapper.h
 *
 *  Created on: Jul 8, 2014
 *      Author: Dale
 */

#pragma once

#include <string>

#include "state_save.h"

class CPU;
class PPU;
class MapperBase;

class Cart
{
public:
    enum MirrorMode
    {
        HORIZONTAL,
        VERTICAL,
        SINGLE_SCREEN_A,
        SINGLE_SCREEN_B
    };

    Cart(const std::string& fileName, const std::string& saveDir);
    ~Cart();

    const std::string& GetGameName();

    void SetSaveDirectory(const std::string& saveDir);
    void AttachCPU(CPU* cpu);
    void AttachPPU(PPU* ppu);
    MirrorMode GetMirrorMode();

    uint8_t PrgRead(uint16_t address);
    void PrgWrite(uint8_t M, uint16_t address);

    uint8_t ChrRead(uint16_t address);
    void ChrWrite(uint8_t M, uint16_t address);

    StateSave::Ptr SaveState();
    void LoadState(const StateSave::Ptr& state);

private:
    MapperBase* Mapper;
};
