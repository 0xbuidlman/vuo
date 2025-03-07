/**
 * @file
 * vuo.vertices.make.triangle node implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make Triangle Mesh",
					 "keywords" : [ "equilateral", "3-gon", "3gon", "shape" ],
					 "version" : "2.0.0",
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoOutputData(VuoMesh) mesh
)
{
	*mesh = VuoMesh_makeEquilateralTriangle();
}
