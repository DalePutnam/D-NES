#ifndef _DNES_H_
#define _DNES_H_

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

class INES;

constexpr int SUCCESS = 0;

DNES_DLL INES* CreateNES();
DNES_DLL void DestroyNES(INES* nes);

DNES_DLL const char* GetErrorMessageFromCode(int code);

class INESCallback
{
public:
    virtual void OnFrameComplete(INES* nes) = 0;
    virtual void OnError(INES* nes) = 0;

    virtual ~INESCallback() = default;
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

class INESLogCallback
{
public:
    virtual void LogMessage(LogLevel level, const char* message) = 0;

    virtual ~INESLogCallback() = default;
};

class INES
{
public:
    enum class State
    {
        CREATED,
        READY,
        RUNNING,
        PAUSED,
        STOPPED,
        ERROR
    };

    virtual int LoadGame(const char* romFile, const char* saveFile = nullptr) = 0;

    virtual int SetWindowHandle(void* handle) = 0;

    virtual int SetCallback(INESCallback* callback) = 0;

    virtual void SetLogLevel(LogLevel level) = 0;
    virtual void SetLogPattern(const char* pattern) = 0;
    virtual int SetLogFile(const char* file) = 0;
    virtual int SetLogCallback(INESLogCallback* callback) = 0;

    virtual State GetState() = 0;

    virtual void SetControllerOneState(uint8_t state) = 0;
    virtual uint8_t GetControllerOneState() = 0;

    virtual int StartCpuLog(const char* logFile) = 0;
    virtual void StopCpuLog() = 0;

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
    virtual void SetPulseTwoVolume(float volume) = 0;
    virtual void SetTriangleVolume(float volume) = 0;
    virtual void SetNoiseVolume(float volume) = 0;
    virtual void SetDmcVolume(float volume) = 0;

    // Launch the emulator on a new thread.
    // This function returns immediately.
    virtual int Start() = 0;

    // Instructs the emulator to stop and then blocks until it does.
    // Once this function returns this object may be safely deleted.
    virtual int Stop() = 0;

    virtual void Resume() = 0;
    virtual void Pause() = 0;
    virtual int Reset() = 0;

    virtual int SaveState(const char* file) = 0;
    virtual int LoadState(const char* file) = 0;

    virtual int GetCurrentErrorCode() = 0;

protected:
    virtual ~INES() = default;
};

}; // namespace dnes

#endif // _DNES_H_
