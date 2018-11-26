#pragma once

#if defined(__linux)

#include "iaudio_platform.h"

class AlsaPlatform : public IAudioPlatform
{
public:
	virtual bool Initialize(int sampleRate);
	virtual void CleanUp();
	virtual void Reset();
	virtual void Flush();
	virtual uint32_t GetNumPendingSamples();
	virtual void SubmitSample(float sample);
	virtual uint32_t GetSampleRate();
private:
};

#endif