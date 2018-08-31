#include "audio_stream.h"

#ifdef _WIN32
#include "xaudio2_backend.h"
#endif

const int AudioStream::DEFAULT_SAMPLE_RATE = 44100;

AudioStream::AudioStream(uint32_t sampleRate)
{
#ifdef _WIN32
	_backend = std::unique_ptr<IAudioBackend>(new XAudio2Backend());
#endif

	_backend->Initialize(sampleRate);
}

AudioStream::~AudioStream()
{
	_backend->CleanUp();
}

void AudioStream::Reset()
{
	_backend->Reset();
}

void AudioStream::Flush()
{
	_backend->Flush();
}

uint32_t AudioStream::GetNumPendingSamples()
{
	return _backend->GetNumPendingSamples();
}

void AudioStream::SubmitSample(float sample)
{
	_backend->SubmitSample(sample);
}

uint32_t AudioStream::GetSampleRate()
{
	return _backend->GetSampleRate();
}
