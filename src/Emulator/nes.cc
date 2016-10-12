/*
 * nes.cc
 *
 *  Created on: Aug 28, 2014
 *      Author: Dale
 */

#include <exception>

#include "nes.h"
#include "mappers/nrom.h"

NES::NES(const NesParams& params) :
    stop(true),
    pause(false),
    nmi(false),
    apu(*new APU(*this)), // APU first since it may throw an exception
    cpu(*new CPU(*this, params.CpuLogEnabled)),
    ppu(*new PPU(*this)),
    cart(Cart::Create(params.RomPath, cpu))
{
    apu.AttachCPU(cpu);
    apu.AttachCart(cart);

    cpu.AttachPPU(ppu);
    cpu.AttachAPU(apu);
    cpu.AttachCart(cart);

    ppu.AttachCPU(cpu);
    ppu.AttachCart(cart);

    ppu.SetFrameLimitEnabled(params.FrameLimitEnabled);
    ppu.SetNtscDecodingEnabled(params.NtscDecoderEnabled);

    apu.SetMuted(params.SoundMuted);
    apu.SetFiltersEnabled(params.FiltersEnabled);
	
	// Get just the file name from the rom path

	const std::string& romPath = params.RomPath;
	for (int i = romPath.length() - 1; i >= 0; --i)
	{
		if (romPath[i] == '\\' || romPath[i] == '/')
		{
			gameName = romPath.substr(i + 1, romPath.length() - i - 1);
			break;
		}
	}
}

std::string& NES::GetGameName()
{
    return gameName;
}

void NES::SetControllerOneState(uint8_t state)
{
    cpu.SetControllerOneState(state);
}

uint8_t NES::GetControllerOneState()
{
    return cpu.GetControllerOneState();
}

void NES::EnableCPULog()
{
    cpu.EnableLog();
}

void NES::DisableCPULog()
{
    cpu.DisableLog();
}

void NES::GetNameTable(int table, uint8_t* pixels)
{
    ppu.GetNameTable(table, pixels);
}

void NES::GetPatternTable(int table, int palette, uint8_t* pixels)
{
    ppu.GetPatternTable(table, palette, pixels);
}

void NES::GetPalette(int palette, uint8_t* pixels)
{
    ppu.GetPalette(palette, pixels);
}

void NES::GetPrimarySprite(int sprite, uint8_t* pixels)
{
    ppu.GetPrimaryOAM(sprite, pixels);
}

void NES::PpuSetFrameLimitEnabled(bool enabled)
{
    ppu.SetFrameLimitEnabled(enabled);
}

void NES::PpuSetNtscDecoderEnabled(bool enabled)
{
    ppu.SetNtscDecodingEnabled(enabled);
}

void NES::ApuSetMuted(bool muted)
{
    apu.SetMuted(muted);
}

void NES::ApuSetFiltersEnabled(bool enabled)
{
    apu.SetFiltersEnabled(enabled);
}

bool NES::IsStopped()
{
    bool isStopped;
    stopMutex.lock();
    isStopped = stop;
    stopMutex.unlock();
    return isStopped;
}

bool NES::IsPaused()
{
    return cpu.IsPaused();
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
    stopMutex.lock();
    stop = false;
    stopMutex.unlock();

    try
    {
        cpu.Run();
    }
    catch (std::exception& e)
    {
        if (OnError)
        {
            OnError(e.what());
        }
    }

    stopMutex.lock();
    stop = true;
    stopMutex.unlock();
}

void NES::Stop()
{
    stopMutex.lock();
    stop = true;
    stopMutex.unlock();

    if (cpu.IsPaused())
    {
        cpu.Resume();
    }

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
    cpu.Resume();
}

void NES::Pause()
{
    cpu.Pause();
}

void NES::Reset() {}

NES::~NES()
{
    delete &cpu;
    delete &ppu;
    delete &cart;
}

