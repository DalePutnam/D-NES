/*
 * mapper.cc
 *
 *  Created on: Jul 8, 2014
 *      Author: Dale
 */

#include <vector>
#include <sstream>
#include <exception>
#include <boost/algorithm/string.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

#include "cart.h"
#include "mappers/mapper_base.h"
#include "mappers/nrom.h"
#include "mappers/sxrom.h"

Cart::Cart(const std::string& filename)
{
    boost::iostreams::mapped_file_source* file = new boost::iostreams::mapped_file_source(filename);

    std::vector<std::string> stringList;
    boost::algorithm::split(stringList, filename, boost::is_any_of("\\/"));
    std::string gameName = stringList.back().substr(0, stringList.back().length() - 4);

    if (file->is_open())
    {
        const char* data = file->data();

        if (data[0] == 'N' && data[1] == 'E' && data[2] == 'S' && data[3] == 0x1A)
        {
            uint8_t flags6 = data[6];
            uint8_t flags7 = data[7];
            uint8_t mapper_number = (flags7 & 0xF0) | (flags6 >> 4);

            switch (mapper_number)
            {
            case 0x00:
                mapper = new NROM(file);
                break;
            case 0x01:
                mapper = new SXROM(file, gameName);
                break;
            default:
                std::ostringstream oss;
                oss << "Cart: Mapper " << static_cast<int>(mapper_number) << " specified by " << filename << " does not exist or is not supported.";
                file->close();
                delete file;
                throw std::runtime_error(oss.str());
            }
        }
        else
        {
            throw std::runtime_error("Cart: Invalid ROM format");
        }
    }
    else
    {
        throw std::runtime_error("Cart: Unable to open " + filename);
    }
}

Cart::~Cart()
{
    delete mapper;
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

