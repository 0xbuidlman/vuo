/**
 * @file
 * vuo.scene.shader.material node implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoSceneObject.h"

VuoModuleMetadata({
					 "title" : "Change Shader for Material",
					  "keywords" : [ "swap", "replace", "texture" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ "ChangeTrafficSignal.vuo" ]
					 }
				 });

void nodeEvent
(
	VuoInputData(VuoSceneObject) object,
	VuoInputData(VuoText) material,
	VuoInputData(VuoShader) shader,
	VuoOutputData(VuoSceneObject) shadedObject
)
{
	VuoSceneObject copy = VuoSceneObject_copy(object);

	bool applyToAll = (strlen(material) == 0);
	VuoSceneObject_apply(&copy, ^(VuoSceneObject *currentObject, float modelviewMatrix[16]){
							if (!currentObject->shader)
								return;

							if (applyToAll || strstr(currentObject->shader->name, material))
								currentObject->shader = shader;
						 });

	*shadedObject = copy;
}
