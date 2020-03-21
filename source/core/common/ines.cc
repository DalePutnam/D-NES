#include <fstream>

#include "nes_exception.h"
#include "ines.h"

// Read header
struct iNesHeader
{
    uint8_t signature[4];
    uint8_t prgSize;
    uint8_t chrSize;
    uint8_t flags6;
    uint8_t flags7;
    uint8_t flags8;
    uint8_t flags9;
    uint8_t flags10;
    uint8_t flags11;
    uint8_t flags12;
    uint8_t flags13;
    uint8_t flags14;
    uint8_t flags15;
};

iNesFile::iNesFile(const std::string& file)
{
    // Open rom file
    std::ifstream romStream(file.c_str(), std::ifstream::in | std::ifstream::binary);
    if (!romStream.good())
    {
        throw NesException("iNesFile", "Unable to open ROM.");
    }

    // Read header
    iNesHeader header;
    romStream.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!romStream.good())
    {
        throw NesException("iNesFile", "Unable read ROM header.");
    }

    // Check header signature
    if (!(header.signature[0] == 'N' &&
          header.signature[1] == 'E' &&
          header.signature[2] == 'S' &&
          header.signature[3] == '\x1A'))
    {
        throw NesException("iNesFile", "Invalid ROM header.");
    }

    // Check version
    if ((header.flags7 & 0xC) == 0x08)
    {
        _version = Version::iNes2_0;
    }
    else if ((header.flags7 & 0xC) == 0x00 &&
             (header.flags12 == 0 && header.flags13 == 0 && header.flags14 == 0 && header.flags15 == 0))
    {
        _version = Version::iNes1_0;
    }
    else
    {
        _version = Version::Archaic;
    }

    // Skip trainer if present
    if ((header.flags6 & 0x4) != 0)
    {
        romStream.seekg(512);
    }

    // Get ROMs
    if (_version == Version::iNes2_0)
    {
        uint32_t romSize;
        std::unique_ptr<uint8_t[]> rom;

        romSize = ((header.flags9 & 0xF) << 8 | header.prgSize) * 0x4000;
        rom = std::make_unique<uint8_t[]>(romSize);

        romStream.read(reinterpret_cast<char*>(rom.get()), romSize);
        if (!romStream.good())
        {
            throw NesException("iNesFile", "Unable to read PRG ROM.");
        }

        _prgRom.reset(rom.release());
        _prgRomSize = romSize;

        romSize = ((header.flags9 & 0xF0) << 4 | header.chrSize) * 0x2000;
        rom = std::make_unique<uint8_t[]>(romSize);

        romStream.read(reinterpret_cast<char*>(rom.get()), romSize);
        if (!romStream.good())
        {
            throw NesException("iNesFile", "Unable to read CHR ROM.");
        }

        _chrRom.reset(rom.release());
        _chrRomSize = romSize;

        uint32_t currentPosition = romStream.tellg();
        romStream.seekg(0, romStream.end);
        romSize = static_cast<uint32_t>(romStream.tellg()) - currentPosition;
        romStream.seekg(currentPosition, romStream.beg);

        rom = std::make_unique<uint8_t[]>(romSize);

        romStream.read(reinterpret_cast<char*>(rom.get()), romSize);
        if (!romStream.good() && !romStream.eof())
        {
            throw NesException("iNesFile", "Unable to read miscellaneous ROM.");
        }

        _miscRom.reset(rom.release());
        _miscRomSize = romSize;
        _numMiscRoms = header.flags14 & 0x3;
    }
    else
    {
        uint32_t romSize;
        std::unique_ptr<uint8_t[]> rom;

        romSize = header.prgSize * 0x4000;
        rom = std::make_unique<uint8_t[]>(romSize);

        romStream.read(reinterpret_cast<char*>(rom.get()), romSize);
        if (!romStream.good())
        {
            throw NesException("iNesFile", "Unable to read PRG ROM.");
        }

        _prgRom.reset(rom.release());
        _prgRomSize = romSize;

        romSize = header.chrSize * 0x2000;
        rom = std::make_unique<uint8_t[]>(romSize);

        romStream.read(reinterpret_cast<char*>(rom.get()), romSize);
        if (!romStream.good())
        {
            throw NesException("iNesFile", "Unable to read CHR ROM.");
        }

        _chrRom.reset(rom.release());
        _chrRomSize = romSize;

        _miscRomSize = 0;
        _numMiscRoms = 0;
    }

    // Get mapper number from header
    if (_version == Version::iNes2_0)
    {
        _mapperNumber = ((header.flags8 & 0xF) << 8) | (header.flags7 & 0xF0) | (header.flags6 >> 4);
        _subMapperNumber = header.flags9 >> 4;
    }
    else if (_version == Version::iNes1_0)
    {
        _mapperNumber = (header.flags7 & 0xF0) | (header.flags6 >> 4);
        _subMapperNumber = 0;
    }
    else
    {
        _mapperNumber = (header.flags6 >> 4);
        _subMapperNumber = 0;
    }

    // Get Battery backed flag
    _hasNonVolatileMemory = (header.flags6 & 0x2) != 0;

    // Get (NV)Ram sizes from header
    if (_version == Version::iNes2_0)
    {
        _prgRamSize = 64 << (header.flags10 & 0xF);
        _prgNvRamSize = 64 << (header.flags10 >> 4);
        _chrRamSize = 64 << (header.flags11 & 0xF);
        _chrNvRamSize = 64 << (header.flags11 >> 4);
    }
    else
    {
        uint32_t ramSize;
        if (_version == Version::iNes1_0)
        {
            ramSize = header.flags8 == 0 ? 0x2000 : header.flags8 * 0x2000;
        }
        else
        {
            ramSize = 0x2000;
        }
        
        if (_hasNonVolatileMemory)
        {
            _prgNvRamSize = ramSize;
            _prgRamSize = 0;
        }
        else
        {
            _prgRamSize = ramSize;
            _prgNvRamSize = 0;
        }

        _chrRamSize = header.chrSize == 0 ? 0x2000 : 0;
        _chrNvRamSize = 0;
    }

    if ((header.flags6 & 0x8) != 0)
    {
        _mirroring = Mirroring::FourScreen;
    }
    else if ((header.flags6 & 0x1) != 0)
    {
        _mirroring = Mirroring::Vertical;
    }
    else
    {
        _mirroring = Mirroring::Horizontal;
    }
}

iNesFile::Version iNesFile::GetVersion()
{
    return _version;
}

uint16_t iNesFile::GetMapperNumber()
{
    return _mapperNumber;
}

uint16_t iNesFile::GetSubmapperNumber()
{
    return _subMapperNumber;
}

uint32_t iNesFile::GetPrgRomSize()
{
    return _prgRomSize;
}

uint32_t iNesFile::GetChrRomSize()
{
    return _chrRomSize;
}

uint32_t iNesFile::GetMiscRomSize()
{
    return _miscRomSize;
}

uint32_t iNesFile::GetNumMiscRoms()
{
    return _numMiscRoms;
}

uint32_t iNesFile::GetPrgRamSize()
{
    return _prgRamSize;
}

uint32_t iNesFile::GetPrgNvRamSize()
{
    return _prgNvRamSize;
}

uint32_t iNesFile::GetChrRamSize()
{
    return _chrRamSize;
}

uint32_t iNesFile::GetChrNvRamSize()
{
    return _chrNvRamSize;
}

bool iNesFile::HasNonVolatileMemory()
{
    return _hasNonVolatileMemory;
}

iNesFile::Mirroring iNesFile::GetMirroring()
{
    return _mirroring;
}

std::unique_ptr<uint8_t[]> iNesFile::GetPrgRom()
{
    return std::move(_prgRom);
}

std::unique_ptr<uint8_t[]> iNesFile::GetChrRom()
{
    return std::move(_chrRom);
}

std::unique_ptr<uint8_t[]> iNesFile::GetMiscRom()
{
    return std::move(_miscRom);
}