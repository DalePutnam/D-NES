#include "mmc3.h"
#include "ppu.h"

MMC3::MMC3(NES& nes, iNesFile& file)
    : MapperBase(nes, file)
    , _prgMode(0)
    , _chrMode(0)
    , _registerAddress(0)
    , _chrReg0(0)
    , _chrReg1(0)
    , _chrReg2(0)
    , _chrReg3(0)
    , _chrReg4(0)
    , _chrReg5(0)
    , _prgReg0(0)
    , _prgReg1(0)
    , _prgRamEnabled(false)
    , _prgRamWriteProtect(false)
    , _lastReadHigh(false)
    , _lastRiseCycle(0)
    , _irqCounter(0)
    , _irqReloadValue(0)
    , _irqEnabled(false)
    , _irqPending(false)
{
}

void MMC3::SaveState(StateSave::Ptr& state)
{
    MapperBase::SaveState(state);
    
    state->StoreValue(_prgMode);
    state->StoreValue(_chrMode);
    state->StoreValue(_registerAddress);
    state->StoreValue(_chrReg0);
    state->StoreValue(_chrReg1);
    state->StoreValue(_chrReg2);
    state->StoreValue(_chrReg3);
    state->StoreValue(_chrReg4);
    state->StoreValue(_chrReg5);
    state->StoreValue(_prgReg0);
    state->StoreValue(_prgReg1);
    state->StoreValue(_lastRiseCycle);
    state->StoreValue(_irqCounter);
    state->StoreValue(_irqReloadValue);

    state->StorePackedValues(_prgRamEnabled,
                             _prgRamWriteProtect,
                             _irqEnabled,
                             _irqPending);
}

void MMC3::LoadState(const StateSave::Ptr& state)
{
    MapperBase::LoadState(state);

    state->ExtractValue(_prgMode);
    state->ExtractValue(_chrMode);
    state->ExtractValue(_registerAddress);
    state->ExtractValue(_chrReg0);
    state->ExtractValue(_chrReg1);
    state->ExtractValue(_chrReg2);
    state->ExtractValue(_chrReg3);
    state->ExtractValue(_chrReg4);
    state->ExtractValue(_chrReg5);
    state->ExtractValue(_prgReg0);
    state->ExtractValue(_prgReg1);
    state->ExtractValue(_lastRiseCycle);
    state->ExtractValue(_irqCounter);
    state->ExtractValue(_irqReloadValue);

    state->ExtractPackedValues(_prgRamEnabled,
                               _prgRamWriteProtect,
                               _irqEnabled,
                               _irqPending);
}

uint8_t MMC3::CpuRead(uint16_t address)
{
    if (address >= 0x6000 && address < 0x8000)
    {
        if (_mirroring != iNesFile::FourScreen && _prgRamEnabled)
        {
            return _prgRam[address - 0x6000];
        }
        else
        {
            return 0x00;
        }
    }
    else if (address >= 0x8000)
    {
        if (_prgMode == 0)
        {
            if (address < 0xA000)
            {
                return _prgRom[(address - 0x8000) + (_prgReg0 * 0x2000)];
            }
            else if (address < 0xC000)
            {   
                return _prgRom[(address - 0xA000) + (_prgReg1 * 0x2000)];
            }
            else if (address < 0xE000)
            {
                return _prgRom[(address - 0xC000) + (_prgRomSize - 0x4000)];
            }
            else
            {
                return _prgRom[(address - 0xE000) + (_prgRomSize - 0x2000)];
            }
        }
        else
        {
            if (address < 0xA000)
            {
                return _prgRom[(address - 0x8000) + (_prgRomSize - 0x4000)];
            }
            else if (address < 0xC000)
            {   
                return _prgRom[(address - 0xA000) + (_prgReg1 * 0x2000)];
            }
            else if (address < 0xE000)
            {
                return _prgRom[(address - 0xC000) + (_prgReg0 * 0x2000)];
            }
            else
            {
                return _prgRom[(address - 0xE000) + (_prgRomSize - 0x2000)];
            }
        }
    }
    else
    {
        return 0x00;
    }
}

void MMC3::CpuWrite(uint8_t M, uint16_t address)
{
    if (address >= 0x6000 && address < 0x8000)
    {
        if (_mirroring != iNesFile::FourScreen && _prgRamEnabled && !_prgRamWriteProtect)
        {
            _prgRam[address - 0x6000] = M;
        }
    }
    else if (address == 0x8000)
    {
        _chrMode = M >> 7;
        _prgMode = (M >> 6) & 0x1;
        _registerAddress = M & 0x7;
    }
    else if (address == 0x8001)
    {
        switch (_registerAddress)
        {
        case 0:
            _chrReg0 = M;
            break;
        case 1:
            _chrReg1 = M;
            break;
        case 2:
            _chrReg2 = M;
            break;
        case 3:
            _chrReg3 = M;
            break;
        case 4:
            _chrReg4 = M;
            break;
        case 5:
            _chrReg5 = M;
            break;
        case 6:
            _prgReg0 = M;
            break;
        case 7:
            _prgReg1 = M;
            break;
        default:
            assert(false); // Unreachable
        }
    }
    else if (address == 0xA000 && _mirroring != iNesFile::Mirroring::FourScreen)
    {
        if ((M & 0x1) == 0)
        {
            _mirroring = iNesFile::Mirroring::Vertical;
        }
        else
        {
            _mirroring = iNesFile::Mirroring::Horizontal;
        }
    }
    else if (address == 0xA001)
    {
        _prgRamEnabled = !!(M & 0x80);
        _prgRamWriteProtect = !!(M & 0x40);
    }
    else if (address == 0xC000)
    {
        _irqReloadValue = M;
    }
    else if (address == 0xC001)
    {
        _irqCounter = 0;
    }
    else if (address == 0xE000)
    {
        _irqEnabled = false;
        _irqPending = false;
    }
    else if (address == 0xE001)
    {
        _irqEnabled = true;
    }
}

void MMC3::SetPpuAddress(uint16_t address)
{
    MapperBase::SetPpuAddress(address);
    ClockIRQCounter(address);
}

uint8_t MMC3::PpuRead()
{
    return MMC3::PpuPeek(_ppuAddress);
}

void MMC3::PpuWrite(uint8_t M)
{
    if (_ppuAddress >= 0x2000)
    {
        DefaultNameTableWrite(M, _ppuAddress);
    }
}

uint8_t MMC3::PpuPeek(uint16_t address)
{
    if (address < 0x2000)
    {
        if (_chrMode == 0)
        {
            if (address < 0x0800)
            {
                return _chrRom[address + ((_chrReg0 >> 1) * 0x0800)];
            }
            else if (address < 0x1000)
            {
                return _chrRom[(address - 0x0800) + ((_chrReg1 >> 1) * 0x0800)];
            }
            else if (address < 0x1400)
            {
                return _chrRom[(address - 0x1000) + (_chrReg2 * 0x0400)];
            }
            else if (address < 0x1800)
            {
                return _chrRom[(address - 0x1400) + (_chrReg3 * 0x0400)];
            }
            else if (address < 0x1C00)
            {
                return _chrRom[(address - 0x1800) + (_chrReg4 * 0x0400)];
            }
            else
            {
                return _chrRom[(address - 0x1C00) + (_chrReg5 * 0x0400)];
            }
        }
        else
        {
            if (address < 0x0400)
            {
                return _chrRom[address + (_chrReg2 * 0x0400)];
            }
            else if (address < 0x0800)
            {
                return _chrRom[(address - 0x0400) + (_chrReg3 * 0x0400)];
            }
            else if (address < 0x0C00)
            {
                return _chrRom[(address - 0x0800) + (_chrReg4 * 0x0400)];
            }
            else if (address < 0x1000)
            {
                return _chrRom[(address - 0x0C00) + (_chrReg5 * 0x0400)];
            }
            else if (address < 0x1800)
            {
                return _chrRom[(address - 0x1000) + ((_chrReg0 >> 1) * 0x0800)];
            }
            else
            {
                return _chrRom[(address - 0x1800) + ((_chrReg1 >> 1) * 0x0800)];
            }
        }
    }
    else
    {
        return DefaultNameTableRead(address);
    }
}

bool MMC3::CheckIRQ()
{
    return _irqPending;
}

void MMC3::ClockIRQCounter(uint16_t address)
{
    if ((address & 0x1000) != 0)
    {
        if (!_lastReadHigh && _nes.GetPpu().GetClock() - _lastRiseCycle >= 16)
        {
            if (_irqCounter == 0)
            {
                _irqCounter = _irqReloadValue;
            }
            else
            {
                _irqCounter--;
            }        

            if (_irqEnabled && _irqCounter == 0)
            {
                _irqPending = true;
            }  
        }

        if (!_lastReadHigh)
        {
            _lastRiseCycle = _nes.GetPpu().GetClock();
        }
        
        _lastReadHigh = true;   
    }
    else
    {
        _lastReadHigh = false;
    }
}