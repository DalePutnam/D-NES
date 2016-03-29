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
#include <functional>
#include <boost/cstdint.hpp>

#include "cpu.h"
#include "ppu.h"
#include "clock.h"
#include "mappers/cart.h"
#include "Interfaces/idisplay.h"

struct NesParams
{
    std::string RomPath;
    bool CpuLogEnabled;
    bool FrameLimitEnabled;
    std::function<void(uint8_t*)>* FrameCompleteCallback;

    NesParams():
        RomPath(""),
        CpuLogEnabled(false),
        FrameLimitEnabled(false),
        FrameCompleteCallback(nullptr)
    {}
};

class NES
{
    bool stop;
    bool pause;
    bool nmi;

    std::string gameName;

	CPU& cpu;
	PPU& ppu;
    Cart& cart;

    std::mutex stopMutex;
    std::mutex pauseMutex;

public:
    NES(const NesParams& params);
    ~NES();

    std::string& GetGameName();

    bool IsStopped();
    bool IsPaused();

    void SetControllerOneState(uint8_t state);
    uint8_t GetControllerOneState();

	void EnableFrameLimit();
	void DisableFrameLimit();
	
	void EnableCPULog();
    void DisableCPULog();

    void GetNameTable(int table, uint8_t* pixels);
    void GetPatternTable(int table, int palette, uint8_t* pixels);
    void GetPalette(int palette, uint8_t* pixels);
    void GetPrimaryOAM(int sprite, uint8_t* pixels);

    void Run();
    void Stop();
    void Resume();
    void Pause();
    void Reset();
};

#endif /* NES_H_ */
