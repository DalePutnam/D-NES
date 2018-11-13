#include "igl_platform.h"

#if defined(_WIN32)
#include "wgl_platform.h"
#elif defined(__linux)
#endif


std::unique_ptr<IGLPlatform> IGLPlatform::CreateGLPlatform()
{
#if defined(_WIN32)
	return std::unique_ptr<IGLPlatform>(new WGLPlatform());
#elif defined(__linux)
#endif
}