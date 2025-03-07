/**
 * @file
 * VuoSceneObjectRenderer implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoSceneObjectRenderer.h"
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
					 "title" : "VuoSceneObjectRenderer",
					 "dependencies" : [
						 "VuoSceneObject",
						 "VuoShader",
						 "VuoGlContext",
						 "VuoGlPool",
						 "OpenGL.framework"
					 ]
				 });
#endif
}

/**
 * OpenGL attribute locations.
 */
typedef struct
{
	GLint position;
	GLint normal;
	GLint tangent;
	GLint bitangent;
	GLint textureCoordinate;

	unsigned int expectedOutputPrimitiveCount;
} VuoSceneObjectRenderer_Attributes;

/**
 * Internal state data for a VuoSceneObjectRenderer instance.
 */
struct VuoSceneObjectRendererInternal
{
	VuoGlContext glContext;

	VuoShader shader;

	GLuint shamTexture;
	GLuint shamFramebuffer;

	GLuint query;

	GLuint vertexArray;

	VuoSceneObjectRenderer_Attributes pointAttributes;
	VuoSceneObjectRenderer_Attributes lineAttributes;
	VuoSceneObjectRenderer_Attributes triangleAttributes;
};

void VuoSceneObjectRenderer_destroy(VuoSceneObjectRenderer sor);

/**
 * Creates a reference-counted object for applying a shader to a @ref VuoSceneObject.
 *
 * @threadAny
 */
VuoSceneObjectRenderer VuoSceneObjectRenderer_make(VuoGlContext glContext, VuoShader shader)
{
	if (!VuoShader_isTransformFeedback(shader))
	{
		VLog("Error '%s' is not a transform feedback shader.", shader->name);
		return NULL;
	}

	struct VuoSceneObjectRendererInternal *sceneObjectRenderer = (struct VuoSceneObjectRendererInternal *)malloc(sizeof(struct VuoSceneObjectRendererInternal));

	CGLContextObj cgl_ctx = (CGLContextObj)glContext;
	sceneObjectRenderer->glContext = glContext;

	// Fetch the shader's attribute locations
	bool havePoints    = VuoShader_getAttributeLocations(shader, VuoMesh_Points,              cgl_ctx, &sceneObjectRenderer->pointAttributes.position,    &sceneObjectRenderer->pointAttributes.normal,    &sceneObjectRenderer->pointAttributes.tangent,    &sceneObjectRenderer->pointAttributes.bitangent,    &sceneObjectRenderer->pointAttributes.textureCoordinate   );
	bool haveLines     = VuoShader_getAttributeLocations(shader, VuoMesh_IndividualLines,     cgl_ctx, &sceneObjectRenderer->lineAttributes.position,     &sceneObjectRenderer->lineAttributes.normal,     &sceneObjectRenderer->lineAttributes.tangent,     &sceneObjectRenderer->lineAttributes.bitangent,     &sceneObjectRenderer->lineAttributes.textureCoordinate    );
	bool haveTriangles = VuoShader_getAttributeLocations(shader, VuoMesh_IndividualTriangles, cgl_ctx, &sceneObjectRenderer->triangleAttributes.position, &sceneObjectRenderer->triangleAttributes.normal, &sceneObjectRenderer->triangleAttributes.tangent, &sceneObjectRenderer->triangleAttributes.bitangent, &sceneObjectRenderer->triangleAttributes.textureCoordinate);
	if (!havePoints || !haveLines || !haveTriangles)
	{
		VLog("Error: '%s' is missing programs for: %s %s %s", shader->name, havePoints? "" : "points", haveLines? "" : "lines", haveTriangles? "" : "triangles");
		free(sceneObjectRenderer);
		return NULL;
	}

	sceneObjectRenderer->pointAttributes.expectedOutputPrimitiveCount    = VuoShader_getExpectedOutputPrimitiveCount(shader, VuoMesh_Points);
	sceneObjectRenderer->lineAttributes.expectedOutputPrimitiveCount     = VuoShader_getExpectedOutputPrimitiveCount(shader, VuoMesh_IndividualLines);
	sceneObjectRenderer->triangleAttributes.expectedOutputPrimitiveCount = VuoShader_getExpectedOutputPrimitiveCount(shader, VuoMesh_IndividualTriangles);

	VuoRegister(sceneObjectRenderer, VuoSceneObjectRenderer_destroy);

	sceneObjectRenderer->shader = shader;
	VuoRetain(sceneObjectRenderer->shader);

	glGenVertexArrays(1, &sceneObjectRenderer->vertexArray);
	glBindVertexArray(sceneObjectRenderer->vertexArray);

	// http://stackoverflow.com/questions/24112671/transform-feedback-without-a-framebuffer
	sceneObjectRenderer->shamTexture = VuoGlTexturePool_use(cgl_ctx, GL_RGBA, 1, 1, GL_RGBA);
	VuoGlTexture_retain(sceneObjectRenderer->shamTexture);
	glGenFramebuffers(1, &sceneObjectRenderer->shamFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, sceneObjectRenderer->shamFramebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneObjectRenderer->shamTexture, 0);

	glGenQueries(1, &sceneObjectRenderer->query);

	glEnable(GL_RASTERIZER_DISCARD_EXT);

	return (VuoSceneObjectRenderer)sceneObjectRenderer;
}

/**
 * Helper for @ref VuoSceneObjectRenderer_draw.
 * Applies a shader to a single @c sceneObject's VuoMesh (ignoring its childObjects).
 */
void VuoSceneObjectRenderer_drawSingle(CGLContextObj cgl_ctx, struct VuoSceneObjectRendererInternal *sceneObjectRenderer, VuoSceneObject *sceneObject, float modelviewMatrix[16])
{
	if (!sceneObject->mesh)
		return;

	VuoRetain(sceneObject->mesh);

	VuoMesh newMesh = VuoMesh_make(sceneObject->mesh->submeshCount);

	for (unsigned int i = 0; i < sceneObject->mesh->submeshCount; ++i)
	{
		VuoSubmesh submesh = sceneObject->mesh->submeshes[i];


		VuoMesh_ElementAssemblyMethod outputPrimitiveMode;
		GLuint outputPrimitiveGlMode;
		VuoSceneObjectRenderer_Attributes attributes;
		int primitiveVertexMultiplier;
		if (submesh.elementAssemblyMethod == VuoMesh_IndividualTriangles
		 || submesh.elementAssemblyMethod == VuoMesh_TriangleFan
		 || submesh.elementAssemblyMethod == VuoMesh_TriangleStrip)
		{
			outputPrimitiveMode = VuoMesh_IndividualTriangles;
			outputPrimitiveGlMode = GL_TRIANGLES;
			attributes = sceneObjectRenderer->triangleAttributes;
			primitiveVertexMultiplier = 3;
		}
		else if (submesh.elementAssemblyMethod == VuoMesh_IndividualLines
			  || submesh.elementAssemblyMethod == VuoMesh_LineStrip)
		{
			outputPrimitiveMode = VuoMesh_IndividualLines;
			outputPrimitiveGlMode = GL_LINES;
			attributes = sceneObjectRenderer->lineAttributes;
			primitiveVertexMultiplier = 2;
		}
		else // if (submesh.elementAssemblyMethod == VuoMesh_Points)
		{
			outputPrimitiveMode = VuoMesh_Points;
			outputPrimitiveGlMode = GL_POINTS;
			attributes = sceneObjectRenderer->pointAttributes;
			primitiveVertexMultiplier = 1;
		}


		// Attach the input combinedBuffer for rendering.
		glBindBuffer(GL_ARRAY_BUFFER, submesh.glUpload.combinedBuffer);

		int bufferCount = 0;
		++bufferCount; // position
		if (submesh.glUpload.normalOffset)
			++bufferCount;
		if (submesh.glUpload.tangentOffset)
			++bufferCount;
		if (submesh.glUpload.bitangentOffset)
			++bufferCount;
		if (submesh.glUpload.textureCoordinateOffset)
			++bufferCount;


		GLuint programName = VuoShader_activate(sceneObjectRenderer->shader, submesh.elementAssemblyMethod, sceneObjectRenderer->glContext);


		GLint modelviewMatrixUniform = glGetUniformLocation(programName, "modelviewMatrix");
		if (modelviewMatrixUniform != -1)
			glUniformMatrix4fv(modelviewMatrixUniform, 1, GL_FALSE, modelviewMatrix);


		GLint modelviewMatrixInverseUniform = glGetUniformLocation(programName, "modelviewMatrixInverse");
		if (modelviewMatrixInverseUniform != -1)
		{
			float modelviewMatrixInverse[16];
			VuoTransform_invertMatrix4x4(modelviewMatrix, modelviewMatrixInverse);
			glUniformMatrix4fv(modelviewMatrixInverseUniform, 1, GL_FALSE, modelviewMatrixInverse);
		}


		glEnableVertexAttribArray(attributes.position);
		glVertexAttribPointer(attributes.position, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d)*bufferCount, (void*)0);
		if (submesh.glUpload.normalOffset && attributes.normal >= 0)
		{
			glEnableVertexAttribArray(attributes.normal);
			glVertexAttribPointer(attributes.normal, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d)*bufferCount, submesh.glUpload.normalOffset);
		}
		if (submesh.glUpload.tangentOffset && attributes.tangent >= 0)
		{
			glEnableVertexAttribArray(attributes.tangent);
			glVertexAttribPointer(attributes.tangent, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d)*bufferCount, submesh.glUpload.tangentOffset);
		}
		if (submesh.glUpload.bitangentOffset && attributes.bitangent >= 0)
		{
			glEnableVertexAttribArray(attributes.bitangent);
			glVertexAttribPointer(attributes.bitangent, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d)*bufferCount, submesh.glUpload.bitangentOffset);
		}
		if (submesh.glUpload.textureCoordinateOffset && attributes.textureCoordinate >= 0)
		{
			glEnableVertexAttribArray(attributes.textureCoordinate);
			glVertexAttribPointer(attributes.textureCoordinate, 4 /* XYZW */, GL_FLOAT, GL_FALSE, sizeof(VuoPoint4d)*bufferCount, submesh.glUpload.textureCoordinateOffset);
		}


		// Attach the input elementBuffer for rendering.
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, submesh.glUpload.elementBuffer);


		// Create and attach the output combinedBuffer.
		// The output buffer will always contain all 5 vertex attributes (position, normal, tangent, bitangent, textureCoordinate).
		unsigned long outputVertexCount = VuoSubmesh_getSplitVertexCount(submesh) * attributes.expectedOutputPrimitiveCount;
		unsigned long singleOutputBufferSize = sizeof(VuoPoint4d)*outputVertexCount;
		unsigned long combinedOutputBufferSize = singleOutputBufferSize*5;
		GLuint combinedOutputBuffer = VuoGlPool_use(VuoGlPool_ArrayBuffer, combinedOutputBufferSize);
		VuoGlPool_retain(combinedOutputBuffer);
		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER_EXT, combinedOutputBuffer);
		glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER_EXT, combinedOutputBufferSize, NULL, GL_STATIC_READ);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER_EXT, 0, combinedOutputBuffer);


		// Execute the filter.
		GLenum mode = VuoSubmesh_getGlMode(submesh);
		glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN_EXT, sceneObjectRenderer->query);
		glBeginTransformFeedbackEXT(outputPrimitiveGlMode);

		if (submesh.elementCount)
			glDrawElements(mode, submesh.elementCount, GL_UNSIGNED_INT, (void*)0);
		else
			glDrawArrays(mode, 0, submesh.vertexCount);

		glEndTransformFeedbackEXT();
		glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN_EXT);

		glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER_EXT, 0);

		VuoShader_deactivate(sceneObjectRenderer->shader, submesh.elementAssemblyMethod, sceneObjectRenderer->glContext);


		GLuint actualPrimitives = 0;
		glGetQueryObjectuiv(sceneObjectRenderer->query, GL_QUERY_RESULT, &actualPrimitives);


		// Print out the result of the filter, for debugging.
//		glFlush();
//		GLfloat feedback[actualPrimitives*primitiveVertexMultiplier*4*5];
//		glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER_EXT, 0, sizeof(feedback), feedback);
//		for (int vertex = 0; vertex < actualPrimitives*primitiveVertexMultiplier; vertex++)
//		{
//			for (int coordinate = 0; coordinate < 3; ++coordinate)
//				VLog("\t%f", feedback[vertex*4*bufferCount + coordinate]);
//			VL();
//		}


		newMesh->submeshes[i] = VuoSubmesh_makeGl(
					actualPrimitives*primitiveVertexMultiplier, combinedOutputBuffer, combinedOutputBufferSize,
					(void*)(sizeof(VuoPoint4d)*1),
					(void*)(sizeof(VuoPoint4d)*2),
					(void*)(sizeof(VuoPoint4d)*3),
					(void*)(sizeof(VuoPoint4d)*4),
					0, 0, 0, // Since transform feedback doesn't provide an elementBuffer, render this submesh using glDrawArrays().
					outputPrimitiveMode);
		newMesh->submeshes[i].faceCullingMode = submesh.faceCullingMode;
		newMesh->submeshes[i].primitiveSize = submesh.primitiveSize;
	}

//	VuoRetain(newMesh);
	VuoRelease(sceneObject->mesh);
	sceneObject->mesh = newMesh;
}

/**
 * Produces a new @ref VuoSceneObject by rendering @c sceneObject using @c shader's GLSL vertex shader.
 *
 * `VuoSubmesh`es are left unchanged if they have an elementAssemblyMethod that differs from the shader's inputElementType.
 *
 * @threadAnyGL
 * (Additionally, the caller is responsible for ensuring that the same @c VuoSceneObjectRenderer is not used simultaneously on multiple threads.)
 */
VuoSceneObject VuoSceneObjectRenderer_draw(VuoSceneObjectRenderer sor, VuoSceneObject sceneObject)
{
	if (!sor)
		return VuoSceneObject_makeEmpty();

	VuoSceneObject sceneObjectCopy = VuoSceneObject_copy(sceneObject);

	struct VuoSceneObjectRendererInternal *sceneObjectRenderer = (struct VuoSceneObjectRendererInternal *)sor;
	CGLContextObj cgl_ctx = (CGLContextObj)sceneObjectRenderer->glContext;

	VuoSceneObject_apply(&sceneObjectCopy, ^(VuoSceneObject *currentObject, float modelviewMatrix[16]) {
							 VuoSceneObjectRenderer_drawSingle(cgl_ctx, sceneObjectRenderer, currentObject, modelviewMatrix);
						 });

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return sceneObjectCopy;
}

/**
 * Destroys and deallocates the image renderer.
 *
 * @threadAny
 */
void VuoSceneObjectRenderer_destroy(VuoSceneObjectRenderer sor)
{
	struct VuoSceneObjectRendererInternal *sceneObjectRenderer = (struct VuoSceneObjectRendererInternal *)sor;
	CGLContextObj cgl_ctx = (CGLContextObj)sceneObjectRenderer->glContext;

	VuoRelease(sceneObjectRenderer->shader);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
	VuoGlTexture_release(GL_RGBA, 1, 1, sceneObjectRenderer->shamTexture);
//	glBindFramebuffer(GL_FRAMEBUFFER, 0);	// handled by glDeleteFramebuffers
	glDeleteFramebuffers(1, &sceneObjectRenderer->shamFramebuffer);

	glDeleteVertexArrays(1, &sceneObjectRenderer->vertexArray);

	glDisable(GL_RASTERIZER_DISCARD_EXT);

	glDeleteQueries(1, &sceneObjectRenderer->query);

	free(sceneObjectRenderer);
}
