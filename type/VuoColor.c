/**
 * @file
 * VuoColor implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoColor.h"
#include "VuoList_VuoColor.h"
#include "VuoText.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Color",
					 "description" : "A color, with component values specifying red, green, blue, and alpha (opacity).",
					 "keywords" : [ "rgba" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						"VuoList_VuoColor",
						"VuoReal",
						"VuoText"
					 ]
				 });
#endif
/// @}

/**
 * @ingroup VuoColor
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "r" : 0.5,
 *     "g" : 1,
 *     "b" : 0,
 *     "a" : 1
 *   }
 * }
 */
VuoColor VuoColor_makeFromJson(json_object * js)
{
	VuoColor color = {0,0,0,0};
	json_object *o = NULL;

	if (json_object_object_get_ex(js, "r", &o))
		color.r = VuoReal_makeFromJson(o);

	if (json_object_object_get_ex(js, "g", &o))
		color.g = VuoReal_makeFromJson(o);

	if (json_object_object_get_ex(js, "b", &o))
		color.b = VuoReal_makeFromJson(o);

	if (json_object_object_get_ex(js, "a", &o))
		color.a = VuoReal_makeFromJson(o);

	return color;
}

/**
 * @ingroup VuoColor
 * Encodes @c value as a JSON object.
 */
json_object * VuoColor_getJson(const VuoColor value)
{
	json_object *js = json_object_new_object();

	json_object_object_add(js, "r", VuoReal_getJson(value.r));

	json_object_object_add(js, "g", VuoReal_getJson(value.g));

	json_object_object_add(js, "b", VuoReal_getJson(value.b));

	json_object_object_add(js, "a", VuoReal_getJson(value.a));

	return js;
}

/**
 * @ingroup VuoColor
 * Returns a compact string representation of @c value (comma-separated components).
 */
char * VuoColor_getSummary(const VuoColor value)
{
	return VuoText_format("%g, %g, %g, %g", value.r, value.g, value.b, value.a);
}

/**
 * Returns a @c VuoColor with the given hue, saturation, lightness, alpha.
 *
 * Assumes each value is in the range [0,1].
 */
VuoColor VuoColor_makeWithHSLA(VuoReal h, VuoReal s, VuoReal l, VuoReal a)
{
	// http://axonflux.com/handy-rgb-to-hsl-and-rgb-to-hsv-color-model-c

	float r, g, b;

	if (s == 0)
	{
		r = g = b = l;
	}
	else
	{
		float (^hue2rgb)(float p, float q, float t) = ^(float p, float q, float t) {
			if (t < 0.f) t += 1.f;
			if (t > 1.f) t -= 1.f;
			if (t < 1.f/6.f) return p + (q - p) * 6.f * t;
			if (t < 1.f/2.f) return q;
			if (t < 2.f/3.f) return p + (q - p) * (2.f/3.f - t) * 6.f;
			return p;
		};

		float q = l < 0.5f ? l * (1.f + s) : l + s - l * s;
		float p = 2.f * l - q;
		r = hue2rgb(p, q, h + 1.f/3.f);
		g = hue2rgb(p, q, h);
		b = hue2rgb(p, q, h - 1.f/3.f);
	}

	return VuoColor_makeWithRGBA(r, g, b, a);
}

/**
 * Gets the hue, saturation, lightness, hue, alpha of a color.
 *
 * Each value is in the range [0,1].
 */
void VuoColor_getHSLA(VuoColor color, VuoReal *h, VuoReal *s, VuoReal *l, VuoReal *a)
{
	// http://axonflux.com/handy-rgb-to-hsl-and-rgb-to-hsv-color-model-c

	VuoReal r, g, b;
	VuoColor_getRGBA(color, &r, &g, &b, a);

	float max = fmax(r, fmax(g, b));
	float min = fmin(r, fmin(g, b));
	*h = *s = *l = (max + min) / 2.f;

	if (max == min)
	{
		*h = *s = 0;
	}
	else
	{
		float d = max - min;
		*s = *l > 0.5f ? d / (2.f - max - min) : d / (max + min);
		if (max == r)
			*h = (g - b) / d + (g < b ? 6.f : 0.f);
		else if (max == g)
			*h = (b - r) / d + 2.f;
		else
			*h = (r - g) / d + 4.f;
		*h /= 6.f;
	}
}

/**
 * Returns the weighted (by alpha) average of @c colors.
 *
 * If there are no colors in the list, returns transparent black.
 */
VuoColor VuoColor_average(VuoList_VuoColor colors)
{
	VuoColor result = VuoColor_makeWithRGBA(0,0,0,0);
	unsigned long colorCount = VuoListGetCount_VuoColor(colors);
	for (unsigned long i = 1; i <= colorCount; ++i)
	{
		VuoColor color = VuoListGetValue_VuoColor(colors, i);
		result.r += color.r * color.a;
		result.g += color.g * color.a;
		result.b += color.b * color.a;
		result.a += color.a;
	}

	if (result.a < 0.00001)
		return VuoColor_makeWithRGBA(0,0,0,0);

	result.r /= result.a;
	result.g /= result.a;
	result.b /= result.a;
	result.a /= colorCount;
	return result;
}

/**
 * Returns true if both colors have the same intensity and alpha values.
 */
bool VuoColor_areEqual(const VuoColor value1, const VuoColor value2)
{
	return VuoReal_areEqual(value1.r, value2.r)
		&& VuoReal_areEqual(value1.g, value2.g)
		&& VuoReal_areEqual(value1.b, value2.b)
		&& VuoReal_areEqual(value1.a, value2.a);
}
