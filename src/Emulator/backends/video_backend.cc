
#include "video_backend.h"

#ifdef __linux

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <cairo/cairo-xlib.h>

static Window GetX11WindowHandle(void* handle)
{
    return gdk_x11_window_get_xid(gtk_widget_get_window(reinterpret_cast<GtkWidget*>(handle)));
}

#endif

VideoBackend::VideoBackend(void* windowHandle)
    : FrontBuffer(new uint8_t[256*240*4])
    , BackBuffer(new uint8_t[256*240*4])
    , PixelIndex(0)
    , StopRendering(false)
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
    for (;;)
    {
        std::unique_lock<std::mutex> lock(RenderLock);

        if (StopRendering) break;

        RenderCv.wait(lock);

        UpdateSurfaceSize();
        DrawFrame();

        if (ShowingFps)
        {
            DrawFps();
        }
    }
}

void VideoBackend::UpdateSurfaceSize()
{
    XWindowAttributes attributes;
    XGetWindowAttributes(XDisplay, XParentWindow, &attributes);

    WindowWidth = attributes.width;
    WindowHeight = attributes.height;

    XResizeWindow(XDisplay, XWindow, WindowWidth, WindowHeight);
    cairo_xlib_surface_set_size(CairoXSurface, WindowWidth, WindowHeight);
}

void VideoBackend::DrawFrame()
{
    cairo_surface_t* frame = cairo_image_surface_create_for_data(FrontBuffer + (256*8*4), CAIRO_FORMAT_ARGB32, 256, 224, 1024);
    cairo_save(CairoContext);
    cairo_scale(CairoContext, WindowWidth / 256.0, WindowHeight / 224.0);
    cairo_set_source_surface(CairoContext, frame, 0, 0);
    cairo_pattern_set_filter(cairo_get_source(CairoContext), CAIRO_FILTER_NEAREST);
    cairo_paint(CairoContext);
    cairo_surface_destroy(frame);
    cairo_restore(CairoContext);
}

void VideoBackend::DrawFps()
{
    cairo_select_font_face(CairoContext, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(CairoContext, 16.0);

    std::string fps = std::to_string(CurrentFps);
    cairo_move_to(CairoContext, 8.0, 24.0);
    cairo_set_source_rgb(CairoContext, 1.0, 1.0, 1.0);
    cairo_show_text(CairoContext, fps.c_str());
    //cairo_text_path(CairoContext, fps.c_str());
    //cairo_set_source_rgb(CairoContext, 1.0, 1.0, 1.0);
    //cairo_fill_preserve(CairoContext);
    //cairo_fill(CairoContext);
}

#ifdef _WIN32
#endif

#ifdef __linux
void VideoBackend::InitXWindow(void* handle)
{
    XParentWindow = GetX11WindowHandle(handle);
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

    CairoXSurface = cairo_xlib_surface_create(XDisplay, XWindow, attributes.visual, 256, 256);
    CairoContext = cairo_create(CairoXSurface);
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
