/*
 * mapper.cc
 *
 *  Created on: Jul 8, 2014
 *      Author: Dale
 */
#include <iostream>
#include <fstream>
#include <sstream>

#include "cart.h"
#include "nrom.h"
#include "sxrom.h"

Cart& Cart::Create(std::string& filename, Clock& clock, NES& nes)
{
    // Open file stream to ROM file
    std::ifstream rom(filename.c_str(), std::ifstream::in | std::ifstream::binary);

    if (!rom.fail())
    {
        // Sanity check (Any iNES ROM should start with NES followed by MS-DOS EOF)
        char header[4];
        rom.read(header, 4);

        // NOTE: The last && in the if statement and rom.clear() are specific to windows as they
        // are due to an MS-DOS EOF present in the ROM header, on other platforms this
        // may need to be revised.
        if (header[0] == 'N' && header[1] == 'E' && header[2] == 'S' && header[3] == 0x1A)
        {
            // Read Bytes 6 and 7 to determine the mapper number
            rom.clear();
            rom.seekg(0x06, rom.beg);
            unsigned char flags6 = rom.get();
            unsigned char flags7 = rom.get();
            unsigned char mapper_number = (flags7 & 0xF0) | (flags6 >> 4);
            rom.close();

            switch (mapper_number)
            {
            case 0x00:
                return *new NROM(filename);
            case 0x01:
                return *new SXROM(filename, clock, nes);
            default:
                std::ostringstream oss;
                oss << "Mapper " << (int)mapper_number << " specified by " << filename << " does not exist.";
                throw oss.str();
            }
        }
        else
        {
            throw std::string("Invalid ROM format");
        }
    }
    else
    {
        std::ostringstream oss;
        oss << "Unable to open " << filename;
        throw oss.str();
    }

    throw "WTF";
}

Cart::~Cart() {}


