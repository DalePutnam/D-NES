/*
 * mapper.h
 *
 *  Created on: Jul 8, 2014
 *      Author: Dale
 */

#pragma once

#include <string>
#include <memory>

#include "state_save.h"

class CPU;
class PPU;
class MapperBase;

class Cart
{
public:
    Cart(const std::string& fileName);
    ~Cart();

    const std::string& GetGameName();

    void AttachCPU(CPU* cpu);
    void AttachPPU(PPU* ppu);
    
    void SaveNativeSave(const std::string& saveDir);
    void LoadNativeSave(const std::string& saveDir);

    uint8_t CpuRead(uint16_t address);
    void CpuWrite(uint8_t M, uint16_t address);

    void SetPpuAddress(uint16_t address);
    uint8_t PpuRead();
    void PpuWrite(uint8_t M);

    uint8_t PpuPeek(uint16_t address);

    StateSave::Ptr SaveState();
    void LoadState(const StateSave::Ptr& state);

    bool CheckIRQ();

private:
    std::string _gameName;

    std::unique_ptr<MapperBase> _mapper;
};
