#include "audio_backend.h"
#include "iaudio_platform.h"

const int AudioBackend::DEFAULT_SAMPLE_RATE = 44100;

AudioBackend::AudioBackend(uint32_t sampleRate)
{
	_backend = IAudioPlatform::CreateAudioPlatform();
	_backend->Initialize(sampleRate);
}

AudioBackend::~AudioBackend()
{
	_backend->CleanUp();
}

void AudioBackend::Reset()
{
	_backend->Reset();
}

void AudioBackend::Flush()
{
	_backend->Flush();
}

uint32_t AudioBackend::GetNumPendingSamples()
{
	return _backend->GetNumPendingSamples();
}

void AudioBackend::SubmitSample(float sample)
{
	_backend->SubmitSample(sample);
}

uint32_t AudioBackend::GetSampleRate()
{
	return _backend->GetSampleRate();
}
