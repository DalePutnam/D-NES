#if defined(__linux)

#include "alsa_platform.h"

bool AlsaPlatform::Initialize(int sampleRate)
{
    return true;
}

void AlsaPlatform::CleanUp()
{

}

void AlsaPlatform::Reset()
{

}

void AlsaPlatform::Flush()
{

}

uint32_t AlsaPlatform::GetNumPendingSamples()
{
    return 0;
}

void AlsaPlatform::SubmitSample(float sample)
{

}

uint32_t AlsaPlatform::GetSampleRate()
{
    return 44100;
}

#endif