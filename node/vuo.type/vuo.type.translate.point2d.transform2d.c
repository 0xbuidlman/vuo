/**
 * @file
 * vuo.type.translate.point2d.transform2d node implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert 2D Point to Translation",
					 "keywords" : [ "position", "matrix", "trs", "angle", "axis" ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoPoint2d, {"default":{"x":1, "y":1}}) translation,
		VuoOutputData(VuoTransform2d) transform
)
{
	*transform = VuoTransform2d_make(translation, 0., VuoPoint2d_make(1,1));
}
