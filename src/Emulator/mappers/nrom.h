/*
 * nrom.h
 *
 *  Created on: Jul 8, 2014
 *      Author: Dale
 */

#ifndef NROM_H_
#define NROM_H_

#include <boost/iostreams/device/mapped_file.hpp>

#include "cart.h"

class NROM : public Cart
{
    MirrorMode mirroring;
    int chrSize;
    int prgSize;

    const int8_t* prg;
    const int8_t* chr;
    int8_t* chrRam;

public:
    MirrorMode GetMirrorMode();

    uint8_t PrgRead(uint16_t address);
    void PrgWrite(uint8_t M, uint16_t address);

    uint8_t ChrRead(uint16_t address);
    void ChrWrite(uint8_t M, uint16_t address);

    NROM(const std::string& filename);
    ~NROM();
};



#endif /* NROM_H_ */
