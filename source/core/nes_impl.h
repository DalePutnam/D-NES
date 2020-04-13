#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <iostream>
#include <chrono>
#include <ctime>

#include <dnes/dnes.h>
#include <spdlog/spdlog.h>

class CPU;
class PPU;
class APU;
class Cart;
class VideoBackend;
class AudioBackend;

class NESImpl : public dnes::INES
{
public:
    // External interface

    int LoadGame(const char* path) override;

    int SetWindowHandle(void* handle) override;

    int SetCallback(dnes::INESCallback* callback) override;

    void SetLogLevel(dnes::LogLevel level) override;
    void SetLogPattern(const char* pattern) override;
    int SetLogFile(const char* file) override;
    int SetLogCallback(dnes::INESLogCallback* callback) override;

    const char* GetGameName() override;

    State GetState() override;

    void SetControllerOneState(uint8_t state) override;
    uint8_t GetControllerOneState() override;

    int SetCpuLogEnabled(bool enabled) override;
    void SetNativeSaveDirectory(const char* saveDir) override;
    void SetStateSaveDirectory(const char* saveDir) override;

    void SetTargetFrameRate(uint32_t rate) override;
    void SetTurboModeEnabled(bool enabled) override;

    int GetFrameRate() override;
    void GetNameTable(int table, uint8_t* pixels) override;
    void GetPatternTable(int table, int palette, uint8_t* pixels) override;
    void GetPalette(int palette, uint8_t* pixels) override;
    void GetSprite(int sprite, uint8_t* pixels) override;
    void SetNtscDecoderEnabled(bool enabled) override;
    void SetFpsDisplayEnabled(bool enabled) override;
    void SetOverscanEnabled(bool enabled) override;

    void ShowMessage(const char* message, uint32_t duration) override;

    void SetAudioEnabled(bool enabled) override;
    void SetMasterVolume(float volume) override;

    void SetPulseOneVolume(float volume) override;
    float GetPulseOneVolume() override;
    void SetPulseTwoVolume(float volume) override;
    float GetPulseTwoVolume() override;
    void SetTriangleVolume(float volume) override;
    float GetTriangleVolume() override;
    void SetNoiseVolume(float volume) override;
    float GetNoiseVolume() override;
    void SetDmcVolume(float volume) override;
    float GetDmcVolume() override;

    int Start() override;
    int Stop() override;
    void Resume() override;
    void Pause() override;

    int Reset() override;

    int SaveState(int slot) override;
    int LoadState(int slot) override;

    int GetCurrentErrorCode() override;

public:
    // Internal interface

    NESImpl();
    ~NESImpl();

    spdlog::logger* GetLogger()
    {
        return Logger.get();
    }

private:
    // Main run function, launched in a new thread by NES::Start
    void Run();

    void SetErrorCode(int code);

    std::atomic<State> CurrentState{State::READY};

    std::atomic<bool> StopRequested{false};
    std::atomic<bool> PauseRequested{false};

    std::mutex ControlMutex;
    std::condition_variable ControlCv;

    std::thread NesThread;
    std::string StateSaveDirectory;
    std::atomic<int> CurrentErrorCode;

    std::unique_ptr<APU> Apu;
    std::unique_ptr<CPU> Cpu;
    std::unique_ptr<PPU> Ppu;
    std::unique_ptr<Cart> Cartridge;
    std::unique_ptr<VideoBackend> VideoOut;
    std::unique_ptr<AudioBackend> AudioOut;

    void* WindowHandle{nullptr};

    dnes::INESCallback* Callback{nullptr};

    std::atomic<bool> ShowFps{false};
    std::atomic<bool> OverscanEnabled{true};
    std::atomic<uint32_t> TargetFrameRate{60};

    std::string NativeSaveDirectory;

    std::shared_ptr<spdlog::logger> Logger;
    dnes::LogLevel LogLevel{dnes::LogLevel::ERROR};
    std::string LogPattern;

    std::unique_ptr<dnes::INESLogCallback> LogCallback{};
};