#include "gl_util.h"

THREAD_LOCAL PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = nullptr;
THREAD_LOCAL PFNGLBINDVERTEXARRAYPROC glBindVertexArray = nullptr;
THREAD_LOCAL PFNGLGENBUFFERSPROC glGenBuffers = nullptr;
THREAD_LOCAL PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
THREAD_LOCAL PFNGLBUFFERDATAPROC glBufferData = nullptr;
THREAD_LOCAL PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;
THREAD_LOCAL PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = nullptr;
THREAD_LOCAL PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray = nullptr;
THREAD_LOCAL PFNGLCREATESHADERPROC glCreateShader = nullptr;
THREAD_LOCAL PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
THREAD_LOCAL PFNGLCOMPILESHADERPROC glCompileShader = nullptr;
THREAD_LOCAL PFNGLGETSHADERIVPROC glGetShaderiv = nullptr;
THREAD_LOCAL PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;
THREAD_LOCAL PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
THREAD_LOCAL PFNGLATTACHSHADERPROC glAttachShader = nullptr;
THREAD_LOCAL PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;
THREAD_LOCAL PFNGLGETPROGRAMIVPROC glGetProgramiv = nullptr;
THREAD_LOCAL PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
THREAD_LOCAL PFNGLDETACHSHADERPROC glDetachShader = nullptr;
THREAD_LOCAL PFNGLDELETESHADERPROC glDeleteShader = nullptr;
THREAD_LOCAL PFNGLUSEPROGRAMPROC glUseProgram = nullptr;
THREAD_LOCAL PFNGLUNIFORM1IPROC glUniform1i = nullptr;
THREAD_LOCAL PFNGLUNIFORM2FPROC glUniform2f = nullptr;
THREAD_LOCAL PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;

namespace
{
THREAD_LOCAL bool functionsInitialized = false;
}

void InitializeGLFunctions()
{
	if (!functionsInitialized)
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

		functionsInitialized = true;
	}
}