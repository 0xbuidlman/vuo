/**
 * @file
 * VuoVerticesParametric interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "node.h"

VuoList_VuoVertices VuoVerticesParametric_generate(VuoReal time, VuoText xExp, VuoText yExp, VuoText zExp, VuoInteger uSubdivisions, VuoInteger vSubdivisions, bool closeU, VuoReal uMin, VuoReal uMax, bool closeV, VuoReal vMin, VuoReal vMax);

#ifdef __cplusplus
}
#endif
