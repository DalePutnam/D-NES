/*
 * nes.cc
 *
 *  Created on: Aug 28, 2014
 *      Author: Dale
 */

#include <exception>

#include "nes.h"
#include "mappers/nrom.h"

NES::NES(const NesParams& params)
    : Apu(nullptr)
    , Cpu(nullptr)
    , Ppu(nullptr)
    , Cartridge(nullptr)
{
    try
    {
        Apu = new APU;
        Cpu = new CPU;
        Ppu = new PPU;
        Cartridge = new Cart(params.RomPath, params.SavePath);
    }
    catch (std::runtime_error& e)
    {
        delete Apu;
        delete Cpu;
        delete Ppu;
        delete Cartridge;

        throw e;
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

    Cpu->SetLogEnabled(params.CpuLogEnabled);

    Ppu->SetFrameLimitEnabled(params.FrameLimitEnabled);
    Ppu->SetNtscDecodingEnabled(params.NtscDecoderEnabled);

    Apu->SetMuted(params.SoundMuted);
    Apu->SetFiltersEnabled(params.FiltersEnabled);
    Apu->SetMasterVolume(params.MasterVolume);
    Apu->SetPulseOneVolume(params.PulseOneVolume);
    Apu->SetPulseTwoVolume(params.PulseTwoVolume);
    Apu->SetTriangleVolume(params.TriangleVolume);
    Apu->SetNoiseVolume(params.NoiseVolume);
    Apu->SetDmcVolume(params.DmcVolume);
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

void NES::CpuSetLogEnabled(bool enabled)
{
    Cpu->SetLogEnabled(enabled);
}

void NES::SetNativeSaveDirectory(const std::string& saveDir)
{
    Cartridge->SetSaveDirectory(saveDir);
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

void NES::GetPrimarySprite(int sprite, uint8_t* pixels)
{
    Ppu->GetPrimaryOAM(sprite, pixels);
}

void NES::PpuSetFrameLimitEnabled(bool enabled)
{
    Ppu->SetFrameLimitEnabled(enabled);
}

void NES::PpuSetNtscDecoderEnabled(bool enabled)
{
    Ppu->SetNtscDecodingEnabled(enabled);
}

void NES::ApuSetMuted(bool muted)
{
    Apu->SetMuted(muted);
}

void NES::ApuSetFiltersEnabled(bool enabled)
{
    Apu->SetFiltersEnabled(enabled);
}

void NES::ApuSetMasterVolume(float volume)
{
    Apu->SetMasterVolume(volume);
}

void NES::ApuSetPulseOneVolume(float volume)
{
    Apu->SetPulseOneVolume(volume);
}

float NES::ApuGetPulseOneVolume()
{
    return Apu->GetPulseOneVolume();
}

void NES::ApuSetPulseTwoVolume(float volume)
{
    Apu->SetPulseTwoVolume(volume);
}

float NES::ApuGetPulseTwoVolume()
{
    return Apu->GetPulseTwoVolume();
}

void NES::ApuSetTriangleVolume(float volume)
{
    Apu->SetTriangleVolume(volume);
}

float NES::ApuGetTriangleVolume()
{
    return Apu->GetTriangleVolume();
}

void NES::ApuSetNoiseVolume(float volume)
{
    Apu->SetNoiseVolume(volume);
}

float NES::ApuGetNoiseVolume()
{
    return Apu->GetNoiseVolume();
}

void NES::ApuSetDmcVolume(float volume)
{
    Apu->SetDmcVolume(volume);
}

float NES::ApuGetDmcVolume()
{
    return Apu->GetDmcVolume();
}

bool NES::IsStopped()
{
    return !NesThread.joinable();
}

bool NES::IsPaused()
{
    return Cpu->IsPaused();
}

void NES::Start()
{
    if (!NesThread.joinable())
    {
        NesThread = std::thread(&NES::Run, this);
    }
    else
    {
        throw std::runtime_error("There is already a thread running on this NES instance.");
    }
}

void NES::Run()
{
    try
    {
        Cpu->Run();
    }
    catch (std::exception& e)
    {
        if (OnError)
        {
            OnError(e.what());
        }
    }
}

void NES::Stop()
{
    Cpu->Stop();

    if (NesThread.joinable())
    {
        if (std::this_thread::get_id() == NesThread.get_id())
        {
            throw std::runtime_error("NES Thread tried to stop itself!");
        }

        NesThread.join();
    }
}

void NES::Resume()
{
    Cpu->Resume();
}

void NES::Pause()
{
    Cpu->Pause();
}

void NES::Reset() {}

NES::~NES()
{
    delete Apu;
    delete Cpu;
    delete Ppu;
    delete Cartridge;
}
