#ifndef APU_H_
#define APU_H_

#include <cstdint>

class NES;
class CPU;
class Cart;

class APU
{
public:
    enum PulseRegister { One, Two, Three, Four };
    enum TriangleRegister { One, Two, Three };
    enum NoiseRegister { One, Two, Three };
    enum DmcRegister { One, Two, Three, Four };

private:
    static const uint8_t LengthCounterLookupTable[32];

    class PulseUnit
    {
        static const uint8_t Sequences[4];

        uint16_t Timer;
        uint8_t SequenceCount;
        uint8_t LengthCounter;
        uint8_t DutyCycle;
        uint8_t Envelope;
        uint8_t ShiftCount;
        uint8_t SweepDivider;
        bool LengthHalt;
        bool VolumeFlag;
        bool SweepFlag;
        bool SweepReloadFlag;
        bool NegateFlag;
        bool StartFlag;
    public:
        PulseUnit();

        void WriteRegister(PulseRegister reg, uint8_t value);

        void ClockTimer();
        void ClockSweep();
        void ClockEnvelope();
        void ClockLengthCounter();

        uint8_t operator()();
    };

    struct TriangleUnit
    {
        TriangleUnit();
    };

    struct NoiseUnit
    {
        NoiseUnit();
    };

    struct DmcUnit
    {
        DmcUnit();
    };

    NES& Nes;
    CPU* Cpu;
    Cart* cart;

    PulseUnit PulseOne;
    PulseUnit PulseTwo;
    TriangleUnit Triangle;
    NoiseUnit Noise;
    DmcUnit Dmc;

    uint64_t Clock;
    uint32_t SequenceCount;

    bool SequenceMode; // True: 5-step sequence, False: 4-step sequence
    bool InterruptInhibit;

    bool PulseOneEnabled;
    bool PulseTwoEnabled;
    bool TriangleEnabled;
    bool NoiseEnabled;
    bool DmcEnabled;

public:
    APU(NES& nes);
    ~APU();

    void AttachCPU(CPU& cpu);
    void AttachCart(Cart& cart);

    void Step();

    bool CheckIRQ();

    void WritePulseOneRegister(PulseRegister reg, uint8_t value);
    void WritePulseTwoRegister(PulseRegister reg, uint8_t value);
    void WriteTriangleRegister(TriangleRegister reg, uint8_t value);
    void WriteNoiseRegister(NoiseRegister reg, uint8_t value);
    void WriteDmcRegister(DmcRegister reg, uint8_t value);

    void WriteAPUControl(uint8_t value);
    void WriteAPUStatus(uint8_t value);
    void WriteAPUFrameCounter(uint8_t value);
    void ReadAPUFrameCounter(uint8_t value);
};

#endif // APU_H_
