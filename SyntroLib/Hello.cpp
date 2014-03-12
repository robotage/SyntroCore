//
//  Copyright (c) 2014 Scott Ellis and Richard Barnett
//	
//  This file is part of SyntroLib
//
//  SyntroLib is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  SyntroLib is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with SyntroLib.  If not, see <http://www.gnu.org/licenses/>.
//

#include "Hello.h"
#include "SyntroUtils.h"
#include "SyntroSocket.h"
#include "SyntroClock.h"

// Hello

Hello::Hello(SyntroComponentData *data, const QString& logTag) : SyntroThread(QString("HelloThread"), logTag)
{
	int	i;
	HELLOENTRY	*helloEntry;

	m_componentData = data;
	m_logTag = logTag;
	helloEntry = m_helloArray;
	for (i = 0; i < SYNTRO_MAX_CONNECTEDCOMPONENTS; i++, helloEntry++)
		helloEntry->inUse = false;
	m_parentThread = NULL;				// default to do not send messages to parent thread
	m_helloSocket = NULL;
	m_socketFlags = 0;					// set to 1 for reuse socket port
}

Hello::~Hello()
{
	m_parentThread = NULL;
	killTimer(m_timer);
	delete m_helloSocket;
	m_helloSocket = NULL;
}

void Hello::initThread()
{
	// behavior changes if Control rather than any other component
	m_control = strcmp(m_componentData->getMyComponentType(), COMPTYPE_CONTROL) == 0;

	if (m_componentData->getMyHelloSocket() != NULL) {
		m_helloSocket = m_componentData->getMyHelloSocket();
	} else {
		m_helloSocket = new SyntroSocket(m_logTag);
		if (m_helloSocket->sockCreate(SOCKET_HELLO + m_componentData->getMyInstance(), SOCK_DGRAM, m_socketFlags) == 0)
		{
			logError(QString("Failed to open socket on instance %1").arg(m_componentData->getMyInstance()));
			return;
		}
	}
	m_helloSocket->sockSetThread(this);
	m_helloSocket->sockSetReceiveMsg(HELLO_ONRECEIVE_MESSAGE);
	m_helloSocket->sockEnableBroadcast(1);

	m_timer = startTimer(HELLO_INTERVAL);
	m_controlTimer = SyntroClock();
}


//	getHelloEntry - returns an entry as a formatted string
//
//	Returns NULL if entry is not in use.
//	Not very re-entrant!

char *Hello::getHelloEntry(int index)
{
	static char buf[200];
	HELLOENTRY	*helloEntry;

	if ((index < 0) || (index >= SYNTRO_MAX_CONNECTEDCOMPONENTS)) {
		logWarn(QString("getHelloEntry with out of range index %1").arg(index));
		return NULL;
	}

	QMutexLocker locker(&m_lock);

	helloEntry = m_helloArray + index;
	if (!helloEntry->inUse)
		return NULL;

	sprintf(buf, "%-20s %-20s %d.%d.%d.%d",
			helloEntry->hello.componentType,
			qPrintable(SyntroUtils::displayUID(&helloEntry->hello.componentUID)),
			helloEntry->hello.IPAddr[0], helloEntry->hello.IPAddr[1], helloEntry->hello.IPAddr[2], 
				helloEntry->hello.IPAddr[3]);

	return buf;
}

//	CopyHelloEntry - returns an copy of a Hello entry
//

void Hello::copyHelloEntry(int index, HELLOENTRY *dest)
{
	if ((index < 0) || (index >= SYNTRO_MAX_CONNECTEDCOMPONENTS)) {
		logWarn(QString("copyHelloEntry with out of range index %1").arg(index));
		return;
	}

	m_lock.lock();
	*dest = m_helloArray[index];
	m_lock.unlock();
}


bool Hello::findComponent(HELLOENTRY *foundHelloEntry, SYNTRO_UID *UID)
{
	int i;
	HELLOENTRY *helloEntry;

	QMutexLocker locker(&m_lock);

	helloEntry = m_helloArray;
	for (i = 0; i < SYNTRO_MAX_CONNECTEDCOMPONENTS; i++, helloEntry++) {
		if (!helloEntry->inUse)
			continue;
		if (!SyntroUtils::compareUID(&(helloEntry->hello.componentUID), UID))
			continue;

// found it!!
		memcpy(foundHelloEntry, helloEntry, sizeof(HELLOENTRY));
		return true;
	}
	return false;							// not found
}

bool Hello::findComponent(HELLOENTRY *foundHelloEntry, char *appName, char *componentType)
{
	int i;
	HELLOENTRY *helloEntry;

	QMutexLocker locker(&m_lock);

	helloEntry = m_helloArray;
	for (i = 0; i < SYNTRO_MAX_CONNECTEDCOMPONENTS; i++, helloEntry++) {
		if (!helloEntry->inUse)
			continue;
		if (strlen(appName) > 0) {
			if (strcmp(appName, (char *)(helloEntry->hello.appName)) != 0)
				continue;							// node names don't match
		}
		if (strlen(componentType) > 0) {
			if (strcmp(componentType, (char *)(helloEntry->hello.componentType)) != 0)
				continue;							// node names don't match
		}

// found it!!
		memcpy(foundHelloEntry, helloEntry, sizeof(HELLOENTRY));
		return true;
	}
	return false;											// not found
}


bool Hello::findBestControl(HELLOENTRY *foundHelloEntry)
{
	int i;
	int highestPriority = 0;
	int bestControl = -1;
	HELLOENTRY *helloEntry = NULL;

	QMutexLocker locker(&m_lock);

	helloEntry = m_helloArray;
	for (i = 0; i < SYNTRO_MAX_CONNECTEDCOMPONENTS; i++, helloEntry++) {
		if (!helloEntry->inUse)
			continue;
		if (strcmp(COMPTYPE_CONTROL, (char *)(helloEntry->hello.componentType)) != 0)
			continue;										// not a SyntroControl
		
		if ((helloEntry->hello.priority > highestPriority) || (bestControl == -1)) {
			highestPriority = helloEntry->hello.priority;
			bestControl = i;
		}
	}

	if (bestControl == -1)
		return false;										// no SyntroControls found

// found it!!
	memcpy(foundHelloEntry, m_helloArray + bestControl, sizeof(HELLOENTRY));
	return true;
}


void Hello::processHello()
{
	int	i, free, instance;
	HELLOENTRY	*helloEntry;

	if ((m_RXHello.helloSync[0] != HELLO_SYNC0) || (m_RXHello.helloSync[1] != HELLO_SYNC1) || 
			(m_RXHello.helloSync[2] != HELLO_SYNC2) || (m_RXHello.helloSync[3] != HELLO_SYNC3)) {
		logWarn(QString("Received invalid Hello sync"));
		return;												// invalid sync
	}

	// check it's from correct subnet - must be the same

	if (!SyntroUtils::isInMySubnet(m_RXHello.IPAddr))
		return;												// just ignore if not from my subnet

	m_RXHello.appName[SYNTRO_MAX_APPNAME-1] = 0;			// make sure zero terminated
	m_RXHello.componentType[SYNTRO_MAX_COMPTYPE-1] = 0;		// make sure zero terminated
	instance = SyntroUtils::convertUC2ToInt(m_RXHello.componentUID.instance);	// get the instance from where the hello came
	free = -1;

	QMutexLocker locker(&m_lock);

	// Only put the entry in the table if the hello came from a SyntroControl

	if (instance == INSTANCE_CONTROL) {												
		// See if we already have this in the table

		helloEntry = m_helloArray;
		for (i = 0; i < SYNTRO_MAX_CONNECTEDCOMPONENTS; i++, helloEntry++) {
			if (!helloEntry->inUse) {
				if (free == -1)
					free = i;								// first free entry - might need later
				continue;									// unused entry
			}
			if (matchHello(&(helloEntry->hello), &m_RXHello)) {
				helloEntry->lastHello = SyntroClock();		// up date last received time
				memcpy(&(helloEntry->hello), &m_RXHello, sizeof(HELLO)); // copy information just in case
				sprintf(helloEntry->IPAddr, "%d.%d.%d.%d",
				m_RXHello.IPAddr[0], m_RXHello.IPAddr[1], m_RXHello.IPAddr[2], m_RXHello.IPAddr[3]);
				return;
			}
		}
		if (i == SYNTRO_MAX_CONNECTEDCOMPONENTS){
			if (free == -1) {								// too many components!
				logError(QString("Too many components in Hello table %1").arg(i));
				return;
			}
			helloEntry = m_helloArray+free;					// setup new entry
			helloEntry->inUse = true;
			helloEntry->lastHello = SyntroClock();			// up date last received time
			memcpy(&(helloEntry->hello), &m_RXHello, sizeof(HELLO));
			sprintf(helloEntry->IPAddr, "%d.%d.%d.%d",
			m_RXHello.IPAddr[0], m_RXHello.IPAddr[1], m_RXHello.IPAddr[2], m_RXHello.IPAddr[3]);
			TRACE2("Hello added %s, %s", qPrintable(SyntroUtils::displayUID(&helloEntry->hello.componentUID)), 
				helloEntry->hello.appName);
			emit helloDisplayEvent(this);
		}

		HELLOENTRY	*messageHelloEntry;
		if (m_parentThread != NULL) {
			messageHelloEntry = (HELLOENTRY *)malloc(sizeof(HELLOENTRY));
			memcpy(messageHelloEntry, helloEntry, sizeof(HELLOENTRY));
			m_parentThread->postThreadMessage(HELLO_STATUS_CHANGE_MESSAGE, HELLO_UP, messageHelloEntry);
		}
	} else {
		HELLO	*hello;
		if ((m_parentThread != NULL) && m_control)
		{
			hello = (HELLO *)malloc(sizeof(HELLO));
			memcpy(hello, &m_RXHello, sizeof(HELLO));
			m_parentThread->postThreadMessage(HELLO_STATUS_CHANGE_MESSAGE, HELLO_BEACON, hello);
		}
	}
}

bool Hello::matchHello(HELLO *a, HELLO *b)
{
	return SyntroUtils::compareUID(&(a->componentUID), &(b->componentUID));
}


void Hello::processTimers()
{
	int	i;
	HELLOENTRY *helloEntry;
	qint64 now = SyntroClock();

	if (m_control && SyntroUtils::syntroTimerExpired(now, m_controlTimer, HELLO_CONTROL_HELLO_INTERVAL)) {
		m_controlTimer = now;
		sendHelloBeacon();
	}

	helloEntry = m_helloArray;
	for (i = 0; i < SYNTRO_MAX_CONNECTEDCOMPONENTS; i++, helloEntry++) {
		if (!helloEntry->inUse)
			continue;								// not in use

		if (SyntroUtils::syntroTimerExpired(now, helloEntry->lastHello, HELLO_LIFETIME)) {	// entry has timed out

			HELLOENTRY	*messageHelloEntry;

			if (m_parentThread != NULL) {
				messageHelloEntry = (HELLOENTRY *)malloc(sizeof(HELLOENTRY));
				memcpy(messageHelloEntry, helloEntry, sizeof(HELLOENTRY));
				m_parentThread->postThreadMessage(HELLO_STATUS_CHANGE_MESSAGE, HELLO_DOWN, messageHelloEntry);
			}
//			TRACE1("Hello deleted %s", helloEntry->IPAddr);
			helloEntry->inUse = false;
			emit helloDisplayEvent(this);
		}
	}
}

void Hello::sendHelloBeacon()
{
	if (m_helloSocket == NULL)
		return;												// called too soon?
    HELLO hello = m_componentData->getMyHeartbeat().hello;
    m_helloSocket->sockSendTo(&(hello), sizeof(HELLO), SOCKET_HELLO + INSTANCE_CONTROL, NULL);
}

void Hello::sendHelloBeaconResponse(HELLO *reqHello)
{
	int instance;

    HELLO hello = m_componentData->getMyHeartbeat().hello;

    instance = SyntroUtils::convertUC2ToInt(reqHello->componentUID.instance);
    m_helloSocket->sockSendTo(&(hello), sizeof(HELLO), SOCKET_HELLO + instance, NULL);
}

void Hello::deleteEntry(HELLOENTRY *helloEntry)
{
	helloEntry->inUse = false;
	emit helloDisplayEvent(this);
}

// Hello message handlers

bool Hello::processMessage(SyntroThreadMsg* msg)
{
	switch(msg->message) {
		case HELLO_ONRECEIVE_MESSAGE:
			while (m_helloSocket->sockPendingDatagramSize() != -1) {
				m_helloSocket->sockReceiveFrom(&m_RXHello, sizeof(HELLO), m_IpAddr, &m_port);
				processHello();
			}
			return true;
	}
	return false;
}

void Hello::timerEvent(QTimerEvent *)
{
	processTimers();
}
		
