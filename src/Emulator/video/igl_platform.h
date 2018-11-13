#pragma once

#include <memory>
#include <cstdint>

class IGLPlatform
{
public:
	static std::unique_ptr<IGLPlatform> CreateGLPlatform(void* windowHandle);

	virtual void InitializeContext() = 0;
	virtual void DestroyContext() = 0;
	virtual void SwapBuffers() = 0;
	virtual void UpdateSurfaceSize(uint32_t* width, uint32_t* height) = 0;
};