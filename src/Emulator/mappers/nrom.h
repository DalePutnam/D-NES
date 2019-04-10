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
    NROM(iNesFile& file);

    uint8_t CpuRead(uint16_t address) override;
    void CpuWrite(uint8_t M, uint16_t address) override;

protected:
    uint8_t ChrRead(uint16_t address) override;
    void ChrWrite(uint8_t M, uint16_t address) override;
    
private:
    uint8_t* _chr;
};
