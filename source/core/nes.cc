#include <fstream>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "nes.h"

#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "cart.h"
#include "audio/audio_backend.h"
#include "video/opengl_backend.h"
#include "video/video_manager.h"
#include "common/error_handling.h"
#include "common/file.h"

namespace
{
constexpr const char* DEFAULT_LOG_PATTERN = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v";

std::shared_ptr<spdlog::logger> createStderrLoggerHelper()
{
    static auto logger = []() -> auto {
        auto logger = spdlog::stderr_color_mt("stderr_log");
        spdlog::drop("stderr_log");

        return logger;
    }();

    return logger;
}

std::shared_ptr<spdlog::logger> createFileLoggerHelper(const std::string& fileName)
{
    auto logger = spdlog::basic_logger_mt("file", fileName, true);
    spdlog::drop("file");

    return logger;
}

std::shared_ptr<spdlog::logger> createCallbackLoggerHelper(dnes::INESLogCallback* callback)
{
    class CallbackSink : public spdlog::sinks::base_sink<std::mutex>
    {
        using base_sink = spdlog::sinks::base_sink<std::mutex>;

    public:
        CallbackSink(dnes::INESLogCallback* callback) : _callback(callback) {}

    protected:
        void sink_it_(const spdlog::details::log_msg& msg) override
        {
            spdlog::memory_buf_t formatted;
            base_sink::formatter_->format(msg, formatted);
            
            _callback->LogMessage(static_cast<dnes::LogLevel>(msg.level), fmt::to_string(formatted).c_str());
        }

        void flush_() override {}

    private:
        dnes::INESLogCallback* _callback;
    };

    auto logger = spdlog::create<CallbackSink>("callback", callback);
    spdlog::drop("callback");

    return logger;
}

}; // anonymous namespace

NES::NES()
    : Logger(createStderrLoggerHelper())
{
    SetLogLevel(dnes::LogLevel::ERROR);
    SetLogPattern(DEFAULT_LOG_PATTERN);

    VideoOut = std::make_unique<VideoManager>();

    Cpu = std::make_unique<CPU>(*this);
    Ppu = std::make_unique<PPU>(*VideoOut);
    Apu = std::make_unique<APU>(/*AudioOut.get()*/);
    Cartridge = std::make_unique<Cart>(*this);

    Apu->AttachCPU(Cpu.get());
    Apu->AttachCart(Cartridge.get());

    Cpu->AttachPPU(Ppu.get());
    Cpu->AttachAPU(Apu.get());
    Cpu->AttachCart(Cartridge.get());

    Ppu->AttachCPU(Cpu.get());
    Ppu->AttachCart(Cartridge.get());

    // CPU Settings
    Cpu->SetLogEnabled(false);

    // PPU Settings

    // APU Settings
    Apu->SetTurboModeEnabled(false);
    Apu->SetAudioEnabled(true);
    Apu->SetMasterVolume(1.f);
    Apu->SetPulseOneVolume(1.f);
    Apu->SetPulseTwoVolume(1.f);
    Apu->SetTriangleVolume(1.f);
    Apu->SetNoiseVolume(1.f);
    Apu->SetDmcVolume(1.f);

    CurrentState = State::READY;
}

NES::~NES()
{
    GetLogger()->flush();
}

int NES::LoadGame(const char* path)
{
    if (CurrentState != State::READY)
    {
        SPDLOG_LOGGER_ERROR(GetLogger(), "NES: Failed to load game, emulation has already started.");
        return ERROR_LOAD_GAME_AFTER_START;
    }

    GameName = file::getNameFromPath(path);

    try
    {
        iNesFile romFile(path);
        return Cartridge->Initialize(&romFile);
    }
    catch (std::string& msg)
    {
        SPDLOG_LOGGER_ERROR(GetLogger(), "iNesFile: {}", msg);
        return ERROR_LOAD_GAME_FAILED;
    }

    return dnes::SUCCESS;
}

int NES::SetWindowHandle(void* handle)
{
    if (CurrentState != State::READY)
    {
        return ERROR_SET_WINDOW_HANDLE_AFTER_START;
    }

    VideoOut->SetBackend(std::make_unique<OpenGLBackend>(*VideoOut, handle));

    return dnes::SUCCESS;
}

int NES::SetCallback(dnes::INESCallback* callback)
{
    if (CurrentState != State::READY)
    {
        return ERROR_SET_CALLBACK_AFTER_START;
    }

    Callback = callback;

    return dnes::SUCCESS;
}

void NES::SetLogLevel(dnes::LogLevel level)
{
    LogLevel = level;
    Logger->set_level(static_cast<spdlog::level::level_enum>(LogLevel));
}

void NES::SetLogPattern(const char* pattern)
{
    LogPattern = pattern;
    Logger->set_pattern(LogPattern);
}

int NES::SetLogFile(const char* file)
{
    if (CurrentState != State::READY)
    {
        return ERROR_SET_LOG_FILE_AFTER_START;
    }

    Logger->flush();

    if (file == nullptr)
    {
        Logger = createStderrLoggerHelper();
        Logger->set_level(static_cast<spdlog::level::level_enum>(LogLevel));
        Logger->set_pattern(LogPattern);

        return dnes::SUCCESS;
    }

    try
    {
        Logger = createFileLoggerHelper(file);
        Logger->set_level(static_cast<spdlog::level::level_enum>(LogLevel));
        Logger->set_pattern(LogPattern);

        return dnes::SUCCESS;
    }
    catch (spdlog::spdlog_ex& ex)
    {
        SPDLOG_LOGGER_ERROR(GetLogger(), ex.what());
        return ERROR_OPEN_LOG_FILE_FAILED; 
    }
}

int NES::SetLogCallback(dnes::INESLogCallback* callback)
{
    if (CurrentState != State::READY)
    {
        return ERROR_SET_LOG_CALLBACK_AFTER_START;
    }

    Logger->flush();

    if (callback == nullptr)
    {
        Logger = createStderrLoggerHelper();
        Logger->set_level(static_cast<spdlog::level::level_enum>(LogLevel));
        Logger->set_pattern(LogPattern);

        return dnes::SUCCESS;
    }

    Logger = createCallbackLoggerHelper(callback);
    Logger->set_level(static_cast<spdlog::level::level_enum>(LogLevel));
    Logger->set_pattern(LogPattern);

    return dnes::SUCCESS;
}

const char* NES::GetGameName()
{
    return GameName.c_str();
}

void NES::SetControllerOneState(uint8_t state)
{
    Cpu->SetControllerOneState(state);
}

uint8_t NES::GetControllerOneState()
{
    return Cpu->GetControllerOneState();
}

int NES::SetCpuLogEnabled(bool enabled)
{
    try
    {
        Pause();
        Cpu->SetLogEnabled(enabled);
        Resume();
    }
    catch (NesException& ex)
    {
        return ERROR_FAILED_TO_OPEN_CPU_LOG_FILE;
    }

    return dnes::SUCCESS;
}

void NES::SetNativeSaveDirectory(const char* saveDir)
{
    NativeSaveDirectory = saveDir;
}

void NES::SetStateSaveDirectory(const char* saveDir)
{
    StateSaveDirectory = saveDir;
}

void NES::SetTargetFrameRate(uint32_t rate)
{
    TargetFrameRate = rate;

    if (AudioOut)
    {
        Apu->SetTargetFrameRate(TargetFrameRate);
    }
}

void NES::SetTurboModeEnabled(bool enabled)
{
    Apu->SetTurboModeEnabled(enabled);
}

int NES::GetFrameRate()
{
    return Ppu->GetFrameRate();
}

void NES::GetNameTable(int table, uint8_t* pixels)
{
    Ppu->GetNameTable(table, pixels);
}

void NES::GetPatternTable(int table, int palette, uint8_t* pixels)
{
    Ppu->GetPatternTable(table, palette, pixels);
}

void NES::GetPalette(int palette, uint8_t* pixels)
{
    Ppu->GetPalette(palette, pixels);
}

void NES::GetSprite(int sprite, uint8_t* pixels)
{
    Ppu->GetPrimaryOAM(sprite, pixels);
}

void NES::SetNtscDecoderEnabled(bool enabled)
{

}

void NES::SetFpsDisplayEnabled(bool enabled)
{
    ShowFps = enabled;

    if (VideoOut)
    {
        VideoOut->SetShowFps(enabled);
    }
}

void NES::SetOverscanEnabled(bool enabled)
{
    OverscanEnabled = enabled;

    if (VideoOut)
    {
        VideoOut->SetOverscanEnabled(enabled);
    }
}

void NES::ShowMessage(const char* message, uint32_t duration)
{
    if (!VideoOut)
    {
        return;
    }

    VideoOut->ShowMessage(message, duration);
}

void NES::SetAudioEnabled(bool enabled)
{
    Apu->SetAudioEnabled(enabled);
}

void NES::SetMasterVolume(float volume)
{
    Apu->SetMasterVolume(volume);
}

void NES::SetPulseOneVolume(float volume)
{
    Apu->SetPulseOneVolume(volume);
}

float NES::GetPulseOneVolume()
{
    return Apu->GetPulseOneVolume();
}

void NES::SetPulseTwoVolume(float volume)
{
    Apu->SetPulseTwoVolume(volume);
}

float NES::GetPulseTwoVolume()
{
    return Apu->GetPulseTwoVolume();
}

void NES::SetTriangleVolume(float volume)
{
    Apu->SetTriangleVolume(volume);
}

float NES::GetTriangleVolume()
{
    return Apu->GetTriangleVolume();
}

void NES::SetNoiseVolume(float volume)
{
    Apu->SetNoiseVolume(volume);
}

float NES::GetNoiseVolume()
{
    return Apu->GetNoiseVolume();
}

void NES::SetDmcVolume(float volume)
{
    Apu->SetDmcVolume(volume);
}

float NES::GetDmcVolume()
{
    return Apu->GetDmcVolume();
}

NES::State NES::GetState()
{
    return CurrentState;
}

int NES::Start()
{
    if (!Cartridge)
    {
        return ERROR_START_WITHOUT_GAME_LOADED;
    }

    if (CurrentState == State::RUNNING || CurrentState == State::PAUSED)
    {
        return ERROR_START_ALREADY_STARTED;
    }

    if (CurrentState == State::STOPPED)
    {
        return ERROR_START_AFTER_STOP;
    }

    if (CurrentState == State::ERROR)
    {
        return ERROR_START_AFTER_ERROR;
    }

    NesThread = std::thread(&NES::Run, this);

    return dnes::SUCCESS;
}

int NES::Stop()
{
    if (CurrentState == State::READY)
    {
        return ERROR_STOP_NOT_STARTED;
    }

    if (CurrentState == State::STOPPED)
    {
        return ERROR_STOP_ALREADY_STOPPED;
    }

    if (CurrentState == State::ERROR)
    {
        return ERROR_STOP_AFTER_ERROR;
    }

    StopRequested = true;
    Resume();

    NesThread.join();

    return dnes::SUCCESS;
}

void NES::Resume()
{
    std::unique_lock<std::mutex> lock(ControlMutex);

    if (CurrentState != State::PAUSED)
    {
        return;
    }

    ControlCv.notify_all();
}

void NES::Pause()
{
    std::unique_lock<std::mutex> lock(ControlMutex);

    if (CurrentState != State::RUNNING)
    {
        return;
    }

    PauseRequested = true;

    ControlCv.wait(lock);
}

int NES::Reset()
{
    return ERROR_UNIMPLEMENTED;
}

int NES::SaveState(int slot)
{
    if (CurrentState != State::RUNNING && CurrentState != State::PAUSED)
    {
        return ERROR_STATE_SAVE_NOT_RUNNING;
    }

    std::string extension = "state" + std::to_string(slot);
    std::string fileName = file::createFullPath(GetGameName(), extension, StateSaveDirectory);
    std::ofstream saveStream(fileName.c_str(), std::ofstream::out | std::ofstream::binary);

    if (!saveStream.good())
    {
        return ERROR_STATE_SAVE_LOAD_FILE_ERROR;
    }

    Pause();

    size_t componentStateSize;
    ::StateSave::Ptr componentState;

    componentState = Cpu->SaveState();
    componentStateSize = componentState->GetSize();

    saveStream.write(reinterpret_cast<char*>(&componentStateSize), sizeof(size_t));
    saveStream.write(componentState->GetBuffer(), componentStateSize);

    componentState = Ppu->SaveState();
    componentStateSize = componentState->GetSize();

    saveStream.write(reinterpret_cast<char*>(&componentStateSize), sizeof(size_t));
    saveStream.write(componentState->GetBuffer(), componentStateSize);

    componentState = Apu->SaveState();
    componentStateSize = componentState->GetSize();

    saveStream.write(reinterpret_cast<char*>(&componentStateSize), sizeof(size_t));
    saveStream.write(componentState->GetBuffer(), componentStateSize);

    componentState = Cartridge->SaveState();
    componentStateSize = componentState->GetSize();

    saveStream.write(reinterpret_cast<char*>(&componentStateSize), sizeof(size_t));
    saveStream.write(componentState->GetBuffer(), componentStateSize);

    if (VideoOut != nullptr)
    {
        VideoOut->ShowMessage("Saved State " + std::to_string(slot), 5);
    }

    Resume();

    return dnes::SUCCESS;
}

int NES::LoadState(int slot)
{
    if (CurrentState != State::RUNNING && CurrentState != State::PAUSED)
    {
        return ERROR_STATE_LOAD_NOT_RUNNING;
    }

    std::string extension = "state" + std::to_string(slot);
    std::string fileName = file::createFullPath(GetGameName(), extension, StateSaveDirectory);
    std::ifstream saveStream(fileName.c_str(), std::ifstream::in | std::ifstream::binary);

    if (!saveStream.good())
    {
        return ERROR_STATE_SAVE_LOAD_FILE_ERROR;
    }

    Pause();
  
    size_t componentStateSize;
    std::unique_ptr<char[]> componentState;

    saveStream.read(reinterpret_cast<char*>(&componentStateSize), sizeof(size_t));
    componentState = std::make_unique<char[]>(componentStateSize);
    saveStream.read(componentState.get(), componentStateSize);

    Cpu->LoadState(StateSave::New(componentState, componentStateSize));

    saveStream.read(reinterpret_cast<char*>(&componentStateSize), sizeof(size_t));
    componentState = std::make_unique<char[]>(componentStateSize);
    saveStream.read(componentState.get(), componentStateSize);

    Ppu->LoadState(StateSave::New(componentState, componentStateSize));

    saveStream.read(reinterpret_cast<char*>(&componentStateSize), sizeof(size_t));
    componentState = std::make_unique<char[]>(componentStateSize);
    saveStream.read(componentState.get(), componentStateSize);

    Apu->LoadState(StateSave::New(componentState, componentStateSize));

    saveStream.read(reinterpret_cast<char*>(&componentStateSize), sizeof(size_t));
    componentState = std::make_unique<char[]>(componentStateSize);
    saveStream.read(componentState.get(), componentStateSize);

    Cartridge->LoadState(StateSave::New(componentState, componentStateSize));

    if (VideoOut != nullptr)
    {
        VideoOut->ShowMessage("Loaded State " + std::to_string(slot), 5);
    }

    Resume();

    return dnes::SUCCESS;
}

int NES::GetCurrentErrorCode()
{
    return CurrentErrorCode;
}

void NES::Run()
{
    try
    {
        VideoOut->Prepare();

        AudioOut = std::make_unique<AudioBackend>();
        Apu->SetBackend(AudioOut.get());
        Apu->SetTargetFrameRate(TargetFrameRate);

        Cartridge->LoadNativeSave(NativeSaveDirectory);

        CurrentState = State::RUNNING;

        while (!StopRequested)
        {
            while (!Ppu->EndOfFrame())
            {
                Cpu->Step();
            }

            Callback->OnFrameComplete(this);

            if (PauseRequested)
            {
                std::unique_lock<std::mutex> lock(ControlMutex);

                PauseRequested = false;
                CurrentState = State::PAUSED;

                ControlCv.notify_all();
                ControlCv.wait(lock);

                CurrentState = State::RUNNING;
            }
        }

        StopRequested = false;
        CurrentState = State::STOPPED;

        Cartridge->SaveNativeSave(NativeSaveDirectory);

        VideoOut->Finalize();
    }
    catch (NesException& ex)
    {
        SetErrorCode(ERROR_RUNTIME_ERROR);

        if (Callback != nullptr)
        {
            Callback->OnError(this);
        }
    }

    std::unique_lock<std::mutex> lock(ControlMutex);
    ControlCv.notify_all();
}

void NES::SetErrorCode(int code)
{
    CurrentErrorCode = code;
}

namespace dnes
{

INES* CreateNES()
{
    return new NES();
}

void DestroyNES(INES* nes)
{
    if (NES* nesimpl = dynamic_cast<NES*>(nes))
    {
        delete nesimpl;
    }
}

const char* GetErrorMessageFromCode(int code)
{
    auto it = ERROR_CODE_TO_MESSAGE_MAP.find(code);

    if (it == ERROR_CODE_TO_MESSAGE_MAP.end())
    {
        return "Unrecognized error code";
    }

    return it->second.c_str();
}

}; // namespace dnes