/*
 * nes.h
 *
 *  Created on: Aug 8, 2014
 *      Author: Dale
 */

#ifndef NES_H_
#define NES_H_

#include <string>
#include <iostream>
#include <mutex>

#include "cpu.h"
#include "Interfaces/idisplay.h"
#include "ppu.h"
#include "mappers/cart.h"

class NES
{
    unsigned int clock;
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

    unsigned int GetClock();
    unsigned int GetScanline();
    void IncrementClock(int increment);
    void RaiseNMI();
    bool NMIRaised();
    std::string& GetGameName();

    bool IsStopped();
    bool IsPaused();

    void EnableCPULog();
    void DisableCPULog();

    void GetNameTable(int table, unsigned char* pixels);
    void GetPatternTable(int table, int palette, unsigned char* pixels);
    void GetPalette(int palette, unsigned char* pixels);
    void GetPrimaryOAM(int sprite, unsigned char* pixels);
    void GetSecondaryOAM(int sprite, unsigned char* pixels);

    void Start();
    void Stop();
    void Resume();
    void Pause();
    void Reset();

    ~NES();
};

#endif /* NES_H_ */
