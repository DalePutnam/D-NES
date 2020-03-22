#pragma once

#include <atomic>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "nes.h"

class CPU;
class APU;
class PPU;
class Cart;
class VideoBackend;
class AudioBackend;
class NesException;

class NESImpl : public NES
{
public:
    NESImpl() = default;
    ~NESImpl() = default;

    bool Initialize(const char* path, void* handle = nullptr);

    void SetCallback(NESCallback* callback);

    const char* GetGameName();

    State GetState();

    void SetControllerOneState(uint8_t state);
    uint8_t GetControllerOneState();

    void SetCpuLogEnabled(bool enabled);
    void SetNativeSaveDirectory(const char* saveDir);
    void SetStateSaveDirectory(const char* saveDir);

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

    void ShowMessage(const char* message, uint32_t duration);

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
    bool Start();

    // Instructs the emulator to stop and then blocks until it does.
    // Once this function returns this object may be safely deleted.
    bool Stop();

    void Resume();
    void Pause();
    bool Reset();

    const char* SaveState(int slot);
    const char* LoadState(int slot);

    const char* GetErrorMessage();

private:
    // Main run function, launched in a new thread by NES::Start
    void Run();

    void SetError(NesException& ex);
    void SetError(const std::string& component, const std::string& message);

    std::atomic<State> CurrentState{State::Created};

    std::atomic<bool> StopRequested{false};
    std::atomic<bool> PauseRequested{false};

    std::mutex ControlMutex;
    std::condition_variable ControlCv;

    std::thread NesThread;
    std::string StateSaveDirectory;
    std::string ErrorMessage;

    std::unique_ptr<APU> Apu;
    std::unique_ptr<CPU> Cpu;
    std::unique_ptr<PPU> Ppu;
    std::unique_ptr<Cart> Cartridge;
    std::unique_ptr<VideoBackend> VideoOut;
    std::unique_ptr<AudioBackend> AudioOut;

    NESCallback* Callback{nullptr};
};
