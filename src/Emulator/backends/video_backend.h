#pragma once

#include <mutex>
#include <chrono>
#include <vector>

#if defined(_WIN32)
#include <Windows.h>
#include <gl/GL.h>
#undef DrawText
#elif defined(__linux)   
#include <X11/Xlib.h>
#include <GL/glx.h>
#endif

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
	void DrawFps(uint32_t fps);
	void DrawMessages();
	void DrawText(const std::string& text, uint32_t xPos, uint32_t yPos);

	void Swap();
	void UpdateSurfaceSize();

	bool OverscanEnabled;
	bool ShowingFps;
    uint32_t WindowWidth;
    uint32_t WindowHeight;
    uint32_t CurrentFps;

	std::mutex MessageMutex;
    std::vector<std::pair<std::string, std::chrono::steady_clock::time_point> > Messages;

	GLuint FrameProgramId;
	GLuint FrameVertexArrayId;
	GLuint FrameVertexBuffer;
	GLuint FrameTextureId;
	GLuint TextProgramId;
	GLuint TextTextureId;
	GLuint TextVertexBuffer;
	GLuint TextUVBuffer;

#ifdef _WIN32
    HWND Window;
	HDC WindowDc;
	HGLRC OglContext;
#elif defined(__linux)   
    Display* DisplayPtr;
    Window ParentWindowHandle;
    Window WindowHandle;
    XVisualInfo* XVisualInfoPtr;
    GLXContext OglContext;
#endif
};
