/*
 * mapper.cc
 *
 *  Created on: Jul 8, 2014
 *      Author: Dale
 */

#include <fstream>
#include <sstream>
#include <exception>

#include "cart.h"
#include "mappers/mapper_base.h"
#include "mappers/nrom.h"
#include "mappers/sxrom.h"

using namespace std;

Cart::Cart(const string& fileName, const string& saveDir)
{
    ifstream romStream(fileName.c_str(), std::ifstream::in | std::ifstream::binary);

    if (romStream.good())
    {
        char header[16];
        romStream.read(header, 16);
        romStream.close();

        if (header[0] == 'N' && header[1] == 'E' && header[2] == 'S' && header[3] == '\x1A')
        {
            uint8_t flags6 = header[6];
            uint8_t flags7 = header[7];
            uint8_t mapperNumber = (flags7 & 0xF0) | (flags6 >> 4);

            switch (mapperNumber)
            {
            case 0x00:
                Mapper = new NROM(fileName, saveDir);
                break;
            case 0x01:
                Mapper = new SXROM(fileName, saveDir);
                break;
            default:
                ostringstream oss;
                oss << "Cart: Mapper " << static_cast<int>(mapperNumber) << " specified by " << fileName << " does not exist or is not supported.";
                throw runtime_error(oss.str());
            }
        }
		else
		{
			throw runtime_error("Cart: Unable to load " + fileName + ": Bad header");
		}
    }
    else
    {
        throw runtime_error("Cart: Unable to open " + fileName + ": " + strerror(errno));
    }

}

Cart::~Cart()
{
    delete Mapper;
}

const string& Cart::GetGameName()
{
    return Mapper->GetGameName();
}

void Cart::SetSaveDirectory(const std::string& saveDir)
{
    Mapper->SetSaveDirectory(saveDir);
}

void Cart::AttachCPU(CPU* cpu)
{
    Mapper->AttachCPU(cpu);
}

Cart::MirrorMode Cart::GetMirrorMode()
{
    return Mapper->GetMirrorMode();
}

uint8_t Cart::PrgRead(uint16_t address)
{
    return Mapper->PrgRead(address);
}

void Cart::PrgWrite(uint8_t M, uint16_t address)
{
    Mapper->PrgWrite(M, address);
}

uint8_t Cart::ChrRead(uint16_t address)
{
    return Mapper->ChrRead(address);
}

void Cart::ChrWrite(uint8_t M, uint16_t address)
{
    Mapper->ChrWrite(M, address);
}

int Cart::GetStateSize()
{
    return Mapper->GetStateSize();
}

void Cart::SaveState(char* state)
{
    Mapper->SaveState(state);
}

void Cart::LoadState(const char* state)
{
    Mapper->LoadState(state);
}
