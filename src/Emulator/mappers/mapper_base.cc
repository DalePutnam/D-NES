#include <exception>

#include "../cpu.h"
#include "mapper_base.h"

MapperBase::MapperBase(boost::iostreams::mapped_file_source* file)
    : prgSize(0)
    , prg(nullptr)
    , chrSize(0)
    , chr(nullptr)
    , cpu(nullptr)
    , romFile(file)
{
    if (romFile == nullptr)
    {
        throw std::runtime_error("MapperBase: ROM file is null.");
    }

    if (!romFile->is_open())
    {
        throw std::runtime_error("MapperBase: ROM file is not open.");
    }
}

MapperBase::~MapperBase()
{
    romFile->close();
    delete romFile;
}

void MapperBase::AttachCPU(CPU* cpu)
{
    this->cpu = cpu;
}
