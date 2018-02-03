#include <string>
#include <cassert>

#ifdef __linux
#include <cairo/cairo-xlib.h>
#endif

#include "video_backend.h"
#include "gl\glext.h"

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

thread_local PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
thread_local PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
thread_local PFNGLGENBUFFERSPROC glGenBuffers;
thread_local PFNGLBINDBUFFERPROC glBindBuffer;
thread_local PFNGLBUFFERDATAPROC glBufferData;
thread_local PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
thread_local PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
thread_local PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
thread_local PFNGLCREATESHADERPROC glCreateShader;
thread_local PFNGLSHADERSOURCEPROC glShaderSource;
thread_local PFNGLCOMPILESHADERPROC glCompileShader;
thread_local PFNGLGETSHADERIVPROC glGetShaderiv;
thread_local PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
thread_local PFNGLCREATEPROGRAMPROC glCreateProgram;
thread_local PFNGLATTACHSHADERPROC glAttachShader;
thread_local PFNGLLINKPROGRAMPROC glLinkProgram;
thread_local PFNGLGETPROGRAMIVPROC glGetProgramiv;
thread_local PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
thread_local PFNGLDETACHSHADERPROC glDetachShader;
thread_local PFNGLDELETESHADERPROC glDeleteShader;
thread_local PFNGLUSEPROGRAMPROC glUseProgram;
thread_local PFNGLUNIFORM1IPROC glUniform1i;
thread_local PFNGLUNIFORM2FPROC glUniform2f;
thread_local PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;

void InitializeFunctions() {
	glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
	glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
	glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
	glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
	glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
	glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
	glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
	glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glDisableVertexAttribArray");
	glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
	glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
	glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
	glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
	glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
	glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
	glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
	glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
	glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
	glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
	glDetachShader = (PFNGLDETACHSHADERPROC)wglGetProcAddress("glDetachShader");
	glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
	glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
	glUniform1i = (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i");
	glUniform2f = (PFNGLUNIFORM2FPROC)wglGetProcAddress("glUniform2f");
	glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
}

}

VideoBackend::VideoBackend(void* windowHandle)
    : PixelIndex(0)
    , StopRendering(false)
    , OverscanEnabled(true)
    , CurrentFps(0)
{
#ifdef _WIN32
	Window = reinterpret_cast<HWND>(windowHandle);
	FrontBuffer = new uint8_t[256 * 240 * 4];
	BackBuffer = new uint8_t[256 * 240 * 4];
	initialized = false;
#elif defined(__linux)
    InitXWindow(windowHandle);
    InitCairo();

    FrontBuffer = new uint8_t[256 * 240 * 4];
    BackBuffer = new uint8_t[256 * 240 * 4];
#endif

    //RenderThread = std::thread(&VideoBackend::RenderWorker, this);
}

VideoBackend::~VideoBackend()
{/*
    {
        std::unique_lock<std::mutex> lock(RenderLock);
        StopRendering = true;
        RenderCv.notify_all();
    }
    
    RenderThread.join();*/

#ifdef _WIN32
	delete[] BackBuffer;
	delete[] FrontBuffer;
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

void VideoBackend::Prepare()
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

	WinDc = GetDC(Window);

	int pf = ChoosePixelFormat(WinDc, &pfd);
	assert(SetPixelFormat(WinDc, pf, &pfd));

	Oglc = wglCreateContext(WinDc);
	assert(wglMakeCurrent(WinDc, Oglc));

	InitializeFunctions();

	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

	static const std::string vertexShaderPrg =
		"#version 330 core\n\
		 layout(location = 0) in vec3 vert;\n\
		 out vec2 uv;\n\
		 void main() {\n\
			gl_Position = vec4((vert.x*2.0)-1.0, (vert.y*2.0)-1.0, 1.0, 1.0);\
			uv = vec2(vert.x, -vert.y);\n\
	     }\n";

	static const std::string fragmentShaderPrg =
		"#version 330 core\n\
		 in vec2 uv;\n\
		 out vec3 color;\n\
		 uniform sampler2D sampler;\n\
		 void main() {\n\
			color = texture(sampler, uv).rgb;\n\
		 }\n";

	GLint result = GL_FALSE;
	int infoLogLength;

	const char* vertexPrgPtr = vertexShaderPrg.c_str();
	glShaderSource(vertexShaderId, 1, &vertexPrgPtr, NULL);
	glCompileShader(vertexShaderId);

	glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &result);
	glGetShaderiv(vertexShaderId, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (infoLogLength > 0) {
		std::vector<char> VertexShaderErrorMessage(infoLogLength + 1);
		glGetShaderInfoLog(vertexShaderId, infoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}

	const char* fragmentPrgPtr = fragmentShaderPrg.c_str();
	glShaderSource(fragmentShaderId, 1, &fragmentPrgPtr, NULL);
	glCompileShader(fragmentShaderId);

	glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &result);
	glGetShaderiv(fragmentShaderId, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (infoLogLength > 0) {
		std::vector<char> FragmentShaderErrorMessage(infoLogLength + 1);
		glGetShaderInfoLog(fragmentShaderId, infoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}

	ProgramId = glCreateProgram();
	glAttachShader(ProgramId, vertexShaderId);
	glAttachShader(ProgramId, fragmentShaderId);
	glLinkProgram(ProgramId);

	glGetProgramiv(ProgramId, GL_LINK_STATUS, &result);
	glGetProgramiv(ProgramId, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (infoLogLength > 0) {
		std::vector<char> ProgramErrorMessage(infoLogLength + 1);
		glGetProgramInfoLog(ProgramId, infoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}

	glDetachShader(ProgramId, vertexShaderId);
	glDetachShader(ProgramId, fragmentShaderId);

	glDeleteShader(vertexShaderId);
	glDeleteShader(fragmentShaderId);

	glGenVertexArrays(1, &VertexArrayId);
	glBindVertexArray(VertexArrayId);

	static const GLfloat g_vertex_buffer_data[] = {
		0.0f,  0.0f, 0.0f,
		0.0f,  1.0f, 0.0f,
		1.0f,  0.0f, 0.0f,
		1.0f,  1.0f, 0.0f
	};

	// Generate 1 buffer, put the resulting identifier in vertexbuffer
	glGenBuffers(1, &VertexBuffer);
	// The following commands will talk about our 'vertexbuffer' buffer
	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
	// Give our vertices to OpenGL.
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	glGenTextures(1, &TextureId);
	glBindTexture(GL_TEXTURE_2D, TextureId);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

void VideoBackend::Finalize()
{
	wglMakeCurrent(WinDc, NULL);
	wglDeleteContext(Oglc);
}

void VideoBackend::DrawFrame(uint8_t * fb)
{
	UpdateSurfaceSize();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, WindowWidth, WindowHeight);

	glUseProgram(ProgramId);

	glBindTexture(GL_TEXTURE_2D, TextureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, OverscanEnabled ? 224 : 240, 0, GL_BGRA, GL_UNSIGNED_BYTE, OverscanEnabled ? fb + 8192 : fb);

	GLint loc = glGetUniformLocation(ProgramId, "screenSize");
	glUniform2f(loc, WindowWidth, WindowHeight);

	loc = glGetUniformLocation(ProgramId, "frameSize");
	glUniform2f(loc, 256, OverscanEnabled ? 224 : 240);

	glUniform1i(TextureId, 0);

	// 1st attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);

	// Draw the triangle !
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisableVertexAttribArray(0);

	SwapBuffers(WinDc);
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
	/*
    // Also need to swap bitmaps on Windows
    HBITMAP btemp = FrontBitmap;
    FrontBitmap = BackBitmap;
    BackBitmap = btemp;*/
#endif

    PixelIndex = 0;

    //RenderCv.notify_all();
	UpdateSurfaceSize();
	DrawFrame();
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
		/*
        DeleteObject(IntermediateBitmap);

        HDC windowDC = GetDC(Window);
        IntermediateBitmap = CreateCompatibleBitmap(windowDC, WindowWidth, WindowHeight);
        ReleaseDC(Window, windowDC);*/
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
	if (!initialized) {
		InitOgl();
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, WindowWidth, WindowHeight);

	glUseProgram(ProgramId);

	glBindTexture(GL_TEXTURE_2D, TextureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, OverscanEnabled ? 224 : 240, 0, GL_BGRA, GL_UNSIGNED_BYTE, OverscanEnabled ? BackBuffer + 8192 : BackBuffer);

	GLint loc = glGetUniformLocation(ProgramId, "screenSize");
	glUniform2f(loc, WindowWidth, WindowHeight);

	loc = glGetUniformLocation(ProgramId, "frameSize");
	glUniform2f(loc, 256, OverscanEnabled ? 224 : 240);

	glUniform1i(TextureId, 0);

	// 1st attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);
	// Draw the triangle !
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // Starting from vertex 0; 3 vertices total -> 1 triangle
	glDisableVertexAttribArray(0);

	SwapBuffers(WinDc);
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

//#ifdef _WIN32
//void VideoBackend::DrawFps(HDC dc)
//#elif defined(__linux)
//void VideoBackend::DrawFps()
//#endif
//{
//    std::string fps = std::to_string(CurrentFps);
//#ifdef _WIN32
//	/*
//    SetBkMode(dc, TRANSPARENT);
//    SetTextColor(dc, RGB(255, 255, 255));
//    HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
//
//    SIZE textSize;
//    GetTextExtentPoint32(dc, fps.c_str(), static_cast<int>(fps.length()), &textSize);
//
//    RECT rect;
//    rect.top = 8;
//    rect.left = WindowWidth - textSize.cx - 8 - (FontMetric.tmInternalLeading*2) + 1;
//    rect.bottom = 8 + textSize.cy;
//    rect.right = WindowWidth - 7;
//
//    // Draw Backing Rectangle
//    FillRect(dc, &rect, brush);
//
//    rect.top = 8;
//    rect.left = WindowWidth - textSize.cx - 8 - FontMetric.tmInternalLeading + 1;
//    rect.bottom = 8 + textSize.cy;
//    rect.right = WindowWidth - 7;
//
//    // Draw FPS text
//    DrawText(dc, fps.c_str(), static_cast<int>(fps.length()), &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
//
//    DeleteObject(brush);
//	*/
//#elif defined(__linux)  
//    cairo_text_extents_t extents;
//    cairo_text_extents(CairoContext, fps.c_str(), &extents);
//    
//    cairo_set_source_rgb(CairoContext, 0.0, 0.0, 0.0);
//    cairo_rectangle(CairoContext, WindowWidth - extents.x_advance - 11.0, 8.0, extents.x_advance + 4.0, extents.height + 6.0);
//    cairo_fill(CairoContext);
//    
//    cairo_move_to(CairoContext, WindowWidth - extents.x_advance - 9.0, 11.0 + extents.height);
//    cairo_set_source_rgb(CairoContext, 1.0, 1.0, 1.0);
//    cairo_show_text(CairoContext, fps.c_str());
//#endif
//}
//#ifdef _WIN32
//void VideoBackend::DrawMessages(HDC dc)
//#elif defined(__linux)
//void VideoBackend::DrawMessages()
//#endif
//{
//    if (Messages.empty())
//    {
//        return;
//    }
//
//    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
//    std::vector<std::pair<std::string, std::chrono::steady_clock::time_point> > remaining;
//    for (auto& entry : Messages)
//    {
//        if (entry.second > now)
//        {
//            remaining.push_back(entry);
//        }
//    }
//
//    Messages.swap(remaining);
//
//    // Draw Messages
//#ifdef _WIN32
//	/*
//    SetBkMode(dc, TRANSPARENT);
//    SetTextColor(dc, RGB(255, 255, 255));
//    HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
//    
//    int offsetY = 8;
//    for (auto& entry : Messages)
//    {
//        std::string& message = entry.first;
//
//        SIZE textSize;
//        GetTextExtentPoint32(dc, message.c_str(), static_cast<int>(message.length()), &textSize);
//
//        RECT rect;
//        rect.top = offsetY;
//        rect.left = 7;
//        rect.bottom = offsetY + textSize.cy;
//        rect.right = 7 + textSize.cx + (FontMetric.tmInternalLeading*2) + 1;
//
//        // Draw Backing Rectangle
//        FillRect(dc, &rect, brush);
//
//        rect.top = offsetY;
//        rect.left = 7 + FontMetric.tmInternalLeading;
//        rect.bottom = offsetY + textSize.cy;
//        rect.right = 7 + FontMetric.tmInternalLeading + textSize.cx;
//
//        // Draw Message Text
//        DrawText(dc, message.c_str(), static_cast<int>(message.length()), &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
//
//        offsetY += textSize.cy;
//    }
//
//    DeleteObject(brush);
//	*/
//#elif defined(__linux)
//    double offsetY = 8.0;
//    for (auto& entry : Messages)
//    {
//        cairo_text_extents_t extents;
//        cairo_text_extents(CairoContext, entry.first.c_str(), &extents);
//
//        cairo_set_source_rgb(CairoContext, 0.0, 0.0, 0.0);
//        cairo_rectangle(CairoContext, 7.0, offsetY, extents.x_advance + 4.0, extents.height + 6.0);
//        cairo_fill(CairoContext);
//
//        cairo_move_to(CairoContext, 9.0, offsetY + 3.0 - extents.y_bearing);
//        cairo_set_source_rgb(CairoContext, 1.0, 1.0, 1.0);
//        cairo_show_text(CairoContext, entry.first.c_str());
//
//        offsetY += extents.height + 6.0;
//    }
//#endif        
//}

#ifdef _WIN32
void VideoBackend::InitOgl()
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

	WinDc = GetDC(Window);

	int pf = ChoosePixelFormat(WinDc, &pfd);
	assert(SetPixelFormat(WinDc, pf, &pfd));

	Oglc = wglCreateContext(WinDc);
	assert(wglMakeCurrent(WinDc, Oglc));

	std::string text = (char*)glGetString(GL_VERSION);

	glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
	glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
	glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
	glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
	glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
	glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
	glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
	glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glDisableVertexAttribArray");
	glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
	glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
	glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
	glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
	glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
	glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
	glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
	glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
	glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
	glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
	glDetachShader = (PFNGLDETACHSHADERPROC)wglGetProcAddress("glDetachShader");
	glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
	glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
	glUniform1i = (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i");
	glUniform2f = (PFNGLUNIFORM2FPROC)wglGetProcAddress("glUniform2f");
	glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");

	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

	static const std::string vertexShaderPrg =
		"#version 330 core\n\
		layout(location = 0) in vec3 vert;\n\
		out vec2 uv;\n\
		uniform vec2 screenSize;\
		uniform vec2 frameSize;\
		void main() {\n\
			float wRatio = screenSize.x / frameSize.x;\
			float hRatio = screenSize.y / frameSize.y;\
			if (wRatio < hRatio) {\n\
				float height = frameSize.y * wRatio;\n\
				float hclip = (height / screenSize.y) * 2.0;\n\
				float ypos = (screenSize.y - height) / 2.0;\n\
				float yclip = ((ypos / screenSize.y) * 2.0) - 1.0;\n\
				gl_Position.y = yclip + (hclip * vert.y);\n\
				gl_Position.x = (vert.x * 2.0) - 1.0;\n\
			} else if (hRatio <= wRatio) {\n\
				float width = frameSize.x * hRatio;\n\
				float wclip = (width / screenSize.x) * 2.0;\n\
				float xpos = (screenSize.x - width) / 2.0;\n\
				float xclip = ((xpos / screenSize.x) * 2.0) - 1.0;\n\
				gl_Position.x = xclip + (wclip * vert.x);\n\
				gl_Position.y = (vert.y * 2.0) - 1.0;\n\
			}\n\
			gl_Position.z = 1.0; \n\
			gl_Position.w = 1.0; \n\
			uv.x = vert.x; \
			uv.y = -vert.y; \
	}\n";

	//uv.x = (vert.x + 1.0) / 2.0; \
	//uv.y = (vert.y + 1.0) / -2.0; \
	//gl_Position.xyz = vert; \n\

	static const std::string fragmentShaderPrg =
		"                                  \
		#version 330 core\n                \
		in vec2 uv;\n                      \
		out vec3 color;\n                  \
		uniform sampler2D sampler;\n       \
		void main() {\n                    \
			color = texture(sampler, uv).rgb;\n\
		}\n                                \
		";

	GLint result = GL_FALSE;
	int infoLogLength;

	const char* vertexPrgPtr = vertexShaderPrg.c_str();
	glShaderSource(vertexShaderId, 1, &vertexPrgPtr, NULL);
	glCompileShader(vertexShaderId);

	glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &result);
	glGetShaderiv(vertexShaderId, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (infoLogLength > 0) {
		std::vector<char> VertexShaderErrorMessage(infoLogLength + 1);
		glGetShaderInfoLog(vertexShaderId, infoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}

	const char* fragmentPrgPtr = fragmentShaderPrg.c_str();
	glShaderSource(fragmentShaderId, 1, &fragmentPrgPtr, NULL);
	glCompileShader(fragmentShaderId);

	glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &result);
	glGetShaderiv(fragmentShaderId, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (infoLogLength > 0) {
		std::vector<char> FragmentShaderErrorMessage(infoLogLength + 1);
		glGetShaderInfoLog(fragmentShaderId, infoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}

	ProgramId = glCreateProgram();
	glAttachShader(ProgramId, vertexShaderId);
	glAttachShader(ProgramId, fragmentShaderId);
	glLinkProgram(ProgramId);

	glGetProgramiv(ProgramId, GL_LINK_STATUS, &result);
	glGetProgramiv(ProgramId, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (infoLogLength > 0) {
		std::vector<char> ProgramErrorMessage(infoLogLength + 1);
		glGetProgramInfoLog(ProgramId, infoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}

	glDetachShader(ProgramId, vertexShaderId);
	glDetachShader(ProgramId, fragmentShaderId);

	glDeleteShader(vertexShaderId);
	glDeleteShader(fragmentShaderId);

	glGenVertexArrays(1, &VertexArrayId);
	glBindVertexArray(VertexArrayId);

	static const GLfloat g_vertex_buffer_data[] = {
		0.0f, 0.0f, 0.0f,
		0.0f,  1.0f, 0.0f,
		 1.0f, 0.0f, 0.0f,
		 1.0f,  1.0f, 0.0f
	};

	// Generate 1 buffer, put the resulting identifier in vertexbuffer
	glGenBuffers(1, &VertexBuffer);
	// The following commands will talk about our 'vertexbuffer' buffer
	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
	// Give our vertices to OpenGL.
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	glGenTextures(1, &TextureId);
	glBindTexture(GL_TEXTURE_2D, TextureId);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	initialized = true;
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
