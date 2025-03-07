/**
 * @file
 * vuo.scene.copy node implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Copy 3D Object with Transforms",
					 "keywords" : [ "duplicate", "clone", "array", "instance", "instantiate", "populate", "replicate" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
	VuoInputData(VuoSceneObject) object,
	VuoInputData(VuoList_VuoTransform) transforms,
	VuoOutputData(VuoSceneObject) copies
)
{
	*copies = VuoSceneObject_makeEmpty();
	copies->childObjects = VuoListCreate_VuoSceneObject();

	for(int i = 0; i < VuoListGetCount_VuoTransform(transforms); i++)
	{
		VuoTransform transform = VuoListGetValue_VuoTransform(transforms, i+1);

		VuoListAppendValue_VuoSceneObject(copies->childObjects, VuoSceneObject_make(
			object.mesh,
			object.shader,
			transform,
			object.childObjects
			));
	}
}
