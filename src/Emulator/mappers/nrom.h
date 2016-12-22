/*
 * nrom.h
 *
 *  Created on: Jul 8, 2014
 *      Author: Dale
 */

#pragma once

#include "mapper_base.h"

class NROM : public MapperBase
{
public:
    NROM(boost::iostreams::mapped_file_source* file);
    ~NROM();

    Cart::MirrorMode GetMirrorMode() override;

    uint8_t PrgRead(uint16_t address) override;
    void PrgWrite(uint8_t M, uint16_t address) override;

    uint8_t ChrRead(uint16_t address) override;
    void ChrWrite(uint8_t M, uint16_t address) override;

private:
    Cart::MirrorMode mirroring;
    int8_t* chrRam;
};
