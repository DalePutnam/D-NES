/*
 * mapper.cc
 *
 *  Created on: Jul 8, 2014
 *      Author: Dale
 */

#include <fstream>
#include <sstream>
#include <exception>
#include <cstring>

#include "cart.h"
#include "ines.h"
#include "mappers/mapper_base.h"
#include "mappers/nrom.h"
#include "mappers/mmc1.h"
#include "mappers/uxrom.h"
#include "mappers/cnrom.h"
#include "mappers/mmc3.h"

#include "error_handling.h"

Cart::Cart(NES& nes)
    : _nes(nes)
{
}

Cart::~Cart()
{
}

void Cart::Initialize(const char* romFile, const char* saveFile)
{
    try
    {
        _iNesFile = std::make_unique<iNesFile>(romFile);    
    }
    catch (NesException& ex)
    {
        SPDLOG_LOGGER_ERROR(_nes.GetLogger(), "{}", ex.what());
    }

    _saveFile = saveFile;

    switch (_iNesFile->GetMapperNumber())
    {
    case 0x00:
        _mapper = std::make_unique<NROM>(_nes, *_iNesFile);
        break;
    case 0x01:
        _mapper = std::make_unique<MMC1>(_nes, *_iNesFile);
        break;
    case 0x02:
        _mapper = std::make_unique<UXROM>(_nes, *_iNesFile);
        break;
    case 0x03:
        _mapper = std::make_unique<CNROM>(_nes, *_iNesFile);
        break;
    case 0x04:
        _mapper = std::make_unique<MMC3>(_nes, *_iNesFile);
        break;
    default:
        SPDLOG_LOGGER_ERROR(_nes.GetLogger(), "Mapper {} is unsupported", _iNesFile->GetMapperNumber());
        throw NesException(ERROR_UNSUPPORTED_MAPPER);
    }
}

void Cart::SaveNvRam()
{
    if (_saveFile.empty())
    {
        SPDLOG_LOGGER_INFO(_nes.GetLogger(), "No save file specified, save data will not be written");
        return;
    }

    std::ofstream saveStream(_saveFile.c_str(), std::ofstream::out | std::ofstream::binary);

    if (!saveStream.good())
    {
        SPDLOG_LOGGER_ERROR(_nes.GetLogger(), "Could not open save file {}, save data not written", _saveFile);
        throw NesException(ERROR_FAILED_TO_SAVE_NV_RAM);
    }

    _mapper->SaveNativeSave(saveStream);
}

void Cart::LoadNvRam()
{
    if (_saveFile.empty())
    {
        SPDLOG_LOGGER_INFO(_nes.GetLogger(), "No save file specified, save data will not be read");
        return;
    }

    std::ifstream saveStream(_saveFile.c_str(), std::ifstream::in | std::ofstream::binary);

    if (!saveStream.good())
    {
        SPDLOG_LOGGER_INFO(_nes.GetLogger(), "Could not open save file {}, assuming no save data yet exists", _saveFile);
    }
    
    _mapper->LoadNativeSave(saveStream);
}

uint8_t Cart::CpuRead(uint16_t address)
{
    return _mapper->CpuRead(address);
}

void Cart::CpuWrite(uint8_t M, uint16_t address)
{
    _mapper->CpuWrite(M, address);
}

void Cart::SetPpuAddress(uint16_t address)
{
    _mapper->SetPpuAddress(address);
}

uint8_t Cart::PpuRead()
{
    return _mapper->PpuRead();
}

void Cart::PpuWrite(uint8_t M)
{
    _mapper->PpuWrite(M);
}

uint8_t Cart::PpuPeek(uint16_t address)
{
    return _mapper->PpuPeek(address);
}

StateSave::Ptr Cart::SaveState()
{
    StateSave::Ptr state = StateSave::New();
    _mapper->SaveState(state);

    return state;
}

void Cart::LoadState(const StateSave::Ptr& state)
{
    _mapper->LoadState(state);
}

bool Cart::CheckIRQ()
{
    return _mapper->CheckIRQ();
}