#pragma once

#include <atomic>
#include <chrono>
#include <queue>
#include <cstdint>

class NES;
class CPU;
class Cart;
class AudioStream;
class VideoBackend;

class APU
{
public:
	APU(AudioStream* aout, VideoBackend* vb);
    ~APU();

    void AttachCPU(CPU* cpu);
    void AttachCart(Cart* cart);

    void Step();
    bool CheckIRQ();
    bool CheckDmaRequest();
    uint16_t GetDmaAddress();
    void WriteDmaByte(uint8_t byte);

    void ResetFrameTimer();
    void SetTargetFrameRate(uint32_t rate);

    void WritePulseOneRegister(uint8_t reg, uint8_t value);
    void WritePulseTwoRegister(uint8_t reg, uint8_t value);
    void WriteTriangleRegister(uint8_t reg, uint8_t value);
    void WriteNoiseRegister(uint8_t reg, uint8_t value);
    void WriteDmcRegister(uint8_t reg, uint8_t value);

    void WriteAPUStatus(uint8_t value);
    uint8_t ReadAPUStatus();
    void WriteAPUFrameCounter(uint8_t value);

    void SetTurboModeEnabled(bool enabled);
    void SetAudioEnabled(bool mute);
    void SetFiltersEnabled(bool enabled);

    float GetMasterVolume();
    void SetMasterVolume(float volume);
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

    static const int STATE_SIZE;
    void SaveState(char* state);
    void LoadState(const char* state);

private:
    static const uint8_t LengthCounterLookupTable[32];

    class PulseUnit
    {
    public:
        PulseUnit(bool IsPulseUnitOne);

        void WriteRegister(uint8_t reg, uint8_t value);
        void SetEnabled(bool enabled);
        bool GetEnabled();
        uint8_t GetLengthCounter();
		uint8_t GetLevel();

        void ClockTimer();
        void ClockSweep();
        void ClockEnvelope();
        void ClockLengthCounter();

        static const int STATE_SIZE;
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

        operator float ();
		uint8_t GetLevel();

        static const int STATE_SIZE;
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
		uint8_t GetLevel();

        void ClockTimer();
        void ClockEnvelope();
        void ClockLengthCounter();

        static const int STATE_SIZE;
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
        bool GetEnabled();
		uint16_t GetSampleBytesRemaining();
		uint8_t GetLevel();

        void ClockTimer();

		void ClearInterrupt();
		bool CheckIRQ();
        bool CheckDmaRequest();
        uint16_t GetDmaAddress();
        void WriteDmaByte(uint8_t byte);

        static const int STATE_SIZE;
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
        bool InterruptFlag;
        bool InterruptEnabledFlag;
        bool SampleLoopFlag;
        bool SampleBufferEmptyFlag;
        bool DmaRequest;
        bool SilenceFlag;
    };

	class MixerUnit
	{
	public:
		MixerUnit(APU& apu);

		void Clock();
		void ResetFrameTimer();
		void SetTargetFrameRate(uint32_t rate);

	private:
		void Reset();
		void GenerateSample();
		void LimitFrameRate();

		APU& Apu;

		bool TurboModeEnabled;
		bool AudioEnabled;

		uint32_t PulseOneAccumulator;
		uint32_t PulseTwoAccumulator;
		uint32_t TriangleAccumulator;
		uint32_t NoiseAccumulator;
		uint32_t DmcAccumulator;

		uint32_t CyclesPerSample;
		uint32_t CycleRemainder;
		uint32_t CycleCount;
		uint32_t ExtraCycle;
		uint32_t ExtraCount;
		uint32_t TargetFramePeriod;
		uint32_t TargetCpuFrequency;
		uint32_t SamplesPerFrame;
		uint32_t FrameSampleCount;
		std::chrono::steady_clock::time_point FramePeriodStart;
	};

    CPU* Cpu;
    Cart* Cartridge;
    AudioStream* AudioOut;
	VideoBackend* VideoOut;

    PulseUnit PulseOne;
    PulseUnit PulseTwo;
    TriangleUnit Triangle;
    NoiseUnit Noise;
    DmcUnit Dmc;
	MixerUnit Mixer;

    uint64_t Clock;
    uint32_t SequenceCount;

    bool LongSequenceFlag; // True: 5-step sequence, False: 4-step sequence
    bool InterruptInhibit;
    bool FrameInterruptFlag;
    bool FrameResetFlag;
    uint8_t FrameResetCountdown;

    std::atomic<bool> TurboModeEnabled;
	std::atomic<bool> AudioEnabled;

    // Volume Controls
    std::atomic<float> MasterVolume;
    std::atomic<float> PulseOneVolume;
    std::atomic<float> PulseTwoVolume;
    std::atomic<float> TriangleVolume;
    std::atomic<float> NoiseVolume;
    std::atomic<float> DmcVolume;
};
