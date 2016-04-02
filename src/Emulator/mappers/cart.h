/*
 * mapper.h
 *
 *  Created on: Jul 8, 2014
 *      Author: Dale
 */

#ifndef MAPPER_H_
#define MAPPER_H_

#include <string>
#include <boost/iostreams/device/mapped_file.hpp>

class NES;
class CPU;

class Cart
{
protected:
    boost::iostreams::mapped_file_source& file;

public:
    static Cart& Create(const std::string& filename, NES& nes, CPU& cpu);

    enum MirrorMode
    {
        HORIZONTAL,
        VERTICAL,
        SINGLE_SCREEN_A,
        SINGLE_SCREEN_B
    };

    Cart(const std::string& filename);
    virtual ~Cart();

    virtual MirrorMode GetMirrorMode() = 0;

    virtual uint8_t PrgRead(uint16_t address) = 0;
    virtual void PrgWrite(uint8_t M, uint16_t address) = 0;

    virtual uint8_t ChrRead(uint16_t address) = 0;
    virtual void ChrWrite(uint8_t M, uint16_t address) = 0;
};




#endif /* MAPPER_H_ */
