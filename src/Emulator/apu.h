//#ifndef APU_H_
//#define APU_H_
#if 0
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
        uint16_t TimerPeriod;
        uint8_t SequenceCount;
        uint8_t LengthCounter;
        uint8_t DutyCycle;
        uint8_t EnvelopeDividerVolume;
        uint8_t EnvelopeDividerCounter;
        uint8_t EnvelopeCounter;
        uint8_t SweepShiftCount;
        uint8_t SweepDivider;
        uint8_t SweepDividerCounter;
        bool LengthHaltEnvelopeLoopFlag;
        bool ConstantVolumeFlag;
        bool SweepEnableFlag;
        bool SweepReloadFlag;
        bool SweepNegateFlag;
        bool EnvelopeStartFlag;
        bool EnabledFlag;
        bool PulseOneFlag;
    public:
        PulseUnit(bool IsPulseUnitOne);

        void WriteRegister(PulseRegister reg, uint8_t value);
        void SetEnabled(bool enabled);
        bool GetEnabled();

        void ClockTimer();
        void ClockSweep();
        void ClockEnvelope();
        void ClockLengthCounter();

        uint8_t operator()();
    };

    class TriangleUnit
    {
        static const uint8_t Sequence[32];

        uint16_t Timer;
        uint16_t TimerPeriod;
        uint8_t SequenceCount;
        uint8_t LinearCounter;
        uint8_t LinearCounterPeriod;
        uint8_t LengthCounter;
        bool LengthHaltControlFlag;
        bool LinearCounterReloadFlag;
        bool EnabledFlag;
    public:
        TriangleUnit();

        void WriteRegister(TriangleRegister reg, uint8_t value);
        void SetEnabled(bool enabled);
        bool GetEnabled();

        void ClockTimer();
        void ClockLinearCounter();
        void ClockLengthCounter();

        uint8_t operator()();
    };

    class NoiseUnit
    {
        static const uint16_t TimerPeriods[16];

        uint16_t Timer;
        uint8_t TimerPeriodIndex;
        uint8_t LinearFeedbackShiftRegister;
        uint8_t LengthCounter;
        uint8_t EnvelopeDividerVolume;
        uint8_t EnvelopeDividerCounter;
        uint8_t EnvelopeCounter;
        bool LengthHaltEnvelopeLoopFlag;
        bool ConstantVolumeFlag;
        bool EnvelopeStartFlag;
        bool ModeFlag;
        bool EnabledFlag;

    public:
        NoiseUnit();

        void WriteRegister(NoiseRegister reg, uint8_t value);
        void SetEnabled(bool enabled);
        bool GetEnabled();

        void ClockTimer();
        void ClockEnvelope();
        void ClockLengthCounter();

        uint8_t operator()();
    };

    class DmcUnit
    {
        static const uint16_t TimerPeriods[16];

        APU& Apu;

        uint16_t Timer;
        uint8_t TimerPeriodIndex;
        uint8_t OutputLevel;
        uint16_t SampleAddress;
        uint16_t CurrentAddress;
        uint16_t SampleLength;
        uint16_t SampleBytesRemaining;
        uint8_t SampleBuffer;
        uint8_t SampleShiftRegister;
        uint8_t SampleBitsRemaining;
        uint8_t MemoryStallCountdown;
        bool InterruptFlag;
        bool InterruptEnabledFlag;
        bool SampleLoopFlag;
        bool SampleBufferEmptyFlag;
        bool InMemoryStall;
        bool SilenceFlag;

    public:
        DmcUnit(APU& apu);

        void WriteRegister(DmcRegister reg, uint8_t value);
        void SetEnabled(bool enabled);
        bool GetEnabled();
        void ClearInterrupt();
        bool CheckIRQ();

        void ClockTimer();

        uint8_t operator()();
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

    float PulseOutLookupTable[31];
    float TriangleNoiseDmcOutLookupTable[203];

    bool SequenceMode; // True: 5-step sequence, False: 4-step sequence
    bool InterruptInhibit;
    bool FrameInterruptFlag;

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

    void WriteAPUStatus(uint8_t value);
    uint8_t ReadAPUStatus();
    void WriteAPUFrameCounter(uint8_t value);
};

#endif // APU_H_
