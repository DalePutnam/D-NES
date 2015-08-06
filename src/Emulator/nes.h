/*
 * nes.h
 *
 *  Created on: Aug 8, 2014
 *      Author: Dale
 */

#ifndef NES_H_
#define NES_H_

#include <mutex>
#include <string>
#include <iostream>
#include <boost/cstdint.hpp>

#include "cpu.h"
#include "ppu.h"
#include "clock.h"
#include "mappers/cart.h"
#include "Interfaces/idisplay.h"

class NES
{
    Clock masterClock;
    uint32_t clock;
    int scanline;
    bool stop;
    bool pause;
    bool nmi;

    std::string gameName;

    Cart& cart;
    PPU& ppu;
    CPU& cpu;

    std::mutex stopMutex;
    std::mutex pauseMutex;

public:

    NES(std::string filename, IDisplay& display, bool cpuLogEnabled = false);

    uint32_t GetClock();
    uint32_t GetScanline();
    void IncrementClock(int increment);
    void RaiseNMI();
    bool NMIRaised();
    std::string& GetGameName();

    bool IsStopped();
    bool IsPaused();

    void SetControllerOneState(uint8_t state);
    uint8_t GetControllerOneState();

    void EnableCPULog();
    void DisableCPULog();

    void GetNameTable(int table, uint8_t* pixels);
    void GetPatternTable(int table, int palette, uint8_t* pixels);
    void GetPalette(int palette, uint8_t* pixels);
    void GetPrimaryOAM(int sprite, uint8_t* pixels);
    void GetSecondaryOAM(int sprite, uint8_t* pixels);

    void Start();
    void Stop();
    void Resume();
    void Pause();
    void Reset();

    ~NES();
};

#endif /* NES_H_ */
