#pragma once

#include <mutex>
#include <atomic>
#include <chrono>
#include <cstdint>

#ifdef _WINDOWS
#include <xaudio2.h>
#elif 0
#endif

class CPU;
class Cart;

class APU
{
public:
    APU();
    ~APU();

    void AttachCPU(CPU* cpu);
    void AttachCart(Cart* cart);

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
    void SetMasterVolume(float volume);
    float GetMasterVolume();

    void SetFiltersEnabled(bool enabled);

    void SetPulseOneVolume(float volume);
    float GetPulseOneVolume();
    void SetPulseTwoVolume(float volume);
    float GetPulseTwoVolume();
    void SetTriangleVolume(float volume);
    float GetTriangleVolume();
    void SetNoiseVolume(float volume);
    float GetNoiseVolume();
    void SetDmcVolume(float volume);
    float GetDmcVolume();

private:
    class AudioBackend {
    public:
        AudioBackend();
        ~AudioBackend();
        void SetMuted(bool mute);
        void operator<<(float sample);

    private:
        static constexpr uint32_t NUM_AUDIO_BUFFERS = 500;

        bool IsMuted;
        uint32_t BufferIndex;
        uint32_t CurrentBuffer;
        float* OutputBuffers[NUM_AUDIO_BUFFERS];

#ifdef _WINDOWS
        IXAudio2* XAudio2Instance;
        IXAudio2MasteringVoice* XAudio2MasteringVoice;
        IXAudio2SourceVoice* XAudio2SourceVoice;
#elif 0
#endif
    };


    // Butterworth filter implementation shamelessly stolen from
    // http://stackoverflow.com/questions/8079526/lowpass-and-high-pass-filter-in-c-sharp
    class Filter
    {
    public:
        Filter(float frequency, float resonance, bool isLowPass);
        float operator()(float sample);
        void Reset();

    private:
        float c, a1, a2, a3, b1, b2;
        float InputHistory[2];
        float OutputHistory[2];
    };

    static const uint8_t LengthCounterLookupTable[32];

    class PulseUnit
    {
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

    private:
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
    };

    class TriangleUnit
    {
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

    private:
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
    };

    class NoiseUnit
    {
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

    private:
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
    };

    class DmcUnit
    {
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

    private:
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
    };

    static constexpr uint32_t AUDIO_SAMPLE_RATE = 48000;
    static constexpr uint32_t AUDIO_BUFFER_SIZE = AUDIO_SAMPLE_RATE / 240;
    static constexpr uint32_t CPU_FREQUENCY = 1789773;
    static constexpr uint32_t CYCLES_PER_SAMPLE = CPU_FREQUENCY / AUDIO_SAMPLE_RATE;

    CPU* Cpu;
    Cart* Cartridge;

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

    uint32_t CyclesToNextSample;

    bool IsMuted;
    bool FilteringEnabled;

    Filter HighPass90Hz;
    Filter HighPass440Hz;
    Filter LowPass14KHz;

    AudioBackend Backend;

    std::atomic<float> MasterVolume;
    std::atomic<float> PulseOneVolume;
    std::atomic<float> PulseTwoVolume;
    std::atomic<float> TriangleVolume;
    std::atomic<float> NoiseVolume;
    std::atomic<float> DmcVolume;
};
