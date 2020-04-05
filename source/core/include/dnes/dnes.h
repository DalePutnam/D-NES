#pragma once

#include <cstdint>

#ifdef _WIN32
#   ifdef DNES_EXPORT
#       define DNES_DLL __declspec( dllexport )
#   else
#        define DNES_DLL __declspec( dllimport )
#   endif
#elif defined (__GNUC__)
#    define DNES_DLL __attribute__((visibility("default")))
#else
#    define DNES_DLL
#endif

namespace dnes
{

class NES;

constexpr int SUCCESS = 0;

DNES_DLL NES* CreateNES();
DNES_DLL void DestroyNES(NES* nes);

DNES_DLL const char* GetErrorMessageFromCode(int code);

class NESCallback
{
public:
    virtual void OnFrameComplete(NES* nes) = 0;
    virtual void OnError(NES* nes) = 0;

    virtual ~NESCallback() = default;
};

enum class LogLevel
{
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    CRITICAL = 6,
    NONE = 7
};

class NESLogCallback
{
public:
    virtual void LogMessage(LogLevel level, const char* message) = 0;

    virtual ~NESLogCallback() = default;
};

class NES
{
public:
    enum class State
    {
        READY,
        RUNNING,
        PAUSED,
        STOPPED,
        ERROR
    };

    virtual int LoadGame(const char* path) = 0;

    virtual int SetWindowHandle(void* handle) = 0;

    virtual int SetCallback(NESCallback* callback) = 0;

    virtual void SetLogLevel(LogLevel level) = 0;
    virtual void SetLogPattern(const char* pattern) = 0;
    virtual int SetLogFile(const char* file) = 0;
    virtual int SetLogCallback(NESLogCallback* callback) = 0;

    virtual const char* GetGameName() = 0;

    virtual State GetState() = 0;

    virtual void SetControllerOneState(uint8_t state) = 0;
    virtual uint8_t GetControllerOneState() = 0;

    virtual int SetCpuLogEnabled(bool enabled) = 0;
    virtual void SetNativeSaveDirectory(const char* saveDir) = 0;
    virtual void SetStateSaveDirectory(const char* saveDir) = 0;

    virtual void SetTargetFrameRate(uint32_t rate) = 0;
    virtual void SetTurboModeEnabled(bool enabled) = 0;

    virtual int GetFrameRate() = 0;
    virtual void GetNameTable(int table, uint8_t* pixels) = 0;
    virtual void GetPatternTable(int table, int palette, uint8_t* pixels) = 0;
    virtual void GetPalette(int palette, uint8_t* pixels) = 0;
    virtual void GetSprite(int sprite, uint8_t* pixels) = 0;
    virtual void SetNtscDecoderEnabled(bool enabled) = 0;
    virtual void SetFpsDisplayEnabled(bool enabled) = 0;
    virtual void SetOverscanEnabled(bool enabled) = 0;

    virtual void ShowMessage(const char* msg, uint32_t duration) = 0;

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
    virtual int Start() = 0;

    // Instructs the emulator to stop and then blocks until it does.
    // Once this function returns this object may be safely deleted.
    virtual int Stop() = 0;

    virtual void Resume() = 0;
    virtual void Pause() = 0;
    virtual int Reset() = 0;

    virtual int SaveState(int slot) = 0;
    virtual int LoadState(int slot) = 0;

    virtual int GetCurrentErrorCode() = 0;

protected:
    virtual ~NES() = default;
};

}; // namespace dnes
