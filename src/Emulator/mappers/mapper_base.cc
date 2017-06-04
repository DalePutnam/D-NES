#include <fstream>
#include <exception>
#include <cstring>

#include "../cpu.h"
#include "mapper_base.h"

using namespace std;

MapperBase::MapperBase(const string& fileName, const string& saveDir)
    : Prg(nullptr)
    , Chr(nullptr)
    , ChrRam(nullptr)
    , Wram(nullptr)
    , HasSaveMem(false)
    , SaveDir(saveDir)
    , Cpu(nullptr)
{
    size_t indexForwardSlash = fileName.rfind('/');
    size_t indexBackSlash = fileName.rfind('\\');

    if (indexForwardSlash == string::npos && indexBackSlash == string::npos)
    {
        GameName = fileName;
    }
    else if (indexForwardSlash != string::npos)
    {
        GameName = fileName.substr(indexForwardSlash + 1);
    }
    else if (indexBackSlash != string::npos)
    {
        GameName = fileName.substr(indexBackSlash + 1);
    }
    else
    {
        GameName = fileName.substr(((indexForwardSlash > indexBackSlash) ? indexForwardSlash : indexBackSlash) + 1);
    }

    size_t indexExtension = GameName.find(".nes");
    if (indexExtension != string::npos)
    {
        GameName = GameName.substr(0, indexExtension);
    }

    ifstream romStream(fileName.c_str(), ifstream::in | ifstream::binary);

    if (romStream.good())
    {
        char header[16];
        romStream.read(header, 16);

        PrgSize = header[4] * 0x4000;
        ChrSize = header[5] * 0x2000;
        WramSize = header[8] ? header[8] * 0x2000 : 0x2000;

        if (header[6] & 0x1)
        {
            Mirroring = Cart::VERTICAL;
        }
        else
        {
            Mirroring = Cart::HORIZONTAL;
        }

        if (PrgSize == 0)
        {
            throw runtime_error("MapperBase: Failed to load ROM, invalid PRG size");
        }

        if (header[6] & 0x2)
        {
            HasSaveMem = true;
            string saveFile = saveDir + "/" + GameName + ".sav";
            ifstream saveStream(saveFile.c_str(), ifstream::in | ifstream::binary);

            if (saveStream.good())
            {
                char* buf = new char[WramSize];
                saveStream.read(buf, WramSize);
                Wram = reinterpret_cast<uint8_t*>(buf);
            }
            else
            {
                Wram = new uint8_t[WramSize];
                memset(Wram, 0, sizeof(uint8_t) * WramSize);
            }
        }
        else
        {
            Wram = new uint8_t[WramSize];
            memset(Wram, 0, sizeof(uint8_t) * WramSize);
        }

        if (header[6] & 0x4)
        {
            // Skip trainer if present
            romStream.seekg(0x200, romStream.cur);
        }

        char* buf = new char[PrgSize];
        romStream.read(buf, PrgSize);
        Prg = reinterpret_cast<uint8_t*>(buf);

        if (ChrSize != 0)
        {
            buf = new char[ChrSize];
            romStream.read(buf, ChrSize);
            Chr = reinterpret_cast<uint8_t*>(buf);
        }
        else
        {
            ChrRam = new uint8_t[0x2000];
            memset(ChrRam, 0, sizeof(uint8_t) * 0x2000);
        }
    }
    else
    {
        throw runtime_error("MapperBase: Unable to open " + fileName);
    }
}

MapperBase::~MapperBase()
{
    if (HasSaveMem)
    {
        string saveFile = SaveDir + "/" + GameName + ".sav";
        ofstream saveStream(saveFile.c_str(), ifstream::out | ifstream::binary);

        if (saveStream.good())
        {
            char* buf = reinterpret_cast<char*>(Wram);
            saveStream.write(buf, WramSize);
        }
    }

    delete[] Wram;
    delete[] Prg;

    if (ChrSize != 0)
    {
        delete[] Chr;
    }
    else
    {
        delete[] ChrRam;
    }
}

const string& MapperBase::GetGameName()
{
    return GameName;
}

void MapperBase::SetSaveDirectory(const std::string& saveDir)
{
    std::lock_guard<std::mutex> lock(MapperMutex);

    SaveDir = saveDir;
}

void MapperBase::AttachCPU(CPU* cpu)
{
    Cpu = cpu;
}
