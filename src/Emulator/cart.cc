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
#include "file.h"
#include "nes_exception.h"
#include "mappers/mapper_base.h"
#include "mappers/nrom.h"
#include "mappers/sxrom.h"
#include "mappers/uxrom.h"
#include "mappers/cnrom.h"
#include "mappers/mmc3.h"

using namespace std;


Cart::Cart(const string& fileName)
{
    _gameName = file::stripExtension(file::getNameFromPath(fileName));

    iNesFile file(fileName);

    uint16_t mapperNumber = file.GetMapperNumber();

    switch (mapperNumber)
    {
    case 0x00:
        _mapper = std::make_unique<NROM>(file);
        break;
    case 0x01:
        _mapper = std::make_unique<SXROM>(file);
        break;
    case 0x02:
        _mapper = std::make_unique<UXROM>(file);
        break;
    case 0x03:
        _mapper = std::make_unique<CNROM>(file);
        break;
    case 0x04:
        _mapper = std::make_unique<MMC3>(file);
        break;
    default:
        ostringstream oss;
        oss << "Cart: Mapper " << static_cast<int>(mapperNumber) << " specified by " << fileName << " does not exist or is not supported.";
        throw NesException("Cart", oss.str());
    }
}

Cart::~Cart()
{
}

const string& Cart::GetGameName()
{
    return _gameName;
}

void Cart::SetSaveDirectory(const std::string& saveDir)
{
    _saveDirectory = saveDir;
}

void Cart::AttachCPU(CPU* cpu)
{
    _mapper->AttachCPU(cpu);
}

void Cart::AttachPPU(PPU* ppu)
{
    _mapper->AttachPPU(ppu);
}

Cart::MirrorMode Cart::GetMirrorMode()
{
    return _mapper->GetMirrorMode();
}

void Cart::SaveNativeSave()
{
    std::string saveFile = file::createFullPath(_gameName, "sav", _saveDirectory);
    std::ofstream saveStream(saveFile.c_str(), std::ofstream::out | std::ofstream::binary);

    if (saveStream.good())
    {
        _mapper->SaveNativeSave(saveStream);
    }
    else
    {
        throw NesException("Cart", "Failed to open save file for writing");
    }
}

void Cart::LoadNativeSave()
{
    std::string saveFile = file::createFullPath(_gameName, "sav", _saveDirectory);
    std::ifstream saveStream(saveFile.c_str(), std::ifstream::in | std::ofstream::binary);

    if (saveStream.good())
    {
        _mapper->LoadNativeSave(saveStream);
    }

    // If the file didn't open just assume save data doesn't exist
}

uint8_t Cart::PrgRead(uint16_t address)
{
    return _mapper->PrgRead(address);
}

void Cart::PrgWrite(uint8_t M, uint16_t address)
{
    _mapper->PrgWrite(M, address);
}

uint8_t Cart::ChrRead(uint16_t address)
{
    return _mapper->ChrRead(address);
}

void Cart::ChrWrite(uint8_t M, uint16_t address)
{
    _mapper->ChrWrite(M, address);
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