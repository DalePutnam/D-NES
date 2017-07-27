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

struct NesParams
{
    std::string RomPath;
    std::string SavePath;
    bool CpuLogEnabled;

	bool TurboModeEnabled;
    // PPU Settings
    bool FrameLimitEnabled;
    bool NtscDecoderEnabled;

    // APU Settings
    bool AudioEnabled;
    bool FiltersEnabled;
    float MasterVolume;
    float PulseOneVolume;
    float PulseTwoVolume;
    float TriangleVolume;
    float NoiseVolume;
    float DmcVolume;

    void* WindowHandle;

    NesParams()
        : RomPath("")
        , SavePath("")
        , CpuLogEnabled(false)
        , FrameLimitEnabled(true)
        , NtscDecoderEnabled(false)
        , AudioEnabled(true)
        , FiltersEnabled(false)
        , MasterVolume(1.0f)
        , PulseOneVolume(1.0f)
        , PulseTwoVolume(1.0f)
        , TriangleVolume(1.0f)
        , NoiseVolume(1.0f)
        , DmcVolume(1.0f)
    {}
};

class CPU;
class APU;
class PPU;
class Cart;
class VideoBackend;

class NES
{
public:
    NES(const NesParams& params);
    ~NES();

    const std::string& GetGameName();

	void BindFrameCompleteCallback(const std::function<void(uint8_t*)>& fn);
	void BindErrorCallback(const std::function<void(std::string)>& fn);

    template<class T>
    void BindFrameCompleteCallback(void(T::*fn)(uint8_t*), T* obj)
    {
        BindFrameCompleteCallback(std::bind(fn, obj, std::placeholders::_1));
    }

    template<class T>
    void BindErrorCallback(void(T::*fn)(std::string), T* obj)
    {
        BindErrorCallback(std::bind(fn, obj, std::placeholders::_1));
    }

    bool IsStopped();
    bool IsPaused();

    void SetControllerOneState(uint8_t state);
    uint8_t GetControllerOneState();

    void SetCpuLogEnabled(bool enabled);
    void SetNativeSaveDirectory(const std::string& saveDir);

	void SetTurboModeEnabled(bool enabled);

    int GetFrameRate();
    void GetNameTable(int table, uint8_t* pixels);
    void GetPatternTable(int table, int palette, uint8_t* pixels);
    void GetPalette(int palette, uint8_t* pixels);
    void GetPrimarySprite(int sprite, uint8_t* pixels);
    void SetFrameLimitEnabled(bool enabled);
    void SetNtscDecoderEnabled(bool enabled);

    void SetAudioEnabled(bool enabled);
    void SetAudioFiltersEnabled(bool enabled);
    void SetMasterVolume(float volume);

    void SetPulseOneVolume(float volume);
    float GetPulseOneVolume();
    void SetPulseTwoVolume(float volume);
    float GetPulseTwoVolume();
    void SetTriangleVolume(float volume);
    float GetTriangleVolume();
    void SetNoiseVolume(float volume);
    float GetNoiseVolume();
    void SetDmcVolume(float volume);
    float GetDmcVolume();

    // Launch the emulator on a new thread.
    // This function returns immediately.
    void Start();

    // Instructs the emulator to stop and then blocks until it does.
    // Once this function returns this object may be safely deleted.
    void Stop();

    void Resume();
    void Pause();
    void Reset();

    void SaveState(int slot, const std::string& savePath);
    void LoadState(int slot, const std::string& savePath);

private:
    // Main run function, launched in a new thread by NES::Start
    void Run();

    std::thread NesThread;

    APU* Apu;
    CPU* Cpu;
    PPU* Ppu;
    Cart* Cartridge;
    VideoBackend* VB;

    std::function<void(std::string)> OnError;
};
