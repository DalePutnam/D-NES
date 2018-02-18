#include <string>
#include <cassert>

#ifdef __linux
#include <cairo/cairo-xlib.h>
#endif

#include "video_backend.h"
#include "gl\glext.h"
#include "font_small.h"

#if defined(_WIN32)
#pragma comment(lib, "opengl32.lib")
#define LOAD_OGL_FUNC(name) wglGetProcAddress(name)
#elif defined(__linux)
#define LOAD_OGL_FUNC(name) glXGetProcAddress(name)
#endif

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

const std::string frameVertexShader =
	R"(#version 330 core
	layout(location = 0) in vec3 vert;
	out vec2 uv;
	void main() {
		gl_Position = vec4((vert.x*2.0)-1.0, (vert.y*2.0)-1.0, 1.0, 1.0);
		uv = vec2(vert.x, -vert.y);
	})";

const std::string frameFragmentShader =
	R"(#version 330 core
	in vec2 uv;
	out vec3 color;
	uniform sampler2D sampler;
	void main() {
		color = texture(sampler, uv).rgb;
	})";

const std::string textVertexShader =
	R"(#version 330 core
	layout(location = 0) in vec2 inVertex;
	layout(location = 1) in vec2 inUV;
	out vec2 outUV;

	uniform vec2 screenSize;
	void main() {
		vec2 halfScreen = screenSize / 2;
		vec2 clipSpace = inVertex - halfScreen;
		clipSpace /= halfScreen;
		gl_Position = vec4(clipSpace, 0, 1);

		outUV = inUV;
	})";

const std::string textFragmentShader =
	R"(#version 330 core
	in vec2 outUV;
	out vec3 color;
	uniform sampler2D sampler;
	void main() {
		color = texture(sampler, outUV).rgb;
	})";

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
thread_local PFNGLACTIVETEXTUREPROC glActiveTexture;

thread_local std::unique_ptr<std::string> compileErrorMessage = std::make_unique<std::string>();

void InitializeFunctions()
{
	glGenVertexArrays = reinterpret_cast<PFNGLGENVERTEXARRAYSPROC>(LOAD_OGL_FUNC("glGenVertexArrays"));
	glBindVertexArray = reinterpret_cast<PFNGLBINDVERTEXARRAYPROC>(LOAD_OGL_FUNC("glBindVertexArray"));
	glGenBuffers = reinterpret_cast<PFNGLGENBUFFERSPROC>(LOAD_OGL_FUNC("glGenBuffers"));
	glBindBuffer = reinterpret_cast<PFNGLBINDBUFFERPROC>(LOAD_OGL_FUNC("glBindBuffer"));
	glBufferData = reinterpret_cast<PFNGLBUFFERDATAPROC>(LOAD_OGL_FUNC("glBufferData"));
	glEnableVertexAttribArray = reinterpret_cast<PFNGLENABLEVERTEXATTRIBARRAYPROC>(LOAD_OGL_FUNC("glEnableVertexAttribArray"));
	glVertexAttribPointer = reinterpret_cast<PFNGLVERTEXATTRIBPOINTERPROC>(LOAD_OGL_FUNC("glVertexAttribPointer"));
	glDisableVertexAttribArray = reinterpret_cast<PFNGLDISABLEVERTEXATTRIBARRAYPROC>(LOAD_OGL_FUNC("glDisableVertexAttribArray"));
	glCreateShader = reinterpret_cast<PFNGLCREATESHADERPROC>(LOAD_OGL_FUNC("glCreateShader"));
	glShaderSource = reinterpret_cast<PFNGLSHADERSOURCEPROC>(LOAD_OGL_FUNC("glShaderSource"));
	glCompileShader = reinterpret_cast<PFNGLCOMPILESHADERPROC>(LOAD_OGL_FUNC("glCompileShader"));
	glGetShaderiv = reinterpret_cast<PFNGLGETSHADERIVPROC>(LOAD_OGL_FUNC("glGetShaderiv"));
	glGetShaderInfoLog = reinterpret_cast<PFNGLGETSHADERINFOLOGPROC>(LOAD_OGL_FUNC("glGetShaderInfoLog"));
	glCreateProgram = reinterpret_cast<PFNGLCREATEPROGRAMPROC>(LOAD_OGL_FUNC("glCreateProgram"));
	glAttachShader = reinterpret_cast<PFNGLATTACHSHADERPROC>(LOAD_OGL_FUNC("glAttachShader"));
	glLinkProgram = reinterpret_cast<PFNGLLINKPROGRAMPROC>(LOAD_OGL_FUNC("glLinkProgram"));
	glGetProgramiv = reinterpret_cast<PFNGLGETPROGRAMIVPROC>(LOAD_OGL_FUNC("glGetProgramiv"));
	glGetProgramInfoLog = reinterpret_cast<PFNGLGETPROGRAMINFOLOGPROC>(LOAD_OGL_FUNC("glGetProgramInfoLog"));
	glDetachShader = reinterpret_cast<PFNGLDETACHSHADERPROC>(LOAD_OGL_FUNC("glDetachShader"));
	glDeleteShader = reinterpret_cast<PFNGLDELETESHADERPROC>(LOAD_OGL_FUNC("glDeleteShader"));
	glUseProgram = reinterpret_cast<PFNGLUSEPROGRAMPROC>(LOAD_OGL_FUNC("glUseProgram"));
	glUniform1i = reinterpret_cast<PFNGLUNIFORM1IPROC>(LOAD_OGL_FUNC("glUniform1i"));
	glUniform2f = reinterpret_cast<PFNGLUNIFORM2FPROC>(LOAD_OGL_FUNC("glUniform2f"));
	glGetUniformLocation = reinterpret_cast<PFNGLGETUNIFORMLOCATIONPROC>(LOAD_OGL_FUNC("glGetUniformLocation"));
	glActiveTexture = reinterpret_cast<PFNGLACTIVETEXTUREPROC>(LOAD_OGL_FUNC("glActiveTexture"));
}

GLint CompileShaders(const std::string& vertexShader, const std::string& fragmentShader, GLuint* programId)
{
	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

	GLint result = GL_FALSE;
	int infoLogLength;

	const char* vertexPrgPtr = vertexShader.c_str();
	glShaderSource(vertexShaderId, 1, &vertexPrgPtr, NULL);
	glCompileShader(vertexShaderId);

	glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &result);
	glGetShaderiv(vertexShaderId, GL_INFO_LOG_LENGTH, &infoLogLength);

	if (result == GL_FALSE)
	{
		if (infoLogLength > 0)
		{
			char* errorMessage = new char[infoLogLength];
			glGetShaderInfoLog(vertexShaderId, infoLogLength, NULL, errorMessage);
			compileErrorMessage = std::make_unique<std::string>(errorMessage);
			delete[] errorMessage;
		}

		return GL_FALSE;
	}

	const char* fragmentPrgPtr = fragmentShader.c_str();
	glShaderSource(fragmentShaderId, 1, &fragmentPrgPtr, NULL);
	glCompileShader(fragmentShaderId);

	glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &result);
	glGetShaderiv(fragmentShaderId, GL_INFO_LOG_LENGTH, &infoLogLength);

	if (result == GL_FALSE)
	{
		if (infoLogLength > 0)
		{
			char* errorMessage = new char[infoLogLength];
			glGetShaderInfoLog(fragmentShaderId, infoLogLength, NULL, errorMessage);
			compileErrorMessage = std::make_unique<std::string>(errorMessage);
			delete[] errorMessage;
		}

		return GL_FALSE;
	}

	GLint prgId = glCreateProgram();
	glAttachShader(prgId, vertexShaderId);
	glAttachShader(prgId, fragmentShaderId);
	glLinkProgram(prgId);

	glGetProgramiv(prgId, GL_LINK_STATUS, &result);
	glGetProgramiv(prgId, GL_INFO_LOG_LENGTH, &infoLogLength);

	if (result == GL_FALSE)
	{
		if (infoLogLength > 0)
		{
			char* errorMessage = new char[infoLogLength];
			glGetShaderInfoLog(prgId, infoLogLength, NULL, errorMessage);
			compileErrorMessage = std::make_unique<std::string>(errorMessage);
			delete[] errorMessage;
		}

		return GL_FALSE;
	}

	glDetachShader(prgId, vertexShaderId);
	glDetachShader(prgId, fragmentShaderId);

	glDeleteShader(vertexShaderId);
	glDeleteShader(fragmentShaderId);

	*programId = prgId;

	return GL_TRUE;
}
}

VideoBackend::VideoBackend(void* windowHandle)
    : OverscanEnabled(true)
    , CurrentFps(0)
{
#ifdef _WIN32
	Window = reinterpret_cast<HWND>(windowHandle);
#elif defined(__linux)
    InitXWindow(windowHandle);
    InitCairo();

    FrontBuffer = new uint8_t[256 * 240 * 4];
    BackBuffer = new uint8_t[256 * 240 * 4];
#endif
}

VideoBackend::~VideoBackend()
{
#ifdef _WIN32
#elif __linux
    DestroyCairo();
    DestroyXWindow();
#endif

    
}

void VideoBackend::Prepare()
{
#if defined(_WIN32)
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
#elif defined(__linux)
#endif

	InitializeFunctions();

	if (!CompileShaders(frameVertexShader, frameFragmentShader, &FrameProgramId))
	{
		Finalize();
		throw std::runtime_error(*compileErrorMessage);
	}

	if (!CompileShaders(textVertexShader, textFragmentShader, &TextProgramId))
	{
		Finalize();
		throw std::runtime_error(*compileErrorMessage);
	}

	glGenVertexArrays(1, &FrameVertexArrayId);
	glBindVertexArray(FrameVertexArrayId);

	static const GLfloat g_vertex_buffer_data[] = {
		0.0f,  0.0f, 0.0f,
		0.0f,  1.0f, 0.0f,
		1.0f,  0.0f, 0.0f,
		1.0f,  1.0f, 0.0f
	};

	// Generate 1 buffer, put the resulting identifier in vertexbuffer
	glGenBuffers(1, &FrameVertexBuffer);
	// The following commands will talk about our 'vertexbuffer' buffer
	glBindBuffer(GL_ARRAY_BUFFER, FrameVertexBuffer);
	// Give our vertices to OpenGL.
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	glGenTextures(1, &FrameTextureId);
	glBindTexture(GL_TEXTURE_2D, FrameTextureId);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glGenTextures(1, &TextTextureId);
	glBindTexture(GL_TEXTURE_2D, TextTextureId);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 128, 0, GL_BGR, GL_UNSIGNED_BYTE, fontBitmap);

	glGenBuffers(1, &TextVertexBuffer);
	glGenBuffers(1, &TextUVBuffer);
}

void VideoBackend::Finalize()
{
#if defined(_WIN32)
	wglMakeCurrent(WinDc, NULL);
	wglDeleteContext(Oglc);
#elif defined(__linux)
#endif
}

void VideoBackend::DrawFrame(uint8_t * fb)
{
	bool overscanEnabled = OverscanEnabled;
	bool showingFps = ShowingFps;
	uint32_t currentFps = CurrentFps;

	UpdateSurfaceSize();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, WindowWidth, WindowHeight);

	glUseProgram(FrameProgramId);

	glBindTexture(GL_TEXTURE_2D, FrameTextureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, overscanEnabled ? 224 : 240, 0, GL_BGRA, GL_UNSIGNED_BYTE, overscanEnabled ? fb + 8192 : fb);
	/*
	GLint loc = glGetUniformLocation(ProgramId, "screenSize");
	glUniform2f(loc, static_cast<float>(WindowWidth), static_cast<float>(WindowHeight));

	loc = glGetUniformLocation(ProgramId, "frameSize");
	glUniform2f(loc, 256, overscanEnabled ? 224.f : 240.f);
	*/

	glUniform1i(FrameTextureId, 0);

	// 1st attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, FrameVertexBuffer);
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

	if (showingFps)
	{
		DrawFps(currentFps);
	}

	DrawMessages();

	SwapBuffers(WinDc);
}

void VideoBackend::DrawFps(uint32_t fps)
{
	std::string fpsStr = std::to_string(fps);

	uint32_t xPos = WindowWidth - 12 - fpsStr.length() * fontBitmapCellWidth;
	uint32_t yPos = WindowHeight - 12 - fontBitmapCellHeight;

	DrawText(fpsStr, xPos, yPos);
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

	uint32_t xPos = 12;
	uint32_t yPos = WindowHeight - 12 - fontBitmapCellHeight;

    for (auto& entry : Messages)
    {
        const std::string& message = entry.first;
        DrawText(message, xPos, yPos);

		yPos -= fontBitmapCellHeight;
    }
}

void VideoBackend::DrawText(const std::string & text, uint32_t xPos, uint32_t yPos)
{
	static constexpr float uvWidth = static_cast<float>(fontBitmapCellWidth) / static_cast<float>(fontBitmapWidth);
	static constexpr float uvHeight = static_cast<float>(fontBitmapCellHeight) / static_cast<float>(fontBitmapHeight);

	std::vector<float> vertices;
	std::vector<float> uvs;

	vertices.reserve(text.length() * 12);
	uvs.reserve(text.length() * 12);

	float x = xPos;
	float y = yPos;

	for (uint32_t i = 0; i < text.length(); ++i)
	{
		float upLeftX = x + i * fontBitmapCellWidth;
		float upLeftY = y + fontBitmapCellHeight;
		float upRightX = x + i * fontBitmapCellWidth + fontBitmapCellWidth;
		float upRightY = y + fontBitmapCellHeight;

		float downRightX = x + i * fontBitmapCellWidth + fontBitmapCellWidth;
		float downRightY = y;
		float downLeftX = x + i * fontBitmapCellWidth;
		float downLeftY = y;

		vertices.push_back(upLeftX);
		vertices.push_back(upLeftY);
		vertices.push_back(downLeftX);
		vertices.push_back(downLeftY);
		vertices.push_back(upRightX);
		vertices.push_back(upRightY);
		vertices.push_back(downRightX);
		vertices.push_back(downRightY);
		vertices.push_back(upRightX);
		vertices.push_back(upRightY);
		vertices.push_back(downLeftX);
		vertices.push_back(downLeftY);

		char character = text[i];
		uint32_t index = character - 32;

		uint32_t col = index % 36;
		uint32_t row = index / 36;

		float uvX = uvWidth * col;
		float uvY = uvHeight * row;

		float uvUpLeftX = uvX;
		float uvUpLeftY = 1.f - uvY;
		float uvUpRightX = uvX + uvWidth;
		float uvUpRightY = 1.f - uvY;

		float uvDownRightX = uvX + uvWidth;
		float uvDownRightY = 1.f - (uvY + uvHeight);
		float uvDownLeftX = uvX;
		float uvDownLeftY = 1.f - (uvY + uvHeight);

		uvs.push_back(uvUpLeftX);
		uvs.push_back(uvUpLeftY);
		uvs.push_back(uvDownLeftX);
		uvs.push_back(uvDownLeftY);
		uvs.push_back(uvUpRightX);
		uvs.push_back(uvUpRightY);
		uvs.push_back(uvDownRightX);
		uvs.push_back(uvDownRightY);
		uvs.push_back(uvUpRightX);
		uvs.push_back(uvUpRightY);
		uvs.push_back(uvDownLeftX);
		uvs.push_back(uvDownLeftY);
	}

	glBindBuffer(GL_ARRAY_BUFFER, TextVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*vertices.size(), vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, TextUVBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*uvs.size(), uvs.data(), GL_STATIC_DRAW);

	glUseProgram(TextProgramId);

	// 1st attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, TextVertexBuffer);
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		2,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);

	// 1st attribute buffer : vertices
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, TextUVBuffer);
	glVertexAttribPointer(
		1,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		2,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);

	GLint loc = glGetUniformLocation(TextProgramId, "screenSize");
	glUniform2f(loc, static_cast<float>(WindowWidth), static_cast<float>(WindowHeight));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, TextTextureId);
	glUniform1i(TextTextureId, 0);

	glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 2);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}

void VideoBackend::SetOverscanEnabled(bool enabled)
{
    OverscanEnabled = enabled;
}

void VideoBackend::ShowFps(bool show)
{
    ShowingFps = show;
}

void VideoBackend::SetFps(uint32_t fps)
{
    CurrentFps = fps;
}

void VideoBackend::ShowMessage(const std::string& message, uint32_t duration)
{
    using namespace std::chrono;

    steady_clock::time_point expires = steady_clock::now() + seconds(duration);
    Messages.push_back(std::make_pair(ToUpperCase(message), expires));
}

void VideoBackend::Swap()
{
	UpdateSurfaceSize();
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
