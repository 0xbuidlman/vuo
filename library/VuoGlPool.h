/**
 * @file
 * VuoGlPool interface.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOGLPOOL_H
#define VUOGLPOOL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoGlContext.h"

#include <stdint.h>

/**
 * Types of OpenGL objects supported by Vuo's GL object pool.
 */
typedef enum
{
	VuoGlPool_ArrayBuffer,
	VuoGlPool_ElementArrayBuffer
//	VuoGlPool_Texture		///< @see VuoGlTexturePool_use
//	VuoGlPool_Framebuffer	///< Cannot be shared between contexts
//	VuoGlPool_VertexArray	///< Cannot be shared between contexts
} VuoGlPoolType;

GLuint VuoGlPool_use(VuoGlPoolType type, unsigned long size);

/**
 * Increments the reference count for @c glBufferName.
 *
 * @threadAny
 */
#define VuoGlPool_retain(glBufferName) VuoGlPool_retainF(glBufferName, __FILE__, __LINE__, __func__);
void VuoGlPool_retainF(GLuint glBufferName, const char *file, unsigned int line, const char *func);

/**
 * Decrements the reference count for @c glBufferName.
 *
 * @threadAny
 */
#define VuoGlPool_release(type, size, glBufferName) VuoGlPool_releaseF(type, size, glBufferName, __FILE__, __LINE__, __func__);
void VuoGlPool_releaseF(VuoGlPoolType type, unsigned long size, GLuint glBufferName, const char *file, unsigned int line, const char *func);

GLuint VuoGlTexturePool_use(VuoGlContext glContext, GLenum internalformat, unsigned short width, unsigned short height, GLenum format);
GLuint VuoGlTexture_getType(GLuint format);

void VuoGlTexture_retain(GLuint glTextureName);
void VuoGlTexture_release(GLenum internalformat, unsigned short width, unsigned short height, GLuint glTextureName);

typedef void * VuoIoSurface;	///< A Mac OS X IOSurface.
VuoIoSurface VuoIoSurfacePool_use(VuoGlContext glContext, unsigned short pixelsWide, unsigned short pixelsHigh, GLuint *outputTexture);
uint32_t VuoIoSurfacePool_getId(VuoIoSurface ioSurface);
void VuoIoSurfacePool_disuse(VuoGlContext glContext, unsigned short pixelsWide, unsigned short pixelsHigh, VuoIoSurface ioSurface, GLuint texture);
void VuoIoSurfacePool_signal(VuoIoSurface ioSurface);

GLuint VuoGlShader_use(VuoGlContext glContext, GLenum type, const char *source);

char *VuoGl_stringForConstant(GLenum constant);

#ifdef __cplusplus
}
#endif

#endif
