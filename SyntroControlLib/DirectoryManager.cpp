//
//  Copyright (c) 2014 Scott Ellis and Richard Barnett
//	
//  This file is part of SyntroControlLib
//
//  SyntroControlLib is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  SyntroControlLib is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with SyntroControlLib.  If not, see <http://www.gnu.org/licenses/>.
//

#include "SyntroLib.h"
#include "DirectoryManager.h"
#include "SyntroServer.h"


DirectoryManager::DirectoryManager(void)
{
	int		i;

	m_logTag = "DirectoryManager";
	for (i = 0; i < SYNTRO_MAX_CONNECTEDCOMPONENTS; i++) {
		memset(m_directory+1, 0, sizeof(DM_CONNECTEDCOMPONENT));
		m_directory[i].index = i;
		m_directory[i].DE = NULL;
		m_directory[i].valid = false;
		m_directory[i].componentDE = NULL;
	}
	strcpy(m_lastError, "Undefined error");
	m_sequenceID = 0;
}

DirectoryManager::~DirectoryManager(void)
{
}

//-----------------------------------------------------------------------------------
//
//	Public functions. All of these are protected by a lock to ensure consistency
//	in a multitasking environment. Protected routines do not use the lock to avoid
//	potential deadlocks.


void DirectoryManager::DMShutdown()
{
	int		i;

	m_lock.lock();
	for (i = 0; i < SYNTRO_MAX_CONNECTEDCOMPONENTS; i++) {
		freeConnectedComponent(m_directory+i);
	}
	m_lock.unlock();
}

//	DMProcessDE - processes a Directory Entry from a component
//
//	Note - directory string must be zero terminated!!
//
//	The connectedComponent contains the UID of the source of the DE. This is used for switching purposes as the
//	endpoint UID in the service entry may not be the next hop address if there are one or more
//	SyntroControls on the path. connectedComponent->connectedComponentUID is the UID of the next 
//	component in the route to the service. pDE is the pointer to the received DE, nLen is the total length of the DE
//	(which is made up of multiple zero-terminated strings). Returns true if ok, false if an
//	error occurred.

bool DirectoryManager::DMProcessDE(DM_CONNECTEDCOMPONENT *connectedComponent, char *DE, int len)
{
	int DELength;
	int entryLength;
	char *thisEntry, *nextEntry;

	int index;
	char tag[SYNTRO_MAX_TAG];
	char nonTag[SYNTRO_MAX_NONTAG];
	DM_SERVICE *service;
	DM_COMPONENT *component;								// we extract into this for lookup
	DM_COMPONENT *componentLookup;							// result of lookup
	DM_COMPONENT *previousComponent;						// for deleting components at end
	int i;
	bool changed;											// so we know if something changed

	if (DE == NULL)
		return true;
	if (len == 0)
		return true;										// means that there's no change

	TRACE1("New directory from %s", qPrintable(SyntroUtils::displayUID(&(connectedComponent->connectedComponentUID))));
	QMutexLocker locker(&m_lock);

	changed = false;
	component = connectedComponent->componentDE;
	while (component != NULL) {
		component->seenInDE = false;						// set not seen on all components
		component = component->next;
	}

	DELength = len;
	thisEntry = DE;

	while (DELength > 0) {
		entryLength = (int)strlen(thisEntry) + 1;			// find the length of this entry
		nextEntry = thisEntry + entryLength;				// this is where the next entry will start

		m_tagPtr = thisEntry;								// set pointer for tag routines
		if (!getTag(tag))
			break;
		if (strcmp(tag, DETAG_COMP) != 0)
			return false;

//	Read in endpoint component header. See if this already exists in directory

		component = (DM_COMPONENT *)calloc(1, sizeof(DM_COMPONENT));
		if (!getSimpleValue(DETAG_UID, component->UIDStr, sizeof(SYNTRO_UIDSTR)))
			goto deerr;
		if (!getSimpleValue(DETAG_APPNAME, component->appName, SYNTRO_MAX_APPNAME))
			goto deerr;
		if (!getSimpleValue(DETAG_COMPTYPE, component->componentType))
			goto deerr;
		SyntroUtils::UIDSTRtoUID(component->UIDStr, &(component->componentUID));
		if ((componentLookup = findComponent(connectedComponent, &(component->componentUID), 
							component->appName, component->componentType)) != NULL) {	// this component already exists in directory
			if(componentLookup->originalDE == NULL) {			// should never happen but just in case
				logError(QString("DirectoryManager found NULL origde when processing ") + SyntroUtils::displayUID(&(component->componentUID)));
				free(component);
				goto nextde;
			}
			if (strcmp(thisEntry, componentLookup->originalDE) == 0) {		// DE hasn't changed - can terminate processing
				free(component);
				componentLookup->seenInDE = true;			// flag as seen
				goto nextde;								// nothing more to do for this one
			}
			//	if we get here, DE has changed so we need to continue processing
			deleteComponent(connectedComponent, componentLookup);// clear down old entry
		}
		component->seenInDE = true;							// indicate this component has been seen in DE
		changed = true;

//	Now read in the service entries and add them to the service array

		index = 0;											// generates port numbers
		component->serviceCount = 0;							// clear services count
		while(1) {
			if (!getTag(tag))
				break;										// finished
			if (strcmp(tag, DETAG_COMP_END) == 0) {			// hit end of component
				break;
			}
			if (component->serviceCount == SYNTRO_MAX_SERVICESPERCOMPONENT) {
				strcpy(m_lastError, "Too many services in DE");
				goto deerr;
			}
			service = component->services + index;			// this is where the new one goes

			if (!getNonTag(nonTag))						// get the value
				goto deerr;

			if (strcmp(tag, DETAG_MSERVICE) == 0) {		// multicast service
				service->serviceType = SERVICETYPE_MULTICAST;
			} else {
				if (strcmp(tag, DETAG_ESERVICE) == 0) {
					service->serviceType = SERVICETYPE_E2E;
				} else {
					if (strcmp(tag, DETAG_NOSERVICE) == 0) {
						service->serviceType = SERVICETYPE_NOSERVICE;
					} else {
						strcpy(m_lastError, "Incorrect service type in DE");
						goto deerr;
					}
				}
			}
			if (strlen(nonTag) >= SYNTRO_MAX_SERVNAME) {
				strcpy(m_lastError, "Service name too long in DE");
				goto deerr;
			}
			strcpy(service->serviceName, nonTag);
			service->port = index++;
			service->valid = true;

//	Get closing tag - don't really need to be fussy here

			if (!getTag(tag))
				goto deerr;

			component->serviceCount++;						// accept this one and move on
			continue;

deerr:
			logError(m_lastError);
			free(component);
			goto nextde;									// give up on this and go to next
		}

		// if get here, processed component

		service = component->services;
		for (i = 0; i < component->serviceCount; i++, service++) {
			if ((component->services[i].serviceType == SERVICETYPE_E2E) || 
				(component->services[i].serviceType == SERVICETYPE_NOSERVICE)) {
				component->services[i].multicastMap = NULL;
			} else {										// need to allocate a multicast map
				component->services[i].multicastMap = m_server->m_multicastManager.MMAllocateMMap(
							&(connectedComponent->connectedComponentUID),
							&(component->componentUID), 
							component->appName,
							service->serviceName, 
							service->port);
			}
		}
		component->originalDE = (char *)malloc(strlen(thisEntry) + 1);
		strcpy(component->originalDE, thisEntry);			// record the original DE
		buildLocalDE(component);							// generate a new DE with adjusted port numbers
		m_server->m_fastUIDLookup.FULAdd(&(component->componentUID), connectedComponent->data); 
		component->sequenceID = m_sequenceID++;				// alocate new ID
		component->next = connectedComponent->componentDE;	// link in new component
		connectedComponent->componentDE = component;		// save it

nextde:
		thisEntry = nextEntry;								// set up for next entry
		DELength -= entryLength;							// and take off length of last entry
	}

//	Finally, check to see if any components missing

	component = connectedComponent->componentDE;
	previousComponent = NULL;
	while (component != NULL)
	{
		if (!component->seenInDE) {							// this component wasn't seen - delete it
			deleteComponent(connectedComponent, component);
			changed = true;
			if (previousComponent == NULL)
				component = connectedComponent->componentDE;
			else
				component = previousComponent->next;		// jump over the deleted one
		} else {											// it's fine - just move on
			previousComponent = component;
			component = component->next;
		}
	}
	if (changed)
		emit DMNewDirectory(connectedComponent->index);
	return true;
}


//	DMFindService - Process a lookup and fill in structure appropriately
//	Returns true if found, false if not there
//
//	If the incoming lookup indicates success, the routine checks if the 
//	existing response is still valid. If so, this is a quick process and
//	just returns success. If the incoming indicated fail or it did not match
//	the previously successful lookup, a full lookup takes place.
//
//	sourceUID is the UID of the endpoint being registered

bool DirectoryManager::DMFindService(SYNTRO_UID *sourceUID, SYNTRO_SERVICE_LOOKUP *serviceLookup)
{
	QString componentName;
	QString serviceName;
	QString regionName;
	int componentIndex;
	int servicePort;
	int serviceIndex;

	DM_CONNECTEDCOMPONENT *connectedComponent;
	DM_COMPONENT *component;
	DM_SERVICE *service;

	QMutexLocker locker(&m_lock);
	if (!SyntroUtils::crackServicePath(serviceLookup->servicePath, regionName, componentName, serviceName)) {
		serviceLookup->response = SERVICE_LOOKUP_FAIL;
		TRACE1("Path %s is invalid", serviceLookup->servicePath);
		return false;
	}

	if (serviceLookup->response == SERVICE_LOOKUP_SUCCEED) {	// this is a refresh - check all important fields for validity
		componentIndex = SyntroUtils::convertUC2ToInt(serviceLookup->componentIndex);
		servicePort = SyntroUtils::convertUC2ToInt(serviceLookup->remotePort);
		if ((componentIndex < 0) || (componentIndex >= SYNTRO_MAX_CONNECTEDCOMPONENTS)) {
			TRACE2("Lookup refresh with incorrect CompIndex %d for service %s", componentIndex, serviceLookup->servicePath);
			goto fullLookup;
		}
		connectedComponent = m_directory + componentIndex;
		component = connectedComponent->componentDE;
		while (component != NULL)
		{
			if ((componentName.length() > 0) && (strcmp(qPrintable(componentName), component->appName) != 0)) {
				component = component->next;
				continue;									// require a specific component but not this one
			}

			//	Need to check the service entry information

			if (serviceLookup->serviceType == SERVICETYPE_MULTICAST) {
				service = component->services;				// need to match against multicastMap index in this case
				for (serviceIndex = 0; serviceIndex < component->serviceCount; serviceIndex++, service++) {
					if (service->serviceType != SERVICETYPE_MULTICAST)
						continue;
					if (service->multicastMap == NULL)
						continue;							// should not happen but just in case
					if (service->multicastMap->index == servicePort)
						break;
				}
				if (serviceIndex == component->serviceCount) {
					component = component->next;
					continue;								// can't be this component then
				}
			} else {
				if ((servicePort < 0) || (servicePort >= component->serviceCount)) { // not in correct range
					component = component->next;
					continue;								// can't be this component as port outof range
				}
				service = component->services + servicePort;	
				if (service->serviceType != SERVICETYPE_E2E) {
					component = component->next;
					continue;								// can't be this component as wrong type
				}
			}

			if (component->sequenceID != SyntroUtils::convertUC4ToInt(serviceLookup->ID)) {
				component = component->next;
				continue;
			}
			if (strcmp(service->serviceName, qPrintable(serviceName)) != 0) {
				component = component->next;
				continue;
			}
			if (!SyntroUtils::compareUID(&(component->componentUID), &(serviceLookup->lookupUID))) {
				component = component->next;
				continue;
			}

			if ((service->serviceType == SERVICETYPE_MULTICAST) && (service->multicastMap != NULL))
				service->multicastMap->lastLookupRefresh = SyntroClock();

			TRACE3("Expedited lookup from component %s to source %s port %d", 
				qPrintable(SyntroUtils::displayUID(sourceUID)), qPrintable(SyntroUtils::displayUID(&component->componentUID)), 
						SyntroUtils::convertUC2ToInt(serviceLookup->localPort));

			return true;									// all good!
		}
	}

//	Jump to here if refresh lookup failed for some reason

fullLookup:

	connectedComponent = m_directory;
	for (componentIndex = 0; componentIndex < SYNTRO_MAX_CONNECTEDCOMPONENTS; componentIndex++, connectedComponent++) {
		component = connectedComponent->componentDE;
		while (component != NULL)
		{
			if ((componentName.length() > 0) && (strcmp(qPrintable(componentName), component->appName) != 0)) {
				component = component->next;
				continue;							// require a specific component but not this one
			}
			service = component->services;
			for (servicePort = 0; servicePort < component->serviceCount; servicePort++, service++) {
				if (serviceLookup->serviceType != service->serviceType)
					continue;
				if (strcmp(service->serviceName, qPrintable(serviceName)) != 0)
					continue;

				// found it - but it could be a registration request or removal

				if (serviceLookup->response == SERVICE_LOOKUP_REMOVE) { // this is a removal request
					m_server->m_multicastManager.MMDeleteRegistered(sourceUID, SyntroUtils::convertUC2ToUInt(serviceLookup->localPort));
					TRACE3("Removed reg from component %s to source %s port %d", 
						qPrintable(SyntroUtils::displayUID(sourceUID)), 
						qPrintable(SyntroUtils::displayUID(&component->componentUID)), SyntroUtils::convertUC2ToInt(serviceLookup->localPort));
					return true;	
				}

				memcpy(&(serviceLookup->lookupUID), &(component->componentUID), sizeof(SYNTRO_UID));
				SyntroUtils::convertIntToUC4(component->sequenceID, serviceLookup->ID);
				if (serviceLookup->serviceType == SERVICETYPE_MULTICAST)
					SyntroUtils::convertIntToUC2(service->multicastMap->index, serviceLookup->remotePort);
				else
					SyntroUtils::convertIntToUC2(service->port, serviceLookup->remotePort);
				SyntroUtils::convertIntToUC2(componentIndex, serviceLookup->componentIndex);
				if (serviceLookup->serviceType == SERVICETYPE_MULTICAST) {		// must add this to the registered components list
					if (m_server->m_multicastManager.MMCheckRegistered(service->multicastMap, 
								sourceUID, SyntroUtils::convertUC2ToInt(serviceLookup->localPort))) { // already there - just a refresh
						TRACE3("Refreshed reg from component %s to source %s port %d", 
							qPrintable(SyntroUtils::displayUID(sourceUID)), qPrintable(SyntroUtils::displayUID(&component->componentUID)), 
							SyntroUtils::convertUC2ToInt(serviceLookup->localPort));
						serviceLookup->response = SERVICE_LOOKUP_SUCCEED;
						return true;	
					}
					//	Must add as this is a new one
					m_server->m_multicastManager.MMAddRegistered(service->multicastMap, sourceUID, 
								SyntroUtils::convertUC2ToInt(serviceLookup->localPort));
					logDebug(QString("Added reg request from component %1 to source %2 port %3")
						.arg(SyntroUtils::displayUID(sourceUID))
						.arg(SyntroUtils::displayUID(&component->componentUID))
						.arg(SyntroUtils::convertUC2ToInt(serviceLookup->localPort)));

					serviceLookup->response = SERVICE_LOOKUP_SUCCEED;
					return true;
				} else {
					TRACE3("Refreshed E2E lookup from component %s to source %s port %d", 
						qPrintable(SyntroUtils::displayUID(sourceUID)), qPrintable(SyntroUtils::displayUID(&component->componentUID)), 
						SyntroUtils::convertUC2ToInt(serviceLookup->localPort));
					serviceLookup->response = SERVICE_LOOKUP_SUCCEED;
					return true;
				}
			}
			component = component->next;
		}
	}
	//	not found

	serviceLookup->response = SERVICE_LOOKUP_FAIL;
	TRACE1("Lookup for %s failed", serviceLookup->servicePath);
	return false;
}


DM_CONNECTEDCOMPONENT *DirectoryManager::DMAllocateConnectedComponent(void *data)
{
	int slot;
	DM_CONNECTEDCOMPONENT *connectedComponent;

	QMutexLocker locker(&m_lock);
	for (slot = 0, connectedComponent = m_directory; slot < SYNTRO_MAX_CONNECTEDCOMPONENTS; slot++, connectedComponent++) {
		if (!connectedComponent->valid)	
			break;
	}
	if (slot == SYNTRO_MAX_CONNECTEDCOMPONENTS) {
		logError("No more space in directory!");
		return NULL;
	}
	connectedComponent->valid = true;
	connectedComponent->index = slot;
	connectedComponent->data = data;
	return connectedComponent;
}


void DirectoryManager::DMDeleteConnectedComponent(DM_CONNECTEDCOMPONENT *connectedComponent)
{
	m_lock.lock();
	freeConnectedComponent(connectedComponent);
	m_lock.unlock();
}


void DirectoryManager::DMBuildDirectoryMessage(int offset, char **message, int *messageLength, bool trunk)
{
	int length;
	int i;
	DM_COMPONENT *component;
	DM_CONNECTEDCOMPONENT *connectedComponent;
	char *directoryBuffer, *directoryPointer;
	const char *myDE;

	//	First, compute total length of DEs

	QMutexLocker locker(&m_lock);
	length = 0;
	connectedComponent = m_directory;
	for (i = 0; i < SYNTRO_MAX_CONNECTEDCOMPONENTS; i++, connectedComponent++) {
		if (!connectedComponent->valid)
			continue;
		if (trunk && (SyntroUtils::convertUC2ToInt(connectedComponent->connectedComponentUID.instance) < INSTANCE_COMPONENT))
			continue;										// must be a real component for trunk mode
		component = connectedComponent->componentDE;
		while (component != NULL) {
			length += (int)strlen(component->localDE) + 1;	// add in length of this entry and its zero
			component = component->next;
		}
	}
	myDE = m_server->m_componentData.getMyDE();
	length += (int)strlen(myDE) + 1;						// add in my own DE

//	now actually generate the DE

	directoryBuffer = (char *)malloc(length + offset);
	directoryPointer = directoryBuffer + offset;			// start at the offset
	strcpy(directoryPointer, myDE);
	directoryPointer += strlen(directoryPointer) + 1;		// set pointer for more zero terminated DEs

	connectedComponent = m_directory;
	for (i = 0; i < SYNTRO_MAX_CONNECTEDCOMPONENTS; i++, connectedComponent++) {
		if (!connectedComponent->valid)
			continue;
		if (trunk && (SyntroUtils::convertUC2ToInt(connectedComponent->connectedComponentUID.instance) < INSTANCE_COMPONENT))
			continue;										// must be a real component if trunk moe
		component = connectedComponent->componentDE;
		while (component != NULL) {
			strcpy(directoryPointer, component->localDE);
			directoryPointer += strlen(component->localDE) + 1;
			component = component->next;
		}
	}
	*message = directoryBuffer;
	*messageLength = length + offset;								// total length of returned buffer
}

//
//	End of public function section
//
//----------------------------------------------------------------------------

void	DirectoryManager::buildLocalDE(DM_COMPONENT *component)
{
	DM_SERVICE *service;
	int i;
	char *DE;

	if (component->localDE != NULL) {
		free(component->localDE);
		component->localDE = NULL;
	}
	DE = component->localDE = (char *)malloc(strlen(component->originalDE)*2+1);	// pretty much worst case

	DE[0] = 0;
	sprintf(DE, "<%s>", DETAG_COMP);
	sprintf(DE + (int)strlen(DE), "<%s>%s</%s>", DETAG_UID, qPrintable(SyntroUtils::displayUID(&component->componentUID)), DETAG_UID);
	sprintf(DE + (int)strlen(DE), "<%s>%s</%s>", DETAG_APPNAME, component->appName, DETAG_APPNAME);
	sprintf(DE + (int)strlen(DE), "<%s>%s</%s>", DETAG_COMPTYPE, component->componentType, DETAG_COMPTYPE);

	service = component->services;
	for (i = 0; i < component->serviceCount; i++, service++) {
		switch(service->serviceType) {
			case SERVICETYPE_MULTICAST:
				sprintf(DE + (int)strlen(DE), "<%s>%s</%s>", DETAG_MSERVICE, service->serviceName, DETAG_MSERVICE);
				break;

			case SERVICETYPE_E2E:
				sprintf(DE + (int)strlen(DE), "<%s>%s</%s>", DETAG_ESERVICE, service->serviceName, DETAG_ESERVICE);
				break;

			case SERVICETYPE_NOSERVICE:
				sprintf(DE + (int)strlen(DE), "<%s></%s>", DETAG_NOSERVICE, DETAG_NOSERVICE);
				break;
		}
	}
	sprintf(DE + (int)strlen(DE), "</%s>", DETAG_COMP);

}


//	GetLastError - return a pointer to a descriptive string. Call after a routine returns false.

char *DirectoryManager::DMGetLastErr()
{
	return m_lastError;
}


DM_COMPONENT *DirectoryManager::findComponent(DM_CONNECTEDCOMPONENT *connectedComponent, SYNTRO_UID *UID, char *name, char *type)
{
	DM_COMPONENT *component;

	component = connectedComponent->componentDE;
	while (component != NULL) {
		if (SyntroUtils::compareUID(UID, &(component->componentUID))) {	// found correct UID
			if ((strcmp(name, component->appName) == 0) && (strcmp(type, component->componentType) == 0))
				return component;							// found it
			//	Correct UID but mismatch on name or type. Implies a new component on same instance.
			//	Delete this old entry and carry on looking - there can't be two components on the same UID!
			deleteComponent(connectedComponent, component);
		}
		component = component->next;
	}
	return NULL;									// not there
}


void DirectoryManager::freeConnectedComponent(DM_CONNECTEDCOMPONENT *connectedComponent)
{
	if (connectedComponent == NULL || !connectedComponent->valid)
		return;												// if not set up yet

	TRACE1("Freeing connected component %s", qPrintable(SyntroUtils::displayUID(&connectedComponent->connectedComponentUID)));

	m_server->m_multicastManager.MMDeleteRegistered(&(connectedComponent->connectedComponentUID), -1);
	connectedComponent->valid = false;

	while (connectedComponent->componentDE != NULL) {
		deleteComponent(connectedComponent, connectedComponent->componentDE);
	}

	if (connectedComponent->DE != NULL) {
		free(connectedComponent->DE);
		connectedComponent->DE = NULL;
	}
}

void	DirectoryManager::deleteComponent(DM_CONNECTEDCOMPONENT *connectedComponent, DM_COMPONENT *component)
{
	DM_COMPONENT *previousComponent;
	DM_SERVICE *service;
	int i;

	emit DMRemoveDirectory(component);						// *** this must be a direct connection ***

//	unlink from connected component

	if (connectedComponent->componentDE == component) {		// was head of list
		connectedComponent->componentDE = component->next;
	} else {
		previousComponent = connectedComponent->componentDE;
		while (previousComponent->next != component) {
			if (previousComponent->next == NULL) {
				logWarn(QString("deleteComponent not match %1 %2").arg(component->appName).arg(component->componentType));
				return;										// this shouldn't really happen
			}
			previousComponent = previousComponent->next;
		}
		previousComponent->next = component->next;			// link around
	}

//	pDMC is now off the list

	component->componentType[0] = 0;
	component->appName[0] = 0;
	component->UIDStr[0] = 0;
	component->serviceCount = 0;
	service = component->services;

	for (i = 0; i < SYNTRO_MAX_SERVICESPERCOMPONENT; i++, service++) {
		service->valid = false;
		if (service->multicastMap != NULL) {
			m_server->m_multicastManager.MMFreeMMap(service->multicastMap);
			service->multicastMap = NULL;
		}
	}
	if (component->originalDE != NULL) {
		free(component->originalDE);
		component->originalDE = NULL;
	}

	if (component->localDE != NULL) {
		free(component->localDE);
		component->localDE = NULL;
	}
	free(component);
}


//---------------------------------------------------------------------------
//
//	Tag read related routines

//	getStartTag  - used to get a specific next start tag

bool DirectoryManager::getStartTag(const char *startTag)
{
	char tag[SYNTRO_MAX_TAG];

	if (!getTag(tag))
		return false;
	if (strcmp(tag, startTag) == 0)
		return true;
	else {
		sprintf(m_lastError, "getStartTag - wrong start tag %s instead of %s", tag, startTag);
		return false;
	}
}

//	skipToStartTag  - used to skip until a specific start tag is found

bool DirectoryManager::skipToStartTag(const char *startTag)
{
	char tag[SYNTRO_MAX_TAG];

	while (1) {
		if (!getTag(tag))
			return false;
		if (strcmp(tag, startTag) == 0)
			return true;
	}
}


//	getEndTag  - used to get a specific end tag

bool DirectoryManager::getEndTag(const char *endTag)
{
	char tag[SYNTRO_MAX_TAG];

	if (!getTag(tag))
		return false;
	if (tag[0] != '/') {
		sprintf(m_lastError, "getEndTag - not end tag %s", tag);
		return false;							// not an end tag
	}
	if (strcmp(tag+1, endTag) == 0)
		return true;
	else {
		sprintf(m_lastError, "getEndTag - wrong tag %s instead of %s", tag, endTag);
		return false;							// not an end tag
	}
}

//	skipToEndTag  - used to skip until a specific end tag is found

bool	DirectoryManager::skipToEndTag(const char *endTag)
{
	char tag[SYNTRO_MAX_TAG];

	while (1) {
		if (!getTag(tag))
			return false;
		if (tag[0] != '/')
			continue;
		if (strcmp(tag+1, endTag) == 0)
			return true;
	}
}


//	Gets a non-tag string. pStr must point to an array of size at least SYNTRO_MAX_NONTAG

bool DirectoryManager::getNonTag(char *str)
{
	char c;
	int pos;

	pos = 0;
	while ((c = *m_tagPtr) != 0) {
		if (c == '<') {										// hit start of tag
			str[pos] = 0;
			return true;
		}
		str[pos++] = c;
		if (pos == SYNTRO_MAX_NONTAG)
			pos--;
		m_tagPtr++;
	}
	strcpy(m_lastError, "getNonTag - hit end of directory before closing tag");
	return false;
}

//	Gets the next tag in the config file. pTag must point to an array of size at least SYNTRO_MAX_TAG

bool DirectoryManager::getTag(char *tag)
{
	int pos;
	char c;
	bool intag;

	pos = 0;
	intag = false;
	while ((c = *m_tagPtr) != 0) {
		m_tagPtr++;
		if (intag) {
			if (c == '>') {									// end of tag
				tag[pos] = 0;
				return true;
			}
			tag[pos++] = c;
			if (pos == SYNTRO_MAX_TAG)
				pos--;
		} else {
			if (c == '<')
				intag = true;
		}
	}
	strcpy(m_lastError, "getTag - hit end of directory before tag");
	return false;
}


//		Value reading routines

//	GetSimpleBool gets a bool value with the specified tag (in pValueTag) using the current position

bool	DirectoryManager::getSimpleBool(const char *valueTag, bool *boolValue)
{
	char value[SYNTRO_MAX_NONTAG];

	if (!getSimpleValue(valueTag, value))
		return false;
	if (strcmp(value, "Y") == 0) {
		*boolValue = true;
		return true;
	}
	if (strcmp(value, "N") == 0) {
		*boolValue = false;
		return true;
	}
	return false;
}

//	GetSimpleInt gets an int value with the specified tag (in pValueTag) using the current position

bool DirectoryManager::getSimpleInt(const char *valueTag, int *intValue)
{
	char value[SYNTRO_MAX_NONTAG];

	if (!getSimpleValue(valueTag, value))
		return false;
	sscanf(value, "%d", intValue);
	return true;
}

//	getSimpleValue gets an stringe with the specified tag (in pValueTag) using the current position.
//	pValue must point to an array of at least SYNTRO_MAX_NONTAG in size.

bool DirectoryManager::getSimpleValue(const char *valueTag, char *value)
{
	if (!getStartTag(valueTag))
		return false;
	if (!getNonTag(value))
		return false;
	return getEndTag(valueTag);
}

//	getSimpleValue gets a string with the specified tag (in valueTag) using the current position.
//	pValue must point to an array of at least maxLen in size.

bool DirectoryManager::getSimpleValue(const char *valueTag, char *value, int maxLen)
{
	char temp[SYNTRO_MAX_NONTAG];

	if (!getStartTag(valueTag))
		return false;
	if (!getNonTag(temp))
		return false;
	if (((int)strlen(temp) >= maxLen) || (maxLen >= SYNTRO_MAX_NONTAG)){
		strcpy(m_lastError, "getSimpleValue - value too long");
		return false;
	}
	strcpy(value, temp);
	return getEndTag(valueTag);
}

