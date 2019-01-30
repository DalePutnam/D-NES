/*
 * nes.h
 *
 *  Created on: Aug 8, 2014
 *      Author: Dale
 */

#pragma once

#include <atomic>
#include <string>
#include <thread>
#include <iostream>
#include <functional>

#include "nes_callback.h"

class CPU;
class APU;
class PPU;
class Cart;
class VideoBackend;
class AudioBackend;

class NES
{
public:
    NES(const std::string& gamePath, const std::string& nativeSavePath = "",
            void* windowHandle = nullptr, NESCallback* callback = nullptr);
    ~NES();

    enum State
    {
        Ready,
        Running,
        Paused,
        Stopped,
        Error
    };

    const std::string& GetGameName();

    State GetState();

    void SetControllerOneState(uint8_t state);
    uint8_t GetControllerOneState();

    void SetCpuLogEnabled(bool enabled);
    void SetNativeSaveDirectory(const std::string& saveDir);
    void SetStateSaveDirectory(const std::string& saveDir);

    void SetTargetFrameRate(uint32_t rate);
    void SetTurboModeEnabled(bool enabled);

    int GetFrameRate();
    void GetNameTable(int table, uint8_t* pixels);
    void GetPatternTable(int table, int palette, uint8_t* pixels);
    void GetPalette(int palette, uint8_t* pixels);
    void GetSprite(int sprite, uint8_t* pixels);
    void SetNtscDecoderEnabled(bool enabled);
    void SetFpsDisplayEnabled(bool enabled);
    void SetOverscanEnabled(bool enabled);

    void ShowMessage(const std::string& message, uint32_t duration);

    void SetAudioEnabled(bool enabled);
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

    void SaveState(int slot);
    void LoadState(int slot);

private:
    // Main run function, launched in a new thread by NES::Start
    void Run();

    std::atomic<State> CurrentState;

    std::thread NesThread;
    std::string StateSaveDirectory;

    APU* Apu;
    CPU* Cpu;
    PPU* Ppu;
    Cart* Cartridge;
    VideoBackend* VideoOut;
    AudioBackend* AudioOut;

    NESCallback* Callback;
};
