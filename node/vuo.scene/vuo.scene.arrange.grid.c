/**
 * @file
 * vuo.scene.arrange.grid node implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Arrange 3D Objects in Grid",
					 "keywords" : [ "align", "place", "position", "lattice", "matrix" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ "AddNoiseToClay.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoSceneObject) objects,
		VuoInputData(VuoBoolean, {"name":"Scale to Fit", "default":true}) scaleToFit,
		VuoInputData(VuoPoint3d, {"default":[{"x":0,"y":0,"z":0}], "suggestedStep":{"x":0.1,"y":0.1,"z":0.1}}) center,
		VuoInputData(VuoReal, {"default":2.0, "suggestedMin":0.0, "suggestedStep":0.1}) width,
		VuoInputData(VuoInteger, {"default":2, "suggestedMin":1}) columns,
		VuoInputData(VuoReal, {"default":2.0, "suggestedMin":0.0, "suggestedStep":0.1}) height,
		VuoInputData(VuoInteger, {"default":2, "suggestedMin":1}) rows,
		VuoInputData(VuoReal, {"default":2.0, "suggestedMin":0.0, "suggestedStep":0.1}) depth,
		VuoInputData(VuoInteger, {"default":1, "suggestedMin":1}) slices,
		VuoOutputData(VuoSceneObject) griddedObject
)
{
	*griddedObject = VuoSceneObject_makeEmpty();
	(*griddedObject).childObjects = VuoListCreate_VuoSceneObject();

	unsigned long objectCount = VuoListGetCount_VuoSceneObject(objects);
	unsigned long currentObject = 1;

	for (unsigned int z = 0; z < slices; ++z)
	{
		VuoReal zPosition = center.z - depth/2 + (depth / slices) * (z + .5);

		for (unsigned int y = 0; y < rows; ++y)
		{
			VuoReal yPosition = center.y + height/2 - (height / rows) * (y + .5);

			for (unsigned int x = 0; x < columns; ++x)
			{
				VuoReal xPosition = center.x - width/2 + (width / columns) * (x + .5);

				VuoSceneObject object = VuoListGetValue_VuoSceneObject(objects, currentObject);

				object.transform.translation = VuoPoint3d_add(object.transform.translation, VuoPoint3d_make(xPosition,yPosition,zPosition));

				if (scaleToFit)
				{
					VuoSceneObject_normalize(&object);
					object.transform.scale = VuoPoint3d_multiply(object.transform.scale, fmin(fmin(width/columns, height/rows), depth/slices));
				}

				VuoListAppendValue_VuoSceneObject((*griddedObject).childObjects, object);

				++currentObject;
				if (currentObject > objectCount)
					return;
			}
		}
	}
}
