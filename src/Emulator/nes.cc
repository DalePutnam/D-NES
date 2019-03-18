/*
 * nes.cc
 *
 *  Created on: Aug 28, 2014
 *      Author: Dale
 */

#include <fstream>
#include <exception>

#include "nes.h"
#include "cpu.h"
#include "apu.h"
#include "ppu.h"
#include "cart.h"
#include "file.h"
#include "state.h"
#include "video/video_backend.h"
#include "audio/audio_backend.h"

NES::NES(const std::string& gamePath, const std::string& savePath,
            void* windowHandle, NESCallback* callback)
    : Apu(nullptr)
    , Cpu(nullptr)
    , Ppu(nullptr)
    , Cartridge(nullptr)
    , VideoOut(nullptr)
    , AudioOut(nullptr)
    , Callback(callback)
{
    try
    {
        if (windowHandle != nullptr)
        {
            VideoOut = new VideoBackend(windowHandle);
        }

        AudioOut = new AudioBackend();
        
        Cpu = new CPU;
        Ppu = new PPU(VideoOut, Callback); // Will be nullptr in HeadlessMode
        Apu = new APU(AudioOut);
        Cartridge = new Cart(gamePath, savePath);
    }
    catch (NesException& e)
    {
        delete Apu;
        delete Cpu;
        delete Ppu;
        delete Cartridge;
        delete VideoOut;
        delete AudioOut;

        throw e;
    }

    if (VideoOut != nullptr)
    {
        VideoOut->ShowFps(false);
        VideoOut->SetOverscanEnabled(true);
    }
    
    Apu->AttachCPU(Cpu);
    Apu->AttachCart(Cartridge);

    Cpu->AttachPPU(Ppu);
    Cpu->AttachAPU(Apu);
    Cpu->AttachCart(Cartridge);

    Ppu->AttachCPU(Cpu);
    Ppu->AttachAPU(Apu);
    Ppu->AttachCart(Cartridge);

    Cartridge->AttachCPU(Cpu);
    Cartridge->AttachPPU(Ppu);

    // CPU Settings
    Cpu->SetLogEnabled(false);

    // PPU Settings
    Ppu->SetTurboModeEnabled(false);
    Ppu->SetNtscDecodingEnabled(false);

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

NES::~NES()
{
    delete Apu;
    delete Cpu;
    delete Ppu;
    delete Cartridge;
    delete VideoOut;
    delete AudioOut;
}

const std::string& NES::GetGameName()
{
    return Cartridge->GetGameName();
}

void NES::SetControllerOneState(uint8_t state)
{
    Cpu->SetControllerOneState(state);
}

uint8_t NES::GetControllerOneState()
{
    return Cpu->GetControllerOneState();
}

void NES::SetCpuLogEnabled(bool enabled)
{
    Cpu->SetLogEnabled(enabled);
}

void NES::SetNativeSaveDirectory(const std::string& saveDir)
{
    Cartridge->SetSaveDirectory(saveDir);
}

void NES::SetStateSaveDirectory(const std::string& saveDir)
{

    StateSaveDirectory = saveDir;
}

void NES::SetTargetFrameRate(uint32_t rate)
{
    Apu->SetTargetFrameRate(rate);
}

void NES::SetTurboModeEnabled(bool enabled)
{
    Ppu->SetTurboModeEnabled(enabled);
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
    Ppu->SetNtscDecodingEnabled(enabled);
}

void NES::SetFpsDisplayEnabled(bool enabled)
{
    VideoOut->ShowFps(enabled);
}

void NES::SetOverscanEnabled(bool enabled)
{
    VideoOut->SetOverscanEnabled(enabled);
}

void NES::ShowMessage(const std::string& message, uint32_t duration)
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

void NES::Start()
{
    if (!NesThread.joinable())
    {
        NesThread = std::thread(&NES::Run, this);
    }
    else
    {
        throw NesException("NES", "There is already a thread running on this NES instance");
    }
}

void NES::Run()
{
    try
    {
		VideoOut->Prepare();

        CurrentState = State::Running;
        Cpu->Run();

		VideoOut->Finalize();
    }
    catch (NesException&)
    {
        CurrentState = State::Error;

        if (Callback != nullptr)
        {
            Callback->OnError(std::current_exception());
        }
    }
}

void NES::Stop()
{
    if (NesThread.joinable())
    {
        Cpu->Stop();

        if (std::this_thread::get_id() == NesThread.get_id())
        {
            throw NesException("NES", "NES Thread tried to stop itself");
        }

        NesThread.join();

        CurrentState = State::Stopped;
    } else {
        throw NesException("NES", "Cannot restart a stopped NES instance");
    }
}

void NES::Resume()
{
    CurrentState = State::Running;
    Cpu->Resume();
}

void NES::Pause()
{
    Cpu->Pause();
    CurrentState = State::Paused;
}

void NES::Reset() {}

void NES::SaveState(int slot)
{

    std::string extension = "state" + std::to_string(slot);
    std::string fileName = file::createFullPath(GetGameName(), extension, StateSaveDirectory);
    std::ofstream saveStream(fileName.c_str(), std::ofstream::out | std::ofstream::binary);

    if (!saveStream.good())
    {
        throw NesException("NES", "Failed to open state save file");
    }

    // Pause the emulator and bring the PPU up to date with the CPU
    Pause();

    size_t componentStateSize;
    ::State::Ptr componentState;

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

    VideoOut->ShowMessage("Saved State " + std::to_string(slot), 5);

    Resume();
}

void NES::LoadState(int slot)
{
    std::string extension = "state" + std::to_string(slot);
    std::string fileName = file::createFullPath(GetGameName(), extension, StateSaveDirectory);
    std::ifstream saveStream(fileName.c_str(), std::ifstream::in | std::ifstream::binary);

    if (!saveStream.good())
    {
        throw NesException("NES", "Failed to open state save file");
    }

    // Pause the emulator and bring the PPU up to date with the CPU
    Pause();
  
    size_t componentStateSize;
    std::unique_ptr<char[]> componentState;

    saveStream.read(reinterpret_cast<char*>(&componentStateSize), sizeof(size_t));
    componentState = std::make_unique<char[]>(componentStateSize);
    saveStream.read(componentState.get(), componentStateSize);

    Cpu->LoadState(::State::New(componentState, componentStateSize));

    saveStream.read(reinterpret_cast<char*>(&componentStateSize), sizeof(size_t));
    componentState = std::make_unique<char[]>(componentStateSize);
    saveStream.read(componentState.get(), componentStateSize);

    Ppu->LoadState(::State::New(componentState, componentStateSize));

    saveStream.read(reinterpret_cast<char*>(&componentStateSize), sizeof(size_t));
    componentState = std::make_unique<char[]>(componentStateSize);
    saveStream.read(componentState.get(), componentStateSize);

    Apu->LoadState(::State::New(componentState, componentStateSize));

    saveStream.read(reinterpret_cast<char*>(&componentStateSize), sizeof(size_t));
    componentState = std::make_unique<char[]>(componentStateSize);
    saveStream.read(componentState.get(), componentStateSize);

    Cartridge->LoadState(::State::New(componentState, componentStateSize));

    VideoOut->ShowMessage("Loaded State " + std::to_string(slot), 5);

    Resume();
}
