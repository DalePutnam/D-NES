#include "video_backend.h"

#ifdef __linux
#include <cairo/cairo-xlib.h>
#endif

VideoBackend::VideoBackend(void* windowHandle)
    : FrontBuffer(new uint8_t[256*240*4])
    , BackBuffer(new uint8_t[256*240*4])
    , PixelIndex(0)
    , StopRendering(false)
    , OverscanEnabled(true)
    , CurrentFps(0)
{
#ifdef _WIN32
#elif __linux
    InitXWindow(windowHandle);
    InitCairo();
#endif

    RenderThread = std::thread(&VideoBackend::RenderWorker, this);
}

VideoBackend::~VideoBackend()
{
    {
        std::unique_lock<std::mutex> lock(RenderLock);
        StopRendering = true;
        RenderCv.notify_all();
    }

    RenderThread.join();

#ifdef _WIN32
#elif __linux
    DestroyCairo();
    DestroyXWindow();
#endif

    delete [] BackBuffer;
    delete [] FrontBuffer;
}

VideoBackend& VideoBackend::operator<<(uint32_t pixel)
{
    uint8_t red = static_cast<uint8_t>((pixel & 0xFF0000) >> 16);
    uint8_t green = static_cast<uint8_t>((pixel & 0x00FF00) >> 8);
    uint8_t blue = static_cast<uint8_t>(pixel & 0x0000FF);

    BackBuffer[PixelIndex++] = blue;
    BackBuffer[PixelIndex++] = green;
    BackBuffer[PixelIndex++] = red;
    BackBuffer[PixelIndex++] = 255;

    if (PixelIndex == 256*240*4)
    {
        Swap();
    }

    return *this;
}

void VideoBackend::SetFramePosition(uint32_t x, uint32_t y)
{
    std::unique_lock<std::mutex> lock(RenderLock);

    PixelIndex = (1024 * y) + (x * 4);
}

void VideoBackend::SetOverscanEnabled(bool enabled)
{
    std::unique_lock<std::mutex> lock(RenderLock);

    OverscanEnabled = enabled;
}

void VideoBackend::ShowFps(bool show)
{
    std::unique_lock<std::mutex> lock(RenderLock);

    ShowingFps = show;
}

void VideoBackend::SetFps(uint32_t fps)
{
    std::unique_lock<std::mutex> lock(RenderLock);

    CurrentFps = fps;
}

void VideoBackend::ShowMessage(const std::string& message, uint32_t duration)
{
    std::unique_lock<std::mutex> lock(RenderLock);

    using namespace std::chrono;

    steady_clock::time_point expires = steady_clock::now() + seconds(duration);

    std::string allcaps;
    for (size_t i = 0; i < message.length(); ++i)
    {
        if (message[i] >= 'a' && message[i] <= 'z')
        {
            allcaps += message[i] - ('a' - 'A');
        }
        else
        {
            allcaps += message[i];
        }
    }
    
    Messages.push_back(std::make_pair(allcaps, expires));
}

void VideoBackend::Swap()
{
    std::unique_lock<std::mutex> lock(RenderLock);

    uint8_t* temp = FrontBuffer;
    FrontBuffer = BackBuffer;
    BackBuffer = temp;

    PixelIndex = 0;

    RenderCv.notify_all();
}

void VideoBackend::RenderWorker()
{
    std::unique_lock<std::mutex> lock(RenderLock);

    for (;;)
    {
        RenderCv.wait(lock);

        if (StopRendering) break;

        UpdateSurfaceSize();
        DrawFrame();

        if (ShowingFps)
        {
            DrawFps();
        }

        DrawMessages();
    }
}

void VideoBackend::UpdateSurfaceSize()
{
#ifdef _WIN32
#elif __linux    
    XWindowAttributes attributes;
    XGetWindowAttributes(XDisplay, XParentWindow, &attributes);

    uint32_t newWidth = static_cast<uint32_t>(attributes.width);
    uint32_t newHeight = static_cast<uint32_t>(attributes.height);

    if (WindowWidth != newWidth || WindowHeight != newHeight)
    {
        WindowWidth = attributes.width;
        WindowHeight = attributes.height;

        XResizeWindow(XDisplay, XWindow, WindowWidth, WindowHeight);
        cairo_xlib_surface_set_size(CairoXSurface, WindowWidth, WindowHeight);
    }
#endif
}

void VideoBackend::DrawFrame()
{
#ifdef _WIN32
#elif __linux   
    cairo_save(CairoContext);

    cairo_surface_t* frame;
    if (OverscanEnabled)
    {
        frame = cairo_image_surface_create_for_data(FrontBuffer + (256*8*4), CAIRO_FORMAT_ARGB32, 256, 224, 1024);
        cairo_scale(CairoContext, WindowWidth / 256.0, WindowHeight / 224.0);
    }
    else
    {
        frame = cairo_image_surface_create_for_data(FrontBuffer, CAIRO_FORMAT_ARGB32, 256, 240, 1024);
        cairo_scale(CairoContext, WindowWidth / 256.0, WindowHeight / 240.0);
    }

    cairo_set_source_surface(CairoContext, frame, 0, 0);
    cairo_pattern_set_filter(cairo_get_source(CairoContext), CAIRO_FILTER_NEAREST);
    cairo_paint(CairoContext);
    cairo_surface_destroy(frame);
    cairo_restore(CairoContext);
#endif
}

void VideoBackend::DrawFps()
{
    std::string fps = std::to_string(CurrentFps);

#ifdef _WIN32
#elif __linux   
    cairo_text_extents_t extents;
    cairo_text_extents(CairoContext, fps.c_str(), &extents);
    
    cairo_set_source_rgb(CairoContext, 0.0, 0.0, 0.0);
    cairo_rectangle(CairoContext, WindowWidth - extents.x_advance - 10.0, 8.0, extents.x_advance + 4.0, extents.height + 6.0);
    cairo_fill(CairoContext);
    
    cairo_move_to(CairoContext, WindowWidth - extents.x_advance - 8.0, 11.0 + extents.height);
    cairo_set_source_rgb(CairoContext, 1.0, 1.0, 1.0);
    cairo_show_text(CairoContext, fps.c_str());
#endif
}

void VideoBackend::DrawMessages()
{
    if (Messages.empty())
    {
        return;
    }

    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    std::vector<std::pair<std::string, std::chrono::steady_clock::time_point> > remaining;
    for (auto& entry : Messages)
    {
        if (entry.second > now)
        {
            remaining.push_back(entry);
        }
    }

    Messages.swap(remaining);

    // Draw Messages
    double offsetY = 8.0;
    for (auto& entry : Messages)
    {
#ifdef _WIN32
#elif __linux
        cairo_text_extents_t extents, extents_first;
        cairo_text_extents(CairoContext, entry.first.c_str(), &extents);
        cairo_text_extents(CairoContext, entry.first.substr(0, 1).c_str(), &extents_first);

        cairo_set_source_rgb(CairoContext, 0.0, 0.0, 0.0);
        cairo_rectangle(CairoContext, 8.0, offsetY, extents.x_advance + 4.0, extents.height + 6.0);
        cairo_fill(CairoContext);

        cairo_move_to(CairoContext, 10.0, offsetY + 3.0 - extents.y_bearing);
        cairo_set_source_rgb(CairoContext, 1.0, 1.0, 1.0);
        cairo_show_text(CairoContext, entry.first.c_str());
#endif        

        offsetY += extents.height + 6.0;
    }
}

#ifdef _WIN32
#endif

#ifdef __linux
void VideoBackend::InitXWindow(void* handle)
{
    XParentWindow = reinterpret_cast<Window>(handle);
    XDisplay = XOpenDisplay(nullptr);

    XWindowAttributes attributes;
    XGetWindowAttributes(XDisplay, XParentWindow, &attributes);

    WindowWidth = attributes.width;
    WindowHeight = attributes.height;

    XWindow = XCreateSimpleWindow(XDisplay, XParentWindow, 0, 0, WindowWidth, WindowHeight, 0, 0, 0);

    XMapWindow(XDisplay, XWindow);
    XSync(XDisplay, false);
}

void VideoBackend::InitCairo()
{
    XWindowAttributes attributes;
    XGetWindowAttributes(XDisplay, XWindow, &attributes);

    CairoXSurface = cairo_xlib_surface_create(XDisplay, XWindow, attributes.visual, attributes.width, attributes.height);
    CairoContext = cairo_create(CairoXSurface);

    cairo_select_font_face(CairoContext, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(CairoContext, 18.0);
}

void VideoBackend::DestroyXWindow()
{
    XDestroyWindow(XDisplay, XWindow);
    XCloseDisplay(XDisplay);
}

void VideoBackend::DestroyCairo()
{
    cairo_destroy(CairoContext);
    cairo_surface_destroy(CairoXSurface);
}
#endif
