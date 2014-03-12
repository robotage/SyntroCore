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

#ifndef MULTICASTMANAGER_H
#define MULTICASTMANAGER_H

#include "SyntroLib.h"

#define	SYNTROSERVER_MAX_MMAPS		100000					// max simultaneous multicast registrations 

#define	MM_REFRESH_INTERVAL		(SYNTRO_CLOCKS_PER_SEC * 5)	// multicast refresh interval

//	MM_REGISTEREDCOMPONENT is used to record who has requested multicast data 

typedef struct _REGISTEREDCOMPONENT
{
	SYNTRO_UID registeredUID;								// the registered UID
	int port;												// the registered port to send data to
	unsigned char sendSeq;									// the next send sequence number
	unsigned char lastAckSeq;								// last received ack sequence number
	qint64 lastSendTime;									// in order to timeout the WFAck condition
	struct _REGISTEREDCOMPONENT	*next;						// so they can be linked together
} MM_REGISTEREDCOMPONENT;

//	MM_MMAP records info about a multicast service

typedef struct
{
	bool valid;												// true if the entry is in use
	int index;												// index of entry
	SYNTRO_UID sourceUID;									// the original UID (i.e. where message came from)
	SYNTRO_UID prevHopUID;									// previous hop UID (which may be different if via tunnel(s))
	MM_REGISTEREDCOMPONENT *head;							// head of the registered component list
	SYNTRO_SERVICE_LOOKUP serviceLookup;					// the lookup structure
	bool registered;										// true if successfully registered for a service
	qint64 lookupSent;										// time last lookup was sent
	qint64 lastLookupRefresh;							// last time a subscriber refreshed its lookup
} MM_MMAP;

class	SyntroServer;

class MulticastManager : public QObject
{
	Q_OBJECT
public:

//	Construction

	MulticastManager(void);
	~MulticastManager(void);
	SyntroServer *m_server;								// this must be set up by SyntroServer

//	MMShutdown - must be called before deleting object

	void MMShutdown();

//	MMAllocateMap allocates a map for a new service - the name of service and port are the params.

	MM_MMAP	*MMAllocateMMap(SYNTRO_UID *previousHopUID, SYNTRO_UID *sourceUID, const char *componentName, const char *serviceName, int port);

//	MMFreeMap deallocates a map when the service no longer exists

	void MMFreeMMap(MM_MMAP *pM);					// frees a multicast map entry

//	MMAddRegistered adds a new registration for a service

	bool MMAddRegistered(MM_MMAP *multicastMap, SYNTRO_UID *UID, int port);

//	MMCheckRegistered checks to see if an endpoint is already registered for a service

	bool MMCheckRegistered(MM_MMAP *multicastMap, SYNTRO_UID *UID, int port);

//	MMDeleteRegistered - deletes all multicast mapentries for specified UID if nPort = -1 
//	else just ones that match the nPort

	void MMDeleteRegistered(SYNTRO_UID *UID, int port);

//	MMForwardMulticastMessage forwards a message to all registered endpoints

	void MMForwardMulticastMessage(int cmd, SYNTRO_MESSAGE *message, int len);

//	MMProcessMulticastAck - handles an ack from a multicast sink

	void MMProcessMulticastAck(SYNTRO_EHEAD *ehead, int len);

//	MMProcessLookupResponse - handles lookup responses

	void MMProcessLookupResponse(SYNTRO_SERVICE_LOOKUP *serviceLookup, int len);

//	MMBackground - must be called once per second

	void MMBackground();

//	Access to m_pMMap should only be made while locked

	MM_MMAP	m_multicastMap[SYNTROSERVER_MAX_MMAPS];				// the multicast map array
	int m_multicastMapSize;										// size of the array actually used
	QMutex m_lock;
	SYNTRO_UID m_myUID;

signals:
	void MMDisplay();
	void MMNewEntry(int index);
	void MMDeleteEntry(int index);
	void MMRegistrationChanged(int index);

protected:
	void sendLookupRequest(MM_MMAP *multicastMap, bool rightNow = false);	// sends a multicast service lookup request
	qint64 m_lastBackground;						// keeps track of interval between backgrounds

	QString m_logTag;
};
#endif // MULTICASTMANAGER_H
