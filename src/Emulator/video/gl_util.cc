#include "gl_util.h"

PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = nullptr;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = nullptr;
PFNGLGENBUFFERSPROC glGenBuffers = nullptr;
PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
PFNGLBUFFERDATAPROC glBufferData = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = nullptr;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray = nullptr;
PFNGLCREATESHADERPROC glCreateShader = nullptr;
PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
PFNGLCOMPILESHADERPROC glCompileShader = nullptr;
PFNGLGETSHADERIVPROC glGetShaderiv = nullptr;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;
PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
PFNGLATTACHSHADERPROC glAttachShader = nullptr;
PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;
PFNGLGETPROGRAMIVPROC glGetProgramiv = nullptr;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
PFNGLDETACHSHADERPROC glDetachShader = nullptr;
PFNGLDELETESHADERPROC glDeleteShader = nullptr;
PFNGLUSEPROGRAMPROC glUseProgram = nullptr;
PFNGLUNIFORM1IPROC glUniform1i = nullptr;
PFNGLUNIFORM2FPROC glUniform2f = nullptr;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = nullptr;

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