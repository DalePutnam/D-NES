#pragma once

#include <cstdint>
#include <memory>

class IGLPlatform
{
public:
	static std::unique_ptr<IGLPlatform> CreateGLPlatform();

	virtual void InitializeWindow(void* windowHandle) = 0;
	virtual void InitializeContext() = 0;
	virtual void DestroyContext() = 0;
	virtual void SwapBuffers() = 0;
	virtual void UpdateSurfaceSize(uint32_t* width, uint32_t* height) = 0;
};