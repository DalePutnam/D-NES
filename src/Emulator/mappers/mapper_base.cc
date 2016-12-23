#include <fstream>
#include <exception>

#include "../cpu.h"
#include "mapper_base.h"

using namespace std;

MapperBase::MapperBase(const string& fileName, const string& saveDir)
    : prg(nullptr)
    , chr(nullptr)
    , chrRam(nullptr)
    , wram(nullptr)
    , hasSaveMem(false)
    , cpu(nullptr)
    , saveDir(saveDir)
{
    size_t indexForwardSlash = fileName.rfind('/');
    size_t indexBackSlash = fileName.rfind('\\');

    if (indexForwardSlash != string::npos && indexForwardSlash > indexBackSlash)
    {
        gameName = fileName.substr(indexForwardSlash + 1);
    }
    else if (indexBackSlash != string::npos && indexBackSlash > indexForwardSlash)
    {
        gameName = fileName.substr(indexBackSlash + 1);
    }
    else
    {
        gameName = fileName;
    }

    size_t indexExtension = gameName.find(".nes");
    if (indexExtension != string::npos)
    {
        gameName = gameName.substr(0, indexExtension);
    }

    ifstream romStream(fileName.c_str(), ifstream::in | ifstream::binary);

    if (romStream.good())
    {
        char header[16];
        romStream.read(header, 16);

        prgSize = header[4] * 0x4000;
        chrSize = header[5] * 0x2000;
        wramSize = header[8] ? header[8] * 0x2000 : 0x2000;

        if (header[6] & 0x1)
        {
            mirroring = Cart::VERTICAL;
        }
        else
        {
            mirroring = Cart::HORIZONTAL;
        }

        if (prgSize == 0)
        {
            throw runtime_error("MapperBase: Failed to load ROM, invalid PRG size");
        }

        if (header[6] & 0x2)
        {
            hasSaveMem = true;
            string saveFile = saveDir + "/" + gameName + ".sav";
            ifstream saveStream(saveFile.c_str(), ifstream::in | ifstream::binary);

            if (saveStream.good())
            {
                char* buf = new char[wramSize];
                saveStream.read(buf, wramSize);
                wram = reinterpret_cast<uint8_t*>(buf);
            }
            else
            {
                wram = new uint8_t[wramSize];
                memset(wram, 0, sizeof(uint8_t) * wramSize);
            }
        }
        else
        {
            wram = new uint8_t[wramSize];
            memset(wram, 0, sizeof(uint8_t) * wramSize);
        }

        if (header[6] & 0x4)
        {
            // Skip trainer if present
            romStream.seekg(0x200, romStream.cur);
        }

        char* buf = new char[prgSize];
        romStream.read(buf, prgSize);
        prg = reinterpret_cast<uint8_t*>(buf);

        if (chrSize != 0)
        {
            buf = new char[chrSize];
            romStream.read(buf, chrSize);
            chr = reinterpret_cast<uint8_t*>(buf);
        }
        else
        {
            chrRam = new uint8_t[0x2000];
            memset(chrRam, 0, sizeof(uint8_t) * 0x2000);
        }
    }
    else
    {
        throw runtime_error("MapperBase: Unable to open " + fileName);
    }
}

MapperBase::~MapperBase()
{
    if (hasSaveMem)
    {
        string saveFile = saveDir + "/" + gameName + ".sav";
        ofstream saveStream(saveFile.c_str(), ifstream::out | ifstream::binary);

        if (saveStream.good())
        {
            char* buf = reinterpret_cast<char*>(wram);
            saveStream.write(buf, wramSize);
        }
    }
}

const string& MapperBase::GetGameName()
{
    return gameName;
}

void MapperBase::AttachCPU(CPU* cpu)
{
    this->cpu = cpu;
}
