/*
 * nes.cc
 *
 *  Created on: Aug 28, 2014
 *      Author: Dale
 */

#include <boost/algorithm/string.hpp>

#include "nes.h"
#include "mappers/nrom.h"

NES::NES(const NesParams& params) :
    stop(true),
    pause(false),
    nmi(false),
    cpu(*new CPU(*this, params.CpuLogEnabled)),
    ppu(*new PPU(*this, params.FrameCompleteCallback)),
    cart(Cart::Create(params.RomPath, *this, cpu))
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

void NES::GetPrimaryOAM(int sprite, uint8_t* pixels)
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

void NES::Run()
{
    stopMutex.lock();
    stop = false;
    stopMutex.unlock();

    cpu.Run();

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

