#ifndef SXROM_H_
#define SXROM_H_

#include <boost/cstdint.hpp>
#include <boost/iostreams/device/mapped_file.hpp>

#include "cart.h"
#include "../clock.h"

class NES;

class SXROM : public Cart
{
    boost::iostreams::mapped_file_source& file;
    boost::iostreams::mapped_file* save;

    Clock& clock;
    NES& nes;

    unsigned long long lastWriteCycle;
    uint8_t counter;
    uint8_t tempRegister;
    uint8_t controlRegister;
    uint8_t chrRegister1;
    uint8_t chrRegister2;
    uint8_t prgRegister;

    int chrSize;
    int prgSize;

    int8_t* wram;
    int8_t* chrRam;
    const int8_t* prg;
    const int8_t* chr;

public:
    MirrorMode GetMirrorMode();

    uint8_t PrgRead(uint16_t address);
    void PrgWrite(uint8_t M, uint16_t address);

    uint8_t ChrRead(uint16_t address);
    void ChrWrite(uint8_t M, uint16_t address);

    SXROM(std::string& filename, Clock& clock, NES& nes);
    ~SXROM();
};



#endif /* SXROM_H_ */
