#include "video_backend.h"
#include "osd_font.h"
#include "igl_platform.h"
#include "nes_exception.h"

namespace
{
static constexpr int32_t FRAME_WIDTH = 256;
static constexpr int32_t FRAME_HEIGHT = 240;
static constexpr int32_t NUM_OVERSCAN_LINES = 16;

const std::string frameVertexShader =
R"(
#version 110
attribute vec3 vert;
varying vec2 uv;
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
}
)";

const std::string frameFragmentShader =
R"(
#version 110
varying vec2 uv;
uniform sampler2D sampler;
void main() {
	vec4 color = texture2D(sampler, uv);
	gl_FragColor = color;
}
)";

const std::string textVertexShader =
R"(
#version 110
attribute vec2 inVertex;
attribute vec2 inUV;
varying vec2 uv;
uniform vec2 screenSize;
void main() {
	vec2 halfScreen = screenSize / 2.0;
	vec2 clipSpace = inVertex - halfScreen;
	clipSpace /= halfScreen;
	gl_Position = vec4(clipSpace, 0.0, 1.0);

	uv = inUV;
}
)";

const std::string textFragmentShader =
R"(
#version 110
varying vec2 uv;
uniform sampler2D sampler;
void main() {
	vec4 color = texture2D(sampler, uv);
	gl_FragColor = color;
}
)";

std::string toUpperCase(const std::string& str)
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

void compileShaders(const std::string& vertexShader, const std::string& fragmentShader, GLuint* programId)
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
			std::string error(errorMessage);
			delete[] errorMessage;
			throw error;
		}
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
			std::string error(errorMessage);
			delete[] errorMessage;
			throw error;
		}
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
			std::string error(errorMessage);
			delete[] errorMessage;
			throw error;
		}
	}

	glDetachShader(prgId, vertexShaderId);
	glDetachShader(prgId, fragmentShaderId);

	glDeleteShader(vertexShaderId);
	glDeleteShader(fragmentShaderId);

	*programId = prgId;
}
}

VideoBackend::VideoBackend(void* windowHandle)
{
	_glPlatform = IGLPlatform::CreateGLPlatform();
	_glPlatform->InitializeWindow(windowHandle);
}

VideoBackend::~VideoBackend()
{
	_glPlatform->DestroyWindow();
}

void VideoBackend::Prepare()
{
	_glPlatform->InitializeContext();

	InitializeGLFunctions();
	
	try
	{
		compileShaders(frameVertexShader, frameFragmentShader, &_frameProgramId);
		compileShaders(textVertexShader, textFragmentShader, &_textProgramId);
	}
	catch (std::string& err)
	{
		Finalize();
		throw NesException("VideoBackend", err);
	}

	glGenVertexArrays(1, &_frameVertexArrayId);
	glBindVertexArray(_frameVertexArrayId);

	static const GLfloat g_vertex_buffer_data[] = {
		0.0f,  0.0f, 0.0f,
		0.0f,  1.0f, 0.0f,
		1.0f,  0.0f, 0.0f,
		1.0f,  1.0f, 0.0f
	};

	// Generate 1 buffer, put the resulting identifier in vertexbuffer
	glGenBuffers(1, &_frameVertexBuffer);
	// The following commands will talk about our 'vertexbuffer' buffer
	glBindBuffer(GL_ARRAY_BUFFER, _frameVertexBuffer);
	// Give our vertices to OpenGL.
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	glGenTextures(1, &_frameTextureId);
	glBindTexture(GL_TEXTURE_2D, _frameTextureId);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glGenTextures(1, &_textTextureId);
	glBindTexture(GL_TEXTURE_2D, _textTextureId);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 128, 0, GL_BGR, GL_UNSIGNED_BYTE, OSD_FONT_BITMAP);

	glGenBuffers(1, &_textVertexBuffer);
	glGenBuffers(1, &_textUVBuffer);
}

void VideoBackend::Finalize()
{
	_glPlatform->DestroyContext();
}

void VideoBackend::SubmitFrame(uint8_t * fb)
{
	bool overscanEnabled = _overscanEnabled;
	bool showingFps = _showingFps;
	uint32_t currentFps = _currentFps;

	UpdateSurfaceSize();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, _windowWidth, _windowHeight);

	glUseProgram(_frameProgramId);

	glBindTexture(GL_TEXTURE_2D, _frameTextureId);

	GLint loc = glGetUniformLocation(_frameProgramId, "screenSize");
	glUniform2f(loc, static_cast<float>(_windowWidth), static_cast<float>(_windowHeight));

	loc = glGetUniformLocation(_frameProgramId, "frameSize");

	if (overscanEnabled)
	{
		fb = fb + (FRAME_WIDTH * (NUM_OVERSCAN_LINES / 2) * 4);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, FRAME_WIDTH, FRAME_HEIGHT - NUM_OVERSCAN_LINES, 0, GL_BGRA, GL_UNSIGNED_BYTE, fb);
		glUniform2f(loc, static_cast<float>(FRAME_WIDTH), static_cast<float>(FRAME_HEIGHT - NUM_OVERSCAN_LINES));
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, FRAME_WIDTH, FRAME_HEIGHT, 0, GL_BGRA, GL_UNSIGNED_BYTE, fb);
		glUniform2f(loc, static_cast<float>(FRAME_WIDTH), static_cast<float>(FRAME_HEIGHT));
	}

	glUniform1i(_frameTextureId, 0);

	// 1st attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, _frameVertexBuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// Draw the triangle !
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisableVertexAttribArray(0);

	if (showingFps)
	{
		DrawFps(currentFps);
	}

	DrawMessages();

	SwapFrameBuffers();
}

void VideoBackend::SwapFrameBuffers()
{
	_glPlatform->SwapBuffers();
}

void VideoBackend::SetFps(uint32_t fps)
{
	_currentFps = fps;
}

void VideoBackend::ShowFps(bool show)
{
	_showingFps = show;
}

void VideoBackend::ShowMessage(const std::string & message, uint32_t duration)
{
	using namespace std::chrono;

	std::unique_lock<std::mutex> lock(_messageMutex);

	steady_clock::time_point expires = steady_clock::now() + seconds(duration);
	_messages.push_back(std::make_pair(toUpperCase(message), expires));
}

void VideoBackend::SetOverscanEnabled(bool enabled)
{
	_overscanEnabled = enabled;
}

void VideoBackend::DrawFps(uint32_t fps)
{
	std::string fpsStr = std::to_string(fps);

	uint32_t xPos = _windowWidth - 12 - static_cast<uint32_t>(fpsStr.length()) * OSD_FONT_BITMAP_CELL_WIDTH;
	uint32_t yPos = _windowHeight - 12 - OSD_FONT_BITMAP_CELL_HEIGHT;

	DrawText(fpsStr, xPos, yPos);
}

void VideoBackend::DrawMessages()
{
	std::unique_lock<std::mutex> lock(_messageMutex);

	if (_messages.empty())
	{
		return;
	}

	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	std::vector<std::pair<std::string, std::chrono::steady_clock::time_point> > remaining;
	for (auto& entry : _messages)
	{
		if (entry.second > now)
		{
			remaining.push_back(entry);
		}
	}

	_messages.swap(remaining);

	uint32_t xPos = 12;
	uint32_t yPos = _windowHeight - 12 - OSD_FONT_BITMAP_CELL_HEIGHT;

	for (auto& entry : _messages)
	{
		const std::string& message = entry.first;
		DrawText(message, xPos, yPos);

		yPos -= OSD_FONT_BITMAP_CELL_HEIGHT;
	}
}

void VideoBackend::DrawText(const std::string & text, uint32_t xPos, uint32_t yPos)
{
	static constexpr float uvWidth = static_cast<float>(OSD_FONT_BITMAP_CELL_WIDTH) / static_cast<float>(OSD_FONT_BITMAP_WIDTH);
	static constexpr float uvHeight = static_cast<float>(OSD_FONT_BITMAP_CELL_HEIGHT) / static_cast<float>(OSD_FONT_BITMAP_HEIGHT);

	std::vector<float> vertices;
	std::vector<float> uvs;

	vertices.reserve(text.length() * 12);
	uvs.reserve(text.length() * 12);

	float x = static_cast<float>(xPos);
	float y = static_cast<float>(yPos);

	for (uint32_t i = 0; i < text.length(); ++i)
	{
		float upLeftX = x + i * OSD_FONT_BITMAP_CELL_WIDTH;
		float upLeftY = y + OSD_FONT_BITMAP_CELL_HEIGHT;
		float upRightX = x + i * OSD_FONT_BITMAP_CELL_WIDTH + OSD_FONT_BITMAP_CELL_WIDTH;
		float upRightY = y + OSD_FONT_BITMAP_CELL_HEIGHT;

		float downRightX = x + i * OSD_FONT_BITMAP_CELL_WIDTH + OSD_FONT_BITMAP_CELL_WIDTH;
		float downRightY = y;
		float downLeftX = x + i * OSD_FONT_BITMAP_CELL_WIDTH;
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

	glBindBuffer(GL_ARRAY_BUFFER, _textVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*vertices.size(), vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, _textUVBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*uvs.size(), uvs.data(), GL_STATIC_DRAW);

	glUseProgram(_textProgramId);

	// 1st attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, _textVertexBuffer);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	// 1st attribute buffer : UVs
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, _textUVBuffer);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

	GLint loc = glGetUniformLocation(_textProgramId, "screenSize");
	glUniform2f(loc, static_cast<float>(_windowWidth), static_cast<float>(_windowHeight));

	glBindTexture(GL_TEXTURE_2D, _textTextureId);
	glUniform1i(_textTextureId, 0);

	glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 2));

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}

void VideoBackend::UpdateSurfaceSize()
{
	_glPlatform->UpdateSurfaceSize(&_windowWidth, &_windowHeight);
}


