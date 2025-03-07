﻿/**
 * @file
 * vuo.type.list.point3d.real.x node implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title": "Convert 3D Point List to Real List",
					  "description": "Creates a list of real numbers using the X coordinate of the input list of 3D points.",
					  "version": "1.0.0"
				 });

void nodeEvent
(
	VuoInputData(VuoList_VuoPoint3d) point3d,
	VuoOutputData(VuoList_VuoReal) x
)
{
	*x = VuoListCreate_VuoReal();
	unsigned long count = VuoListGetCount_VuoPoint3d(point3d);
	for (unsigned long i = 1; i <= count; ++i)
		VuoListAppendValue_VuoReal(*x, VuoListGetValue_VuoPoint3d(point3d, i).x);
}
