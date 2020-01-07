#pragma once

#include <cstdint>
#include <string>
#include <mutex>
#include <vector>
#include <memory>

#include "gl_util.h"

#ifdef _WIN32
#undef DrawText
#endif

class IGLPlatform;

class VideoBackend
{
public:
	VideoBackend(void* windowHandle);
	~VideoBackend();

	void Prepare();
	void Finalize();

	void SubmitFrame(uint8_t* fb);

	void SetFps(uint32_t fps);
	void ShowFps(bool show);
	void ShowMessage(const std::string& message, uint32_t duration);
	void SetOverscanEnabled(bool enabled);

private:
	void DrawFps(uint32_t fps);
	void DrawMessages();
	void DrawText(const std::string& text, uint32_t xPos, uint32_t yPos);
	void UpdateSurfaceSize();
	void SwapFrameBuffers();
	
	bool _overscanEnabled;
	bool _showingFps;
	uint32_t _windowWidth;
	uint32_t _windowHeight;
	uint32_t _currentFps;

	std::mutex _messageMutex;
	std::vector<std::pair<std::string, std::chrono::steady_clock::time_point> > _messages;

	GLuint _frameProgramId;
	GLuint _frameVertexArrayId;
	GLuint _frameVertexBuffer;
	GLuint _frameTextureId;
	GLuint _textProgramId;
	GLuint _textTextureId;
	GLuint _textVertexBuffer;
	GLuint _textUVBuffer;

	std::unique_ptr<IGLPlatform> _glPlatform;
};