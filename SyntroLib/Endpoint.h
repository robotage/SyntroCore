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

#ifndef	_ENDPOINT_H_
#define	_ENDPOINT_H_

#include "syntrolib_global.h"
#include "SyntroUtils.h"
#include "Hello.h"
#include "SyntroLink.h"
#include "SyntroCFSDefs.h"
#include "SyntroComponentData.h"

#define	ENDPOINT_STATE_MAX		256							// max bytes in state message (including trailing zero)

#define	ENDPOINT_BACKGROUND_INTERVAL	(SYNTRO_CLOCKS_PER_SEC)		// this is the polling interval
#define	ENDPOINT_DE_INTERVAL			(SYNTRO_CLOCKS_PER_SEC * 10)// background DE update interval
#define	ENDPOINT_CONNWAIT				(1 * SYNTRO_CLOCKS_PER_SEC)	// interval between connection/beacon attempts
#define ENDPOINT_MULTICAST_TIMEOUT		(10 * SYNTRO_CLOCKS_PER_SEC)// 10 second timeout for unacked multicast send
#define ENDPOINT_REVERSION_BEACON_INTERVAL	(20 * SYNTRO_CLOCKS_PER_SEC)// 20 second interval between reversion beacon requests

#define ENDPOINT_MAX_SYNTROCONTROLS	3						// max number of SyntroControls in priority list


//-------------------------------------------------------------------------------------------
//	Service structure defs

//	SYNTRO_SERVICE_INFO is used to maintain information about a local and remote services

typedef struct
{
	bool inUse;												// true if this service slot is in use
	bool enabled;											// true if the service is operating
	bool local;												// true if this is a local service, false if remote
	int serviceType;										// service type code
	int serviceData;										// the int value that the appClient can set
	void *serviceDataPointer;								// the pointer that the appClient can set

	bool removingService;									// true if disabling due to service removal
	SYNTRO_SERVPATH	servicePath;							// the path of the service
	int destPort;											// destination port for outgoing messages
	qint64 tLastLookup;										// time of last lookup (for timeout purposes)
	qint64 tLastLookupResponse;								// time of last lookup response (for timeout purposes)
	int closingRetries;										// number of times a close has been retried
	int	state;												// state of the service
	SYNTRO_SERVICE_LOOKUP serviceLookup;					// the lookup structure

	int lastReceivedSeqNo;									// sequence number on last received multicast message
	unsigned char nextSendSeqNo;							// the number to use on the next sent multicast message
	unsigned char lastReceivedAck;							// the last ack received
	qint64 lastSendTime;									// time the last multicast frame was sent
} SYNTRO_SERVICE_INFO;

//	local service state defs

enum SYNTRO_LOCAL_SERVICE_STATE
{
	SYNTRO_LOCAL_SERVICE_STATE_INACTIVE = 0,				// no subscribers
	SYNTRO_LOCAL_SERVICE_STATE_ACTIVE						// one or more subscribers
};

//	remote service state defs

enum SYNTRO_REMOTE_SERVICE_STATE
{
	SYNTRO_REMOTE_SERVICE_STATE_NOTINUSE = 0,				// indicates remote service port is not in use
	SYNTRO_REMOTE_SERVICE_STATE_LOOK,						// requests a lookup on a service
	SYNTRO_REMOTE_SERVICE_STATE_LOOKING,					// outstanding lookup
	SYNTRO_REMOTE_SERVICE_STATE_REGISTERED,					// successfully registered
	SYNTRO_REMOTE_SERVICE_STATE_REMOVE,						// request to remove a remote service registration
	SYNTRO_REMOTE_SERVICE_STATE_REMOVING					// remove request has been sent
};


//	SYNTRO_CFS_FILE is used to maintain state about an open SyntroCFS file

typedef struct
{
	bool inUse;												// true if being used
	int clientHandle;										// the index of this entry
	int storeHandle;										// the file handle returned by the file open
	bool structured;										// if in structured mode
	bool openInProgress;									// true if an open request is in progress
	qint64 openReqTime;										// when the open request was sent
	bool open;												// true if file open
	bool readInProgress;									// true if a read has been issued
	bool writeInProgress;									// true if a write has been issued
	bool queryInProgress;
	bool cancelQueryInProgress;
	bool fetchQueryInProgress;
	qint64 readReqTime;										// when the read request was sent
	qint64 writeReqTime;									// when the write request was sent
	qint64 queryReqTime;
	qint64 cancelQueryReqTime;
	qint64 fetchQueryReqTime;
	bool closeInProgress;									// true if a close has been issued
	qint64 closeReqTime;									// when the close request was sent
	qint64 lastKeepAliveSent;								// the time the last keep alive was sent
	qint64 lastKeepAliveReceived;							// the time the last keep alive was received
} SYNTRO_CFS_FILE;


//	SYNTROCFS_CLIENT_EPINFO is used to maintain SyntroCFS info for an endpoint

typedef struct
{
	bool inUse;												// true if in use
	bool dirInProgress;										// true if a directory request is in progress
	qint64 dirReqTime;										// when the last dir request was sent
	SYNTRO_CFS_FILE	cfsFile[SYNTROCFS_MAX_CLIENT_FILES];	// the stream info
} SYNTROCFS_CLIENT_EPINFO;

//-------------------------------------------------------------------------------------------
//	The CEndpoint class itself
//

class SYNTROLIB_EXPORT Endpoint : public SyntroThread
{

public:
	Endpoint(qint64 backgroundInterval, const char *compType);
	virtual ~Endpoint();

//	This routine returns a QString with the status of the SyntroLink

	QString getLinkState();

//	This functions return a pointer to the Hello object

	Hello *getHelloObject();

//	requestDirectory sends a directory request to SyntroControl

	void requestDirectory();

//	setHeartbeatTimers allows control over the default heartbeat system parameters

	void setHeartbeatTimers(int interval, int timeout);

protected:

	SYNTRO_UID m_UID;										// a convenient local copy of this component's UID
	void timerEvent(QTimerEvent *event);


	//----------------------------------------------------------
//	
//	Client level services

//	clientLoadServices loads services from the client settings area.
//	They will all be enabled or disabled on creation based on the enabled flag supplied.
//	Returns true if it went ok, *serviceCOunt is the number of services loaded, *serviceStart
//	is the servicePort of the first service loaded.

	bool clientLoadServices(bool enabled, int *serviceCount, int *serviceStart);

//	clientAddService adds a service to the Endpoint's service list. When enabled, for a local service (local = true)
//	Endpoint will then process the service as a local multicast producer or E2E endpoint depending
//	on the serviceType setting (SERVICETYPE_MULTICAST or SERVICETYPE_E2E). For a remote service
//	(local = false), for multicast Endpoint will try to lookup and register for the service stream.
//	For E2E, Endpoint will try to obtain the UID and port number of the service.
//
//	The function returns -1 if there was an error. Otherwise, it is the service port allocated
//	to this service.
//	
//	The initial service enable state is set by the enabled flag on the call.

	int clientAddService(QString servicePath, int serviceType, bool local, bool enabled = true);

//	clientSetServiceData allows an integer value to be set in the service entry

	bool clientSetServiceData(int servicePort, int value);

//	clientGetServiceData retrieves the integer previously set in a service entry
//	Note that there is no easy way on indicating an error except by returning -1
//	so ideally -1 should not be a valid value!

	int	 clientGetServiceData(int servicePort);

//	clientSetServiceDataPointer allows a pointer to be set in the service entry

	bool clientSetServiceDataPointer(int servicePort, void *value);

//	clientGetServiceDataPointer retrieves the pointer previously set in a service entry
//	Note that there is no easy way on indicating an error except by returning NULL.

	void *clientGetServiceDataPointer(int servicePort);

//	clientEnableService activates a previously stopped service. Returns false if error.

	bool clientEnableService(int servicePort);

//	clientDisableService deactivates a previously enabled service. Return false if error.

	bool clientDisableService(int servicePort);

//	clientIsServiceEnabled returns enabled state

	bool clientIsServiceEnabled(int servicePort);

//	remove a service from the array. Returns true if ok, false if error.

	bool clientRemoveService(int servicePort);

//	clientIsServiceActive returns true if the service is in a data transfer state. For local multicast,
//	this means there are subscribers to the service. For remote multicast, this means that this
//	port is registered for the requested service. For remote E2E, this means that the lookup for the
//	destination service has succeeded.

	bool clientIsServiceActive(int servicePort);

//	clientGetServicePath returns the path of the service

	QString clientGetServicePath(int servicePort);

//	clientGetServiceType returns the type of the service

	int clientGetServiceType(int servicePort);

//	clientIsServiceLocal return true if local, false if remote

	bool clientIsServiceLocal(int servicePort);

//	clientGetRemoteServiceState returns the current state of the remote service. This is the more general
//	version of RemoteServiceAvailable, which only determines if it is registered or not.
//	The returned value is one of SYNTRO_REMOTE_SERVICE_STATE.

	int clientGetRemoteServiceState(int servicePort);

//	clientGetRemoteServiceUID returns a pointer to the remote service's UID. Returns NULL if not in valid state

	SYNTRO_UID *clientGetRemoteServiceUID(int servicePort);

//	clientGetServiceDestPort returns the port to which messages should be sent. Returns -1 if not in valid state

	int clientGetServiceDestPort(int servicePort);

//	clientGetLastSendTime returns the time at which a message was last sent

	qint64 clientGetLastSendTime(int servicePort);

//	clientIsConnected returns true if the SyntroLink is up.

	bool clientIsConnected();

//	these functions are called by the app client to build and send messages

	bool clientClearToSend(int servicePort);				// returns true if can send on a local multicast service
	SYNTRO_EHEAD *clientBuildMessage(int servicePort, int length); // for multicast and remote E2E services
	SYNTRO_EHEAD *clientBuildLocalE2EMessage(int servicePort, 
						SYNTRO_UID *destUID, int destPort, int length); // for local E2E services

	bool clientSendMessage(int servicePort, 
								SYNTRO_EHEAD *message, 
								int length,
								int priority = SYNTROLINK_LOWPRI);
	bool clientSendMulticastAck(int servicePort);			// acks a received multicast message

//----------------------------------------------------------
//	Client app overrides
//
//	these functions can be overriden by the app client

//	appClientInit is called just before CEndpoint starts its timer and runs normally
//	the client can use this for final initialization as the service array has been loaded.

	virtual void appClientInit() {}

//	appClientExit is called when the app is exiting.

	virtual void appClientExit() {}

//	appClientReceiveMulticast is called to allow the app client to process a multicast message
//	length is the length of data after the Syntro_EHEAD. The message must be free()ed when
//	no longer needed.

	virtual void appClientReceiveMulticast(int servicePort, SYNTRO_EHEAD *message, int length);

//	appClientReceiveMulticastAck is called to allow the app client to process a multicast ack message
//	length is the length of data after the Syntro_EHEAD. The message must be free()ed when
//	no longer needed.

	virtual void appClientReceiveMulticastAck(int servicePort, SYNTRO_EHEAD *message, int length);	

//	appClientReceiveE2E is called to allow the app client to process an end to end message
//	length is the length of data after the Syntro_EHEAD. The message must be free()ed when
//	no longer needed.

	virtual void appClientReceiveE2E(int servicePort, SYNTRO_EHEAD *message, int length);	

//	appClientConnected is called when the SyntroLink has been established

	virtual void appClientConnected() { return; }						

//	appClientClosed is callled if the SyntroLink has been closed.

	virtual void appClientClosed() {return; }							

//	appClientHeartbeat is called when a heartbeat is received

	virtual void appClientHeartbeat(SYNTRO_HEARTBEAT *heartbeat, int length);

//	appClientReceiveDirectory is able to process a directory response

	virtual void appClientReceiveDirectory(QStringList directory);

//	appClientBackground is called every background interval timer tick and
//	can be used for any background processing that may be necessary

	virtual void appClientBackground() { return; }							

//	appClientProcessThreadMessage can be used by the app client to receive
//	SyntroThread inter-thread messages that have been posted by another thread.
//	Return true if processed message in the appClient.

	virtual bool appClientProcessThreadMessage(SyntroThreadMsg *) { return false; }


//-------------------------------------------------------------------------------------------
//	SyntroCFS API definitions
//

//	CFSAddEP adds an endpoint number to be trapped for SyntroCFS processing

	void CFSAddEP(int serviceEP);							// adds an endpoint index for SyntroCFS processing

//	CFSAddEP adds an endpoint number to be trapped for SyntroCFS processing

	void CFSDeleteEP(int serviceEP);						// deletes an endpoint index for SyntroCFS processing

//	CFSDir is called to obtain a list of files available from a specific SyntroCFS store
//	Note that only one request can be outstanding for each SyntroCFS EP. The request will be timed out if no response
//	is seen in a defined time. In other words, after a call to CFSDir, the client must wait until
//	CFSDirResponse is called before trying again - a call to CFSDirResponse is guaranteed in all
//	circumstances unless the call returns false, indicating that the request could not be issued.
//  The cfsDirParam allows the client to pass parameters to the SyntroCFS service providing the
//  directory. Currently used to request filtering of the returned directory list. The default is
//  no filtering.

	bool CFSDir(int serviceEP, int cfsDirParam = SYNTROCFS_DIR_PARAM_LIST_ALL);

//	CFSDirReponse is called when a response is obtained or the request fails
//	responseCode contains the SyntroCFS response code, filePaths will contain the paths of the returned files

	virtual void CFSDirResponse(int serviceEP, unsigned int responseCode, QStringList filePaths); 

//	CFSOpenDB is a convenience function that calls CFSOpen with
//  cfsType = SYNTROCFS_TYPE_DATABASE

	int CFSOpenDB(int serviceEP, QString databaseName);

//	CFSOpenStructuredFile is a convenience function that calls CFSOpen with
//  cfsType = SYNTROCFS_TYPE_STRUCTURED_FILE 

	int CFSOpenStructuredFile(int serviceEP, QString filePath);

//	CFSOpenRawFile is a convenience function that calls CFSOpen with
//  cfsType = SYNTROCFS_TYPE_RAW_FILE 

	int CFSOpenRawFile(int serviceEP, QString filePath, int blockSize = 128);

//	CFSOpenResponse is called when a response is obtained or the open request fails in some way
//	responseCode contains the response code which can be success or error. If success, the file
//	is open for file operations.
//	If successful, handle contains the local handle to be used for future operations on the open file.
//	If unsuccessful, the handle return is the handle that was returned by the call to CFSOpen. However,
//	the slot has been returned and so the handle is no longer available for use.
//	fileLength is the length of the file in records or blocks.

	virtual void CFSOpenResponse(int serviceEP, unsigned int responseCode, int handle, unsigned int fileLength);

//	CFSClose is called when it's time to close a file and release the local handle.
//	If there's a local error, CFSClose returns false and there will be no call to CFSCloseResponse.
//	If it return true, the request has been issued and a subsequent call to CFSCloseResponse is
//	guaranteed in all cases.

	bool CFSClose(int serviceEP, int handle);

//	CFSCloseResponse is called when a close operation has completed.
//	No matter what the responseCode, the handle is no longer available and the file
//	can be considered closed.

	virtual void CFSCloseResponse(int serviceEP, unsigned int responseCode, int handle);

//	CFSKeepAliveTimeout is called when an active stream is timed out due to a lack of
//	keep alive responses. The slot is closed and this call informs the client that the file
//	is no longer available.

	virtual void CFSKeepAliveTimeout(int serviceEP, int handle);

//	CFSReadAtIndex is called to read the record or block(s) at the specified index
//	It will return true if the read was issued or else return false if
//	the read could not be issued.

	bool CFSReadAtIndex(int serviceEP, int handle, unsigned int index, int blockCount = 1);

//	CFSReadAtIndexResponse is called when a CFSReadAtIndex completes or else returns an error
//	fileData is a pointer to returned data (if the responseCode is SYNTROCFS_SUCCESS) and the
//	client must free this memory when it no longer needs it.

	virtual void CFSReadAtIndexResponse(int serviceEP, int handle, unsigned int index, 
		unsigned int responseCode, unsigned char *fileData, int length);

//	CFSWriteAtIndex is called to write the record or block(s) at the appropriate place in the file
//	It will return true if the write was issued or else return false if
//	the write could not be issued. The length field is the total length of the data pointed to by
//	fileData. In structured mode, this is the length of the structured record. In block mode,
//	this is the number of bytes to be written (it does not need to be a multiple of the block
//	size).
//	
//	If index is 0, the file length is set to 0 before the write. If index is non zero, the data is appended.

	bool CFSWriteAtIndex(int serviceEP, int handle, unsigned int index, unsigned char *fileData, int length);

//	CFSWriteAtIndexResponse is called when a CFSWriteAtIndex completes or else returns an error.
//	The index same value as in the request.

	virtual void CFSWriteAtIndexResponse(int serviceEP, int handle, unsigned int index, unsigned int responseCode);


	bool CFSQuery(int serviceEP, int handle, QString sql);
	virtual void CFSQueryResponse(int serviceEP, int handle, unsigned int responseCode);

	bool CFSCancelQuery(int serviceEP, int handle);
	virtual void CFSCancelQueryResponse(int serviceEP, int handle, unsigned int responseCode);

	bool CFSFetchQuery(int serviceEP, int handle, int maxRows, int resultType = SYNTROCFS_QUERY_RESULT_TYPE_ROW_DATA);
	virtual void CFSFetchQueryResponse(int serviceEP, int handle, unsigned int responseCode, int firstRow, int lastRow, int param1, int param2, int length, unsigned char *data);

	QString m_logTag;										// to to be prepended to any log messages

//-------------------------------------------------------------------------------------------

private:
	//	CFSOpen is called to open a file from a SyntroCFS store.
	//	The function returns the local handle allocated to the file.
	//	If -1 is returned, this means that too many files are open already.
	int CFSOpen(int serviceEP, QString filePath, int cfsMode, int blockSize);

	SyntroComponentData m_componentData;					// the component data for this component

	QString m_compType;										// type of the component
	
	Hello *m_hello;											// Hello task (only used in local mode)

	bool m_sentDE;											// if a DE has been sent yet on this connection

	int m_timer;											// background timer

	qint64 m_background;									// used to keep track of background polling
	qint64 m_DETimer;										// used to send DEs
	SYNTRO_SERVICE_INFO m_serviceInfo[SYNTRO_MAX_SERVICESPERCOMPONENT];	// my service array
	QMutex m_serviceLock;									// to control access to the service array

	char m_IPAddr[SYNTRO_IPSTR_LEN];						// the IP address string for the target SyntroControl
	int m_port;												// the port to use for the connection
	char m_controlName[ENDPOINT_MAX_SYNTROCONTROLS][SYNTRO_MAX_APPNAME];	// app names for the target SyntroControls
	HELLOENTRY m_helloEntry;								// the HelloEntry we are using
    bool m_encryptLink;                                     // true if encryption (SSL) in use for links

	// heartbeat system defs

	qint64 m_lastHeartbeatSent;								// time last heartbeat sent
	qint64 m_heartbeatSendInterval;							// interval between sent timeouts
	qint64 m_heartbeatTimeoutPeriod;						// period of missed received heartbeats before timeout
	qint64 m_lastHeartbeatReceived;							// when the last heartbeat was received

	qint64 m_lastReversionBeacon;							// when the last reversion beacon was requested
	
	bool m_connected;										// true if connection active
	bool m_connectInProgress;								// true if in middle of connecting the SyntroLink
	bool m_beaconDelay;										// if waiting for beacon to take effect
	bool m_gotHeartbeat;									// true if at least one heartbeat received
	SyntroSocket *m_sock;
	SyntroLink	*m_syntroLink;
	QMutex m_RXLock;										// receive data processing lock
	qint64 m_connWait;										// timer between connection attempts
	QString m_state;										// the state message
	bool m_controlRevert;									// true if revert when higher pri SyntroControl found in pri mode
	bool m_priorityMode;									// true if in priority mode
	int m_controlIndex;										// index in SyntroControl list if not in priority mode

	int m_configHeartbeatInterval;							// the configured heartbeat interval in seconds
	int m_configHeartbeatTimeout;							// the number of intervals before a timeout

	void initThread();
	bool processMessage(SyntroThreadMsg *msg);
	void finishThread();
	void updateState(QString msg);							// sets the new state message
	bool syntroConnect();
	void syntroClose();
	void processReceivedData();
	void processReceivedDataDemux(int cmd, int len, SYNTRO_MESSAGE*syntroMessage);
	void serviceInit();										// init processing for services
	void endpointBackground();
	bool endpointSocketMessage(SyntroThreadMsg *msg);
	void serviceBackground();								// background processing for services
	void sendRemoteServiceLookup(SYNTRO_SERVICE_INFO *remoteService); // send a lookup request message
	void processServiceActivate(SYNTRO_SERVICE_ACTIVATE *serviceActivate);// handles a service activate request
	void processLookupResponse(SYNTRO_SERVICE_LOOKUP *serviceLookup);// handles the response to a service lookup
	void processDirectoryResponse(SYNTRO_DIRECTORY_RESPONSE *directoryResponse, int len);

	void processMulticast(SYNTRO_EHEAD *ehead, int len, int destPort); // process a multicast message
	void processMulticastAck(SYNTRO_EHEAD *ehead, int len, int destPort);// process a multicast ack message
	void processE2E(SYNTRO_EHEAD *ehead, int len, int destPort); // process an end to end message message
	void endpointConnected();								// called when connection is made
	void endpointClosed();									// called when the connection has been closed
	void endpointHeartbeat(SYNTRO_HEARTBEAT *pSH, int nLen); // called when a heartbeat is received

	qint64 m_backgroundInterval;							// the background interval to use

	void buildDE();											// build a new DE
	void forceDE();
	bool sentDE();
	void sendMulticastAck(int servicePort, int seq);		// sends back an ack to the endpoint
	void sendE2EAck(SYNTRO_EHEAD *originalEhead);			// sends an E2E ack back

	bool syntroSendMessage(int cmd, SYNTRO_MESSAGE *syntroMessage, int len, int priority); 

	void linkCloseCleanup();								// do what needs to be done when the SyntroLink goes down


//-------------------------------------------------------------------------------------------
//	SyntroCFS API variables and local functions
//
	void CFSInit();											// set up for run
	SYNTRO_EHEAD *CFSBuildRequest(int remoteServiceEP, int length);	// generate a request buffer with length bytes after CFS header
	void CFSBackground();									// the background function
	bool CFSProcessMessage(SYNTRO_EHEAD *pE2E, int nLen, int dstPort); // see if it is a SyntroCFS messages and process
	void CFSProcessDirResponse(SYNTRO_CFSHEADER *cfsHdr, int dstPort);	// process a file directory response
	void CFSProcessOpenResponse(SYNTRO_CFSHEADER *cfsHdr, int dstPort);	// process a file open response
	void CFSProcessCloseResponse(SYNTRO_CFSHEADER *cfsHdr, int dstPort);	// process a stream close response
	void CFSProcessKeepAliveResponse(SYNTRO_CFSHEADER *cfsHdr, int dstPort);	// process a keep alive response
	void CFSProcessReadAtIndexResponse(SYNTRO_CFSHEADER *cfsHdr, int dstPort);	// process a read at index response
	void CFSProcessWriteAtIndexResponse(SYNTRO_CFSHEADER *cfsHdr, int dstPort);	// process a write at index response
	void CFSProcessQueryResponse(SYNTRO_CFSHEADER *cfsHdr, int dstPort);
	void CFSProcessCancelQueryResponse(SYNTRO_CFSHEADER *cfsHdr, int dstPort);
	void CFSProcessFetchQueryResponse(SYNTRO_CFSHEADER *cfsHdr, int dstPort);

	void CFSSendKeepAlive(int remoteServiceEP, SYNTRO_CFS_FILE *scf);		// sends a keep alive on an open stream
	void CFSTimeoutKeepAlive(int remoteServiceEP, SYNTRO_CFS_FILE *scf);	// handles a keep alive timeout

	SYNTROCFS_CLIENT_EPINFO cfsEPInfo[SYNTRO_MAX_SERVICESPERCOMPONENT];	// the SyntroCFS EP info array


//-------------------------------------------------------------------------------------------
};
#endif	//_ENDPOINT_H

