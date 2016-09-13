/*
 * mapper.cc
 *
 *  Created on: Jul 8, 2014
 *      Author: Dale
 */

#include <fstream>
#include <sstream>
#include <iostream>
#include <exception>

#include "cart.h"
#include "nrom.h"
#include "sxrom.h"

Cart& Cart::Create(const std::string& filename, CPU& cpu)
{
    // Open file stream to ROM file
    std::ifstream rom(filename.c_str(), std::ifstream::in | std::ifstream::binary);

    if (!rom.fail())
    {
        // Sanity check (Any iNES ROM should start with NES followed by MS-DOS EOF)
        int8_t header[4];
        rom.read(reinterpret_cast<char*>(header), 4);

        // NOTE: The last && in the if statement and rom.clear() are specific to windows as they
        // are due to an MS-DOS EOF present in the ROM header, on other platforms this
        // may need to be revised.
        if (header[0] == 'N' && header[1] == 'E' && header[2] == 'S' && header[3] == 0x1A)
        {
            // Read Bytes 6 and 7 to determine the mapper number
            rom.clear();
            rom.seekg(0x06, rom.beg);
            uint8_t flags6 = rom.get();
            uint8_t flags7 = rom.get();
            uint8_t mapper_number = (flags7 & 0xF0) | (flags6 >> 4);
            rom.close();

            switch (mapper_number)
            {
            case 0x00:
                return *new NROM(filename);
            case 0x01:
                return *new SXROM(filename, cpu);
            default:
                std::ostringstream oss;
                oss << "Mapper " << (int)mapper_number << " specified by " << filename << " does not exist.";
                throw std::runtime_error(oss.str());
            }
        }
        else
        {
            throw std::runtime_error("Invalid ROM format");
        }
    }
    else
    {
        std::ostringstream oss;
        oss << "Unable to open " << filename;
        throw std::runtime_error(oss.str());
    }

    throw std::runtime_error("WTF");
}

Cart::Cart(const std::string& filename)
	: file(*new boost::iostreams::mapped_file_source(filename))
{}

Cart::~Cart()
{
    file.close();
    delete &file;
}


