#ifndef APU_H_
#define APU_H_

#include <mutex>
#include <atomic>
#include <cstdint>

#include <xaudio2.h>

class NES;
class CPU;
class Cart;

#define NUM_AUDIO_BUFFERS 500

class APU
{
    // Butterworth filter implementation shamelessly stolen from
    // http://stackoverflow.com/questions/8079526/lowpass-and-high-pass-filter-in-c-sharp
    class Filter
    {
        float c, a1, a2, a3, b1, b2;

        float InputHistory[2];
        float OutputHistory[2];

    public:
        Filter(float frequency, float resonance, uint32_t sampleRate, bool isLowPass);
        void Reset();

        float operator()(float sample);
    };

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

        void WriteRegister(uint8_t reg, uint8_t value);
        void SetEnabled(bool enabled);
        bool GetEnabled();
        uint8_t GetLengthCounter();

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

        void WriteRegister(uint8_t reg, uint8_t value);
        void SetEnabled(bool enabled);
        bool GetEnabled();
        uint8_t GetLengthCounter();

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
        uint16_t LinearFeedbackShiftRegister;
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

        void WriteRegister(uint8_t reg, uint8_t value);
        void SetEnabled(bool enabled);
        bool GetEnabled();
        uint8_t GetLengthCounter();

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

        void WriteRegister(uint8_t reg, uint8_t value);
        void SetEnabled(bool enabled);
        uint16_t GetSampleBytesRemaining();
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

    std::mutex ControlMutex;

    uint64_t Clock;
    uint32_t SequenceCount;

    bool SequenceMode; // True: 5-step sequence, False: 4-step sequence
    bool InterruptInhibit;
    bool FrameInterruptFlag;
    bool FrameResetFlag;
    uint8_t FrameResetCountdown;

    IXAudio2* XAudio2Instance;
    IXAudio2MasteringVoice* XAudio2MasteringVoice;
    IXAudio2SourceVoice* XAudio2SourceVoice;

    uint32_t BufferIndex;
    uint32_t CurrentBuffer;
    uint32_t CyclesPerSample;
    uint32_t CyclesToNextSample;
    float* OutputBuffers[NUM_AUDIO_BUFFERS];
    
    bool IsMuted;
    bool FilteringEnabled;

    Filter HighPass90Hz;
    Filter HighPass440Hz;
    Filter LowPass14KHz;

    std::atomic<float> PulseOneVolume;
    std::atomic<bool> PulseOneEnabled;
    std::atomic<float> PulseTwoVolume;
    std::atomic<bool> PulseTwoEnabled;
    std::atomic<float> TriangleVolume;
    std::atomic<bool> TriangleEnabled;
    std::atomic<float> NoiseVolume;
    std::atomic<bool> NoiseEnabled;
    std::atomic<float> DmcVolume;
    std::atomic<bool> DmcEnabled;
public:
    APU(NES& nes);
    ~APU();

    void AttachCPU(CPU& cpu);
    void AttachCart(Cart& cart);

    void Step();

    bool CheckIRQ();

    void WritePulseOneRegister(uint8_t reg, uint8_t value);
    void WritePulseTwoRegister(uint8_t reg, uint8_t value);
    void WriteTriangleRegister(uint8_t reg, uint8_t value);
    void WriteNoiseRegister(uint8_t reg, uint8_t value);
    void WriteDmcRegister(uint8_t reg, uint8_t value);

    void WriteAPUStatus(uint8_t value);
    uint8_t ReadAPUStatus();
    void WriteAPUFrameCounter(uint8_t value);

    void SetMuted(bool mute);
    void SetFiltersEnabled(bool enabled);
    void SetPulseOneEnabled(bool enabled);
    void SetPulseOneVolume(float volume);
    float GetPulseOneVolume();
    void SetPulseTwoEnabled(bool enabled);
    void SetPulseTwoVolume(float volume);
    float GetPulseTwoVolume();
    void SetTriangleEnabled(bool enabled);
    void SetTriangleVolume(float volume);
    float GetTriangleVolume();
    void SetNoiseEnabled(bool enabled);
    void SetNoiseVolume(float volume);
    float GetNoiseVolume();
    void SetDmcEnabled(bool enabled);
    void SetDmcVolume(float volume);
    float GetDmcVolume();
};

#endif // APU_H_
