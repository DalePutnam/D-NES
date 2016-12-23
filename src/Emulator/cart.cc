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
                mapper = new NROM(fileName, saveDir);
                break;
            case 0x01:
                mapper = new SXROM(fileName, saveDir);
                break;
            default:
                ostringstream oss;
                oss << "Cart: Mapper " << static_cast<int>(mapperNumber) << " specified by " << fileName << " does not exist or is not supported.";
                throw runtime_error(oss.str());
            }
        }
    }
    else
    {
        throw runtime_error("Cart: Unable to open " + fileName);
    }

}

Cart::~Cart()
{
    delete mapper;
}

const string& Cart::GetGameName()
{
    return mapper->GetGameName();
}

void Cart::AttachCPU(CPU* cpu)
{
    mapper->AttachCPU(cpu);
}

Cart::MirrorMode Cart::GetMirrorMode()
{
    return mapper->GetMirrorMode();
}

uint8_t Cart::PrgRead(uint16_t address)
{
    return mapper->PrgRead(address);
}

void Cart::PrgWrite(uint8_t M, uint16_t address)
{
    mapper->PrgWrite(M, address);
}

uint8_t Cart::ChrRead(uint16_t address)
{
    return mapper->ChrRead(address);
}

void Cart::ChrWrite(uint8_t M, uint16_t address)
{
    mapper->ChrWrite(M, address);
}

