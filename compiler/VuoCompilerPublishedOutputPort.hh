/**
 * @file
 * VuoCompilerPublishedOutputPort interface.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERPUBLISHEDOUTPUTPORT_HH
#define VUOCOMPILERPUBLISHEDOUTPUTPORT_HH

#include "VuoCompilerPublishedPort.hh"

/**
 * A published output port.
 */
class VuoCompilerPublishedOutputPort : public VuoCompilerPublishedPort
{
public:
	VuoCompilerPublishedOutputPort(string name, VuoType *type, set<VuoCompilerPort *> connectedPorts, VuoPort *vuoOutPort);
	VuoPort * getVuoPseudoPort(void);
	void setVuoPseudoPort(VuoPort *port);

private:
	VuoPort *vuoOutPort;
};

#endif // VUOCOMPILERPUBLISHEDOUTPUTPORT_HH
