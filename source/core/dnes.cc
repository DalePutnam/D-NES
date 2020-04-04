#include <atomic>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <fstream>

#include "dnes/dnes.h"

#include "cpu.h"
#include "ppu.h"
#include "apu.h"
#include "cart.h"
#include "video/video_backend.h"
#include "audio/audio_backend.h"
#include "common/state_save.h"
#include "common/file.h"
#include "common/error_handling.h"

class NESImpl : public dnes::NES
{
public:
    NESImpl();
    ~NESImpl() = default;

    int LoadGame(const char* path) override;

    int SetWindowHandle(void* handle) override;

    int SetCallback(dnes::NESCallback* callback) override;

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

    // Launch the emulator on a new thread.
    // This function returns immediately.
    int Start() override;

    // Instructs the emulator to stop and then blocks until it does.
    // Once this function returns this object may be safely deleted.
    int Stop() override;

    void Resume() override;
    void Pause() override;
    int Reset() override;

    int SaveState(int slot) override;
    int LoadState(int slot) override;

    int GetCurrentErrorCode() override;

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

    dnes::NESCallback* Callback{nullptr};

    std::atomic<bool> ShowFps{false};
    std::atomic<bool> OverscanEnabled{true};
    std::atomic<uint32_t> TargetFrameRate{60};

    std::string NativeSaveDirectory;
};

NESImpl::NESImpl()
{
    Cpu = std::make_unique<CPU>();
    Ppu = std::make_unique<PPU>(/*VideoOut.get()*/);
    Apu = std::make_unique<APU>(/*AudioOut.get()*/);

    Apu->AttachCPU(Cpu.get());

    Cpu->AttachPPU(Ppu.get());
    Cpu->AttachAPU(Apu.get());

    Ppu->AttachCPU(Cpu.get());

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

int NESImpl::LoadGame(const char* path)
{
    if (CurrentState != State::READY)
    {
        return ERROR_LOAD_GAME_AFTER_START;
    }

    try
    {
        Cartridge = std::make_unique<Cart>(path);
    }
    catch (NesException& e)
    {
        return ERROR_LOAD_GAME_FAILED;
    }

    Cpu->AttachCart(Cartridge.get());
    Apu->AttachCart(Cartridge.get());
    Ppu->AttachCart(Cartridge.get());

    Cartridge->AttachCPU(Cpu.get());
    Cartridge->AttachPPU(Ppu.get());

    return dnes::SUCCESS;
}

int NESImpl::SetWindowHandle(void* handle)
{
    if (CurrentState != State::READY)
    {
        return ERROR_SET_WINDOW_HANDLE_AFTER_START;
    }

    WindowHandle = handle;

    return dnes::SUCCESS;
}

int NESImpl::SetCallback(dnes::NESCallback* callback)
{
    if (CurrentState != State::READY)
    {
        return ERROR_SET_CALLBACK_AFTER_START;
    }

    Callback = callback;

    return dnes::SUCCESS;
}

const char* NESImpl::GetGameName()
{
    if (!Cartridge)
    {
        return nullptr;
    }

    return Cartridge->GetGameName().c_str();
}

void NESImpl::SetControllerOneState(uint8_t state)
{
    Cpu->SetControllerOneState(state);
}

uint8_t NESImpl::GetControllerOneState()
{
    return Cpu->GetControllerOneState();
}

int NESImpl::SetCpuLogEnabled(bool enabled)
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

void NESImpl::SetNativeSaveDirectory(const char* saveDir)
{
    NativeSaveDirectory = saveDir;
}

void NESImpl::SetStateSaveDirectory(const char* saveDir)
{
    StateSaveDirectory = saveDir;
}

void NESImpl::SetTargetFrameRate(uint32_t rate)
{
    TargetFrameRate = rate;

    if (AudioOut)
    {
        Apu->SetTargetFrameRate(TargetFrameRate);
    }
}

void NESImpl::SetTurboModeEnabled(bool enabled)
{
    Apu->SetTurboModeEnabled(enabled);
}

int NESImpl::GetFrameRate()
{
    return Ppu->GetFrameRate();
}

void NESImpl::GetNameTable(int table, uint8_t* pixels)
{
    Ppu->GetNameTable(table, pixels);
}

void NESImpl::GetPatternTable(int table, int palette, uint8_t* pixels)
{
    Ppu->GetPatternTable(table, palette, pixels);
}

void NESImpl::GetPalette(int palette, uint8_t* pixels)
{
    Ppu->GetPalette(palette, pixels);
}

void NESImpl::GetSprite(int sprite, uint8_t* pixels)
{
    Ppu->GetPrimaryOAM(sprite, pixels);
}

void NESImpl::SetNtscDecoderEnabled(bool enabled)
{

}

void NESImpl::SetFpsDisplayEnabled(bool enabled)
{
    ShowFps = enabled;

    if (VideoOut)
    {
        VideoOut->ShowFps(ShowFps);
    }
}

void NESImpl::SetOverscanEnabled(bool enabled)
{
    OverscanEnabled = enabled;

    if (VideoOut)
    {
        VideoOut->SetOverscanEnabled(OverscanEnabled);
    }
}

void NESImpl::ShowMessage(const char* message, uint32_t duration)
{
    if (!VideoOut)
    {
        return;
    }

    VideoOut->ShowMessage(message, duration);
}

void NESImpl::SetAudioEnabled(bool enabled)
{
    Apu->SetAudioEnabled(enabled);
}

void NESImpl::SetMasterVolume(float volume)
{
    Apu->SetMasterVolume(volume);
}

void NESImpl::SetPulseOneVolume(float volume)
{
    Apu->SetPulseOneVolume(volume);
}

float NESImpl::GetPulseOneVolume()
{
    return Apu->GetPulseOneVolume();
}

void NESImpl::SetPulseTwoVolume(float volume)
{
    Apu->SetPulseTwoVolume(volume);
}

float NESImpl::GetPulseTwoVolume()
{
    return Apu->GetPulseTwoVolume();
}

void NESImpl::SetTriangleVolume(float volume)
{
    Apu->SetTriangleVolume(volume);
}

float NESImpl::GetTriangleVolume()
{
    return Apu->GetTriangleVolume();
}

void NESImpl::SetNoiseVolume(float volume)
{
    Apu->SetNoiseVolume(volume);
}

float NESImpl::GetNoiseVolume()
{
    return Apu->GetNoiseVolume();
}

void NESImpl::SetDmcVolume(float volume)
{
    Apu->SetDmcVolume(volume);
}

float NESImpl::GetDmcVolume()
{
    return Apu->GetDmcVolume();
}

NESImpl::State NESImpl::GetState()
{
    return CurrentState;
}

int NESImpl::Start()
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

    NesThread = std::thread(&NESImpl::Run, this);

    return dnes::SUCCESS;
}

int NESImpl::Stop()
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

void NESImpl::Resume()
{
    std::unique_lock<std::mutex> lock(ControlMutex);

    if (CurrentState != State::PAUSED)
    {
        return;
    }

    ControlCv.notify_all();
}

void NESImpl::Pause()
{
    std::unique_lock<std::mutex> lock(ControlMutex);

    if (CurrentState != State::RUNNING)
    {
        return;
    }

    PauseRequested = true;

    ControlCv.wait(lock);
}

int NESImpl::Reset()
{
    return ERROR_UNIMPLEMENTED;
}

int NESImpl::SaveState(int slot)
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

int NESImpl::LoadState(int slot)
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

int NESImpl::GetCurrentErrorCode()
{
    return CurrentErrorCode;
}

void NESImpl::Run()
{
    try
    {

        if (WindowHandle != nullptr)
        {
            VideoOut = std::make_unique<VideoBackend>(WindowHandle);
            VideoOut->Prepare();

            VideoOut->ShowFps(ShowFps);
            VideoOut->SetOverscanEnabled(OverscanEnabled);

            Ppu->SetBackend(VideoOut.get());
        }

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

        if (VideoOut != nullptr)
        {
            VideoOut->Finalize();
        }
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

void NESImpl::SetErrorCode(int code)
{
    CurrentErrorCode = code;
}

namespace dnes
{

NES* CreateNES()
{
    return new NESImpl();
}

void DestroyNES(NES* nes)
{
    if (NESImpl* nesimpl = dynamic_cast<NESImpl*>(nes))
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