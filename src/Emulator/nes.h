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
#include <thread>
#include <iostream>
#include <functional>

#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "mappers/cart.h"

struct NesParams
{
    std::string RomPath;
    bool CpuLogEnabled;

    // PPU Settings
    bool FrameLimitEnabled;
    bool NtscDecoderEnabled;

    // APU Settings
    bool SoundMuted;
    bool FiltersEnabled;

    NesParams() :
        RomPath(""),
        CpuLogEnabled(false),
        FrameLimitEnabled(true),
        NtscDecoderEnabled(false),
        SoundMuted(false),
        FiltersEnabled(false)
    {}
};

class NES
{
    bool stop;
    bool pause;
    bool nmi;

    std::thread nesThread;
    std::string gameName;

    APU& apu;
    CPU& cpu;
    PPU& ppu;
    Cart& cart;

    std::mutex stopMutex;
    std::mutex pauseMutex;

    std::function<void(std::string)> OnError;

    // Main run function, launched in a new thread by NES::Start
    void Run();

public:
    NES(const NesParams& params);
    ~NES();

    std::string& GetGameName();

    void BindFrameCompleteCallback(void(*Fn)(uint8_t*))
    {
        ppu.BindFrameCompleteCallback(Fn);
    }

    template<class T>
    void BindFrameCompleteCallback(void(T::*Fn)(uint8_t*), T* Obj)
    {
        ppu.BindFrameCompleteCallback(Fn, Obj);
    }

    void BindErrorCallback(void(*Fn)(std::string))
    {
        OnError = Fn;
    }

    template<class T>
    void BindErrorCallback(void(T::*Fn)(std::string), T* Obj)
    {
        OnError = std::bind(Fn, Obj, std::placeholders::_1);
    }

    bool IsStopped();
    bool IsPaused();

    void SetControllerOneState(uint8_t state);
    uint8_t GetControllerOneState();

    void EnableCPULog();
    void DisableCPULog();

    void GetNameTable(int table, uint8_t* pixels);
    void GetPatternTable(int table, int palette, uint8_t* pixels);
    void GetPalette(int palette, uint8_t* pixels);
    void GetPrimarySprite(int sprite, uint8_t* pixels);

    void PpuSetFrameLimitEnabled(bool enabled);
    void PpuSetNtscDecoderEnabled(bool enabled);

    void ApuSetMuted(bool muted);
    void ApuSetFiltersEnabled(bool enabled);

    // Launch the emulator on a new thread.
    // This function returns immediately.
    void Start();

    // Instructs the emulator to stop and then blocks until it does.
    // Once this function returns this object may be safely deleted.
    void Stop();

    void Resume();
    void Pause();
    void Reset();
};

#endif /* NES_H_ */
