/*
 * mapper.h
 *
 *  Created on: Jul 8, 2014
 *      Author: Dale
 */

#ifndef MAPPER_H_
#define MAPPER_H_

#include <string>

#include "../clock.h"

class NES;

class Cart
{
public:
    static Cart& Create(std::string& filename, Clock& clock, NES& nes);

    enum MirrorMode
    {
        HORIZONTAL,
        VERTICAL,
        SINGLE_SCREEN_A,
        SINGLE_SCREEN_B
    };

    virtual MirrorMode GetMirrorMode() = 0;

    virtual unsigned char PrgRead(unsigned short int address) = 0;
    virtual void PrgWrite(unsigned char M, unsigned short int address) = 0;

    virtual unsigned char ChrRead(unsigned short int address) = 0;
    virtual void ChrWrite(unsigned char M, unsigned short int address) = 0;

    virtual ~Cart();
};




#endif /* MAPPER_H_ */
