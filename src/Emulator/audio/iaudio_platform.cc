#include "iaudio_platform.h"

#if defined(_WIN32)
#include "xaudio2_platform.h"
#elif defined(__linux)
#include "alsa_platform.h"
#endif

std::unique_ptr<IAudioPlatform> IAudioPlatform::CreateAudioPlatform()
{
#if defined(_WIN32)
	return std::unique_ptr<IAudioPlatform>(new XAudio2Platform());
#elif defined(__linux)
	return std::unique_ptr<IAudioPlatform>(new AlsaPlatform());
#endif
}