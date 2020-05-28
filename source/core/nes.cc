#include <fstream>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "nes.h"

#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "cart.h"
#include "audio/alsa_backend.h"
#include "audio/null_audio_backend.h"
#include "video/opengl_backend.h"
#include "video/null_video_backend.h"
#include "error.h"

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

    VideoOut = std::make_unique<NullVideoBackend>(*this);
    AudioOut = std::make_unique<NullAudioBackend>(*this);

    Cpu = std::make_unique<CPU>(*this);
    Ppu = std::make_unique<PPU>(*this);
    Apu = std::make_unique<APU>(*this);
    Cartridge = std::make_unique<Cart>(*this);

    Apu->AttachCPU(Cpu.get());
    Apu->AttachCart(Cartridge.get());

    Cpu->AttachPPU(Ppu.get());
    Cpu->AttachAPU(Apu.get());
    Cpu->AttachCart(Cartridge.get());

    CurrentState = State::READY;
}

NES::~NES()
{
    GetLogger()->flush();
}

int NES::LoadGame(const char* romFile, const char* saveFile)
{
    if (CurrentState != State::READY)
    {
        SPDLOG_LOGGER_ERROR(GetLogger(), "NES: Failed to load game, emulation has already started.");
        return ERROR_LOAD_GAME_AFTER_START;
    }

    try
    {
        Cartridge->Initialize(romFile, saveFile);
    }
    catch (NesException& ex)
    {
        return ex.errorCode();
    }

    return dnes::SUCCESS;
}

int NES::SetWindowHandle(void* handle)
{
    if (CurrentState != State::READY)
    {
        return ERROR_SET_WINDOW_HANDLE_AFTER_START;
    }

    try
    {
        auto newVideoBackend = std::make_unique<OpenGLBackend>(*this, handle);
        newVideoBackend->SwapSettings(*VideoOut);

        VideoOut = std::move(newVideoBackend);

        auto newAudioBackend = std::make_unique<AlsaBackend>(*this);
        newAudioBackend->SwapSettings(*AudioOut);

        AudioOut = std::move(newAudioBackend);
    }
    catch (NesException& ex)
    {
        return ex.errorCode();
    }

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

void NES::SetControllerOneState(uint8_t state)
{
    Cpu->SetControllerOneState(state);
}

uint8_t NES::GetControllerOneState()
{
    return Cpu->GetControllerOneState();
}

int NES::StartCpuLog(const char* logFile)
{
    Pause();

    try
    {
        Cpu->StartLog(logFile);
    }
    catch (NesException& ex)
    {
        Resume();
        return ex.errorCode();
    }

    Resume();

    return dnes::SUCCESS;
}

void NES::StopCpuLog()
{
    Pause();

    Cpu->StopLog();

    Resume();
}

void NES::SetTargetFrameRate(uint32_t rate)
{
    Apu->SetTargetFrameRate(rate);
}

void NES::SetTurboModeEnabled(bool enabled)
{
    Apu->SetTurboModeEnabled(enabled);
}

int NES::GetFrameRate()
{
    return Ppu->GetFrameRate();
}

int NES::GetNameTable(uint32_t tableIndex, uint8_t* imageBuffer)
{
    try
    {
        Ppu->GetNameTable(tableIndex, imageBuffer);
    }
    catch (NesException& ex)
    {
        return ex.errorCode();
    }

    return dnes::SUCCESS;
}

int NES::GetPatternTable(uint32_t tableIndex, uint32_t paletteIndex, uint8_t* imageBuffer)
{
    try
    {
        Ppu->GetPatternTable(tableIndex, paletteIndex, imageBuffer);
    }
    catch (NesException& ex)
    {
        return ex.errorCode();
    }

    return dnes::SUCCESS;
}

int NES::GetPalette(uint32_t paletteIndex, uint8_t* imageBuffer)
{
    try
    {
        Ppu->GetPalette(paletteIndex, imageBuffer);
    }
    catch (NesException& ex)
    {
        return ex.errorCode();
    }

    return dnes::SUCCESS;
}

int NES::GetSprite(uint32_t spriteIndex, uint8_t* imageBuffer)
{
    try
    {
        Ppu->GetSprite(spriteIndex, imageBuffer);
    }
    catch (NesException& ex)
    {
        return ex.errorCode();
    }

    return dnes::SUCCESS;
}

void NES::SetNtscDecoderEnabled(bool enabled)
{

}

void NES::SetFpsDisplayEnabled(bool enabled)
{
    VideoOut->SetShowFps(enabled);
}

void NES::SetOverscanEnabled(bool enabled)
{
    VideoOut->SetOverscanEnabled(enabled);
}

void NES::ShowMessage(const char* message, uint32_t duration)
{
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

void NES::SetPulseTwoVolume(float volume)
{
    Apu->SetPulseTwoVolume(volume);
}


void NES::SetTriangleVolume(float volume)
{
    Apu->SetTriangleVolume(volume);
}

void NES::SetNoiseVolume(float volume)
{
    Apu->SetNoiseVolume(volume);
}

void NES::SetDmcVolume(float volume)
{
    Apu->SetDmcVolume(volume);
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

int NES::SaveState(const char* file)
{
    if (CurrentState != State::RUNNING && CurrentState != State::PAUSED)
    {
        return ERROR_STATE_SAVE_NOT_RUNNING;
    }

    std::ofstream saveStream(file, std::ofstream::out | std::ofstream::binary);

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
        VideoOut->ShowMessage("Saved State", 5);
    }

    Resume();

    return dnes::SUCCESS;
}

int NES::LoadState(const char* file)
{
    if (CurrentState != State::RUNNING && CurrentState != State::PAUSED)
    {
        return ERROR_STATE_LOAD_NOT_RUNNING;
    }

    std::ifstream saveStream(file, std::ifstream::in | std::ifstream::binary);

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
        VideoOut->ShowMessage("Loaded State", 5);
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
        AudioOut->Initialize();
        VideoOut->Initialize();

        Cartridge->LoadNvRam();

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

        Cartridge->SaveNvRam();

        VideoOut->CleanUp();
        AudioOut->CleanUp();
    }
    catch (NesException& ex)
    {
        SetErrorCode(ex.errorCode());

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
    return ::GetErrorMessageFromCode(code);
}

}; // namespace dnes