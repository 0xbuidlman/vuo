/**
 * @file
 * VuoImageRenderer implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoImageRenderer.h"
#include "VuoGlContext.h"
#include "VuoGlPool.h"

#include <stdlib.h>

#include <IOSurface/IOSurface.h>

#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLMacro.h>
/// @{
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glBindVertexArray glBindVertexArrayAPPLE
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE
/// @}

extern "C"
{
#include "module.h"

#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoImageRenderer",
					 "dependencies" : [
						 "VuoImage",
						 "VuoImageColorDepth",
						 "VuoShader",
						 "VuoGlContext",
						 "VuoGlPool",
						 "OpenGL.framework"
					 ]
				 });
#endif
}

/**
 * Internal state data for a VuoImageRenderer instance.
 */
struct VuoImageRendererInternal
{
	VuoGlContext glContext;

	GLuint outputFramebuffer;

	GLuint vertexArray;
	GLuint quadDataBuffer;
	GLuint quadElementBuffer;
};

/**
 * Positions and texture coordinates for a quad.
 */
static const GLfloat quadData[] = {
	// Positions
	-1, -1, 0, 1,
	 1, -1, 0, 1,
	-1,  1, 0, 1,
	 1,  1, 0, 1,

	// Texture Coordinates
	0, 0, 0, 0,
	1, 0, 0, 0,
	0, 1, 0, 0,
	1, 1, 0, 0
};
static const GLushort quadElements[] = { 0, 1, 2, 3 };	///< The order of @c quadData's elements.
/**
 * An identity matrix.
 */
static const GLfloat unityMatrix[16] = {
	1,0,0,0,
	0,1,0,0,
	0,0,1,0,
	0,0,0,1
};

void VuoImageRenderer_destroy(VuoImageRenderer ir);

/**
 * Creates a reference-counted object for rendering a @ref VuoImage.
 *
 * @threadAny
 */
VuoImageRenderer VuoImageRenderer_make(VuoGlContext glContext)
{
	struct VuoImageRendererInternal *imageRenderer = (struct VuoImageRendererInternal *)malloc(sizeof(struct VuoImageRendererInternal));
	VuoRegister(imageRenderer, VuoImageRenderer_destroy);

	imageRenderer->glContext = glContext;
	CGLContextObj cgl_ctx = (CGLContextObj)glContext;

	glGenVertexArrays(1, &imageRenderer->vertexArray);
	glBindVertexArray(imageRenderer->vertexArray);
	{
		imageRenderer->quadDataBuffer = VuoGlPool_use(VuoGlPool_ArrayBuffer, sizeof(quadData));
		VuoGlPool_retain(imageRenderer->quadDataBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, imageRenderer->quadDataBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadData), quadData, GL_STREAM_DRAW);
/// @todo https://b33p.net/kosada/node/6901
//		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quadData), quadData);

		imageRenderer->quadElementBuffer = VuoGlPool_use(VuoGlPool_ElementArrayBuffer, sizeof(quadElements));
		VuoGlPool_retain(imageRenderer->quadElementBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, imageRenderer->quadElementBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadElements), quadElements, GL_STREAM_DRAW);
/// @todo https://b33p.net/kosada/node/6901
//		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(quadElements), quadElements);
	}
	glBindVertexArray(0);

	glGenFramebuffers(1, &imageRenderer->outputFramebuffer);

	return (VuoImageRenderer)imageRenderer;
}

/**
 * Produces a new @c VuoImage by rendering @c shader.
 *
 * @threadAnyGL
 * (Additionally, the caller is responsible for ensuring that the same @c VuoImageRenderer is not used simultaneously on multiple threads.)
 */
VuoImage VuoImageRenderer_draw(VuoImageRenderer ir, VuoShader shader, unsigned int pixelsWide, unsigned int pixelsHigh, VuoImageColorDepth imageColorDepth)
{
	if (pixelsWide < 1 || pixelsHigh < 1)
		return NULL;

	return VuoImage_make(VuoImageRenderer_draw_internal(ir,shader,pixelsWide,pixelsHigh,imageColorDepth,false,false), VuoImageColorDepth_getGlInternalFormat(GL_RGBA, imageColorDepth), pixelsWide, pixelsHigh);
}

/**
 * Helper for VuoImageRenderer_draw().
 *
 * If @c outputToGlTextureRectangle is true, the caller is responsible for deleting the texture (it should not be thrown into the GL texture pool).
 */
unsigned long int VuoImageRenderer_draw_internal(VuoImageRenderer ir, VuoShader shader, unsigned int pixelsWide, unsigned int pixelsHigh, VuoImageColorDepth imageColorDepth, bool outputToIOSurface, bool outputToGlTextureRectangle)
{
	struct VuoImageRendererInternal *imageRenderer = (struct VuoImageRendererInternal *)ir;

	GLuint outputTexture;
	IOSurfaceID surfID;
	{
		CGLContextObj cgl_ctx = (CGLContextObj)imageRenderer->glContext;

		glViewport(0, 0, pixelsWide, pixelsHigh);

		// Create a new GL Texture Object.
		GLuint textureTarget = (outputToIOSurface || outputToGlTextureRectangle) ? GL_TEXTURE_RECTANGLE_ARB : GL_TEXTURE_2D;
		GLuint textureTargetInternalFormat = VuoImageColorDepth_getGlInternalFormat(GL_RGBA, imageColorDepth);

		VuoIoSurface ioSurface = NULL;
		if (outputToIOSurface)
			ioSurface = VuoIoSurfacePool_use(cgl_ctx, pixelsWide, pixelsHigh, &outputTexture);
		else
		{
			if (outputToGlTextureRectangle)
			{
				glGenTextures(1, &outputTexture);
				glBindTexture(GL_TEXTURE_RECTANGLE_ARB, outputTexture);
				glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, textureTargetInternalFormat, pixelsWide, pixelsHigh, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
			}
			else
				outputTexture = VuoGlTexturePool_use(imageRenderer->glContext, textureTargetInternalFormat, pixelsWide, pixelsHigh, GL_RGBA);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, imageRenderer->outputFramebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textureTarget, outputTexture, 0);

		glClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Execute the shader.
		{
			GLint positionAttribute;
			GLint textureCoordinateAttribute;
			bool ret = VuoShader_getAttributeLocations(shader, VuoMesh_IndividualTriangles, cgl_ctx, &positionAttribute, NULL, NULL, NULL, &textureCoordinateAttribute);
			if (!ret)
			{
				VLog("Error: Failed to get attribute locations.");
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				if (outputToIOSurface)
					VuoIoSurfacePool_disuse(cgl_ctx, pixelsWide, pixelsHigh, ioSurface, outputTexture);
				else
				{
					if (outputToGlTextureRectangle)
						glDeleteTextures(1, &outputTexture);
					else
					{
						VuoGlTexture_retain(outputTexture);
						VuoGlTexture_release(textureTargetInternalFormat, pixelsWide, pixelsHigh, outputTexture);
					}
				}
				return 0;
			}

			unsigned int programName = VuoShader_activate(shader, VuoMesh_IndividualTriangles, cgl_ctx);
			{
				glBindVertexArray(imageRenderer->vertexArray);

				GLint projectionMatrixUniform = glGetUniformLocation(programName, "projectionMatrix");
				glUniformMatrix4fv(projectionMatrixUniform, 1, GL_FALSE, unityMatrix);

				GLint modelviewMatrixUniform = glGetUniformLocation(programName, "modelviewMatrix");
				glUniformMatrix4fv(modelviewMatrixUniform, 1, GL_FALSE, unityMatrix);

				GLint aspectRatioUniform = glGetUniformLocation(programName, "aspectRatio");
				if (aspectRatioUniform != -1)
					glUniform1f(aspectRatioUniform, (float)pixelsWide/(float)pixelsHigh);

				glBindBuffer(GL_ARRAY_BUFFER, imageRenderer->quadDataBuffer);

				glEnableVertexAttribArray(positionAttribute);
				glVertexAttribPointer(positionAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*4, (void*)0);

				if (textureCoordinateAttribute != -1)
				{
					glEnableVertexAttribArray(textureCoordinateAttribute);
					glVertexAttribPointer(textureCoordinateAttribute, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(GLfloat)*4, (void*)(sizeof(GLfloat)*16));
				}

				glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, (void*)0);

				if (textureCoordinateAttribute != -1)
				glDisableVertexAttribArray(textureCoordinateAttribute);

				glDisableVertexAttribArray(positionAttribute);

				glBindVertexArray(0);
			}
			VuoShader_deactivate(shader, VuoMesh_IndividualTriangles, cgl_ctx);
		}

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textureTarget, 0, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		if (outputToIOSurface)
		{
			surfID = VuoIoSurfacePool_getId(ioSurface);
			VuoIoSurfacePool_disuse(cgl_ctx, pixelsWide, pixelsHigh, ioSurface, outputTexture);
		}

		glFlushRenderAPPLE();
	}

	if (outputToIOSurface)
		return surfID;
	else
		return outputTexture;
}

/**
 * Destroys and deallocates the image renderer.
 *
 * @threadAny
 */
void VuoImageRenderer_destroy(VuoImageRenderer ir)
{
	struct VuoImageRendererInternal *imageRenderer = (struct VuoImageRendererInternal *)ir;
	CGLContextObj cgl_ctx = (CGLContextObj)imageRenderer->glContext;

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	VuoGlPool_release(VuoGlPool_ElementArrayBuffer, sizeof(quadElements), imageRenderer->quadElementBuffer);
	VuoGlPool_release(VuoGlPool_ArrayBuffer, sizeof(quadData), imageRenderer->quadDataBuffer);

	glDeleteVertexArrays(1, &imageRenderer->vertexArray);
	glDeleteFramebuffers(1, &imageRenderer->outputFramebuffer);
	free(imageRenderer);
}
