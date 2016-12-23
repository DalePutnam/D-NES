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
    : apu(nullptr)
    , cpu(nullptr)
    , ppu(nullptr)
    , cart(nullptr)
{
    try
    {
        apu = new APU;
        cpu = new CPU;
        ppu = new PPU;
        cart = new Cart(params.RomPath, params.SavePath);
    }
    catch (std::runtime_error& e) 
    {
        delete apu;
        delete cpu;
        delete ppu;
        delete cart;

        throw e;
    }

    apu->AttachCPU(cpu);
    apu->AttachCart(cart);

    cpu->AttachPPU(ppu);
    cpu->AttachAPU(apu);
    cpu->AttachCart(cart);

    ppu->AttachCPU(cpu);
    ppu->AttachCart(cart);

    cart->AttachCPU(cpu);

    cpu->SetLogEnabled(params.CpuLogEnabled);

    ppu->SetFrameLimitEnabled(params.FrameLimitEnabled);
    ppu->SetNtscDecodingEnabled(params.NtscDecoderEnabled);

    apu->SetMuted(params.SoundMuted);
    apu->SetFiltersEnabled(params.FiltersEnabled);
}

const std::string& NES::GetGameName()
{
    return cart->GetGameName();
}

void NES::SetControllerOneState(uint8_t state)
{
    cpu->SetControllerOneState(state);
}

uint8_t NES::GetControllerOneState()
{
    return cpu->GetControllerOneState();
}

void NES::CpuSetLogEnabled(bool enabled)
{
    cpu->SetLogEnabled(enabled);
}

void NES::GetNameTable(int table, uint8_t* pixels)
{
    ppu->GetNameTable(table, pixels);
}

void NES::GetPatternTable(int table, int palette, uint8_t* pixels)
{
    ppu->GetPatternTable(table, palette, pixels);
}

void NES::GetPalette(int palette, uint8_t* pixels)
{
    ppu->GetPalette(palette, pixels);
}

void NES::GetPrimarySprite(int sprite, uint8_t* pixels)
{
    ppu->GetPrimaryOAM(sprite, pixels);
}

void NES::PpuSetFrameLimitEnabled(bool enabled)
{
    ppu->SetFrameLimitEnabled(enabled);
}

void NES::PpuSetNtscDecoderEnabled(bool enabled)
{
    ppu->SetNtscDecodingEnabled(enabled);
}

void NES::ApuSetMuted(bool muted)
{
    apu->SetMuted(muted);
}

void NES::ApuSetFiltersEnabled(bool enabled)
{
    apu->SetFiltersEnabled(enabled);
}

bool NES::IsStopped()
{
    return !nesThread.joinable();
}

bool NES::IsPaused()
{
    return cpu->IsPaused();
}

void NES::Start()
{
    if (!nesThread.joinable())
    {
        nesThread = std::thread(&NES::Run, this);
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
        cpu->Run();
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
    cpu->Stop();

    if (nesThread.joinable())
    {
        if (std::this_thread::get_id() == nesThread.get_id())
        {
            throw std::runtime_error("NES Thread tried to stop itself!");
        }

        nesThread.join();
    }
}

void NES::Resume()
{
    cpu->Resume();
}

void NES::Pause()
{
    cpu->Pause();
}

void NES::Reset() {}

NES::~NES()
{
    delete apu;
    delete cpu;
    delete ppu;
    delete cart;
}

