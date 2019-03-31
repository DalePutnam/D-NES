#pragma once

#include <string>
#include <memory>

class iNesFile
{
public:
    enum Version
    {
        Archaic,
        iNes,
        Nes20
    };

    enum Mirroring
    {   
        Vertical,
        Horizontal,
        FourScreen
    };

    explicit iNesFile(const std::string& file);
    ~iNesFile() = default;

    Version GetVersion();

    uint16_t GetMapperNumber();
    uint16_t GetSubmapperNumber();

    uint32_t GetPrgRomSize();
    uint32_t GetChrRomSize();
    uint32_t GetMiscRomSize();
    uint32_t GetNumMiscRoms();

    uint32_t GetPrgRamSize();
    uint32_t GetPrgNvRamSize();
    uint32_t GetChrRamSize();
    uint32_t GetChrNvRamSize();

    bool HasNonVolatileMemory();

    Mirroring GetMirroring();

    std::unique_ptr<uint8_t[]> GetPrgRom();
    std::unique_ptr<uint8_t[]> GetChrRom();
    std::unique_ptr<uint8_t[]> GetMiscRom();

private:
    Version _version;
    uint16_t _mapperNumber;
    uint16_t _subMapperNumber;

    uint32_t _prgRomSize;
    uint32_t _chrRomSize;
    uint32_t _miscRomSize;
    uint32_t _numMiscRoms;
    uint32_t _prgRamSize;
    uint32_t _prgNvRamSize;
    uint32_t _chrRamSize;
    uint32_t _chrNvRamSize;

    bool _hasNonVolatileMemory;

    Mirroring _mirroring;

    std::unique_ptr<uint8_t[]> _prgRom;
    std::unique_ptr<uint8_t[]> _chrRom;
    std::unique_ptr<uint8_t[]> _miscRom;
};