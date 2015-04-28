/**
 * @file
 * vuo.window.get.dimensions node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Get Window Dimensions",
					 "keywords" : [ "width", "height", "size" ],
					 "version" : "1.0.0",
				 });

void nodeEvent
(
		VuoInputData(VuoWindowReference) window,
		VuoOutputData(VuoReal) width,
		VuoOutputData(VuoReal) height,
		VuoOutputData(VuoInteger) pixelsWide,
		VuoOutputData(VuoInteger) pixelsHigh,
		VuoOutputData(VuoReal) aspectRatio,
		VuoOutputData(VuoReal) top,
		VuoOutputData(VuoReal) right,
		VuoOutputData(VuoReal) bottom,
		VuoOutputData(VuoReal) left
)
{
	if (!window)
		return;

	*width = 2.;
	*left = -1.;
	*right = 1.;

	VuoWindowReference_getContentSize(window, pixelsWide, pixelsHigh);

	*aspectRatio = (VuoReal)*pixelsWide / *pixelsHigh;
	*height = *width / *aspectRatio;
	*top = *height / 2.;
	*bottom = -*top;
}
