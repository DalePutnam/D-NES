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
    NROM(NES& nes, iNesFile& file);

    uint8_t CpuRead(uint16_t address) override;
    void CpuWrite(uint8_t M, uint16_t address) override;

    uint8_t PpuRead() override;
    void PpuWrite(uint8_t M) override;

    uint8_t PpuPeek(uint16_t address) override;
    
private:
    uint8_t* _chr;
};
