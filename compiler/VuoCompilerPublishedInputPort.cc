/**
 * @file
 * VuoCompilerPublishedInputPort implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerPublishedInputPort.hh"

/**
 * Creates a published port that is not connected to any port in a composition.
 *
 * @param name A name for the published port.
 * @param type A type for the published port.
 * @param connectedPorts A non-empty set of ports, all having the same data type.
 * @param triggerPort The trigger port that implements events fired via this published port.
 */
VuoCompilerPublishedInputPort::VuoCompilerPublishedInputPort(string name,
															 VuoType *type,
															 const set<VuoCompilerPort *> &connectedPorts,
															 VuoCompilerTriggerPort *triggerPort)
	: VuoCompilerPublishedPort(name, type, false, connectedPorts)
{
	this->triggerPort = triggerPort;
}

/**
 * Returns the base port belonging to the "vuo.in" pseudo-node and associated with this published port.
 */
VuoPort * VuoCompilerPublishedInputPort::getVuoPseudoPort(void)
{
	if (! getTriggerPort())
		return NULL;

	return getTriggerPort()->getBase();
}

/**
 * Returns the trigger port that implements events fired via this published port.
 */
VuoCompilerTriggerPort * VuoCompilerPublishedInputPort::getTriggerPort(void)
{
	return triggerPort;
}

/**
 * Sets the trigger port that implements events fired via this published port.
 */
void VuoCompilerPublishedInputPort::setTriggerPort(VuoCompilerTriggerPort *port)
{
	this->triggerPort = port;
}

/**
 * Returns the JSON-formatted representation of the initial value of the published port data.
 */
string VuoCompilerPublishedInputPort::getInitialValue(void)
{
	json_object *value = NULL;
	if (json_object_object_get_ex(details, "default", &value))
		return json_object_to_json_string_ext(value, JSON_C_TO_STRING_PLAIN);

	return "";
}

/**
 * Sets the initial value of the published port data. This is the value it has from the time that
 * the composition begins executing until the first time the port data is overwritten.
 *
 * @param initialValueAsString JSON-formatted representation of the initial value.
 */
void VuoCompilerPublishedInputPort::setInitialValue(string initialValueAsString)
{
	setDetail("default", initialValueAsString);
}
