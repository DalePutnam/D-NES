#pragma once

#include "igl_platform.h"
#include "gl_util.h"

class WGLPlatform : public IGLPlatform
{
public:
	virtual void InitializeWindow(void* windowHandle) override;
	virtual void InitializeContext() override;
	virtual void DestroyContext() override;
	virtual void SwapBuffers() override;
	virtual void UpdateSurfaceSize(uint32_t* width, uint32_t* height) override;

private:
	HWND _window;
	HDC _windowDc;
	HGLRC _oglContext;
};