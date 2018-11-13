#pragma once

/*
 * Minimal set of GL definitions required for the OpenGL backend to function
 */

/*
** Copyright (c) 2013-2017 The Khronos Group Inc.
**
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and/or associated documentation files (the
** "Materials"), to deal in the Materials without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Materials, and to
** permit persons to whom the Materials are furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be included
** in all copies or substantial portions of the Materials.
**
** THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/

#define THREAD_LOCAL thread_local

#if defined(_WIN32)

#undef THREAD_LOCAL
#define THREAD_LOCAL
#include <Windows.h>
#include <gl/GL.h>
#define LOAD_OGL_FUNC(name) wglGetProcAddress(name)
#pragma comment(lib, "opengl32.lib")

#elif defined(__linux)

#include <X11/Xlib.h>
#include <GL/glx.h>
#include <GL/gl.h>
#define LOAD_OGL_FUNC(name) glXGetProcAddress(reinterpret_cast<const GLubyte*>(name))

#endif

#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif

#include <stddef.h>
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;
typedef char GLchar;

#define GL_BGR             0x80E0
#define GL_BGRA            0x80E1
#define GL_STATIC_DRAW     0x88E4
#define GL_ARRAY_BUFFER    0x8892
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER   0x8B31
#define GL_COMPILE_STATUS  0x8B81
#define GL_LINK_STATUS     0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84

// Function Pointer Type Definitions
typedef void (APIENTRYP PFNGLGENVERTEXARRAYSPROC) (GLsizei n, GLuint *arrays);
typedef void (APIENTRYP PFNGLBINDVERTEXARRAYPROC) (GLuint array);
typedef void (APIENTRYP PFNGLGENBUFFERSPROC) (GLsizei n, GLuint *buffers);
typedef void (APIENTRYP PFNGLBINDBUFFERPROC) (GLenum target, GLuint buffer);
typedef void (APIENTRYP PFNGLBUFFERDATAPROC) (GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void (APIENTRYP PFNGLENABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef void (APIENTRYP PFNGLVERTEXATTRIBPOINTERPROC) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef void (APIENTRYP PFNGLDISABLEVERTEXATTRIBARRAYPROC) (GLuint index);
typedef GLuint (APIENTRYP PFNGLCREATESHADERPROC) (GLenum type);
typedef void (APIENTRYP PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void (APIENTRYP PFNGLCOMPILESHADERPROC) (GLuint shader);
typedef void (APIENTRYP PFNGLGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETSHADERINFOLOGPROC) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef GLuint(APIENTRYP PFNGLCREATEPROGRAMPROC) (void);
typedef void (APIENTRYP PFNGLATTACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (APIENTRYP PFNGLLINKPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLGETPROGRAMIVPROC) (GLuint program, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETPROGRAMINFOLOGPROC) (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (APIENTRYP PFNGLDETACHSHADERPROC) (GLuint program, GLuint shader);
typedef void (APIENTRYP PFNGLDELETESHADERPROC) (GLuint shader);
typedef void (APIENTRYP PFNGLUSEPROGRAMPROC) (GLuint program);
typedef void (APIENTRYP PFNGLUNIFORM1IPROC) (GLint location, GLint v0);
typedef void (APIENTRYP PFNGLUNIFORM2FPROC) (GLint location, GLfloat v0, GLfloat v1);
typedef GLint(APIENTRYP PFNGLGETUNIFORMLOCATIONPROC) (GLuint program, const GLchar *name);

// Function Pointer Definitions
extern THREAD_LOCAL PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
extern THREAD_LOCAL PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
extern THREAD_LOCAL PFNGLGENBUFFERSPROC glGenBuffers;
extern THREAD_LOCAL PFNGLBINDBUFFERPROC glBindBuffer;
extern THREAD_LOCAL PFNGLBUFFERDATAPROC glBufferData;
extern THREAD_LOCAL PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
extern THREAD_LOCAL PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
extern THREAD_LOCAL PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
extern THREAD_LOCAL PFNGLCREATESHADERPROC glCreateShader;
extern THREAD_LOCAL PFNGLSHADERSOURCEPROC glShaderSource;
extern THREAD_LOCAL PFNGLCOMPILESHADERPROC glCompileShader;
extern THREAD_LOCAL PFNGLGETSHADERIVPROC glGetShaderiv;
extern THREAD_LOCAL PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
extern THREAD_LOCAL PFNGLCREATEPROGRAMPROC glCreateProgram;
extern THREAD_LOCAL PFNGLATTACHSHADERPROC glAttachShader;
extern THREAD_LOCAL PFNGLLINKPROGRAMPROC glLinkProgram;
extern THREAD_LOCAL PFNGLGETPROGRAMIVPROC glGetProgramiv;
extern THREAD_LOCAL PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
extern THREAD_LOCAL PFNGLDETACHSHADERPROC glDetachShader;
extern THREAD_LOCAL PFNGLDELETESHADERPROC glDeleteShader;
extern THREAD_LOCAL PFNGLUSEPROGRAMPROC glUseProgram;
extern THREAD_LOCAL PFNGLUNIFORM1IPROC glUniform1i;
extern THREAD_LOCAL PFNGLUNIFORM2FPROC glUniform2f;
extern THREAD_LOCAL PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;

extern void InitializeGLFunctions();
