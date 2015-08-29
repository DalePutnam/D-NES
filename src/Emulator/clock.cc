#include "clock.h"

Clock::Clock() : clock(0) {}

void Clock::CPUIncrementClock()
{
    uint64_t old = clock;
    clock += 3;

    if (clock % 341 < old % 341)
    {
        if (scanline == 260)
        {
            scanline = -1;
        }
        else
        {
            scanline++;
        }
    }
}

uint64_t Clock::GetClock()
{
    return clock;
}

int Clock::GetScanline()
{
    return scanline;
}