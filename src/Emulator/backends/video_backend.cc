#include <string>
#ifdef __linux
#include <cairo/cairo-xlib.h>
#endif

#include "video_backend.h"

namespace
{
std::string ToUpperCase(const std::string& str)
{
	std::string allcaps;
	for (size_t i = 0; i < str.length(); ++i)
	{
		if (str[i] >= 'a' && str[i] <= 'z')
		{
			allcaps += str[i] - ('a' - 'A');
		}
		else
		{
			allcaps += str[i];
		}
	}

	return allcaps;
}
}

VideoBackend::VideoBackend(void* windowHandle)
    : PixelIndex(0)
    , StopRendering(false)
    , OverscanEnabled(true)
    , CurrentFps(0)
{
#ifdef _WIN32
	InitGDIObjects(windowHandle);
#elif defined(__linux)
    InitXWindow(windowHandle);
    InitCairo();

	FrontBuffer = new uint8_t[256 * 240 * 4];
	BackBuffer = new uint8_t[256 * 240 * 4];
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
	CleanUpGDIObjects();
#elif __linux
    DestroyCairo();
    DestroyXWindow();

	delete[] BackBuffer;
	delete[] FrontBuffer;
#endif

    
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
    Messages.push_back(std::make_pair(ToUpperCase(message), expires));
}

void VideoBackend::Swap()
{
    std::unique_lock<std::mutex> lock(RenderLock);

    uint8_t* temp = FrontBuffer;
    FrontBuffer = BackBuffer;
    BackBuffer = temp;

#ifdef _WIN32
	// Also need to swap bitmaps on Windows
	HBITMAP btemp = FrontBitmap;
	FrontBitmap = BackBitmap;
	BackBitmap = btemp;
#endif

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
    }
}

void VideoBackend::UpdateSurfaceSize()
{
#ifdef _WIN32
	RECT rect;
	GetWindowRect(Window, &rect);

	uint32_t newWidth = rect.right - rect.left;
	uint32_t newHeight = rect.bottom - rect.top;

	if (WindowWidth != newWidth || WindowHeight != newHeight)
	{
		WindowWidth = newWidth;
		WindowHeight = newHeight;

		DeleteObject(IntermediateBitmap);

		HDC windowDC = GetDC(Window);
		IntermediateBitmap = CreateCompatibleBitmap(windowDC, WindowWidth, WindowHeight);
		ReleaseDC(Window, windowDC);
	}

#elif defined(__linux)   
    XWindowAttributes attributes;
    if (XGetWindowAttributes(XDisplay, XParentWindow, &attributes) == 0)
    {
        return;
    }

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
	HDC windowDC = GetDC(Window);
	HDC initialDC = CreateCompatibleDC(windowDC);
	HDC intermediateDC = CreateCompatibleDC(windowDC);

	// Pointers to hold default objects from device contexts
	HGDIOBJ memOldObj;
	HGDIOBJ intOldObj;
	HGDIOBJ intOldFont;

	// Select fonts and bitmaps into device contexts
	intOldFont = SelectObject(intermediateDC, Font);
	intOldObj = SelectObject(intermediateDC, IntermediateBitmap);
	memOldObj = SelectObject(initialDC, FrontBitmap);

	// First we need to blit to an intermediate device context to stretch the frame
	if (OverscanEnabled)
	{
		StretchBlt(intermediateDC, 0, 0, WindowWidth, WindowHeight, initialDC, 0, 8, 256, 224, SRCCOPY);
	}
	else
	{
		StretchBlt(intermediateDC, 0, 0, WindowWidth, WindowHeight, initialDC, 0, 0, 256, 240, SRCCOPY);
	}

	// Draw fps and messages on the intermediate device context
	if (ShowingFps)
	{
		DrawFps(intermediateDC);
	}

	DrawMessages(intermediateDC);

	// Copy final frame to the final device context so it will be displayed 
	BitBlt(windowDC, 0, 0, WindowWidth, WindowHeight, intermediateDC, 0, 0, SRCCOPY);

	// Select default objects back into their device contexts
	SelectObject(initialDC, memOldObj);
	SelectObject(intermediateDC, intOldObj);
	SelectObject(intermediateDC, intOldFont);

	// Clean Up device contexts
	DeleteDC(initialDC);
	DeleteDC(intermediateDC);
	ReleaseDC(Window, windowDC);
#elif defined(__linux)     
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

	if (ShowingFps)
	{
		DrawFps();
	}

	DrawMessages();
#endif
}

#ifdef _WIN32
void VideoBackend::DrawFps(HDC dc)
#elif defined(__linux)
void VideoBackend::DrawFps()
#endif
{
	std::string fps = std::to_string(CurrentFps);
#ifdef _WIN32
	SetBkMode(dc, TRANSPARENT);
	SetTextColor(dc, RGB(255, 255, 255));
	HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));

	SIZE textSize;
	GetTextExtentPoint32(dc, fps.c_str(), static_cast<int>(fps.length()), &textSize);

	RECT rect;
	rect.top = 8;
	rect.left = WindowWidth - textSize.cx - 8 - (FontMetric.tmInternalLeading*2) + 1;
	rect.bottom = 8 + textSize.cy;
	rect.right = WindowWidth - 7;

	// Draw Backing Rectangle
	FillRect(dc, &rect, brush);

	rect.top = 8;
	rect.left = WindowWidth - textSize.cx - 8 - FontMetric.tmInternalLeading + 1;
	rect.bottom = 8 + textSize.cy;
	rect.right = WindowWidth - 7;

	// Draw FPS text
	DrawText(dc, fps.c_str(), static_cast<int>(fps.length()), &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

	DeleteObject(brush);
#elif defined(__linux)  
    cairo_text_extents_t extents;
    cairo_text_extents(CairoContext, fps.c_str(), &extents);
    
    cairo_set_source_rgb(CairoContext, 0.0, 0.0, 0.0);
    cairo_rectangle(CairoContext, WindowWidth - extents.x_advance - 11.0, 8.0, extents.x_advance + 4.0, extents.height + 6.0);
    cairo_fill(CairoContext);
    
    cairo_move_to(CairoContext, WindowWidth - extents.x_advance - 9.0, 11.0 + extents.height);
    cairo_set_source_rgb(CairoContext, 1.0, 1.0, 1.0);
    cairo_show_text(CairoContext, fps.c_str());
#endif
}
#ifdef _WIN32
void VideoBackend::DrawMessages(HDC dc)
#elif defined(__linux)
void VideoBackend::DrawMessages()
#endif
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
#ifdef _WIN32
	SetBkMode(dc, TRANSPARENT);
	SetTextColor(dc, RGB(255, 255, 255));
	HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
	
	int offsetY = 8;
	for (auto& entry : Messages)
	{
		std::string& message = entry.first;

		SIZE textSize;
		GetTextExtentPoint32(dc, message.c_str(), static_cast<int>(message.length()), &textSize);

		RECT rect;
		rect.top = offsetY;
		rect.left = 7;
		rect.bottom = offsetY + textSize.cy;
		rect.right = 7 + textSize.cx + (FontMetric.tmInternalLeading*2) + 1;

		// Draw Backing Rectangle
		FillRect(dc, &rect, brush);

		rect.top = offsetY;
		rect.left = 7 + FontMetric.tmInternalLeading;
		rect.bottom = offsetY + textSize.cy;
		rect.right = 7 + FontMetric.tmInternalLeading + textSize.cx;

		// Draw Message Text
		DrawText(dc, message.c_str(), static_cast<int>(message.length()), &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

		offsetY += textSize.cy;
	}

	DeleteObject(brush);
#elif defined(__linux)
	double offsetY = 8.0;
	for (auto& entry : Messages)
	{
		cairo_text_extents_t extents;
		cairo_text_extents(CairoContext, entry.first.c_str(), &extents);

		cairo_set_source_rgb(CairoContext, 0.0, 0.0, 0.0);
		cairo_rectangle(CairoContext, 7.0, offsetY, extents.x_advance + 4.0, extents.height + 6.0);
		cairo_fill(CairoContext);

		cairo_move_to(CairoContext, 9.0, offsetY + 3.0 - extents.y_bearing);
		cairo_set_source_rgb(CairoContext, 1.0, 1.0, 1.0);
		cairo_show_text(CairoContext, entry.first.c_str());

		offsetY += extents.height + 6.0;
	}
#endif        
}

#ifdef _WIN32
void VideoBackend::InitGDIObjects(void* handle)
{
	std::string error;
	BITMAPINFOHEADER bmih = { 0 };
	BITMAPINFO bmi = { 0 };

	Window = reinterpret_cast<HWND>(handle);

	Font = CreateFont(-20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH, "Consolas");

	// Failed to find consolas, use closest match
	if (Font == NULL)
	{
		Font = CreateFont(-20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH, NULL);

		// Still failed to find a font, error
		if (Font == NULL)
		{
			error = "Failed to initialize message font";
			goto FailedExit;
		}
	}

	HDC winDC = GetDC(Window);

	if (winDC == NULL)
	{
		error = "Failed to get window device context";
		goto FailedExit;
	}

	if (!GetTextMetrics(winDC, &FontMetric))
	{
		error = "Failed to get font metrics";
		goto FailedExit;
	}
	
	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biWidth = 256;
	bmih.biHeight = -240;
	bmih.biPlanes = 1;
	bmih.biBitCount = 32;
	bmih.biCompression = BI_RGB;

	bmi.bmiHeader = bmih;
	bmi.bmiColors->rgbBlue = 0;
	bmi.bmiColors->rgbGreen = 0;
	bmi.bmiColors->rgbRed = 0;
	bmi.bmiColors->rgbReserved = 0;

	void* frontBuffer;
	void* backBuffer;

	FrontBitmap = CreateDIBSection(winDC, &bmi, DIB_RGB_COLORS, &frontBuffer, NULL, 0);

	if (FrontBitmap == NULL)
	{
		error = "Failed to create front frame buffer";
		goto FailedExit;
	}

	BackBitmap = CreateDIBSection(winDC, &bmi, DIB_RGB_COLORS, &backBuffer, NULL, 0);

	if (BackBitmap == NULL)
	{
		error = "Failed to create back frame buffer";
		goto FailedExit;
	}

	FrontBuffer = reinterpret_cast<uint8_t*>(frontBuffer);
	BackBuffer = reinterpret_cast<uint8_t*>(backBuffer);

	ReleaseDC(Window, winDC);

	IntermediateBitmap = NULL;

	return;

FailedExit:
	DeleteObject(FrontBitmap);
	DeleteObject(BackBitmap);
	DeleteObject(Font);

	throw std::runtime_error(error);
}

void VideoBackend::CleanUpGDIObjects()
{
	DeleteObject(IntermediateBitmap);
	DeleteObject(FrontBitmap);
	DeleteObject(BackBitmap);
	DeleteObject(Font);
}
#endif

#ifdef __linux
void VideoBackend::InitXWindow(void* handle)
{
    XParentWindow = reinterpret_cast<Window>(handle);
    XDisplay = XOpenDisplay(nullptr);

    if (XDisplay == nullptr)
    {
        throw std::runtime_error("Failed to connect to X11 Display");
    }

    XWindowAttributes attributes;
    if (XGetWindowAttributes(XDisplay, XParentWindow, &attributes) == 0)
    {
		XCloseDisplay(XDisplay);
        throw std::runtime_error("Failed to retrieve X11 window attributes");
    }

    WindowWidth = attributes.width;
    WindowHeight = attributes.height;

    XWindow = XCreateSimpleWindow(XDisplay, XParentWindow, 0, 0, WindowWidth, WindowHeight, 0, 0, 0);

    if (XMapWindow(XDisplay, XWindow) == 0)
    {
		XDestroyWindow(XDisplay, XWindow);
		XCloseDisplay(XDisplay);
        throw std::runtime_error("Failed to map X11 window");
    }

    XSync(XDisplay, false);
}

void VideoBackend::InitCairo()
{
    XWindowAttributes attributes;
    if (XGetWindowAttributes(XDisplay, XWindow, &attributes) == 0)
    {
		XDestroyWindow(XDisplay, XWindow);
		XCloseDisplay(XDisplay);
        throw std::runtime_error("Failed to retrieve X11 window attributes");
    }

    CairoXSurface = cairo_xlib_surface_create(XDisplay, XWindow, attributes.visual, attributes.width, attributes.height);

    if (CairoXSurface == nullptr)
    {
		XDestroyWindow(XDisplay, XWindow);
		XCloseDisplay(XDisplay);
        throw std::runtime_error("Failed to initialize cairo surface");
    }

    CairoContext = cairo_create(CairoXSurface);

    if (CairoContext == nullptr)
    {
		cairo_surface_destroy(CairoXSurface);
		XDestroyWindow(XDisplay, XWindow);
		XCloseDisplay(XDisplay);
        throw std::runtime_error("Failed to initialize cairo context");
    }

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
