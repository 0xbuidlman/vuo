/**
 * @file
 * vuo.event.changed.VuoReal node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Changed",
					 "description" : "Outputs an event when the input changes to a different value.",
					 "keywords" : [ "pulse", "watcher" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

VuoReal * nodeInstanceInit(void)
{
	VuoReal *lastValue = (VuoReal *)malloc(sizeof(VuoReal));
	VuoRegister(lastValue, free);
	*lastValue = false;
	return lastValue;
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoReal *) lastValue,
		VuoInputData(VuoReal, {"default":0.0}) value,
		VuoInputEvent(VuoPortEventBlocking_Door, value) valueEvent,
		VuoOutputEvent() changed
)
{
	*changed = (**lastValue != value);
	**lastValue = value;
}

void nodeInstanceFini
(
		VuoInstanceData(VuoReal *) lastValue
)
{
}
