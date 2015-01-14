/**
 * @file
 * vuo.layer.combine node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"

VuoModuleMetadata({
					 "title" : "Combine Layers",
					 "keywords" : [ "group", "join", "together" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoTransform2d) transform,
		VuoInputData(VuoList_VuoLayer) layers,
		VuoOutputData(VuoLayer) layer
)
{
	*layer = VuoLayer_makeGroup(layers, transform);
}
