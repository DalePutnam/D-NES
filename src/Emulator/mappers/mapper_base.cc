#include <fstream>
#include <exception>
#include <cstring>

#include "cpu.h"
#include "mapper_base.h"

using namespace std;

#if defined(_WIN32)
static const std::string FILE_SEPARATOR = "\\";
#elif defined(__linux)
static const std::string FILE_SEPARATOR = "/";
#endif

static std::string getGameNameFromPath(const std::string& gamePath)
{
    std::string gameName;

    size_t indexSeparator = gamePath.rfind(FILE_SEPARATOR);
    if (indexSeparator == std::string::npos)
    {
        gameName = gamePath;
    }
    else
    {
        gameName = gamePath.substr(indexSeparator + 1);
    }

    size_t indexExtension = gameName.find(".");
    if (indexExtension != std::string::npos)
    {
        gameName = gameName.substr(0, indexExtension);
    }

    return gameName;
}

static const string NATIVE_SAVE_EXTENSION = "sav";
static std::string createNativeSavePath(const std::string& saveDirectory, const std::string& gameName)
{
    if (saveDirectory.empty())
    {
        return gameName + "." + NATIVE_SAVE_EXTENSION;
    }
    else
    {
        return saveDirectory + FILE_SEPARATOR + gameName + "." + NATIVE_SAVE_EXTENSION;
    }
}

MapperBase::MapperBase(const string& fileName, const string& saveDir)
    : Prg(nullptr)
    , Chr(nullptr)
    , Wram(nullptr)
    , HasSaveMem(false)
    , Cpu(nullptr)
{
    GameName = getGameNameFromPath(fileName);
    SaveFile = createNativeSavePath(saveDir, GameName);

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
            ifstream saveStream(SaveFile.c_str(), ifstream::in | ifstream::binary);

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
            Chr = new uint8_t[0x2000];
            memset(Chr, 0, sizeof(uint8_t) * 0x2000);
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
        ofstream saveStream(SaveFile.c_str(), ifstream::out | ifstream::binary);

        if (saveStream.good())
        {
            char* buf = reinterpret_cast<char*>(Wram);
            saveStream.write(buf, WramSize);
        }
    }

    delete[] Wram;
    delete[] Prg;
    delete[] Chr;
}

const string& MapperBase::GetGameName()
{
    return GameName;
}

void MapperBase::SetSaveDirectory(const std::string& saveDir)
{
    std::lock_guard<std::mutex> lock(MapperMutex);

    SaveFile = createNativeSavePath(saveDir, GameName);
}

void MapperBase::AttachCPU(CPU* cpu)
{
    Cpu = cpu;
}

int MapperBase::GetStateSize()
{
    int totalSize = 0;

    if (ChrSize == 0)
    {
        totalSize += 0x2000;
    }

    totalSize += WramSize == 0 ? 0x2000 : WramSize;

    return totalSize;
}

void MapperBase::SaveState(char* state)
{
    if (ChrSize == 0)
    {
        memcpy(state, Chr, sizeof(uint8_t) * 0x2000);
        state += sizeof(uint8_t) * 0x2000;
    }

    if (WramSize == 0)
    {
        memcpy(state, Wram, sizeof(uint8_t) * 0x2000);
    }
    else
    {
        memcpy(state, Wram, sizeof(uint8_t) * WramSize);
    }
}

void MapperBase::LoadState(const char* state)
{
    if (ChrSize == 0)
    {
        memcpy(Chr, state, sizeof(uint8_t) * 0x2000);
        state += sizeof(uint8_t) * 0x2000;
    }

    if (WramSize == 0)
    {
        memcpy(Wram, state, sizeof(uint8_t) * 0x2000);
    }
    else
    {
        memcpy(Wram, state, sizeof(uint8_t) * WramSize);
    }
}

Cart::MirrorMode MapperBase::GetMirrorMode()
{
	return Mirroring;
}