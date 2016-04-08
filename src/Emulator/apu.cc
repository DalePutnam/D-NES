#include "apu.h"

const uint8_t APU::LengthCounterLookupTable[32] = 
{
    10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};


//**********************************************************************
// APU Pulse Unit
//**********************************************************************

const uint8_t APU::PulseUnit::Sequences[4] = { 0x40, 0x60, 0x78, 0x9F };

APU::PulseUnit::PulseUnit() :
    Timer(0),
    SequenceCount(0),
    LengthCounter(0),
    DutyCycle(0),
    Envelope(0),
    ShiftCount(0),
    SweepDivider(0),
    LengthHalt(false),
    VolumeFlag(false),
    SweepFlag(false),
    SweepReloadFlag(false),
    NegateFlag(false),
    StartFlag(false)
{}

void APU::PulseUnit::WriteRegister(PulseRegister reg, uint8_t value)
{
    switch (reg)
    {
    case PulseRegister::One:
        DutyCycle = (value & 0xC0) >> 6;
        LengthHalt = !!(value & 0x20);
        VolumeFlag = !!(value & 0x10);
        Envelope = (value & 0x0F);
        break;
    case PulseRegister::Two:
        SweepFlag = !!(value & 0x80);;
        SweepDivider = (value & 0x70) >> 4;
        NegateFlag = !!(value & 0x08);
        ShiftCount = (value & 0x07);
        SweepReloadFlag = true;
        break;
    case PulseRegister::Three:
        Timer = (Timer | 0xFF00) | value;
        break;
    case PulseRegister::Four:
        Timer = (Timer | 0x00FF) | (static_cast<uint16_t>(value & 0x07) << 8);

    }
}