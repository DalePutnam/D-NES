#pragma once

#include <mutex>
#include <atomic>
#include <chrono>
#include <cstdint>

#include "audio_backend.h"

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

    void SetFrameLength(int32_t length);

    void WritePulseOneRegister(uint8_t reg, uint8_t value);
    void WritePulseTwoRegister(uint8_t reg, uint8_t value);
    void WriteTriangleRegister(uint8_t reg, uint8_t value);
    void WriteNoiseRegister(uint8_t reg, uint8_t value);
    void WriteDmcRegister(uint8_t reg, uint8_t value);

    void WriteAPUStatus(uint8_t value);
    uint8_t ReadAPUStatus();
    void WriteAPUFrameCounter(uint8_t value);

    void SetAudioEnabled(bool mute);
    void SetMasterVolume(float volume);
    void SetFiltersEnabled(bool enabled);

    float GetMasterVolume();

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

    static int STATE_SIZE;
    void SaveState(char* state);
    void LoadState(const char* state);

private:
    static const uint32_t CpuFrequency;
    static const uint8_t LengthCounterLookupTable[32];

    void GenerateSample();

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

        operator uint8_t ();

        static int STATE_SIZE;
        void SaveState(char* state);
        void LoadState(const char* state);

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

        operator uint8_t ();

        static int STATE_SIZE;
        void SaveState(char* state);
        void LoadState(const char* state);

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

        operator uint8_t ();

        static int STATE_SIZE;
        void SaveState(char* state);
        void LoadState(const char* state);

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

        operator uint8_t ();

        static int STATE_SIZE;
        void SaveState(char* state);
        void LoadState(const char* state);

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

    int32_t CurrentFrameLength;
    uint32_t CyclesToNextSample;
    uint32_t EffectiveCpuFrequency;
    double ExtraCount;
    double Fraction;

    AudioBackend Backend;

    std::atomic<float> MasterVolume;
    std::atomic<float> PulseOneVolume;
    std::atomic<float> PulseTwoVolume;
    std::atomic<float> TriangleVolume;
    std::atomic<float> NoiseVolume;
    std::atomic<float> DmcVolume;
};
