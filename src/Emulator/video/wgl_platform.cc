#if defined(_WIN32)

#include "wgl_platform.h"
#include <stdexcept>

void WGLPlatform::InitializeWindow(void* windowHandle)
{
	_window = reinterpret_cast<HWND>(windowHandle);
}

void WGLPlatform::InitializeContext()
{
	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
		PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
		32,                   // Colordepth of the framebuffer.
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		24,                   // Number of bits for the depthbuffer
		8,                    // Number of bits for the stencilbuffer
		0,                    // Number of Aux buffers in the framebuffer.
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	_windowDc = GetDC(_window);

	int pf = ChoosePixelFormat(_windowDc, &pfd);
	if (pf == 0)
	{
		throw std::runtime_error("Unable to create pixel format");
	}

	if (!SetPixelFormat(_windowDc, pf, &pfd))
	{
		throw std::runtime_error("Unable to set new pixel format");
	}

	_oglContext = wglCreateContext(_windowDc);
	if (_oglContext == NULL)
	{
		throw std::runtime_error("Failed to create OpenGL context");
	}

	if (!wglMakeCurrent(_windowDc, _oglContext))
	{
		throw std::runtime_error("Failed to make OpenGL context current");
	}
}

void WGLPlatform::DestroyWindow()
{

}

void WGLPlatform::DestroyContext()
{
	wglMakeCurrent(_windowDc, NULL);
	wglDeleteContext(_oglContext);
}

void WGLPlatform::SwapBuffers()
{
	::SwapBuffers(_windowDc);
}

void WGLPlatform::UpdateSurfaceSize(uint32_t* width, uint32_t* height)
{
	RECT rect;
	GetWindowRect(_window, &rect);

	uint32_t newWidth = rect.right - rect.left;
	uint32_t newHeight = rect.bottom - rect.top;

	if (*width != newWidth || *height != newHeight)
	{
		*width = newWidth;
		*height = newHeight;
	}
}

#endif