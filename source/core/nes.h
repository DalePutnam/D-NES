#pragma once

#include <cstdint>
#include <string>
#include <exception>

class NesException : public std::exception
{
public:
    explicit NesException(const std::string& component, const std::string& message);
    const char* what() const noexcept override;

private:
    std::string _message;
};

class NESCallback
{
public:
    virtual void OnFrameComplete() = 0;
    virtual void OnError(std::exception_ptr eptr) = 0;

    virtual ~NESCallback() = default;
};

class NES
{
public:
    enum State
    {
        Ready,
        Running,
        Paused,
        Stopped,
        Error
    };

    static NES* Create(const std::string& gamePath, const std::string& nativeSavePath = "",
                       void* windowHandle = nullptr, NESCallback* callback = nullptr);

    virtual const std::string& GetGameName() = 0;

    virtual State GetState() = 0;

    virtual void SetControllerOneState(uint8_t state) = 0;
    virtual uint8_t GetControllerOneState() = 0;

    virtual void SetCpuLogEnabled(bool enabled) = 0;
    virtual void SetNativeSaveDirectory(const std::string& saveDir) = 0;
    virtual void SetStateSaveDirectory(const std::string& saveDir) = 0;

    void virtual SetTargetFrameRate(uint32_t rate) = 0;
    virtual void SetTurboModeEnabled(bool enabled) = 0;

    virtual int GetFrameRate() = 0;
    virtual void GetNameTable(int table, uint8_t* pixels) = 0;
    virtual void GetPatternTable(int table, int palette, uint8_t* pixels) = 0;
    virtual void GetPalette(int palette, uint8_t* pixels) = 0;
    virtual void GetSprite(int sprite, uint8_t* pixels) = 0;
    virtual void SetNtscDecoderEnabled(bool enabled) = 0;
    virtual void SetFpsDisplayEnabled(bool enabled) = 0;
    virtual void SetOverscanEnabled(bool enabled) = 0;

    virtual void ShowMessage(const std::string& message, uint32_t duration) = 0;

    virtual void SetAudioEnabled(bool enabled) = 0;
    virtual void SetMasterVolume(float volume) = 0;

    virtual void SetPulseOneVolume(float volume) = 0;
    virtual float GetPulseOneVolume() = 0;
    virtual void SetPulseTwoVolume(float volume) = 0;
    virtual float GetPulseTwoVolume() = 0;
    virtual void SetTriangleVolume(float volume) = 0;
    virtual float GetTriangleVolume() = 0;
    virtual void SetNoiseVolume(float volume) = 0;
    virtual float GetNoiseVolume() = 0;
    virtual void SetDmcVolume(float volume) = 0;
    virtual float GetDmcVolume() = 0;

    // Launch the emulator on a new thread.
    // This function returns immediately.
    virtual void Start() = 0;

    // Instructs the emulator to stop and then blocks until it does.
    // Once this function returns this object may be safely deleted.
    virtual void Stop() = 0;

    virtual void Resume() = 0;
    virtual void Pause() = 0;
    virtual void Reset() = 0;

    virtual void SaveState(int slot) = 0;
    virtual void LoadState(int slot) = 0;

    virtual ~NES() = default;
};
