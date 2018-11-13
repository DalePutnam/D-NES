#include "iaudio_platform.h"

#if defined(_WIN32)
#include "xaudio2_platform.h"
#elif defined(__linux)
#endif

std::unique_ptr<IAudioPlatform> IAudioPlatform::CreateAudioPlatform()
{
#if defined(_WIN32)
	return std::unique_ptr<IAudioPlatform>(new XAudio2Platform());
#elif defined(__linux)
#endif
}