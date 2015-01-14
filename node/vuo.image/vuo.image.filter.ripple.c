/**
 * @file
 * vuo.image.filter.ripple node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include "VuoImageRenderer.h"

VuoModuleMetadata({
					 "title" : "Ripple Image",
					 "keywords" : [ "wave", "sinusoidal", "sine", "cosine", "undulate", "ruffle", "swish", "swing", "wag", "flap", "sway", "billow", "water" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoGlContext",
						 "VuoImageRenderer"
					 ],
					 "node": {
						 "isInterface" : false
					 }
				 });


static const char * fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	uniform sampler2D texture;
	uniform float angle;
	uniform float amplitude;
	uniform float wavelength;
	uniform float phase;
	varying vec4 fragmentTextureCoordinate;

	void main()
	{
		float samplerPhase = cos(angle)*fragmentTextureCoordinate.x + sin(angle)*fragmentTextureCoordinate.y;
		float offset = sin(samplerPhase/wavelength + phase) * amplitude;
		gl_FragColor = texture2D(texture, fragmentTextureCoordinate.xy + vec2(cos(angle)*offset,sin(angle)*offset));
	}
);


struct nodeInstanceData
{
	VuoShader shader;
	VuoImageRenderer imageRenderer;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->shader = VuoShader_make("Ripple Image", VuoShader_getDefaultVertexShader(), fragmentShaderSource);
	VuoRetain(instance->shader);
	instance->imageRenderer = VuoImageRenderer_make();
	VuoRetain(instance->imageRenderer);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoReal, {"default":135.0,"suggestedMin":0,"suggestedMax":360,"suggestedStep":1}) angle,
		VuoInputData(VuoReal, {"default":0.1,"suggestedMin":0,"suggestedMax":1}) amplitude,
		VuoInputData(VuoReal, {"default":0.05,"suggestedMin":0.00001,"suggestedMax":0.05}) wavelength,
		VuoInputData(VuoReal, {"default":0.0,"suggestedMin":0,"suggestedMax":1}) phase,
		VuoOutputData(VuoImage) rippledImage
)
{
	if (! image)
		return;

	{
		VuoGlContext glContext = VuoGlContext_use();

		// Associate the input image with the shader.
		VuoShader_resetTextures((*instance)->shader);
		VuoShader_addTexture((*instance)->shader, glContext, "texture", image);

		// Feed parameters to the shader.
		VuoShader_setUniformFloat((*instance)->shader, glContext, "angle", angle*M_PI/180.);
		VuoShader_setUniformFloat((*instance)->shader, glContext, "amplitude", amplitude);
		VuoShader_setUniformFloat((*instance)->shader, glContext, "wavelength", wavelength*M_PI*2.);
		VuoShader_setUniformFloat((*instance)->shader, glContext, "phase", phase*M_PI*2.);

		// Render.
		*rippledImage = VuoImageRenderer_draw((*instance)->imageRenderer, glContext, (*instance)->shader, image->pixelsWide, image->pixelsHigh);

		VuoGlContext_disuse(glContext);
	}
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
}
