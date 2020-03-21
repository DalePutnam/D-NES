/*
 * nes.cc
 *
 *  Created on: Aug 28, 2014
 *      Author: Dale
 */

#include <fstream>
#include <exception>
#include <iostream>

#include "nes.h"
#include "nes_impl.h"

#include "cpu.h"
#include "apu.h"
#include "ppu.h"
#include "cart.h"
#include "file.h"
#include "state_save.h"
#include "video/video_backend.h"
#include "audio/audio_backend.h"

NES* NES::Create(const std::string& gamePath,
                    const std::string& nativeSavePath,
                    void* windowHandle,
                    NESCallback* callback)
{
    return new NESImpl(gamePath, nativeSavePath, windowHandle, callback);
}

NESImpl::NESImpl(const std::string& gamePath, const std::string& savePath,
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
        Cartridge = new Cart(gamePath);
        Cartridge->SetSaveDirectory(savePath);
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
    Ppu->AttachCart(Cartridge);

    Cartridge->AttachCPU(Cpu);
    Cartridge->AttachPPU(Ppu);

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

NESImpl::~NESImpl()
{
    delete Apu;
    delete Cpu;
    delete Ppu;
    delete Cartridge;
    delete VideoOut;
    delete AudioOut;
}

const std::string& NESImpl::GetGameName()
{
    return Cartridge->GetGameName();
}

void NESImpl::SetControllerOneState(uint8_t state)
{
    Cpu->SetControllerOneState(state);
}

uint8_t NESImpl::GetControllerOneState()
{
    return Cpu->GetControllerOneState();
}

void NESImpl::SetCpuLogEnabled(bool enabled)
{
    Cpu->SetLogEnabled(enabled);
}

void NESImpl::SetNativeSaveDirectory(const std::string& saveDir)
{
    Cartridge->SetSaveDirectory(saveDir);
}

void NESImpl::SetStateSaveDirectory(const std::string& saveDir)
{

    StateSaveDirectory = saveDir;
}

void NESImpl::SetTargetFrameRate(uint32_t rate)
{
    Apu->SetTargetFrameRate(rate);
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
    if (VideoOut != nullptr)
    {
        VideoOut->ShowFps(enabled);
    }
}

void NESImpl::SetOverscanEnabled(bool enabled)
{
    if (VideoOut != nullptr)
    {
        VideoOut->SetOverscanEnabled(enabled);
    }
}

void NESImpl::ShowMessage(const std::string& message, uint32_t duration)
{
    if (VideoOut != nullptr)
    {
        VideoOut->ShowMessage(message, duration);
    }
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

void NESImpl::Start()
{
    if (!NesThread.joinable())
    {
        NesThread = std::thread(&NESImpl::Run, this);
    }
    else
    {
        throw NesException("NES", "There is already a thread running on this NES instance");
    }
}

void NESImpl::Run()
{
    try
    {
        if (VideoOut != nullptr)
        {
            VideoOut->Prepare();
        }

        Cartridge->LoadNativeSave();

        CurrentState = State::Running;

        while (CurrentState != State::Stopped)
        {
            while (!Ppu->EndOfFrame())
            {
                Cpu->Step();
            }

            {
                std::unique_lock<std::mutex> lock(PauseMutex);
                if (CurrentState == State::Paused)
                {
                    PauseVariable.notify_all();
                    PauseVariable.wait(lock);
                }
            }
        }

        Cartridge->SaveNativeSave();

        if (VideoOut != nullptr)
        {
            VideoOut->Finalize();
        }
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

void NESImpl::Stop()
{
    if (NesThread.joinable())
    {
        if (std::this_thread::get_id() == NesThread.get_id())
        {
            throw NesException("NES", "NES Thread tried to stop itself");
        }

        {
            std::unique_lock<std::mutex> lock(PauseMutex);

            CurrentState = State::Stopped;

            PauseVariable.notify_all();
        }

        NesThread.join();
    }
    else
    {
        throw NesException("NES", "Cannot restart a stopped NES instance");
    }
}

void NESImpl::Resume()
{
    std::unique_lock<std::mutex> lock(PauseMutex);

    if (CurrentState == State::Stopped)
    {
        return;
    }

    CurrentState = State::Running;
    PauseVariable.notify_all();
    //Cpu->Resume();
}

void NESImpl::Pause()
{
    std::unique_lock<std::mutex> lock(PauseMutex);

    if (CurrentState == State::Stopped)
    {
        return;
    }

    //Cpu->Pause();
    CurrentState = State::Paused;

    PauseVariable.wait(lock);
}

void NESImpl::Reset() {}

void NESImpl::SaveState(int slot)
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
}

void NESImpl::LoadState(int slot)
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
}
