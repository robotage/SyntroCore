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

#include "MulticastManager.h"
#include "SyntroServer.h"
#include "SyntroControlLib.h"


MulticastManager::MulticastManager(void)
{
	int		i;

	m_logTag = "MulticastManager";
	for (i = 0; i < SYNTROSERVER_MAX_MMAPS; i++) {
		memset(m_multicastMap+i, 0, sizeof(MM_MMAP));
		m_multicastMap[i].index = i;
		m_multicastMap[i].valid = false;
		m_multicastMap[i].head = NULL;
	}
	m_multicastMapSize = 0;
	m_lastBackground = SyntroClock();
}

MulticastManager::~MulticastManager(void)
{
}


//-----------------------------------------------------------------------------------
//
//	Public functions. All of these are protected by a lock to ensure consistency
//	in a multitasking environment. Protected routines do not use the lock to avoid
//	potential deadlocks.

void MulticastManager::MMShutdown()
{
	int		i;

	for (i = 0; i < SYNTROSERVER_MAX_MMAPS; i++)
		MMFreeMMap(m_multicastMap+i);
}

bool MulticastManager::MMAddRegistered(MM_MMAP *multicastMap, SYNTRO_UID *UID, int port)
{
	MM_REGISTEREDCOMPONENT *registeredComponent;

	QMutexLocker locker(&m_lock);
	if (!multicastMap->valid) {
		logError("Invalid MMAP referenced in AddRegistered");
		return false;
	}

	//	build REGISTEREDCOMPONENT for new registration

	registeredComponent = (MM_REGISTEREDCOMPONENT *)malloc(sizeof(MM_REGISTEREDCOMPONENT));
	registeredComponent->sendSeq = 0;
	registeredComponent->lastAckSeq = 0;
	memcpy(&(registeredComponent->registeredUID), UID, sizeof(SYNTRO_UID));
	registeredComponent->port = port;

	//	Now safe to link in the new one

	registeredComponent->next = multicastMap->head;
	multicastMap->head = registeredComponent;
	multicastMap->lastLookupRefresh = SyntroClock();		// don't time it out straightaway
	sendLookupRequest(multicastMap, true);					// make sure there's a lookup request for the service
	emit MMRegistrationChanged(multicastMap->index);
	return true;									
}


bool	MulticastManager::MMCheckRegistered(MM_MMAP *multicastMap, SYNTRO_UID *UID, int port)
{
	MM_REGISTEREDCOMPONENT	*registeredComponent;

	QMutexLocker locker(&m_lock);

	if (!multicastMap->valid) {
		logError("Invalid MMAP referenced in CheckRegistered");
		return false;
	}
	registeredComponent = multicastMap->head;
	while (registeredComponent != NULL) {
		if (SyntroUtils::compareUID(UID, &(registeredComponent->registeredUID)) && (registeredComponent->port == port)) {
			multicastMap->lastLookupRefresh = SyntroClock();// somebody still wants it
			return true;								// it is there
		}
		registeredComponent = registeredComponent->next;
	}
	return false;										// not found
}

void MulticastManager::MMDeleteRegistered(SYNTRO_UID *UID, int port)
{
	MM_REGISTEREDCOMPONENT *registeredComponent, *previousRegisteredComponent, *deletedRegisteredComponent;
	int i;
	MM_MMAP *multicastMap;

	QMutexLocker locker(&m_lock);
	multicastMap = m_multicastMap;

	for (i = 0; i < m_multicastMapSize; i++, multicastMap++) {
		if (!multicastMap->valid)
			continue;
		registeredComponent = multicastMap->head;
		previousRegisteredComponent = NULL;
		while (registeredComponent != NULL) {
			if (SyntroUtils::compareUID(UID, &(registeredComponent->registeredUID))) {
				if ((port == -1) || (port == registeredComponent->port)) {	// this is a matched entry
					TRACE3("Deleting multicast registration on %s port %d for %s",
						qPrintable(SyntroUtils::displayUID(&registeredComponent->registeredUID)), registeredComponent->port, 
							qPrintable(SyntroUtils::displayUID(UID)))
					if (previousRegisteredComponent == NULL) {	// its the head of the list
						multicastMap->head = registeredComponent->next;
					} else {								// not the head so relink around
						previousRegisteredComponent->next = registeredComponent->next;
					}
					deletedRegisteredComponent = registeredComponent;
					registeredComponent = registeredComponent->next;
					free(deletedRegisteredComponent);
					emit MMRegistrationChanged(multicastMap->index);
					continue;
				}
			}
			previousRegisteredComponent = registeredComponent;
			registeredComponent = registeredComponent->next;
		}
	}
}


void	MulticastManager::MMForwardMulticastMessage(int cmd, SYNTRO_MESSAGE *message, int len)
{
	MM_REGISTEREDCOMPONENT *registeredComponent;
	SYNTRO_EHEAD *inEhead, *outEhead, *ackEhead;
	unsigned char *msgCopy;
	int multicastMapIndex;
	MM_MMAP *multicastMap;

	QMutexLocker locker (&m_lock);
	inEhead = (SYNTRO_EHEAD *)message;
	multicastMapIndex = SyntroUtils::convertUC2ToUInt(inEhead->destPort);	// get the dest port number (i.e. my slot number)
	if (multicastMapIndex >= m_multicastMapSize) {
		logWarn(QString("Multicast message with illegal DPort %1").arg(multicastMapIndex));
		return;										
	}
	multicastMap = m_multicastMap + multicastMapIndex;		// get pointer to entry
	if (!multicastMap->valid) {
		logWarn(QString("Multicast message on not in use map slot %1").arg(multicastMap->index));
		return;										// not in use - hmmm. Should not happen!
	}
	if (len < (int)sizeof(SYNTRO_EHEAD)) {
		logWarn(QString("Multicast message is too short %1").arg(len));
		return;										
	}
	if (!SyntroUtils::compareUID(&(multicastMap->sourceUID), &(inEhead->sourceUID))) {
		logWarn(QString("UID %1 of incoming multicast didn't match UID of slot %2")
			.arg(SyntroUtils::displayUID(&inEhead->sourceUID)).arg(SyntroUtils::displayUID(&multicastMap->sourceUID)));

		return;
	}
	qint64 now = SyntroClock();
	m_server->m_multicastIn++;
	m_server->m_multicastInRate++;

	registeredComponent = multicastMap->head;
	while (registeredComponent != NULL) {
		if (!SyntroUtils::isSendOK(registeredComponent->sendSeq, registeredComponent->lastAckSeq)) {	// see if we have timed out waiting for ack
			if (!SyntroUtils::syntroTimerExpired(now, registeredComponent->lastSendTime, EXCHANGE_TIMEOUT)){
				registeredComponent = registeredComponent->next;
				continue;							// not yet long enough to declare a timeout
			} else {
				registeredComponent->lastAckSeq = registeredComponent->sendSeq;
				logWarn(QString("WFAck timeout on %1").arg(SyntroUtils::displayUID(&registeredComponent->registeredUID)));
			}
		}
		msgCopy = (unsigned char *)malloc(len);
		memcpy(msgCopy, message, len);
		outEhead = (SYNTRO_EHEAD *)msgCopy;
		SyntroUtils::convertIntToUC2(registeredComponent->port, outEhead->destPort);// this is the receiver's service index that was requested
		SyntroUtils::convertIntToUC2(multicastMapIndex, outEhead->sourcePort);					// this is my slot number (needed for the ack)
		outEhead->destUID = registeredComponent->registeredUID;
		outEhead->sourceUID = multicastMap->sourceUID;
		outEhead->seq = registeredComponent->sendSeq;
		registeredComponent->sendSeq++;
		TRACE2("Forwarding mcast from component %s to %s",
				qPrintable(SyntroUtils::displayUID(&outEhead->sourceUID)), qPrintable(SyntroUtils::displayUID(&registeredComponent->registeredUID)));
		m_server->sendSyntroMessage(&(registeredComponent->registeredUID), cmd, (SYNTRO_MESSAGE *)msgCopy, 
					len, SYNTROLINK_LOWPRI);
		m_server->m_multicastOut++;
		m_server->m_multicastOutRate++;
		registeredComponent->lastSendTime = now;
		registeredComponent = registeredComponent->next;
	}

	// send an ACK unless the recipient is us
	if (!SyntroUtils::compareUID(&m_myUID, &multicastMap->sourceUID)) {
		ackEhead = (SYNTRO_EHEAD *)malloc(sizeof(SYNTRO_EHEAD));
		ackEhead->sourceUID = m_myUID;
		ackEhead->destUID = multicastMap->sourceUID;
		SyntroUtils::copyUC2(ackEhead->sourcePort, inEhead->destPort);
		SyntroUtils::copyUC2(ackEhead->destPort, inEhead->sourcePort);
		ackEhead->seq = inEhead->seq + 1;

		if (!m_server->sendSyntroMessage(&(multicastMap->prevHopUID), SYNTROMSG_MULTICAST_ACK,
				(SYNTRO_MESSAGE *)ackEhead, sizeof(SYNTRO_EHEAD), SYNTROLINK_MEDHIGHPRI)) {
			logWarn(QString("Failed mcast ack to %1").arg(SyntroUtils::displayUID(&multicastMap->prevHopUID)));
		}
	}
}


void	MulticastManager::MMProcessMulticastAck(SYNTRO_EHEAD *ehead, int len)
{
	MM_MMAP *multicastMap;
	MM_REGISTEREDCOMPONENT *registeredComponent;
	int slot;

	if (len < (int)sizeof(SYNTRO_EHEAD)) {
		logWarn(QString("Multicast ack is too short %1").arg(len));
		return;
	}

	slot = SyntroUtils::convertUC2ToUInt(ehead->destPort);					// get the port number
	if (slot >= m_multicastMapSize) {
		logWarn(QString("Invalid dest port %1 for multicast ack from %2").arg(slot).arg(SyntroUtils::displayUID(&ehead->sourceUID)));
		return;
	}
	QMutexLocker locker(&m_lock);

	multicastMap = m_multicastMap + slot;
	if (!multicastMap->valid)
		return;									// probably disconnected or something
	if (!SyntroUtils::compareUID(&(multicastMap->sourceUID), &(ehead->destUID))) {
		logWarn(QString("Multicast ack dest %1 doesn't match source %2")
			.arg(SyntroUtils::displayUID(&ehead->destUID))
			.arg(SyntroUtils::displayUID(&multicastMap->sourceUID)));

		return;
	}

	registeredComponent = multicastMap->head;
	while (registeredComponent != NULL) {
		if (SyntroUtils::compareUID(&(ehead->sourceUID), &(registeredComponent->registeredUID)) && 
					(SyntroUtils::convertUC2ToInt(ehead->sourcePort) == registeredComponent->port)) {
			TRACE2("\nMatched ack from remote component %s port %d", 
					qPrintable(SyntroUtils::displayUID(&ehead->sourceUID)), registeredComponent->port);
			registeredComponent->lastAckSeq = ehead->seq;
			return;
		}
		registeredComponent = registeredComponent->next;
	}

	logWarn(QString("Failed to match ack from %1 port %2").arg(SyntroUtils::displayUID(&ehead->sourceUID))
		.arg(SyntroUtils::convertUC2ToInt(ehead->sourcePort)));
}



MM_MMAP	*MulticastManager::MMAllocateMMap(SYNTRO_UID *prevHopUID, SYNTRO_UID *sourceUID, 
								const char *componentName, const char *serviceName, int port)
{
	int i;
	MM_MMAP *multicastMap;

	QMutexLocker locker(&m_lock);
	multicastMap = m_multicastMap;
	for (i = 0; i < SYNTROSERVER_MAX_MMAPS; i++, multicastMap++) {
		if (!multicastMap->valid)
			break;
	}
	if (i == SYNTROSERVER_MAX_MMAPS) {
		logError("No more multicast maps");
		return NULL;
	}
	if (i >= m_multicastMapSize)
		m_multicastMapSize = i+1;							// update the in use map array size
	multicastMap->head = NULL;
	multicastMap->valid = true;
	multicastMap->prevHopUID = *prevHopUID;					// this is the previous hop UID for the service (i.e. where the data comes from)
	multicastMap->sourceUID = *sourceUID;					// this is the original source of the stream
	sprintf(multicastMap->serviceLookup.servicePath, "%s%c%s", componentName, SYNTRO_SERVICEPATH_SEP, serviceName);
	SyntroUtils::convertIntToUC2(port, multicastMap->serviceLookup.remotePort);// this is the target port (the service port)
	SyntroUtils::convertIntToUC2(i, multicastMap->serviceLookup.localPort);	// this is the index into the MMap array
	multicastMap->serviceLookup.response = SERVICE_LOOKUP_FAIL;// indicate lookup response not valid
	multicastMap->serviceLookup.serviceType = SERVICETYPE_MULTICAST;// indicate multicast
	multicastMap->registered = false;						// indicate not registered
	multicastMap->lookupSent = SyntroClock();				// not important until something registered on it
	TRACE3("Added %s from slot %d to multicast table in slot %d", serviceName, port, i);	
	emit MMNewEntry(i);
	return multicastMap;
}

void MulticastManager::MMFreeMMap(MM_MMAP *multicastMap)
{
	MM_REGISTEREDCOMPONENT	*registeredComponent;

	QMutexLocker locker(&m_lock);
	if (!multicastMap->valid)
		return;
	emit MMDeleteEntry(multicastMap->index);
	multicastMap->valid = false;
	while (multicastMap->head != NULL) {
		registeredComponent = multicastMap->head;
		TRACE3("Freeing MMap %s, component %s port %d", 
			multicastMap->serviceLookup.servicePath,
			qPrintable(SyntroUtils::displayUID(&registeredComponent->registeredUID)), registeredComponent->port);
		multicastMap->head = registeredComponent->next;
		free(registeredComponent);
	}
}


void	MulticastManager::MMProcessLookupResponse(SYNTRO_SERVICE_LOOKUP *serviceLookup, int len)
{
	int index;
	MM_MMAP *multicastMap;

	if (len != sizeof(SYNTRO_SERVICE_LOOKUP)) {
		logWarn(QString("Lookup response too short %1").arg(len));
		return;			
	}
	index = SyntroUtils::convertUC2ToUInt(serviceLookup->localPort);		// get the local port
	if (index >= m_multicastMapSize) {
		logWarn(QString("Lookup response from %1 port %2 to incorrect local port %3")
			.arg(SyntroUtils::displayUID(&serviceLookup->lookupUID))
			.arg(SyntroUtils::convertUC2ToInt(serviceLookup->remotePort))
			.arg(SyntroUtils::convertUC2ToInt(serviceLookup->localPort)));

		return;
	}

	QMutexLocker locker(&m_lock);
	multicastMap = m_multicastMap + index;
	if (!multicastMap->valid) {
		logWarn(QString("Lookup response to unused local mmap from %1").arg(serviceLookup->servicePath));
		return;
	}

	if (serviceLookup->response == SERVICE_LOOKUP_FAIL) {		// the endpoint is not there!
		if (multicastMap->serviceLookup.response == SERVICE_LOOKUP_SUCCEED) {	// was ok but went away
			TRACE1("Service %s no longer available", multicastMap->serviceLookup.servicePath);
		}
		else {
			TRACE1("Service %s unavailable", multicastMap->serviceLookup.servicePath);
		}
		multicastMap->serviceLookup.response = SERVICE_LOOKUP_FAIL;	// indicate that the entry is invalid
		multicastMap->registered = false;					// and we can't be registere
		return;
	}
	if (multicastMap->serviceLookup.response == SERVICE_LOOKUP_SUCCEED) {	// we had a previously valid entry
		if ((SyntroUtils::convertUC4ToInt(serviceLookup->ID) == SyntroUtils::convertUC4ToInt(multicastMap->serviceLookup.ID)) 
			&& (SyntroUtils::compareUID(&(serviceLookup->lookupUID), &(multicastMap->serviceLookup.lookupUID))) 
			&& (SyntroUtils::convertUC2ToInt(serviceLookup->remotePort) == SyntroUtils::convertUC2ToInt(multicastMap->serviceLookup.remotePort))) {
				TRACE1("Reconfirmed %s", serviceLookup->servicePath);
		}
	} else {
//	If we get here, something changed
		TRACE3("Service %s mapped to %s port %d", serviceLookup->servicePath, 
			qPrintable(SyntroUtils::displayUID(&serviceLookup->lookupUID)), SyntroUtils::convertUC2ToInt(serviceLookup->remotePort));
		multicastMap->serviceLookup = *serviceLookup;		// record data
	}
	multicastMap->registered = true;
	emit MMDisplay();
}


void	MulticastManager::MMBackground()
{
	MM_MMAP *multicastMap;
	int index;

	qint64 now = SyntroClock();

	if (!SyntroUtils::syntroTimerExpired(now, m_lastBackground, SYNTRO_CLOCKS_PER_SEC))
		return;

	m_lastBackground = now;
	emit MMDisplay();
	QMutexLocker locker(&m_lock);
	multicastMap = m_multicastMap;
	for (index = 0; index < m_multicastMapSize; index++, multicastMap++) {
		if (!multicastMap->valid)
			continue;
		if (SyntroUtils::compareUID(&(multicastMap->sourceUID), &m_myUID))
			continue;										// don't do anything more for SyntroCOntrol services
		if (multicastMap->head == NULL)
			continue;										// no registrations so don't refresh
		// Note - this timer check is only if there's a stuck registration for some reason - it
		// stops continual data transfer when nobody really wants it. Hopefully someone will time the
		// stuck registration out!
		if (SyntroUtils::syntroTimerExpired(SyntroClock(), multicastMap->lastLookupRefresh, MULTICAST_REFRESH_TIMEOUT)) {
			TRACE2("Too long since last incoming lookup request on %s local port %d",
					qPrintable(SyntroUtils::displayUID(&multicastMap->sourceUID)), index);
			continue;										// don't send a lookup request as nobody interested
		}
		sendLookupRequest(multicastMap);
	}
}


//----------------------------------------------------------------------------


void	MulticastManager::sendLookupRequest(MM_MMAP *multicastMap, bool rightNow)
{
	SYNTRO_SERVICE_LOOKUP *serviceLookup;
	SYNTRO_SERVICE_ACTIVATE *serviceActivate;

	// no messages to ourself
	if (SyntroUtils::compareUID(&m_myUID, &multicastMap->prevHopUID))
		return;

	qint64 now = SyntroClock();
	if (!rightNow && !SyntroUtils::syntroTimerExpired(now, multicastMap->lookupSent, SERVICE_LOOKUP_INTERVAL))
		return;											// too early to send again

	if (SyntroUtils::convertUC2ToInt(multicastMap->prevHopUID.instance) < INSTANCE_COMPONENT) {
		serviceLookup = (SYNTRO_SERVICE_LOOKUP *)malloc(sizeof(SYNTRO_SERVICE_LOOKUP));
		*serviceLookup = multicastMap->serviceLookup;
		TRACE2("Sending lookup request for %s from port %d", serviceLookup->servicePath, SyntroUtils::convertUC2ToUInt(serviceLookup->localPort));

		if (!m_server->sendSyntroMessage(&(multicastMap->prevHopUID), SYNTROMSG_SERVICE_LOOKUP_REQUEST,
				(SYNTRO_MESSAGE *)serviceLookup, sizeof(SYNTRO_SERVICE_LOOKUP), SYNTROLINK_MEDHIGHPRI)) {
			logWarn(QString("Failed sending lookup request to %1").arg(SyntroUtils::displayUID(&multicastMap->prevHopUID)));
		}
	}
	else {
		TRACE2("Sending service activate for %s from port %d", 
				multicastMap->serviceLookup.servicePath, SyntroUtils::convertUC2ToUInt(multicastMap->serviceLookup.localPort));
		serviceActivate = (SYNTRO_SERVICE_ACTIVATE *)malloc(sizeof(SYNTRO_SERVICE_ACTIVATE));
		SyntroUtils::copyUC2(serviceActivate->endpointPort, multicastMap->serviceLookup.remotePort);
		SyntroUtils::copyUC2(serviceActivate->componentIndex, multicastMap->serviceLookup.componentIndex);
		SyntroUtils::copyUC2(serviceActivate->syntroControlPort, multicastMap->serviceLookup.localPort);
		serviceActivate->response = multicastMap->serviceLookup.response;
		m_server->sendSyntroMessage(&(multicastMap->prevHopUID), 
				SYNTROMSG_SERVICE_ACTIVATE, (SYNTRO_MESSAGE *)serviceActivate, sizeof(SYNTRO_SERVICE_ACTIVATE), SYNTROLINK_MEDHIGHPRI);
	}
	multicastMap->lookupSent = now;
}



