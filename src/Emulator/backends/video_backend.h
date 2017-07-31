/* Experimental Video Backend */
#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <vector>
#include <condition_variable>

#ifdef _WIN32
#elif __linux
#include <X11/Xlib.h>
#include <cairo/cairo.h>
#endif

class VideoBackend
{
public:
    VideoBackend(void* windowHandle);
    ~VideoBackend();
    VideoBackend& operator<<(uint32_t pixel);

    void SetFramePosition(uint32_t x, uint32_t y);
    void SetOverscanEnabled(bool enabled);

    void ShowFps(bool show);
    void SetFps(uint32_t fps);
    void ShowMessage(const std::string& message, uint32_t duration);

private:
    void Swap();
    void RenderWorker();

    void UpdateSurfaceSize();
    void DrawFrame();
    void DrawFps();
    void DrawMessages();

    std::thread RenderThread;
    std::mutex RenderLock;
    std::condition_variable RenderCv;

    uint8_t* FrontBuffer;
    uint8_t* BackBuffer;
    uint32_t PixelIndex;

    uint32_t WindowWidth;
    uint32_t WindowHeight;

    bool StopRendering;

    bool OverscanEnabled;

    uint32_t CurrentFps;
    bool ShowingFps;

    std::vector<std::pair<std::string, std::chrono::steady_clock::time_point> > Messages;

#ifdef _WIN32
#elif __linux
    void InitXWindow(void* handle);
    void InitCairo();
    void DestroyXWindow();
    void DestroyCairo();

    Display* XDisplay;
    Window XParentWindow;
    Window XWindow;

    cairo_t* CairoContext;
    cairo_surface_t* CairoXSurface;
#endif
};
