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

    Apu->SetAudioEnabled(params.AudioEnabled);
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

void NES::SetCpuLogEnabled(bool enabled)
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

void NES::SetFrameLimitEnabled(bool enabled)
{
    Ppu->SetFrameLimitEnabled(enabled);
}

void NES::SetNtscDecoderEnabled(bool enabled)
{
    Ppu->SetNtscDecodingEnabled(enabled);
}

void NES::SetAudioEnabled(bool enabled)
{
    Apu->SetAudioEnabled(enabled);
}

void NES::SetAudioFiltersEnabled(bool enabled)
{
    Apu->SetFiltersEnabled(enabled);
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

void NES::BindFrameCompleteCallback(const std::function<void(uint8_t*)>& fn)
{
	Ppu->BindFrameCompleteCallback(fn);
}

void NES::BindErrorCallback(const std::function<void(std::string)>& fn)
{
	OnError = fn;
}

void NES::SaveState(int slot, const std::string& savePath)
{
    std::string fileName = savePath + "/" + GetGameName() + ".state" + std::to_string(slot);
    std::ofstream saveStream(fileName.c_str(), std::ofstream::out | std::ofstream::binary);

    if (!saveStream.good())
    {
        throw std::runtime_error("NES: Failed to open state save file");
    }

    // Pause the emulator and bring the PPU up to date with the CPU
    Cpu->Pause();
    Ppu->Run();

    char* state = new char[CPU::STATE_SIZE];
    Cpu->SaveState(state);
    saveStream.write(state, CPU::STATE_SIZE);
    delete[] state;

    state = new char[PPU::STATE_SIZE];
    Ppu->SaveState(state);
    saveStream.write(state, PPU::STATE_SIZE);
    delete[] state;

    state = new char[APU::STATE_SIZE];
    Apu->SaveState(state);
    saveStream.write(state, APU::STATE_SIZE);
    delete[] state;

    state = new char[Cartridge->GetStateSize()];
    Cartridge->SaveState(state);
    saveStream.write(state, Cartridge->GetStateSize());
    delete[] state;

    Cpu->Resume();
}

void NES::LoadState(int slot, const std::string& savePath)
{
    std::string fileName = savePath + "/" + GetGameName() + ".state" + std::to_string(slot);
    std::ifstream saveStream(fileName.c_str(), std::ifstream::in | std::ifstream::binary);

    if (!saveStream.good())
    {
        throw std::runtime_error("NES: Failed to open state save file");
    }

    // Pause the emulator and bring the PPU up to date with the CPU
    Cpu->Pause();
    Ppu->Run();

    char* state = new char[CPU::STATE_SIZE];
    saveStream.read(state, CPU::STATE_SIZE);
    Cpu->LoadState(state);
    delete[] state;

    state = new char[PPU::STATE_SIZE];
    saveStream.read(state, PPU::STATE_SIZE);
    Ppu->LoadState(state);
    delete[] state;

    state = new char[APU::STATE_SIZE];
    saveStream.read(state, APU::STATE_SIZE);
    Apu->LoadState(state);
    delete[] state;

    state = new char[Cartridge->GetStateSize()];
    saveStream.read(state, Cartridge->GetStateSize());
    Cartridge->LoadState(state);
    delete[] state;

    Cpu->Resume();
}

NES::~NES()
{
    delete Apu;
    delete Cpu;
    delete Ppu;
    delete Cartridge;
}
