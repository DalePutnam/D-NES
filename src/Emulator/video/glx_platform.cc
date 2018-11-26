#if defined(__linux)

#include "glx_platform.h"

void GLXPlatform::InitializeWindow(void* windowHandle)
{
    _parentWindowHandle = reinterpret_cast<Window>(windowHandle);
    _display = XOpenDisplay(nullptr);

    if (_display == nullptr)
    {
        throw std::runtime_error("Failed to connect to X11 Display");
    }

    XWindowAttributes attributes;
    if (XGetWindowAttributes(_display, _parentWindowHandle, &attributes) == 0)
    {
        XCloseDisplay(_display);
        throw std::runtime_error("Failed to retrieve X11 window attributes");
    }

	GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };

	_xVisualInfo = glXChooseVisual(_display, 0, att);
	if (_xVisualInfo == nullptr) {
		XCloseDisplay(_display);
		throw std::runtime_error("Failed to choose GLX visual settings");
	}

	Colormap colorMap = XCreateColormap(_display, _parentWindowHandle, _xVisualInfo->visual, AllocNone);

	XSetWindowAttributes setWindowAttributes;
	setWindowAttributes.colormap = colorMap;
	setWindowAttributes.event_mask = ExposureMask | KeyPressMask;

	_windowHandle = XCreateWindow(_display, _parentWindowHandle, 0, 0, attributes.width, attributes.height, 0, _xVisualInfo->depth,
							InputOutput, _xVisualInfo->visual, CWColormap | CWEventMask, &setWindowAttributes);

    if (XMapWindow(_display, _windowHandle) == 0)
    {
        XDestroyWindow(_display, _windowHandle);
        XCloseDisplay(_display);
        throw std::runtime_error("Failed to map X11 window");
    }
}

void GLXPlatform::InitializeContext()
{
	_oglContext = glXCreateContext(_display, _xVisualInfo, NULL, GL_TRUE);
	if (_oglContext == NULL) {
		throw std::runtime_error("Failed to create GLX context");
	}

	if (!glXMakeCurrent(_display, _windowHandle, _oglContext)) {
		throw std::runtime_error("Failed to make GLX context current");
	}
}

void GLXPlatform::DestroyWindow()
{
    XDestroyWindow(_display, _windowHandle);
    XCloseDisplay(_display);
}

void GLXPlatform::DestroyContext()
{
    glXMakeCurrent(_display, None, NULL);
	glXDestroyContext(_display, _oglContext);
}

void GLXPlatform::SwapBuffers()
{
    glXSwapBuffers(_display, _windowHandle);
}

void GLXPlatform::UpdateSurfaceSize(uint32_t* width, uint32_t* height)
{
    XWindowAttributes attributes;
    if (XGetWindowAttributes(_display, _parentWindowHandle, &attributes) == 0)
    {
        return;
    }

    uint32_t newWidth = static_cast<uint32_t>(attributes.width);
    uint32_t newHeight = static_cast<uint32_t>(attributes.height);

    if (*width != newWidth || *height != newHeight)
    {
        *width = attributes.width;
        *height = attributes.height;

        XResizeWindow(_display, _windowHandle, *width, *height);
    }
}

#endif