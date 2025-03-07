/**
 * @file
 * vuo.window.size node implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoWindowProperty.h"

VuoModuleMetadata({
					 "title" : "Change Window Size",
					 "keywords" : [ "width", "height", "dimensions", "properties" ],
					 "version" : "1.0.0",
					 "node" : {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":640}) width,
		VuoInputData(VuoInteger, {"default":480}) height,
		VuoOutputData(VuoWindowProperty) property
)
{
	(*property).type = VuoWindowProperty_Size;
	(*property).width = width;
	(*property).height = height;
}
