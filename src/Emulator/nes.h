/*
 * nes.h
 *
 *  Created on: Aug 8, 2014
 *      Author: Dale
 */

#pragma once

#include <mutex>
#include <string>
#include <thread>
#include <iostream>
#include <functional>

#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "cart.h"

struct NesParams
{
    std::string RomPath;
    std::string SavePath;
    bool CpuLogEnabled;

    // PPU Settings
    bool FrameLimitEnabled;
    bool NtscDecoderEnabled;

    // APU Settings
    bool SoundMuted;
    bool FiltersEnabled;
    float MasterVolume;
    float PulseOneVolume;
    float PulseTwoVolume;
    float TriangleVolume;
    float NoiseVolume;
    float DmcVolume;

    NesParams()
        : RomPath("")
        , SavePath("")
        , CpuLogEnabled(false)
        , FrameLimitEnabled(true)
        , NtscDecoderEnabled(false)
        , SoundMuted(false)
        , FiltersEnabled(false)
        , MasterVolume(1.0f)
        , PulseOneVolume(1.0f)
        , PulseTwoVolume(1.0f)
        , TriangleVolume(1.0f)
        , NoiseVolume(1.0f)
        , DmcVolume(1.0f)
    {}
};

class NES
{
public:
    NES(const NesParams& params);
    ~NES();

    const std::string& GetGameName();

    void BindFrameCompleteCallback(void(*Fn)(uint8_t*))
    {
        Ppu->BindFrameCompleteCallback(Fn);
    }

    template<class T>
    void BindFrameCompleteCallback(void(T::*Fn)(uint8_t*), T* Obj)
    {
        Ppu->BindFrameCompleteCallback(Fn, Obj);
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

    void CpuSetLogEnabled(bool enabled);

    void GetNameTable(int table, uint8_t* pixels);
    void GetPatternTable(int table, int palette, uint8_t* pixels);
    void GetPalette(int palette, uint8_t* pixels);
    void GetPrimarySprite(int sprite, uint8_t* pixels);

    void PpuSetFrameLimitEnabled(bool enabled);
    void PpuSetNtscDecoderEnabled(bool enabled);

    void ApuSetMuted(bool muted);
    void ApuSetFiltersEnabled(bool enabled);
    void ApuSetMasterVolume(float volume);
    void ApuSetPulseOneVolume(float volume);
    float ApuGetPulseOneVolume();
    void ApuSetPulseTwoVolume(float volume);
    float ApuGetPulseTwoVolume();
    void ApuSetTriangleVolume(float volume);
    float ApuGetTriangleVolume();
    void ApuSetNoiseVolume(float volume);
    float ApuGetNoiseVolume();
    void ApuSetDmcVolume(float volume);
    float ApuGetDmcVolume();

    // Launch the emulator on a new thread.
    // This function returns immediately.
    void Start();

    // Instructs the emulator to stop and then blocks until it does.
    // Once this function returns this object may be safely deleted.
    void Stop();

    void Resume();
    void Pause();
    void Reset();

private:
    // Main run function, launched in a new thread by NES::Start
    void Run();

    std::thread NesThread;

    APU* Apu;
    CPU* Cpu;
    PPU* Ppu;
    Cart* Cartridge;

    std::function<void(std::string)> OnError;
};
