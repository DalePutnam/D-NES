#ifndef SXROM_H_
#define SXROM_H_

#include "cart.h"
#include "boost/iostreams/device/mapped_file.hpp"

class NES;

class SXROM : public Cart
{
    boost::iostreams::mapped_file_source& file;
    boost::iostreams::mapped_file* save;
    NES& nes;

    unsigned long lastWriteCycle;
    unsigned char counter;
    unsigned char tempRegister;
    unsigned char controlRegister;
    unsigned char chrRegister1;
    unsigned char chrRegister2;
    unsigned char prgRegister;

    int chrSize;
    int prgSize;

    char* wram;
    char* chrRam;
    const char* prg;
    const char* chr;

public:
    MirrorMode GetMirrorMode();

    unsigned char PrgRead(unsigned short int address);
    void PrgWrite(unsigned char M, unsigned short int address);

    unsigned char ChrRead(unsigned short int address);
    void ChrWrite(unsigned char M, unsigned short int address);

    SXROM(std::string& filename, NES& nes);
    ~SXROM();
};



#endif /* SXROM_H_ */
