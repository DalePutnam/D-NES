/* Experimental Video Backend */
#pragma once

#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <condition_variable>

#if defined(_WIN32)
#include <Windows.h>
#elif defined(__linux)   
#include <X11/Xlib.h>
#include <cairo/cairo.h>
#endif

#include <gl/GL.h>

class VideoBackend
{
public:
    VideoBackend(void* windowHandle);
    ~VideoBackend();

	void Prepare();
	void Finalize();

	void DrawFrame(uint8_t* fb);
    void SetOverscanEnabled(bool enabled);

    void ShowFps(bool show);
    void SetFps(uint32_t fps);
    void ShowMessage(const std::string& message, uint32_t duration);

private:
    void Swap();

    void UpdateSurfaceSize();
    void DrawFrame();
	void DrawFps(uint32_t fps);

    uint32_t WindowWidth;
    uint32_t WindowHeight;

    bool OverscanEnabled;

    uint32_t CurrentFps;
    bool ShowingFps;

    std::vector<std::pair<std::string, std::chrono::steady_clock::time_point> > Messages;

	GLuint FrameProgramId;
	GLuint FontProgramId;
	GLuint VertexArrayId;
	GLuint VertexBuffer;
	GLuint FrameTextureId;
	GLuint FontTextureId;

#ifdef _WIN32
    HWND Window;
	HDC WinDc;
	HGLRC Oglc;
#elif defined(__linux)   
    void InitXWindow(void* handle);
    void InitCairo();
    void DestroyXWindow();
    void DestroyCairo();

    void DrawFps();
    void DrawMessages();

    Display* XDisplay;
    Window XParentWindow;
    Window XWindow;

    cairo_t* CairoContext;
    cairo_surface_t* CairoXSurface;
#endif
};
