#include "wgl_platform.h"

#include <stdexcept>

WGLPlatform::WGLPlatform(void * windowHandle)
{
	Window = reinterpret_cast<HWND>(windowHandle);
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

	WindowDc = GetDC(Window);

	int pf = ChoosePixelFormat(WindowDc, &pfd);
	if (pf == 0)
	{
		throw std::runtime_error("Unable to create pixel format");
	}

	if (!SetPixelFormat(WindowDc, pf, &pfd))
	{
		throw std::runtime_error("Unable to set new pixel format");
	}

	OglContext = wglCreateContext(WindowDc);
	if (OglContext == NULL)
	{
		throw std::runtime_error("Failed to create OpenGL context");
	}

	if (!wglMakeCurrent(WindowDc, OglContext))
	{
		throw std::runtime_error("Failed to make OpenGL context current");
	}
}

void WGLPlatform::DestroyContext()
{
	wglMakeCurrent(WindowDc, NULL);
	wglDeleteContext(OglContext);
}

void WGLPlatform::SwapBuffers()
{
	::SwapBuffers(WindowDc);
}

void WGLPlatform::UpdateSurfaceSize(uint32_t* width, uint32_t* height)
{
	RECT rect;
	GetWindowRect(Window, &rect);

	uint32_t newWidth = rect.right - rect.left;
	uint32_t newHeight = rect.bottom - rect.top;

	if (*width != newWidth || *height != newHeight)
	{
		*width = newWidth;
		*height = newHeight;
	}
}
