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

    void SetCpuLogEnabled(bool enabled) override;
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
    bool Start() override;

    // Instructs the emulator to stop and then blocks until it does.
    // Once this function returns this object may be safely deleted.
    bool Stop() override;

    void Resume() override;
    void Pause() override;
    bool Reset() override;

    const char* SaveState(int slot) override;
    const char* LoadState(int slot) override;

    const char* GetErrorMessage() override;

private:
    // Main run function, launched in a new thread by NES::Start
    void Run();

    void SetError(NesException& ex);
    void SetError(const std::string& component, const std::string& message);

    std::atomic<State> CurrentState{State::Ready};

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

    CurrentState = State::Ready;
}

int NESImpl::LoadGame(const char* path)
{
    if (CurrentState != State::Ready)
    {
        return 1;
    }

    try
    {
        Cartridge = std::make_unique<Cart>(path);
    }
    catch (NesException& e)
    {
        return 1;
    }

    Cpu->AttachCart(Cartridge.get());
    Apu->AttachCart(Cartridge.get());
    Ppu->AttachCart(Cartridge.get());

    Cartridge->AttachCPU(Cpu.get());
    Cartridge->AttachPPU(Ppu.get());

    return 0;
}

int NESImpl::SetWindowHandle(void* handle)
{
    if (CurrentState != State::Ready)
    {
        return 1;
    }

    WindowHandle = handle;

    return 0;
}

int NESImpl::SetCallback(dnes::NESCallback* callback)
{
    if (CurrentState != State::Ready)
    {
        return 1;
    }

    Callback = callback;

    return 0;
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
    if (!Cpu)
    {
        return;
    }

    Cpu->SetControllerOneState(state);
}

uint8_t NESImpl::GetControllerOneState()
{
    if (!Cpu)
    {
        return 0;
    }

    return Cpu->GetControllerOneState();
}

void NESImpl::SetCpuLogEnabled(bool enabled)
{
    if (!Cpu)
    {
        return;
    }

    try
    {
        Pause();
        Cpu->SetLogEnabled(enabled);
        Resume();
    }
    catch (NesException& ex)
    {
    }
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
    if (!Apu)
    {
        return;
    }

    Apu->SetTurboModeEnabled(enabled);
}

int NESImpl::GetFrameRate()
{
    if (!Ppu)
    {
        return 0;
    }

    return Ppu->GetFrameRate();
}

void NESImpl::GetNameTable(int table, uint8_t* pixels)
{
    if (!Ppu)
    {
        return;
    }

    Ppu->GetNameTable(table, pixels);
}

void NESImpl::GetPatternTable(int table, int palette, uint8_t* pixels)
{
    if (!Ppu)
    {
        return;
    }

    Ppu->GetPatternTable(table, palette, pixels);
}

void NESImpl::GetPalette(int palette, uint8_t* pixels)
{
    if (!Ppu)
    {
        return;
    }

    Ppu->GetPalette(palette, pixels);
}

void NESImpl::GetSprite(int sprite, uint8_t* pixels)
{
    if (!Ppu)
    {
        return;
    }

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
    if (!Apu)
    {
        return;
    }

    Apu->SetAudioEnabled(enabled);
}

void NESImpl::SetMasterVolume(float volume)
{
    if (!Apu)
    {
        return;
    }

    Apu->SetMasterVolume(volume);
}

void NESImpl::SetPulseOneVolume(float volume)
{
    if (!Apu)
    {
        return;
    }

    Apu->SetPulseOneVolume(volume);
}

float NESImpl::GetPulseOneVolume()
{
    if (!Apu)
    {
        return 0.f;
    }

    return Apu->GetPulseOneVolume();
}

void NESImpl::SetPulseTwoVolume(float volume)
{
    if (!Apu)
    {
        return;
    }

    Apu->SetPulseTwoVolume(volume);
}

float NESImpl::GetPulseTwoVolume()
{
    if (!Apu)
    {
        return 0.f;
    }

    return Apu->GetPulseTwoVolume();
}

void NESImpl::SetTriangleVolume(float volume)
{
    if (!Apu)
    {
        return;
    }

    Apu->SetTriangleVolume(volume);
}

float NESImpl::GetTriangleVolume()
{
    if (!Apu)
    {
        return 0.f;
    }

    return Apu->GetTriangleVolume();
}

void NESImpl::SetNoiseVolume(float volume)
{
    if (!Apu)
    {
        return;
    }

    Apu->SetNoiseVolume(volume);
}

float NESImpl::GetNoiseVolume()
{
    if (!Apu)
    {
        return 0.f;
    }

    return Apu->GetNoiseVolume();
}

void NESImpl::SetDmcVolume(float volume)
{
    if (!Apu)
    {
        return;
    }

    Apu->SetDmcVolume(volume);
}

float NESImpl::GetDmcVolume()
{
    if (!Apu)
    {
        return 0.f;
    }

    return Apu->GetDmcVolume();
}

NESImpl::State NESImpl::GetState()
{
    return CurrentState;
}

bool NESImpl::Start()
{
    if (!Cartridge)
    {
        SetError("NES", "A game has not yet beed loaded");
        return false;
    }

    if (CurrentState == State::Running || CurrentState == State::Paused)
    {
        SetError("NES", "This NES instance is already running");
        return false;
    }

    if (CurrentState == State::Stopped)
    {
        SetError("NES", "A stopped NES instance cannot be started again");
        return false;
    }

    if (CurrentState == State::Error)
    {
        // Return false, but don't overwrite the current error
        return false;
    }

    NesThread = std::thread(&NESImpl::Run, this);

    return true;
}

bool NESImpl::Stop()
{
    if (CurrentState == State::Stopped)
    {
        SetError("NES", "This NES instance is already stopped");
        return false;
    }

    if (CurrentState == State::Error)
    {
        // Return false, but don't overwrite the current error
        return false;
    }


    StopRequested = true;
    Resume();

    NesThread.join();

    return true;
}

void NESImpl::Resume()
{
    std::unique_lock<std::mutex> lock(ControlMutex);

    if (CurrentState != State::Paused)
    {
        return;
    }

    ControlCv.notify_all();
}

void NESImpl::Pause()
{
    std::unique_lock<std::mutex> lock(ControlMutex);

    if (CurrentState != State::Running)
    {
        return;
    }

    PauseRequested = true;

    ControlCv.wait(lock);
}

bool NESImpl::Reset()
{
    return false;
}

const char* NESImpl::SaveState(int slot)
{
    if (CurrentState != State::Running && CurrentState != State::Paused)
    {
        return "Emulator not running, cannot save state";
    }

    std::string extension = "state" + std::to_string(slot);
    std::string fileName = file::createFullPath(GetGameName(), extension, StateSaveDirectory);
    std::ofstream saveStream(fileName.c_str(), std::ofstream::out | std::ofstream::binary);

    if (!saveStream.good())
    {
        return "Failed to open state save file";
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

    return nullptr;
}

const char*  NESImpl::LoadState(int slot)
{
    if (CurrentState != State::Running && CurrentState != State::Paused)
    {
        return "Emulator not running, cannot load state";
    }

    std::string extension = "state" + std::to_string(slot);
    std::string fileName = file::createFullPath(GetGameName(), extension, StateSaveDirectory);
    std::ifstream saveStream(fileName.c_str(), std::ifstream::in | std::ifstream::binary);

    if (!saveStream.good())
    {
        return "Failed to open state save file";
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

    return nullptr;
}

const char* NESImpl::GetErrorMessage()
{
    return ErrorMessage.c_str();
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

        CurrentState = State::Running;

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
                CurrentState = State::Paused;

                ControlCv.notify_all();
                ControlCv.wait(lock);

                CurrentState = State::Running;
            }
        }

        StopRequested = false;
        CurrentState = State::Stopped;

        Cartridge->SaveNativeSave(NativeSaveDirectory);

        if (VideoOut != nullptr)
        {
            VideoOut->Finalize();
        }
    }
    catch (NesException& ex)
    {
        SetError(ex);

        if (Callback != nullptr)
        {
            Callback->OnError(this);
        }
    }

    std::unique_lock<std::mutex> lock(ControlMutex);
    ControlCv.notify_all();
}

void NESImpl::SetError(NesException& ex)
{
    CurrentState = State::Error;
    ErrorMessage = ex.what();   
}

void NESImpl::SetError(const std::string& component, const std::string& message)
{
    NesException ex(component, message);
    SetError(ex);
}

namespace dnes
{

NES* createNES()
{
    return new NESImpl();
}

void destroyNES(NES* nes)
{
    if (NESImpl* nesimpl = dynamic_cast<NESImpl*>(nes))
    {
        delete nesimpl;
    }
}

}; // namespace dnes