/*
 * nes.cc
 *
 *  Created on: Aug 28, 2014
 *      Author: Dale
 */

#include <exception>
#include <boost/algorithm/string.hpp>

#include "nes.h"
#include "mappers/nrom.h"

NES::NES(const NesParams& params) :
    stop(true),
    pause(false),
    nmi(false),
    cpu(*new CPU(*this, params.CpuLogEnabled)),
    ppu(*new PPU(*this)),
    cart(Cart::Create(params.RomPath, cpu))
{
    cpu.AttachPPU(ppu);
    cpu.AttachCart(cart);

    ppu.AttachCPU(cpu);
    ppu.AttachCart(cart);

    if (!params.FrameLimitEnabled)
    {
        ppu.DisableFrameLimit();
    }

    std::vector<std::string> stringList;
    boost::algorithm::split(stringList, params.RomPath, boost::is_any_of("\\/"));
    gameName = stringList.back().substr(0, stringList.back().length() - 4);
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

void NES::EnableFrameLimit()
{
	ppu.EnableFrameLimit();
}

void NES::DisableFrameLimit()
{
	ppu.DisableFrameLimit();
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

