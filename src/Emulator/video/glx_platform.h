#pragma once

#if defined(__linux)

#include "igl_platform.h"
#include "gl_util.h"

class GLXPlatform : public IGLPlatform
{
public:
	virtual void InitializeWindow(void* windowHandle);
	virtual void InitializeContext();
    virtual void DestroyWindow();
	virtual void DestroyContext();
	virtual void SwapBuffers();
	virtual void UpdateSurfaceSize(uint32_t* width, uint32_t* height);

private:
    Display* _display;
    Window _parentWindowHandle;
    Window _windowHandle;
    XVisualInfo* _xVisualInfo;
    GLXContext _oglContext;
    uint32_t _windowHeight;
    uint32_t _windowWidth;
};

#endif