/*
 * Nes Master Clock
 */

#ifndef CLOCK_H_
#define CLOCK_H_

#include <boost/cstdint.hpp>

class Clock
{
    int scanline;
    uint64_t clock;

public:
    Clock();
    
    void CPUIncrementClock();
    uint64_t GetClock();
    int GetScanline();
};

#endif /* CLOCK_H_ */