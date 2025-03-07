/**
 * @file
 * VuoRendererPublishedPort interface.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUORENDERERPUBLISHEDPORT_HH
#define VUORENDERERPUBLISHEDPORT_HH

#include "VuoBaseDetail.hh"
#include "VuoNode.hh"
#include "VuoPort.hh"
#include "VuoPublishedPort.hh"
#include "VuoRendererItem.hh"
#include "VuoRendererCable.hh"

/**
 * A published input or output port.
 */
class VuoRendererPublishedPort : public VuoRendererItem, public VuoBaseDetail<VuoPublishedPort>
{
public:
	VuoRendererPublishedPort(VuoPublishedPort *basePublishedPort);

	QRectF boundingRect(void) const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	void setName(string name);
	bool canAccommodateInternalPort(VuoRendererPort *internalPort, bool eventOnlyConnection);
	bool isCompatibleAliasWithoutSpecializationForInternalPort(VuoRendererPort *port, bool eventOnlyConnection);
	bool isCompatibleAliasWithSpecializationForInternalPort(VuoRendererPort *port, bool eventOnlyConnection);
	bool isCompatibleAliasWithSpecializationForInternalPort(VuoRendererPort *internalPort, bool eventOnlyConnection, VuoRendererPort **portToSpecialize, string &specializedTypeName);
	bool canBeMergedWith(VuoPublishedPort *otherExternalPort, bool mergeWillAddData);
	void addConnectedPort(VuoPort *port);
	void removeConnectedPort(VuoPort *port);
	QPoint getCompositionViewportPos(void) const;
	void setCompositionViewportPos(QPoint pos);
	VuoRendererPort * getPortRepresentation();
	void setCurrentlyActive(bool active);
	bool getCurrentlyActive();

private:
	static VuoRendererPort * createPortRepresentation(string name, VuoType *type, bool isPublishedInput);
	void updateNameRect();

	QPoint compositionViewportPos;
	VuoRendererPort *portRepresentation;
	bool isActive;

};

#endif // VUORENDERERPUBLISHEDPORT_HH
