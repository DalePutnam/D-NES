#include <string>
#include <cassert>
#include <memory>

#include "video_backend.h"
#include "glext.h"
#include "font.h"

#if defined(_WIN32)
#pragma comment(lib, "opengl32.lib")
#define LOAD_OGL_FUNC(name) wglGetProcAddress(name)
#elif defined(__linux)
#define LOAD_OGL_FUNC(name) glXGetProcAddress(reinterpret_cast<const GLubyte*>(name))
#endif

namespace
{
static constexpr int32_t FRAME_WIDTH = 256;
static constexpr int32_t FRAME_HEIGHT = 240;
static constexpr int32_t NUM_OVERSCAN_LINES = 16;

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
	uniform vec2 screenSize;
	uniform vec2 frameSize;
	void main() {
		float wRatio = screenSize.x / frameSize.x;
		float hRatio = screenSize.y / frameSize.y;
		if (wRatio < hRatio) {
			float height = frameSize.y * wRatio;
			float hclip = (height / screenSize.y) * 2.0;
			float ypos = (screenSize.y - height) / 2.0;
			float yclip = ((ypos / screenSize.y) * 2.0) - 1.0;
			gl_Position.y = yclip + (hclip * vert.y);
			gl_Position.x = (vert.x * 2.0) - 1.0;
		} else if (hRatio <= wRatio) {
			float width = frameSize.x * hRatio;
			float wclip = (width / screenSize.x) * 2.0;
			float xpos = (screenSize.x - width) / 2.0;
			float xclip = ((xpos / screenSize.x) * 2.0) - 1.0;
			gl_Position.x = xclip + (wclip * vert.x);
			gl_Position.y = (vert.y * 2.0) - 1.0;
		}
		gl_Position.z = 1.0; 
		gl_Position.w = 1.0; 
		uv.x = vert.x; 
		uv.y = -vert.y; 
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
	out vec2 uv;

	uniform vec2 screenSize;
	void main() {
		vec2 halfScreen = screenSize / 2;
		vec2 clipSpace = inVertex - halfScreen;
		clipSpace /= halfScreen;
		gl_Position = vec4(clipSpace, 0, 1);

		uv = inUV;
	})";

const std::string textFragmentShader =
	R"(#version 330 core
	in vec2 uv;
	out vec3 color;
	uniform sampler2D sampler;
	void main() {
		color = texture(sampler, uv).rgb;
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
#if defined(_WIN32)
	Window = reinterpret_cast<HWND>(windowHandle);
#elif defined(__linux)
    ParentWindowHandle = reinterpret_cast<Window>(windowHandle);
    DisplayPtr = XOpenDisplay(nullptr);

    if (DisplayPtr == nullptr)
    {
        throw std::runtime_error("Failed to connect to X11 Display");
    }

    XWindowAttributes attributes;
    if (XGetWindowAttributes(DisplayPtr, ParentWindowHandle, &attributes) == 0)
    {
        XCloseDisplay(DisplayPtr);
        throw std::runtime_error("Failed to retrieve X11 window attributes");
    }

    WindowWidth = attributes.width;
    WindowHeight = attributes.height;

	GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };

	XVisualInfoPtr = glXChooseVisual(DisplayPtr, 0, att);
	if (XVisualInfoPtr == nullptr) {
		XCloseDisplay(DisplayPtr);
		throw std::runtime_error("Failed to choose GLX visual settings");
	}

	Colormap colorMap = XCreateColormap(DisplayPtr, ParentWindowHandle, XVisualInfoPtr->visual, AllocNone);

	XSetWindowAttributes setWindowAttributes;
	setWindowAttributes.colormap = colorMap;
	setWindowAttributes.event_mask = ExposureMask | KeyPressMask;

	WindowHandle = XCreateWindow(DisplayPtr, ParentWindowHandle, 0, 0, WindowWidth, WindowHeight, 0, XVisualInfoPtr->depth,
							InputOutput, XVisualInfoPtr->visual, CWColormap | CWEventMask, &setWindowAttributes);

    if (XMapWindow(DisplayPtr, WindowHandle) == 0)
    {
        XDestroyWindow(DisplayPtr, WindowHandle);
        XCloseDisplay(DisplayPtr);
        throw std::runtime_error("Failed to map X11 window");
    }
#endif
}

VideoBackend::~VideoBackend()
{
#if defined(__linux)
    XDestroyWindow(DisplayPtr, WindowHandle);
    XCloseDisplay(DisplayPtr);
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

	WindowDc = GetDC(Window);

	int pf = ChoosePixelFormat(WindowDc, &pfd);
	assert(SetPixelFormat(WindowDc, pf, &pfd));

	OglContext = wglCreateContext(WindowDc);
	assert(wglMakeCurrent(WindowDc, OglContext));
#elif defined(__linux)
	OglContext = glXCreateContext(DisplayPtr, XVisualInfoPtr, NULL, GL_TRUE);
	if (OglContext == NULL) {
		throw std::runtime_error("Failed to create GLX context");
	}

	if (!glXMakeCurrent(DisplayPtr, WindowHandle, OglContext)) {
		throw std::runtime_error("Failed to make GLX context current");
	}
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
	wglMakeCurrent(WindowDc, NULL);
	wglDeleteContext(OglContext);
#elif defined(__linux)
	glXMakeCurrent(DisplayPtr, None, NULL);
	glXDestroyContext(DisplayPtr, OglContext);
#endif
}

void VideoBackend::DrawFrame(uint8_t* fb)
{
	bool overscanEnabled = OverscanEnabled;
	bool showingFps = ShowingFps;
	uint32_t currentFps = CurrentFps;

	UpdateSurfaceSize();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, WindowWidth, WindowHeight);

	glUseProgram(FrameProgramId);

	glBindTexture(GL_TEXTURE_2D, FrameTextureId);

	GLint loc = glGetUniformLocation(FrameProgramId, "screenSize");
	glUniform2f(loc, static_cast<float>(WindowWidth), static_cast<float>(WindowHeight));

	loc = glGetUniformLocation(FrameProgramId, "frameSize");

	if (overscanEnabled)
	{
		fb = fb + (FRAME_WIDTH * (NUM_OVERSCAN_LINES / 2) * 4);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, FRAME_WIDTH, FRAME_HEIGHT - NUM_OVERSCAN_LINES, 0, GL_BGRA, GL_UNSIGNED_BYTE, fb);
		glUniform2f(loc, FRAME_WIDTH, FRAME_HEIGHT - NUM_OVERSCAN_LINES);
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, FRAME_WIDTH, FRAME_HEIGHT, 0, GL_BGRA, GL_UNSIGNED_BYTE, fb);
		glUniform2f(loc, FRAME_WIDTH, FRAME_HEIGHT);
	}

	glUniform1i(FrameTextureId, 0);

	// 1st attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, FrameVertexBuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// Draw the triangle !
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisableVertexAttribArray(0);

	if (showingFps)
	{
		DrawFps(currentFps);
	}

	DrawMessages();

	Swap();
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
	std::unique_lock<std::mutex> lock(MessageMutex);

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
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// 1st attribute buffer : UVs
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, TextUVBuffer);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	GLint loc = glGetUniformLocation(TextProgramId, "screenSize");
	glUniform2f(loc, static_cast<float>(WindowWidth), static_cast<float>(WindowHeight));

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

	std::unique_lock<std::mutex> lock(MessageMutex);

    steady_clock::time_point expires = steady_clock::now() + seconds(duration);
    Messages.push_back(std::make_pair(ToUpperCase(message), expires));
}

void VideoBackend::Swap()
{
#if defined(_WIN32)
	SwapBuffers(WindowDc);
#elif defined(__linux)
	glXSwapBuffers(DisplayPtr, WindowHandle);
#endif
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
    if (XGetWindowAttributes(DisplayPtr, ParentWindowHandle, &attributes) == 0)
    {
        return;
    }

    uint32_t newWidth = static_cast<uint32_t>(attributes.width);
    uint32_t newHeight = static_cast<uint32_t>(attributes.height);

    if (WindowWidth != newWidth || WindowHeight != newHeight)
    {
        WindowWidth = attributes.width;
        WindowHeight = attributes.height;

        XResizeWindow(DisplayPtr, WindowHandle, WindowWidth, WindowHeight);
    }
#endif
}
