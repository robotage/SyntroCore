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

#include "Endpoint.h"
#include "SyntroUtils.h"
#include "SyntroSocket.h"
#include "SyntroClock.h"
#include "SyntroRecord.h"

//#define ENDPOINT_TRACE
//#define CFS_TRACE

/*!
    \class Endpoint
    \brief Endpoint is the main interface between app clients and Syntro.
	\inmodule SyntroLib
 
    The Endpoint class implements many of the functions that a Syntro app 
	client will need to interact with the cloud. In most cases, a Syntro app has 
	a client class that inherits Endpoint. Endpoint itself inherits SyntroThread. 
	The normal sequence for use involves constructing a new client class that inherits, 
	performing any initialization that might be require on that object and then calling 
	the resumeThread() function on the object. 
*/

/*!
	\fn	virtual void Endpoint::appClientConnected()

	This function is called when the SyntroLink has been established and is ready for data transfer.
*/

/*!
	\fn	virtual void Endpoint::appClientClosed()

	This function is called when the SyntroLink is no longer available for data transfer.
*/

/*!
	\fn	virtual void Endpoint::appClientBackground()

	appClientBackground is called every time Endpoint’s background timer expires. 
	The interval is set using the Endpoint constructor. The app client can use this call for 
	any background processing that it needs to do. Since this is in the Endpoint thread, 
	there should not be any lengthy processing in this function. Instead, this function 
	should initiate a worker thread to perform the lengthy processing and pick up the 
	results at a later point rather than blocking.
*/

/*!
	\fn virtual void Endpoint::appClientExit()

	appClientExit() is called during the Ednpoint destructor execution and the client can use 
	this to perform any last minute clean up before termination.
*/

/*!
	\fn	virtual void Endpoint::appClientInit()

	appClientInit is called just before Endpoint starts its timer and exits its initialization 
	phase. The client app can use this call for final initialization as Endpoint has prepared 
	everything else for execution.
*/

/*!
	\fn	virtual bool Endpoint::appClientProcessThreadMessage(SyntroThreadMsg *msg)

	Overriding this function allows the app client to process a message (\a msg) posted to Endpoint’s thread. 
	See the SyntroThread documentation for more details.
*/


//----------------------------------------------------------
//
//	Client level functions

/*!
	Loads services from the client settings area of the settings file. 
	They will all be enabled or disabled on creation based on the \a enabled flag supplied. 
	Returns true if it the services were processed correctly, false if there was an error, 
	\a serviceCount is the number of services loaded, \a serviceStart is the service port of 
	the first service loaded. Service ports are assigned consecutively.

	The client settings area is an array in the settings file. 
	The array name is defined by SYNTRO_PARAMS_CLIENT_SERVICES in SyntroUtils.h. 
	Each service entry consists of three values as follows:

	\list
	\li		SYNTRO_PARAMS_CLIENT_SERVICE_TYPE. This is the type of the service which can be SYNTRO_PARAMS_SERVICE_TYPE_MULTICAST or SYNTRO_PARAMS_SERVICE_TYPE_E2E.
	\li		SYNTRO_PARAMS_CLIENT_SERVICE_LOCATION. This can be either SYNTRO_PARAMS_SERVICE_LOCATION_LOCAL or SYNTRO_PARAMS_SERVICE_LOCATION_REMOTE. If local, this means that the service is being provided by this component. If remote, it means that the component wishes to use a service provided by some other component.
	\li		SYNTRO_PARAMS_CLIENT_SERVICE_NAME. This is the service name to be used for the service.
	\endlist
*/

bool Endpoint::clientLoadServices(bool enabled, int *serviceCount, int *serviceStart)
{
	QSettings *settings = SyntroUtils::getSettings();
	int count = settings->beginReadArray(SYNTRO_PARAMS_CLIENT_SERVICES);

	int serviceType;
	bool local;
	bool first = true;
	int port;
	
	for (int i = 0; i < count; i++) {
		settings->setArrayIndex(i);
		local = settings->value(SYNTRO_PARAMS_CLIENT_SERVICE_LOCATION).toString() == SYNTRO_PARAMS_SERVICE_LOCATION_LOCAL;
		QString serviceTypeString = settings->value(SYNTRO_PARAMS_CLIENT_SERVICE_TYPE).toString();
		if (serviceTypeString == SYNTRO_PARAMS_SERVICE_TYPE_MULTICAST) {
			serviceType = SERVICETYPE_MULTICAST;
		} else if (serviceTypeString == SYNTRO_PARAMS_SERVICE_TYPE_E2E) {
			serviceType = SERVICETYPE_E2E;
		}
		else {
			logWarn(QString("Unrecognized type for service %1").arg(settings->value(SYNTRO_PARAMS_CLIENT_SERVICE_NAME).toString()));
			serviceType = SERVICETYPE_E2E;			// fall back to this
		}
		port = clientAddService(settings->value(SYNTRO_PARAMS_CLIENT_SERVICE_NAME).toString(), serviceType, local, enabled);
		if (first) {
			*serviceStart = port;
			first = false;
		}
	}
	settings->endArray();
	*serviceCount = count;

	delete settings;
	return true;
}

/*!
	This function adds a service to Endpoint’s service list. \a servicePath is the service 
	name for the service. \a serviceType is one of SYNTRO_PARAMS_SERVICE_TYPE_MULTICAST for a 
	multicast service or SYNTRO_PARAMS_SERVICE_TYPE_E2E for an E2E service. If \a local is true 
	then Endpoint will process the service as a multicast data producer or E2E service provider 
	and add the servicePath to the Syntro cloud directory. Otherwise, Endpoint will process the 
	service as a data consumer or service user and try to look up the servicePath in the Syntro 
	cloud directory and subscribe to the service if it is multicast. If \a enabled is true then 
	Endpoint will regard the service as active perform look ups etc as necessary. If it is false, 
	Endpoint will not process the service apart from adding it to the service table. The service 
	can be enabled later by calling clientEnableService().

	The function returns -1 if there is an error. Otherwise, the return value is the servicePort 
	that should be used for further interactions with this service.
*/

int Endpoint::clientAddService(QString servicePath, int serviceType, bool local, bool enabled)
{
	int servicePort;
	SYNTRO_SERVICE_INFO *service;

	QMutexLocker locker(&m_serviceLock);

	service = m_serviceInfo;
	for (servicePort = 0; servicePort < SYNTRO_MAX_SERVICESPERCOMPONENT; servicePort++, service++) {
		if (!service->inUse)
			break;
	}
	if (servicePort == SYNTRO_MAX_SERVICESPERCOMPONENT) {
		logError(QString("Service info array full"));
		return -1;
	}
	if (servicePath.length() > SYNTRO_MAX_SERVPATH - 1) {
		logError(QString("Service path too long (%1) %2").arg(servicePath.length()).arg(servicePath));
		return -1;
	}
	service->inUse = true;
	service->enabled = enabled;
	service->local = local;
	service->removingService = false;
	service->serviceType = serviceType;
	strcpy(service->servicePath, qPrintable(servicePath));
	service->state = SYNTRO_LOCAL_SERVICE_STATE_INACTIVE;
	service->serviceData = -1;
	service->serviceDataPointer = NULL;
	if (!local) {
		strcpy(service->serviceLookup.servicePath, qPrintable(servicePath));
		service->serviceLookup.serviceType = serviceType;
		service->state = SYNTRO_REMOTE_SERVICE_STATE_LOOK;	// flag for immediate lookup request
		service->tLastLookup = SyntroClock();
	}
	buildDE();
	forceDE();
	return servicePort;
}

/*!
	This function enables a previous disabled service referenced by \a servicePort. The functions returns false if any error occurred.
*/

bool Endpoint::clientEnableService(int servicePort)
{
	SYNTRO_SERVICE_INFO *service;

	QMutexLocker locker(&m_serviceLock);

	if ((servicePort < 0) || (servicePort >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
		logWarn(QString("Request to enable service on out of range port %1").arg(servicePort));
		return false;
	}
	service = m_serviceInfo + servicePort;
	if (!service->inUse) {
		logWarn(QString("Attempt to enable service on not in use port %1").arg(servicePort));
		return false;
	}
	if (service->removingService) {
		logWarn(QString("Attempt to enable service on port %1 that is being removed").arg(servicePort));
		return false;
	}
	service->enabled = true;
	if (service->local) {
		service->state = SYNTRO_LOCAL_SERVICE_STATE_INACTIVE;
		buildDE();
		forceDE();
		return true;
	} else {
		service->state = SYNTRO_REMOTE_SERVICE_STATE_LOOK;
		service->tLastLookup = SyntroClock();
		return true;
	}
}

/*!
	Returns true if the service referenced by \a servicePort is active, else false if not enabled or error. 
	The meaning of “active” depends on whether the service is local or remote. For a local multicast service, 
	active means that there is at least subscriber to the service. For a remote multicast or E2E service, 
	active means that the look up for the specified service path has succeeded.
*/

bool	Endpoint::clientIsServiceActive(int servicePort)
{
	SYNTRO_SERVICE_INFO *service;

	QMutexLocker locker(&m_serviceLock);

	if ((servicePort < 0) || (servicePort >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
		logWarn(QString("Tried to status service in out of range port %1").arg(servicePort));
		return false;
	}
	service = m_serviceInfo + servicePort;
	if (!service->inUse) {
		logWarn(QString("Tried to status service on not in use port %1").arg(servicePort));
		return false;
	}
	if (!service->enabled)
		return false;
	if (service->local) {
		return service->state == SYNTRO_LOCAL_SERVICE_STATE_ACTIVE;
	} else {
		return service->state == SYNTRO_REMOTE_SERVICE_STATE_REGISTERED;
	}
}

/*!
	Returns true if the service referenced by \a servicePort is enabled, else false if not enabled or error.
*/

bool	Endpoint::clientIsServiceEnabled(int servicePort)
{
	SYNTRO_SERVICE_INFO *service;

	QMutexLocker locker(&m_serviceLock);

	if ((servicePort < 0) || (servicePort >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
		logWarn(QString("Tried to get enable status service in out of range port %1").arg(servicePort));
		return false;
	}
	service = m_serviceInfo + servicePort;
	if (!service->inUse) {
		logWarn(QString("Tried to get enable status service on not in use port %1").arg(servicePort));
		return false;
	}
	if (service->local) {
		return service->enabled;
	} else {
		if (!service->enabled)
			return false;
		if (service->state == SYNTRO_REMOTE_SERVICE_STATE_REMOVE)
			return false;
		if (service->state == SYNTRO_REMOTE_SERVICE_STATE_REMOVING)
			return false;
		return true;
	}
}

/*!
	This function disables a previous enabled service referenced by \a servicePort. The functions returns false if any error occurred.
*/

bool Endpoint::clientDisableService(int servicePort)
{
	SYNTRO_SERVICE_INFO *service;

	if ((servicePort < 0) || (servicePort >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
		logWarn(QString("Request to disable service out of range port %1").arg(servicePort));
		return false;
	}
	service = m_serviceInfo + servicePort;
	if (!service->inUse) {
		logWarn(QString("Attempt to disable service on not in use port %1").arg(servicePort));
		return false;
	}
	if (service->local) {
		forceDE();
		service->enabled = false;
		return true;
	} else {
		switch (service->state) {
			case SYNTRO_REMOTE_SERVICE_STATE_REMOVE:
			case SYNTRO_REMOTE_SERVICE_STATE_REMOVING:
				logWarn(QString("Attempt to disable service on port %1 that's already being disabled").arg(servicePort));
				return false;

			case SYNTRO_REMOTE_SERVICE_STATE_LOOK:
				if (service->removingService) {
					service->inUse = false;					// being removed
					service->removingService = false;
				}
				service->enabled = false;
				return true;

			case SYNTRO_REMOTE_SERVICE_STATE_LOOKING:
			case SYNTRO_REMOTE_SERVICE_STATE_REGISTERED:
				service->state = SYNTRO_REMOTE_SERVICE_STATE_REMOVE; // indicate we want to remove whatever happens
				return true;

			default:
				logDebug(QString("Disable service on port %1 with state %2").arg(servicePort).arg(service->state));
				service->enabled = false;					// just disable then
				if (service->removingService) {
					service->inUse = false;
					service->removingService = false;
				}
				return false;
		}
	}
}

/*!
	This function removes the service referenced by \a servicePort from Endpoint’s service table. 
	The function returns false if any error occurred. servicePort is no longer valid for that service after the call.
*/

bool Endpoint::clientRemoveService(int servicePort)
{
	SYNTRO_SERVICE_INFO *service;

	QMutexLocker locker(&m_serviceLock);

	if ((servicePort < 0) || (servicePort >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
		logWarn(QString("Tried to remove service in out of range port %1").arg(servicePort));
		return false;
	}
	service = m_serviceInfo + servicePort;
	if (!service->inUse) {
		logWarn(QString("Tried to remove a service on not in use port %1").arg(servicePort));
		return false;
	}
	if (!service->enabled) {
		service->inUse = false;								// if not enabled, just mark as not in use
		return true;
	}
	if (service->local) {
		service->enabled = false;
		service->inUse = false;
		buildDE();
		forceDE();
		return true;
	} else {
		service->removingService = true;
		locker.unlock();									// deadlock if don't do this!
		return clientDisableService(servicePort);
	}
}

/*!
	Maps \a servicePort into a service path. Returns an empty string if there was an error.
*/

QString Endpoint::clientGetServicePath(int servicePort)
{
	SYNTRO_SERVICE_INFO *service;

	QMutexLocker locker(&m_serviceLock);

	if ((servicePort < 0) || (servicePort >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
		logWarn(QString("Tried to get dest port for service in out of range port %1").arg(servicePort));
		return "";
	}
	service = m_serviceInfo + servicePort;
	if (!service->enabled) {
		logWarn(QString("Tried to get dest port for service on disabled port %1").arg(servicePort));
		return "";
	}
	if (!service->inUse) {
		logWarn(QString("Tried to get dest port for service on not in use port %1").arg(servicePort));
		return "";
	}
	return QString(service->servicePath);
}

/*! 
	Returns the service type of \a servicePort. Possible values are:

	\list
	\li SERVICETYPE_MULTICAST
	\li SERVICETYPE_E2E
	\li -1 (error)
	\endlist
*/

int Endpoint::clientGetServiceType(int servicePort)
{
	SYNTRO_SERVICE_INFO *service;

	QMutexLocker locker(&m_serviceLock);

	if ((servicePort < 0) || (servicePort >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
		logWarn(QString("Tried to get dest port for service in out of range port %1").arg(servicePort));
		return -1;
	}
	service = m_serviceInfo + servicePort;
	if (!service->inUse) {
		logWarn(QString("Tried to get dest port for service on not in use port %1").arg(servicePort));
		return -1;
	}
	return service->serviceType;
}

/*!
	Returns true if the service referenced by \a servicePort is a local service, else false if not remote or error.
*/

bool Endpoint::clientIsServiceLocal(int servicePort)
{
	SYNTRO_SERVICE_INFO *service;

	QMutexLocker locker(&m_serviceLock);

	if ((servicePort < 0) || (servicePort >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
		logWarn(QString("Tried to get dest port for service in out of range port %1").arg(servicePort));
		return false;
	}
	service = m_serviceInfo + servicePort;
	if (!service->inUse) {
		logWarn(QString("Tried to get dest port for service on not in use port %1").arg(servicePort));
		return false;
	}
	return service->local;
}

/*!
	Returns destination port to be used for a message on a service which is in state 
	SYNTRO_REMOTE_SERVICE_STATE_REGISTERED. If the state of the port referenced by \a servicePort 
	is in a different state, -1 will be returned.
*/

int	Endpoint::clientGetServiceDestPort(int servicePort)
{
	SYNTRO_SERVICE_INFO *service;

	QMutexLocker locker(&m_serviceLock);

	if ((servicePort < 0) || (servicePort >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
		logWarn(QString("Tried to get dest port for service in out of range port %1").arg(servicePort));
		return -1;
	}
	service = m_serviceInfo + servicePort;
	if (!service->enabled) {
		logWarn(QString("Tried to get dest port for service on disabled port %1").arg(servicePort));
		return -1;
	}
	if (!service->inUse) {
		logWarn(QString("Tried to get dest port for service on not in use port %1").arg(servicePort));
		return -1;
	}
	if (service->local) {
		if (service->state != SYNTRO_LOCAL_SERVICE_STATE_ACTIVE) {
			logWarn(QString("Tried to get dest port for inactive service port %1").arg(servicePort));
			return -1;
		}
		return service->destPort;
	} else {
		if (service->state != SYNTRO_REMOTE_SERVICE_STATE_REGISTERED) {
			logWarn(QString("Tried to get dest port for inactive service port %1 in state %2").arg(servicePort).arg(service->state));
			return -1;
		}
		return SyntroUtils::convertUC2ToInt(service->serviceLookup.remotePort);
	}
}

/*!
	This function returns the actual state of Endpoint’s state machine for the remote service \a servicePort. Possible values are:

	\list
	\li SYNTRO_REMOTE_SERVICE_STATE_NOTINUSE. The state for a table entry that’s not in use.
	\li	SYNTRO_REMOTE_SERVICE_STATE_LOOK. Requests a lookup on the service path.
	\li	SYNTRO_REMOTE_SERVICE_STATE_LOOKING. A lookup request is outstanding.
	\li	SYNTRO_REMOTE_SERVICE_STATE_REGISTERED. Successfully registered or looked up.
	\li	SYNTRO_REMOTE_SERVICE_STATE_REMOVE. Requests removal of a remote service registration.
	\li	SYNTRO_REMOTE_SERVICE_STATE_REMOVING. A remove request is in progress.
	\endlist
*/

int Endpoint::clientGetRemoteServiceState(int servicePort)
{
	SYNTRO_SERVICE_INFO *remoteService;

	if ((servicePort < 0) || (servicePort >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
		logWarn(QString("Request for service state on out of range port %1").arg(servicePort));
		return SYNTRO_REMOTE_SERVICE_STATE_NOTINUSE;
	}
	remoteService = m_serviceInfo + servicePort;

	if (!remoteService->inUse) {
		return SYNTRO_REMOTE_SERVICE_STATE_NOTINUSE;								
	}
	if (remoteService->local) {
		return SYNTRO_REMOTE_SERVICE_STATE_NOTINUSE;								
	}
	if (!remoteService->enabled) {
		return SYNTRO_REMOTE_SERVICE_STATE_NOTINUSE;								
	}
	return remoteService->state;
}

/*!
	Returns a pointer to the UID of a remote service which is in state SYNTRO_REMOTE_SERVICE_STATE_REGISTERED. 
	If the state of the port referenced by \a servicePort is in a different state, NULL will be returned.
*/

SYNTRO_UID	*Endpoint::clientGetRemoteServiceUID(int servicePort)
{
	SYNTRO_SERVICE_INFO *remoteService;

	if ((servicePort < 0) || (servicePort >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
		logWarn(QString("GetRemoteServiceUID on out of range port %1").arg(servicePort));
		return NULL;
	}
	remoteService = m_serviceInfo + servicePort;
	if (!remoteService->inUse) {
		logWarn(QString("GetRemoteServiceUID on not in use port %1").arg(servicePort));
		return NULL;								
	}
	if (remoteService->local) {
		logWarn(QString("GetRemoteServiceUID on port %1 that is a local service port").arg(servicePort));
		return NULL;								
	}
	if (remoteService->state != SYNTRO_REMOTE_SERVICE_STATE_REGISTERED) {
		logWarn(QString("GetRemoteServiceUID on port %1 in state %2").arg(servicePort).arg(remoteService->state));
		return NULL;
	}

	return &remoteService->serviceLookup.lookupUID;
}

/*!
	Returns true if Endpoint’s SyntroLink is connected and operating normally.
*/

bool Endpoint::clientIsConnected()
{
	return m_connected;
}

/*!
	This function allows an integer user data \a value to be added to the service entry indicated by \a servicePort.
*/

bool Endpoint::clientSetServiceData(int servicePort, int value)
{
	SYNTRO_SERVICE_INFO *service;

	QMutexLocker locker(&m_serviceLock);

	if ((servicePort < 0) || (servicePort >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
		logWarn(QString("Tried to set data value for service in out of range port %1").arg(servicePort));
		return false;
	}
	service = m_serviceInfo + servicePort;
	if (!service->inUse) {
		logWarn(QString("Tried to set data value on not in use port %1").arg(servicePort));
		return false;
	}
	service->serviceData = value;
	return true;
}

/*!
	 Retrieves and returns the \a servicePort's value set by a previous clientSetServiceData() call.
*/

int Endpoint::clientGetServiceData(int servicePort)
{
	SYNTRO_SERVICE_INFO *service;

	QMutexLocker locker(&m_serviceLock);

	if ((servicePort < 0) || (servicePort >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
		logWarn(QString("Tried to get data value for service in out of range port %1").arg(servicePort));
		return -1;
	}
	service = m_serviceInfo + servicePort;
	if (!service->inUse) {
		logWarn(QString("Tried to get data value on not in use port %1").arg(servicePort));
		return -1;
	}
	return service->serviceData;
}

/*!
	Saves a user data pointer \a value to be added to the service entry indicated by \a servicePort.
*/

bool Endpoint::clientSetServiceDataPointer(int servicePort, void *value)
{
	SYNTRO_SERVICE_INFO *service;

	QMutexLocker locker(&m_serviceLock);

	if ((servicePort < 0) || (servicePort >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
		logWarn(QString("Tried to set data value for service in out of range port %1").arg(servicePort));
		return false;
	}
	service = m_serviceInfo + servicePort;
	if (!service->inUse) {
		logWarn(QString("Tried to set data value on not in use port %1").arg(servicePort));
		return false;
	}
	service->serviceDataPointer = value;
	return true;
}

/*!
	 Retrieves the \a servicePort's pointer value set by a previous clientSetServiceDataPointer() call.
*/

void *Endpoint::clientGetServiceDataPointer(int servicePort)
{
	SYNTRO_SERVICE_INFO *service;

	QMutexLocker locker(&m_serviceLock);

	if ((servicePort < 0) || (servicePort >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
		logWarn(QString("Tried to get data value for service in out of range port %1").arg(servicePort));
		return NULL;
	}
	service = m_serviceInfo + servicePort;
	if (!service->inUse) {
		logWarn(QString("Tried to get data value on not in use port %1").arg(servicePort));
		return NULL;
	}
	return service->serviceDataPointer;
}

/*!
	Returns the last time at which a multicast message was sent on the service associated
	with \a servicePort. Returns -1 if servicePort is not in an appropriate state.
*/

qint64 Endpoint::clientGetLastSendTime(int servicePort)
{
	SYNTRO_SERVICE_INFO *service;

	QMutexLocker locker(&m_serviceLock);

	if ((servicePort < 0) || (servicePort >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
		logWarn(QString("Tried to get data value for service in out of range port %1").arg(servicePort));
		return -1;
	}
	service = m_serviceInfo + servicePort;
	if (!service->inUse) {
		logWarn(QString("Tried to get data value on not in use port %1").arg(servicePort));
		return -1;
	}
	if (!service->enabled) {
		logWarn(QString("Tried to get data value on disabled port %1").arg(servicePort));
		return -1;
	}
	return service->lastSendTime;
}


//----------------------------------------------------------
//
//	send side functions that the app client can call

/*!
	Returns true if the service referenced by \a servicePort is a local multicast service and 
	the acknowledge window is open. This function should always be called before sending 
	multicast data on a local service.
*/

bool Endpoint::clientClearToSend(int servicePort)
{
	SYNTRO_SERVICE_INFO *service;

	QMutexLocker locker(&m_serviceLock);

	if ((servicePort < 0) || (servicePort >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
		logWarn(QString("clientClearToSend with illegal port %1").arg(servicePort));
		return false;
	}

	service = m_serviceInfo + servicePort;
	if (!service->enabled) {
		logWarn(QString("Tried to get clear to send on disabled port %1").arg(servicePort));
		return false;
	}
	if (!service->inUse) {
		logWarn(QString("Tried to get clear to send on not in use port %1").arg(servicePort));
		return false;
	}

	// within the send/ack window ?
	if (SyntroUtils::isSendOK(service->nextSendSeqNo, service->lastReceivedAck)) {
		return true;
	}

	// if we haven't timed out, wait some more
	if (!SyntroUtils::syntroTimerExpired(SyntroClock(),service->lastSendTime, ENDPOINT_MULTICAST_TIMEOUT)) {
		return false;
	}

	// we timed out, reset our sequence numbers
	service->lastReceivedAck = service->nextSendSeqNo;
	return true;
}

/*!
	This functions builds a malloced SYNTRO_EHEAD structure with \a length bytes of data space, 
	filled in with header data for the service referenced by \a servicePort and returns a pointer to it. 
	The caller is responsible for making sure that it is freed at some point. The function will return 
	NULL if there is any problem with the call (which can include the service not being available for 
	message transmission). This is appropriate for multicast and remote E2E services.
*/

SYNTRO_EHEAD *Endpoint::clientBuildMessage(int servicePort, int length)
{
	SYNTRO_SERVICE_INFO *service;

	QMutexLocker locker(&m_serviceLock);
	SYNTRO_EHEAD *message;

	if ((servicePort < 0) || (servicePort >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
		logWarn(QString("clientBuildMessage with illegal port %1").arg(servicePort));
		return NULL;
	}

	service = m_serviceInfo + servicePort;
	if (!service->enabled) {
		logWarn(QString("clientBuildMessage on disabled port %1").arg(servicePort));
		return NULL;
	}
	if (!service->inUse) {
		logWarn(QString("clientBuildMessage on not in use port %1").arg(servicePort));
		return NULL;
	}

	locker.unlock();
	if (service->serviceType == SERVICETYPE_MULTICAST) {
		message = SyntroUtils::createEHEAD(&(m_UID), 
					servicePort, 
					&(m_UID), 
					clientGetServiceDestPort(servicePort), 
					0, 
					length);		
	} else {
		if (service->local) {
			logWarn(QString("clientBuildMessage on local service port %1").arg(servicePort));
			return NULL;
		}

		message = SyntroUtils::createEHEAD(&(m_UID), 
					servicePort, 
					clientGetRemoteServiceUID(servicePort),										 
					clientGetServiceDestPort(servicePort),										 
					0, 
					length);		
	}
	return message;
}

/*!
	This functions builds a malloced SYNTRO_EHEAD structure with \a length bytes of data space, 
	filled in with header data for the service referenced by servicePort and returns a pointer 
	to it. The caller is responsible for making sure that it is freed at some point. 
	The function will return NULL if there is any problem with the call (which can include the 
	service not being available for message transmission). This is appropriate for local E2E services.
*/

SYNTRO_EHEAD *Endpoint::clientBuildLocalE2EMessage(int clientPort, SYNTRO_UID *destUID, int destPort, int length)
{
	SYNTRO_SERVICE_INFO *service;

	QMutexLocker locker(&m_serviceLock);
	SYNTRO_EHEAD *message;

	if ((clientPort < 0) || (clientPort >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
		logWarn(QString("clientBuildLocalE2EMessage with illegal port %1").arg(clientPort));
		return NULL;
	}

	service = m_serviceInfo + clientPort;
	if (!service->enabled) {
		logWarn(QString("clientBuildLocalE2EMessage on disabled port %1").arg(clientPort));
		return NULL;
	}
	if (!service->inUse) {
		logWarn(QString("clientBuildLocalE2EMessage on not in use port %1").arg(clientPort));
		return NULL;
	}
	if (!service->local) {
		logWarn(QString("clientBuildLocalE2EMessage on remote service port %1").arg(clientPort));
		return NULL;
	}
	if (service->serviceType != SERVICETYPE_E2E) {
		logWarn(QString("clientBuildLocalE2EMessage on port %1 that's not E2E").arg(clientPort));
		return NULL;
	}

	message = SyntroUtils::createEHEAD(&(m_UID), 
					clientPort, 
					destUID, 
					destPort, 
					0, 
					length);		
	
	return message;
}

/*!
	clientSendMessage() is used to send a message with length total bytes after the SYNTRO_EHEAD header on an active service referenced by servicePort with priority priority. Valid priority levels in increasing level of importance are:

	\list
	\li SYNTROLINK_LOWPRI
	\li SYNTROLINK_MEDPRI
	\li SYNTROLINK_MEDHIGHPRI
	\li SYNTROLINK_HIGHPRI
	\endlist

	The function returns true if the message was sent successfully, false it it was not. 
	In both cases freeing the memory originally malloced for message is handled by this function.
*/

bool Endpoint::clientSendMessage(int servicePort, SYNTRO_EHEAD *message, int length, int priority)
{
	SYNTRO_SERVICE_INFO *service;

	if (!message || length < 1) {
		logWarn(QString("clientSendMessage called with invalid parameters"));
		return false;
	}

	QMutexLocker locker(&m_serviceLock);

	if ((servicePort < 0) || (servicePort >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
		logWarn(QString("clientSendMessage with illegal port %1").arg(servicePort));
		free(message);
		return false;
	}

	service = m_serviceInfo + servicePort;
	if (!service->enabled) {
		logWarn(QString("clientSendMessage on disabled port %1").arg(servicePort));
		free(message);
		return false;
	}
	if (!service->inUse) {
		logWarn(QString("clientSendMessage on not in use port %1").arg(servicePort));
		free(message);
		return false;
	}

	if (service->serviceType == SERVICETYPE_MULTICAST) {
		if (!service->local) {
			logWarn(QString("Tried to send multicast message on remote service port %1").arg(servicePort));
			free(message);
			return false;
		}
		if (service->state != SYNTRO_LOCAL_SERVICE_STATE_ACTIVE) {
			logWarn(QString("Tried to send multicast message on inactive port %1").arg(servicePort));
			free(message);
			return false;
		}
		message->seq = service->nextSendSeqNo++;
		syntroSendMessage(SYNTROMSG_MULTICAST_MESSAGE, (SYNTRO_MESSAGE *)message, sizeof(SYNTRO_EHEAD) + length, priority);
	} else {
		if (!service->local && (service->state != SYNTRO_REMOTE_SERVICE_STATE_REGISTERED)) {
			logWarn(QString("Tried to send E2E message on remote service without successful lookup on port %1").arg(servicePort));
			free(message);
			return false;
		}
		syntroSendMessage(SYNTROMSG_E2E, (SYNTRO_MESSAGE *)message, sizeof(SYNTRO_EHEAD) + length, priority);
	}
	service->lastSendTime = SyntroClock();
	return true;
}

/*!
	sends a multicast acknowledge to the remote multicast service referenced by servicePort. 
	The acknowledgement number will be the sequence number of the last message received on this 
	service plus one (i.e. it completely opens up the acknowledgement window again). The function 
	returns true if successful, false if an error occurred.
*/

bool Endpoint::clientSendMulticastAck(int servicePort)
{
	SYNTRO_SERVICE_INFO *service;

	QMutexLocker locker(&m_serviceLock);

	if ((servicePort < 0) || (servicePort >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
		logWarn(QString("clientSendMulticastAck with illegal port %1").arg(servicePort));
		return false;
	}

	service = m_serviceInfo + servicePort;

	if (!service->enabled) {
		logWarn(QString("clientSendMulticastAck on disabled port %1").arg(servicePort));
		return false;
	}

	if (!service->inUse) {
		logWarn(QString("clientSendMulticastAck on not in use port %1").arg(servicePort));
		return false;
	}

	if (service->local) {
		logWarn(QString("clientSendMulticastAck on local service port %1").arg(servicePort));
		return false;
	}

	if (service->serviceType != SERVICETYPE_MULTICAST) {
		logWarn(QString("clientSendMulticastAck on service port %1 that isn't multicast").arg(servicePort));
		return false;
	}

	sendMulticastAck(servicePort, service->lastReceivedSeqNo + 1);

	return true;
}


//----------------------------------------------------------
//
//	Default functions for appClient overrides

/*!
	This function is called whenever a heartbeat is received on its SyntroLink. The heartbeat message itself 
	is in heartbeat, length is the number of bytes in the message. heartbeat must be freed in the overriding function.
*/

void Endpoint::appClientHeartbeat(SYNTRO_HEARTBEAT *heartbeat, int)
{
	free(heartbeat);
}

/*!
	Overriding this function allows the app client to process data messages received on multicast services. 
	servicePort contains the port number of the service to which this belongs, the data is in message and the 
	length parameter is the number of bytes after the SYNTRO_EHEAD in the message. message must be freed in 
	the overriding function.
*/

void Endpoint::appClientReceiveMulticast(int servicePort, SYNTRO_EHEAD *message, int)
{
	logWarn(QString("Unexpected multicast reported by Endpoint on port %1").arg(servicePort));
	free(message);
}

/*!
	Overriding this function allows the app client to process acknowledge messages received on multicast services. 
	servicePort contains the port number of the service to which this belongs, the data is in message and the 
	length parameter is the number of bytes after the SYNTRO_EHEAD in the message. message must be freed in 
	the overriding function.
*/

void Endpoint::appClientReceiveMulticastAck(int, SYNTRO_EHEAD *message, int)
{
	free(message);
}

/*!
	Overriding this function allows the app client to process data messages received on E2E services. 
	servicePort contains the port number of the service to which this belongs, the data is in message 
	and the length parameter is the number of bytes after the SYNTRO_EHEAD in the message. message must 
	be freed in the overriding function.
*/

void Endpoint::appClientReceiveE2E(int servicePort, SYNTRO_EHEAD *message, int)
{
	logWarn(QString("Unexpected E2E reported by Endpoint on port %1").arg(servicePort));
	free(message);
}

/*!
	Overrriding this functions allows the app client to process the current directory for the Syntro cloud.
	message must be freed in the overriding function.
*/

void Endpoint::appClientReceiveDirectory(QStringList)
{
	logWarn(QString("Unexpected directory response reported by Endpoint"));
}


//----------------------------------------------------------

/*!
	Constructs a new Endpoint. Typically this would be called with parent set to this. \a parent is the
	parent object, \a settings is a 
	pointer to the QSettings object that was generated when the component’s settings file was read in. 
	\a backgroundInterval is the period in millisconds between executions of Endpoint’s background functions. 
	Typical values for this are between 10 and 100. Setting value too high may limit the total achievable 
	data throughput on the underlying SyntroLink. In addition, setting the value beyond 1000 is not 
	recommended a it may cause internal timers and timeouts to be processed incorrectly.
*/

Endpoint::Endpoint(qint64 backgroundInterval, const char *compType)
						: SyntroThread(QString("Endpoint"), QString(compType))
{

	m_background = SyntroClock();
	m_compType = compType;
	m_DETimer = m_background;
	m_sentDE = false;
	m_connected = false;
	m_connectInProgress = false;
	m_beaconDelay = false;
	m_backgroundInterval = backgroundInterval;
	m_logTag = compType;

	QSettings *settings = SyntroUtils::getSettings();

	m_configHeartbeatInterval = settings->value(SYNTRO_PARAMS_HBINTERVAL, SYNTRO_HEARTBEAT_INTERVAL).toInt();
	m_configHeartbeatTimeout = settings->value(SYNTRO_PARAMS_HBTIMEOUT, SYNTRO_HEARTBEAT_TIMEOUT).toInt();

	delete settings;
}

/*!
	\internal
*/

Endpoint::~Endpoint(void)
{

}

void Endpoint::finishThread()
{
	killTimer(m_timer);

	appClientExit();

	syntroClose();

	m_hello->exitThread();
}

/*!
	\internal
*/

void Endpoint::initThread()
{

	m_state = "...";
	m_sock = NULL;
	m_syntroLink = NULL;
	m_hello = NULL;

	QSettings *settings = SyntroUtils::getSettings();

	m_componentData.init(qPrintable(m_compType), m_configHeartbeatInterval);
	m_UID = m_componentData.getMyUID();

	serviceInit();
	CFSInit();
	m_controlRevert = settings->value(SYNTRO_PARAMS_CONTROLREVERT).toBool();
	m_encryptLink = settings->value(SYNTRO_PARAMS_ENCRYPT_LINK).toBool();

	m_heartbeatSendInterval = m_configHeartbeatInterval * SYNTRO_CLOCKS_PER_SEC;
	m_heartbeatTimeoutPeriod = m_heartbeatSendInterval * m_configHeartbeatTimeout;
	
	int size = settings->beginReadArray(SYNTRO_PARAMS_CONTROL_NAMES);

	for (int control = 0; control < ENDPOINT_MAX_SYNTROCONTROLS; control++)
		m_controlName[control][0] = 0;

	for (int control = 0; (control < ENDPOINT_MAX_SYNTROCONTROLS) && (control < size); control++) {
		settings->setArrayIndex(control);
		strcpy(m_controlName[control], qPrintable(settings->value(SYNTRO_PARAMS_CONTROL_NAME).toString()));
	}

	settings->endArray();

	m_hello = new Hello(&m_componentData, m_logTag);
	m_hello->resumeThread();
	m_componentData.getMyHelloSocket()->moveToThread(m_hello->thread());

	m_connWait = SyntroClock();
	appClientInit();

	m_timer = startTimer(m_backgroundInterval);

	delete settings;
}


/*!
	This function can be called after the Endpoint-dreived class has been created but before resumeThread()
	has been called. It allows the heartbeat system rate to be controlled. \a interval is the interval between heartbeats in seconds.
	\a timout is the number of intervals without a heartbeat response being seen before the Syntro link is timed
	out.
*/

void Endpoint::setHeartbeatTimers(int interval, int timeout)
{
	m_configHeartbeatInterval = interval;
	m_configHeartbeatTimeout = timeout;
}

/*!
	\internal
*/

void Endpoint::timerEvent(QTimerEvent *)
{
	endpointBackground();
	appClientBackground();
}

/*!
	\internal
*/

bool Endpoint::processMessage(SyntroThreadMsg* msg)
{
	if (appClientProcessThreadMessage(msg))
			return true;

	return endpointSocketMessage(msg);
}

void Endpoint::updateState(QString msg)
{
	TRACE0(qPrintable(msg));
	m_state = msg;
}

/*!
	getLinkState() returns a displayable string that describes the current state of the SyntroLink. 
	It is commonly displayed at the bottom left of components in GUI mode and in response to a status 
	request in console mode.
*/

QString Endpoint::getLinkState()
{
	return m_state;
}

/*!
	\internal
*/


void Endpoint::forceDE()
{
	m_lastHeartbeatSent = SyntroClock() - m_heartbeatSendInterval;
	m_DETimer = SyntroClock() - ENDPOINT_DE_INTERVAL;
}

/*!
	\internal
*/


void Endpoint::endpointBackground()
{
	const char *DE;
	SYNTRO_HEARTBEAT *heartbeat;		
	int len;
	HELLOENTRY helloEntry;
	int control;

	if ((!m_connectInProgress) && (!m_connected))
		syntroConnect();
	if (!m_connected)
		return;
	if (m_syntroLink == NULL)
		// in process of shutting down the connection
		return;										
	m_syntroLink->tryReceiving(m_sock);
	processReceivedData();
	m_syntroLink->trySending(m_sock);

	qint64 now = SyntroClock();

//	Do heartbeat and DE background processing

	if (SyntroUtils::syntroTimerExpired(now, m_lastHeartbeatSent, m_heartbeatSendInterval)) {
		if (SyntroUtils::syntroTimerExpired(now, m_DETimer, ENDPOINT_DE_INTERVAL)) {	// time to send a DE
			m_DETimer = now;
			DE = m_componentData.getMyDE();						// get a copy of the DE
			len = (int)strlen(DE)+1;
			heartbeat = (SYNTRO_HEARTBEAT *)malloc(sizeof(SYNTRO_HEARTBEAT) + len);
			*heartbeat = m_componentData.getMyHeartbeat();
			memcpy(heartbeat+1, DE, len);
			syntroSendMessage(SYNTROMSG_HEARTBEAT, (SYNTRO_MESSAGE *)heartbeat, sizeof(SYNTRO_HEARTBEAT) + len, SYNTROLINK_MEDHIGHPRI);
		} else {									// just the heartbeat
			heartbeat = (SYNTRO_HEARTBEAT *)malloc(sizeof(SYNTRO_HEARTBEAT));
			*heartbeat = m_componentData.getMyHeartbeat();
			syntroSendMessage(SYNTROMSG_HEARTBEAT, (SYNTRO_MESSAGE *)heartbeat, sizeof(SYNTRO_HEARTBEAT), SYNTROLINK_MEDHIGHPRI);
		}
		m_lastHeartbeatSent = now;
	}

	if (SyntroUtils::syntroTimerExpired(now, m_lastHeartbeatReceived, m_heartbeatTimeoutPeriod)) {	
		// channel has timed out
		logWarn(QString("SyntroLink timeout"));
		syntroClose();
		endpointClosed();
		updateState("Connection has timed out");
	}

	CFSBackground();

	if (!SyntroUtils::syntroTimerExpired(now, m_background, ENDPOINT_BACKGROUND_INTERVAL))
		return;									// not time yet

	m_background = now;

	serviceBackground();

	if (m_controlRevert) {
		// send revert beacon if necessary

		if (SyntroUtils::syntroTimerExpired(now, m_lastReversionBeacon, ENDPOINT_REVERSION_BEACON_INTERVAL)) {
			m_hello->sendHelloBeacon();
			m_lastReversionBeacon = now;
		}

		for (control = 0; control < m_controlIndex; control++) {
            if (m_hello->findComponent(&helloEntry, m_controlName[control], (char *)COMPTYPE_CONTROL)) {
				// there is a new and better SyntroControl
				logInfo(QString("SyntroControl list reversion"));
				syntroClose();
				endpointClosed();
				updateState("Higher priority SyntroControl available");
				break;
			}
		}

		if ((control == m_controlIndex) && m_priorityMode) {
			if (m_hello->findBestControl(&helloEntry)) {
				if (m_helloEntry.hello.priority < helloEntry.hello.priority) {
					// there is a new and better SyntroControl
					logInfo(QString("SyntroControl priority reversion"));
					syntroClose();
					endpointClosed();
					updateState("Higher priority SyntroControl available");
				}
			}
		} 
	}
}

/*!
	\internal
*/

bool Endpoint::endpointSocketMessage(SyntroThreadMsg *msg)
{
	switch(msg->message) {
		case ENDPOINT_ONCONNECT_MESSAGE:
			m_connected = true;
			m_connectInProgress = false;
			m_gotHeartbeat = false;
			m_lastHeartbeatReceived = m_lastHeartbeatSent = m_lastReversionBeacon = SyntroClock();
			logInfo(QString("SyntroLink connected"));
			forceDE();
			endpointConnected();
			updateState(QString("Connected to %1 in %2 mode %3").arg(m_helloEntry.hello.appName)
					.arg(m_priorityMode ? "priority" : "list").arg(m_encryptLink ? "using SSL" : "unencrypted"));
			return true;

		case ENDPOINT_ONCLOSE_MESSAGE:
			if (m_connected)
				updateState("SyntroControl connection closed");
			m_connected = false;
			m_connectInProgress = false;
			m_beaconDelay = false;
			m_connWait = SyntroClock();
			delete m_sock;
			m_sock = NULL;
			delete m_syntroLink;
			m_syntroLink = NULL;
			linkCloseCleanup();
			endpointClosed();
			return true;

		case ENDPOINT_ONRECEIVE_MESSAGE:
			if (m_sock != NULL) {
#ifdef ENDPOINT_TRACE
				TRACE0("Endpoint data received");
#endif
				m_syntroLink->tryReceiving(m_sock);
				processReceivedData();
				return true;
			}
			TRACE0("Onreceived but didn't process");
			return true;

		case ENDPOINT_ONSEND_MESSAGE:
			if (m_sock != NULL) {
				m_syntroLink->trySending(m_sock);
				return true;
			}
			return true;
	}

	return false;
}

/*!
	\internal
*/

void Endpoint::processReceivedData()
{
	int	cmd;
	int	len;
	SYNTRO_MESSAGE *syntroMessage;
	int	priority;

	QMutexLocker locker(&m_RXLock);

	if (m_syntroLink == NULL)
		return;
	if (m_sock == NULL)
		return;

	for (priority = SYNTROLINK_HIGHPRI; priority <= SYNTROLINK_LOWPRI; priority++) {
		while(m_syntroLink->receive(priority, &cmd, &len, &syntroMessage)) {
			processReceivedDataDemux(cmd, len, syntroMessage);
		}
	}
}

/*!
	\internal
*/

void Endpoint::processReceivedDataDemux(int cmd, int len, SYNTRO_MESSAGE*syntroMessage)
{
	SYNTRO_HEARTBEAT *heartbeat;
	SYNTRO_EHEAD *ehead;
	int destPort;
	qint64 now = SyntroClock();

#ifdef ENDPOINT_TRACE
	TRACE2("Processing received data %d %d", cmd, len);
#endif
	switch (cmd) {
		case SYNTROMSG_HEARTBEAT:						// Heartbeat received
			if (len < (int)sizeof(SYNTRO_HEARTBEAT)) {
				TRACE1("Incorrect length heartbeat received %d", len);
				free(syntroMessage);
				break;
			}
			heartbeat = (SYNTRO_HEARTBEAT *)syntroMessage;
			m_gotHeartbeat = true;
			m_lastHeartbeatReceived = now;
			endpointHeartbeat(heartbeat, len);
			break;

		case SYNTROMSG_MULTICAST_MESSAGE:
			if (len < (int)sizeof(SYNTRO_EHEAD)) {
				logWarn(QString("Multicast size error %1").arg(len));
				free(syntroMessage);
				break;
			}

			len -= sizeof(SYNTRO_EHEAD);
			ehead = (SYNTRO_EHEAD *)syntroMessage;
			destPort = SyntroUtils::convertUC2ToInt(ehead->destPort);

			if ((destPort < 0) || (destPort >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
				logWarn(QString("Multicast message with out of range port number %1").arg(destPort));
				free(syntroMessage);
				break;
			}

			if (len < (int)sizeof(SYNTRO_RECORD_HEADER)) {
				logWarn(QString("Invalid record, too short - length %1 on port %2").arg(len).arg(destPort));
				free(syntroMessage);
				break;
			}

			if (len > SYNTRO_MESSAGE_MAX) {
				logWarn(QString("Record too long - length %1 on port %2").arg(len).arg(destPort));
				free(syntroMessage);
				break;
			}

			processMulticast(ehead, len, destPort);
			break;

		case SYNTROMSG_MULTICAST_ACK:
			if (len < (int)sizeof(SYNTRO_EHEAD)) {
				logWarn(QString("Multicast ack size error %1").arg(len));
				free(syntroMessage);
				break;
			}

			len -= sizeof(SYNTRO_EHEAD);
			ehead = (SYNTRO_EHEAD *)syntroMessage;
			destPort = SyntroUtils::convertUC2ToInt(ehead->destPort);

			if ((destPort < 0) || (destPort >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
				logWarn(QString("Multicast ack message with out of range port number %1").arg(destPort));
				free(syntroMessage);
				break;
			}

			processMulticastAck(ehead, len, destPort);
			break;

		case SYNTROMSG_SERVICE_ACTIVATE:
			if (len != (int)sizeof(SYNTRO_SERVICE_ACTIVATE)) {
				logWarn(QString("Service activate size error %1").arg(len));
				free(syntroMessage);
				break;
			}
			processServiceActivate((SYNTRO_SERVICE_ACTIVATE *)(syntroMessage));
			free(syntroMessage);
			break;

		case SYNTROMSG_SERVICE_LOOKUP_RESPONSE:
			if (len != (int)sizeof(SYNTRO_SERVICE_LOOKUP)) {
				logWarn(QString("Service lookup size error %1").arg(len));
				free(syntroMessage);
				break;
			}
			processLookupResponse((SYNTRO_SERVICE_LOOKUP *)(syntroMessage));
			free(syntroMessage);
			break;

		case SYNTROMSG_DIRECTORY_RESPONSE:
			processDirectoryResponse((SYNTRO_DIRECTORY_RESPONSE *)syntroMessage, len);
			free(syntroMessage);
			break;

		case SYNTROMSG_E2E:
			if (len < (int)sizeof(SYNTRO_EHEAD)) {
				logWarn(QString("E2E size error %1").arg(len));
				free(syntroMessage);
				break;
			}
			len -= sizeof(SYNTRO_EHEAD);
			ehead = (SYNTRO_EHEAD *)syntroMessage;
			destPort = SyntroUtils::convertUC2ToInt(ehead->destPort);

			if ((destPort < 0) || (destPort >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
				logWarn(QString("E2E message with out of range port number %1").arg(destPort));
				free(syntroMessage);
				break;
			}

			if (len > SYNTRO_MESSAGE_MAX) {
				logWarn(QString("E2E message too long - length %1 on port %2").arg(len).arg(destPort));
				free(syntroMessage);
				break;
			}
			if (!CFSProcessMessage(ehead, len, destPort))
				processE2E(ehead, len, destPort);
			break;

		default:
			TRACE1("Unexpected message %d", cmd);
			free(syntroMessage);
			break;
	}
}

/*!
	\internal
*/


void Endpoint::sendMulticastAck(int servicePort, int seq)
{
	SYNTRO_EHEAD	*ehead;
    SYNTRO_UID uid;

    uid = m_componentData.getMyHeartbeat().hello.componentUID;

	ehead = (SYNTRO_EHEAD *)malloc(sizeof(SYNTRO_EHEAD));
	SyntroUtils::copyUC2(ehead->destPort, m_serviceInfo[servicePort].serviceLookup.remotePort);
	SyntroUtils::copyUC2(ehead->sourcePort, m_serviceInfo[servicePort].serviceLookup.localPort);
	memcpy(&(ehead->destUID), &(m_serviceInfo[servicePort].serviceLookup.lookupUID), sizeof(SYNTRO_UID));
    memcpy(&(ehead->sourceUID), &uid, sizeof(SYNTRO_UID));
	ehead->seq = seq;
	syntroSendMessage(SYNTROMSG_MULTICAST_ACK, (SYNTRO_MESSAGE *)ehead, sizeof(SYNTRO_EHEAD), SYNTROLINK_HIGHPRI);
}

/*!
	\internal
*/

void Endpoint::sendE2EAck(SYNTRO_EHEAD *originalEhead)
{
	SYNTRO_EHEAD *ehead;

	ehead = (SYNTRO_EHEAD *)malloc(sizeof(SYNTRO_EHEAD));
	SyntroUtils::copyUC2(ehead->sourcePort, originalEhead->destPort);
	SyntroUtils::copyUC2(ehead->destPort, originalEhead->sourcePort);
	memcpy(&(ehead->sourceUID), &(originalEhead->destUID), sizeof(SYNTRO_UID));
	memcpy(&(ehead->destUID), &(originalEhead->sourceUID), sizeof(SYNTRO_UID));
	ehead->seq = originalEhead->seq;
	syntroSendMessage(SYNTROMSG_E2E, (SYNTRO_MESSAGE *)ehead, sizeof(SYNTRO_EHEAD), SYNTROLINK_HIGHPRI);
}


/*!
	Requests a directory from the connected SyntroControl. When the directory is received, 
	appClientReceiveDirectory() will be called.
*/


void Endpoint::requestDirectory()
{
	SYNTRO_MESSAGE	*message;

	message = (SYNTRO_MESSAGE *)malloc(sizeof(SYNTRO_MESSAGE));
	syntroSendMessage(SYNTROMSG_DIRECTORY_REQUEST, message, sizeof(SYNTRO_MESSAGE), SYNTROLINK_LOWPRI);
}

/*!
	\internal
*/

void Endpoint::serviceBackground()
{
	SYNTRO_SERVICE_INFO *service;
	int servicePort;
	qint64 now;

	QMutexLocker locker(&m_serviceLock);
	now = SyntroClock();
	service = m_serviceInfo;
	for (servicePort = 0; servicePort < SYNTRO_MAX_SERVICESPERCOMPONENT; servicePort++, service++) {
		if (!service->inUse)
			continue;										// not being used
		if (!service->enabled)
			continue;
		if (service->local) {								// local service background
			if (service->state != SYNTRO_LOCAL_SERVICE_STATE_ACTIVE)
				continue;									// no current activation anyway

			if (SyntroUtils::syntroTimerExpired(now, service->tLastLookup, SERVICE_REFRESH_TIMEOUT)) {
				service->state = SYNTRO_LOCAL_SERVICE_STATE_INACTIVE;	// indicate inactive and no data should be sent
				TRACE2("Timed out activation on local service %s port %d", service->servicePath, servicePort);
			}
		} else {											// remote service background
			switch (service->state) {
				case SYNTRO_REMOTE_SERVICE_STATE_LOOK:			// need to start looking up
					service->serviceLookup.response = SERVICE_LOOKUP_FAIL; // indicate we know nothing
					sendRemoteServiceLookup(service);
					service->state = SYNTRO_REMOTE_SERVICE_STATE_LOOKING;
					break;

				case SYNTRO_REMOTE_SERVICE_STATE_LOOKING:
					if (SyntroUtils::syntroTimerExpired(now, service->tLastLookup, SERVICE_LOOKUP_INTERVAL))
						sendRemoteServiceLookup(service);	// try again
					break;

				case SYNTRO_REMOTE_SERVICE_STATE_REGISTERED:
					if (SyntroUtils::syntroTimerExpired(now, service->tLastLookupResponse, SERVICE_REFRESH_TIMEOUT)) {
						logWarn(QString("Refresh timeout on service %1 port %2")
							.arg(service->serviceLookup.servicePath).arg(servicePort));
						service->state = SYNTRO_REMOTE_SERVICE_STATE_LOOK;	// go back to looking
						break;
					}
					if (SyntroUtils::syntroTimerExpired(now, service->tLastLookup, SERVICE_REFRESH_INTERVAL))
						sendRemoteServiceLookup(service);	// do a refresh
					break;

				case SYNTRO_REMOTE_SERVICE_STATE_REMOVE:
					service->serviceLookup.response = SERVICE_LOOKUP_REMOVE; // request the remove
					sendRemoteServiceLookup(service);
					service->state = SYNTRO_REMOTE_SERVICE_STATE_REMOVING;
					service->closingRetries = 0;
					break;

				case SYNTRO_REMOTE_SERVICE_STATE_REMOVING:
					if (SyntroUtils::syntroTimerExpired(now, service->tLastLookup, SERVICE_REMOVING_INTERVAL)) {
						if (++service->closingRetries == SERVICE_REMOVING_MAX_RETRIES) {
							logWarn(QString("Timed out attempt to remove remote registration for service %1 on port %2")
								.arg(service->serviceLookup.servicePath).arg(servicePort));
							if (service->removingService) {
								service->inUse = false;
								service->removingService = false;
							}
							service->enabled = false;
							break;
						}
						sendRemoteServiceLookup(service);
						break;
					}
						break;

				default:
					logWarn(QString("Illegal state %1 on remote service port %2").arg(service->state).arg(servicePort));
					service->state = SYNTRO_REMOTE_SERVICE_STATE_LOOK;	// go back to looking
					break;
			}
		}
	}
}


//----------------------------------------------------------
//
//	Service processing

/*!
	\internal
*/

void Endpoint::serviceInit()
{
	SYNTRO_SERVICE_INFO	*service = m_serviceInfo;

	for (int i = 0; i < SYNTRO_MAX_SERVICESPERCOMPONENT; i++, service++) {
		service->inUse = false;
		service->local = true;
		service->enabled = false;
		service->state = SYNTRO_LOCAL_SERVICE_STATE_INACTIVE;
		service->serviceLookup.response = SERVICE_LOOKUP_FAIL;// indicate lookup response not valid
		service->serviceLookup.servicePath[0] = 0;			 // no service path
		service->serviceData = -1;
		service->serviceDataPointer = NULL;
		SyntroUtils::convertIntToUC2(i, service->serviceLookup.localPort); // this is my local port index

		service->lastReceivedSeqNo = -1;
		service->nextSendSeqNo = 0;
		service->lastReceivedAck = 0;
		service->lastSendTime = 0;
	}	
}

/*!
	\internal
*/

void Endpoint::processServiceActivate(SYNTRO_SERVICE_ACTIVATE *serviceActivate)
{
	int servicePort;
	SYNTRO_SERVICE_INFO	*serviceInfo;

	servicePort = SyntroUtils::convertUC2ToInt(serviceActivate->endpointPort);
	if ((servicePort < 0) || (servicePort >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
		logWarn(QString("Received service activate on out of range port %1").arg(servicePort));
		return;
	}
	serviceInfo = m_serviceInfo + servicePort;

	if (!serviceInfo->inUse) {
		logWarn(QString("Received service activate on not in use port %1").arg(servicePort));
		return;
	}
	if (!serviceInfo->local) {
		logWarn(QString("Received service activate on remote service port %1").arg(servicePort));
		return;
	}
	if (!serviceInfo->enabled) {
		return;
	}
	serviceInfo->destPort = SyntroUtils::convertUC2ToUInt(serviceActivate->syntroControlPort);	// record the other end's port number
	serviceInfo->state = SYNTRO_LOCAL_SERVICE_STATE_ACTIVE;
	serviceInfo->tLastLookup = SyntroClock();
#ifdef ENDPOINT_TRACE
	TRACE2("Received service activate for port %d to SyntroControl port %d", servicePort, serviceInfo->destPort);
#endif
}

/*!
	\internal
*/

void Endpoint::sendRemoteServiceLookup(SYNTRO_SERVICE_INFO *remoteService)
{
	SYNTRO_SERVICE_LOOKUP *serviceLookup;

	if (remoteService->local) {
		logWarn(QString("send remote service lookup on local service port"));
		return;
	}

	serviceLookup = (SYNTRO_SERVICE_LOOKUP *)malloc(sizeof(SYNTRO_SERVICE_LOOKUP));
	*serviceLookup = remoteService->serviceLookup;
#ifdef ENDPOINT_TRACE
	TRACE2("Sending request for %s on local port %d", serviceLookup->servicePath, SyntroUtils::convertUC2ToUInt(serviceLookup->localPort));
#endif
	syntroSendMessage(SYNTROMSG_SERVICE_LOOKUP_REQUEST, (SYNTRO_MESSAGE *)serviceLookup, sizeof(SYNTRO_SERVICE_LOOKUP), SYNTROLINK_MEDHIGHPRI);
	remoteService->tLastLookup = SyntroClock();
}


//	processLookupResponse handles the response to a lookup request,
//	recording the result as required.

/*!
	\internal
*/

void Endpoint::processLookupResponse(SYNTRO_SERVICE_LOOKUP *serviceLookup)
{
	int index;
	SYNTRO_SERVICE_INFO *remoteService;

	index = SyntroUtils::convertUC2ToInt(serviceLookup->localPort);		// get the local port
	if ((index < 0) || (index >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
		logWarn(QString("Lookup response from %1 port %2 to incorrect local port %3") 
					.arg(SyntroUtils::displayUID(&serviceLookup->lookupUID))
					.arg(SyntroUtils::convertUC2ToInt(serviceLookup->remotePort))
					.arg(SyntroUtils::convertUC2ToInt(serviceLookup->localPort)));
		return;
	}

	remoteService = m_serviceInfo + index;
	if (!remoteService->inUse) {
		logWarn(QString("Lookup response on not in use port %1").arg(index));
		return;
	}
	if (remoteService->local) {
		logWarn(QString("Lookup response on local service port %1").arg(index));
		return;
	}
	if (!remoteService->enabled) {
		return;
	}
	switch (remoteService->state) {
		case SYNTRO_REMOTE_SERVICE_STATE_REMOVE:			// we're waiting to remove so just ignore
			break;

		case SYNTRO_REMOTE_SERVICE_STATE_REMOVING:
			if (serviceLookup->response == SERVICE_LOOKUP_REMOVE) {// removal has been confirmed
				if (remoteService->removingService) {
					remoteService->inUse = false;				// all done
					remoteService->removingService = false;
				}
				remoteService->enabled = false;
			}
			break;	

		case SYNTRO_REMOTE_SERVICE_STATE_LOOKING:
			if (serviceLookup->response == SERVICE_LOOKUP_FAIL) {	// the service is not there
#ifdef ENDPOINT_TRACE
				TRACE1("Remote service %s unavailable", remoteService->serviceLookup.servicePath);
#else
				;
#endif
			} else if (serviceLookup->response == SERVICE_LOOKUP_SUCCEED) { // the service is there
#ifdef ENDPOINT_TRACE
				TRACE3("Service %s mapped to %s port %d", 
								serviceLookup->servicePath, 
								SyntroUtils::displayUID(&(serviceLookup->lookupUID)), 
								SyntroUtils::convertUC2ToInt(serviceLookup->remotePort));
#endif
				// got a good response - record the data and change state to registered
			
				remoteService->serviceLookup.lookupUID = serviceLookup->lookupUID;	
				memcpy(remoteService->serviceLookup.remotePort, serviceLookup->remotePort, sizeof(SYNTRO_UC2));
				memcpy(remoteService->serviceLookup.componentIndex, serviceLookup->componentIndex, sizeof(SYNTRO_UC2));
				memcpy(remoteService->serviceLookup.ID, serviceLookup->ID, sizeof(SYNTRO_UC4));
				remoteService->state = SYNTRO_REMOTE_SERVICE_STATE_REGISTERED; // this is now active
				remoteService->tLastLookupResponse = SyntroClock();		// reset the timeout timer
				remoteService->serviceLookup.response = SERVICE_LOOKUP_SUCCEED;
				remoteService->destPort = SyntroUtils::convertUC2ToInt(remoteService->serviceLookup.remotePort);
			}
			break;

		case SYNTRO_REMOTE_SERVICE_STATE_REGISTERED:
			if (serviceLookup->response == SERVICE_LOOKUP_FAIL) {	// the service has gone away
#ifdef ENDPOINT_TRACE
				TRACE1("Remote service %s has gone unavailable", remoteService->serviceLookup.servicePath);
#endif
				remoteService->state = SYNTRO_REMOTE_SERVICE_STATE_LOOK;	// start looking again
			} else if (serviceLookup->response == SERVICE_LOOKUP_SUCCEED) { // the service is still there
				remoteService->tLastLookupResponse = SyntroClock();		// reset the timeout timer
				if ((SyntroUtils::convertUC4ToInt(serviceLookup->ID) == SyntroUtils::convertUC4ToInt(remoteService->serviceLookup.ID))
					&& (SyntroUtils::compareUID(&(serviceLookup->lookupUID), &(remoteService->serviceLookup.lookupUID)))
					&& (SyntroUtils::convertUC2ToInt(serviceLookup->remotePort) == SyntroUtils::convertUC2ToInt(remoteService->serviceLookup.remotePort))
					&& (SyntroUtils::convertUC2ToInt(serviceLookup->componentIndex) == SyntroUtils::convertUC2ToInt(remoteService->serviceLookup.componentIndex))) {
#ifdef ENDPOINT_TRACE
						TRACE1("Reconfirmed %s", serviceLookup->servicePath);
#else
						;
#endif
				} else {
#ifdef ENDPOINT_TRACE
					TRACE3("Service %s remapped to %s port %d", 
								serviceLookup->servicePath, 
								SyntroUtils::displayUID(&(serviceLookup->lookupUID)), 
								SyntroUtils::convertUC2ToInt(serviceLookup->remotePort));
#endif
					// got a changed response - record the data and carry on
			
					remoteService->serviceLookup.lookupUID = serviceLookup->lookupUID;	
					memcpy(remoteService->serviceLookup.remotePort, serviceLookup->remotePort, sizeof(SYNTRO_UC2));
					memcpy(remoteService->serviceLookup.componentIndex, serviceLookup->componentIndex, sizeof(SYNTRO_UC2));
					memcpy(remoteService->serviceLookup.ID, serviceLookup->ID, sizeof(SYNTRO_UC4));
					remoteService->serviceLookup.response = SERVICE_LOOKUP_SUCCEED;
					remoteService->destPort = SyntroUtils::convertUC2ToInt(remoteService->serviceLookup.remotePort);
				}
			}
			break;

		default:
			logWarn(QString("Unexpected state %1 on remote service port %2").arg(remoteService->state).arg(index));
			remoteService->state = SYNTRO_REMOTE_SERVICE_STATE_LOOKING;	// go back to looking
			if (remoteService->removingService) {
				remoteService->inUse = false;
				remoteService->removingService = false;
				remoteService->enabled = false;
			}
			break;
	}
}

/*!
	\internal
*/

void Endpoint::processDirectoryResponse(SYNTRO_DIRECTORY_RESPONSE *directoryResponse, int len)
{
	QStringList dirList;

	QByteArray data(reinterpret_cast<char *>(directoryResponse + 1), len - sizeof(SYNTRO_DIRECTORY_RESPONSE));

	QList<QByteArray> list = data.split(0);

	for (int i = 0; i < list.count(); i++) {
		if (list.at(i).length() > 0)
			dirList << list.at(i);
	}

	appClientReceiveDirectory(dirList);
}

/*!
	\internal
*/

void Endpoint::buildDE()
{
	int servicePort;
	int highestServicePort;
	SYNTRO_SERVICE_INFO *service;

	m_componentData.DESetup();

//	Find highest service number in use in order to keep directory small

	service = m_serviceInfo;
	highestServicePort = 0;
	for (servicePort = 0; servicePort < SYNTRO_MAX_SERVICESPERCOMPONENT; servicePort ++, service++) {
		if (service->inUse && service->enabled)
			highestServicePort = servicePort;
	}

	service = m_serviceInfo;
	for (servicePort = 0; servicePort <= highestServicePort; servicePort ++, service++) {
		if (!service->inUse || !service->enabled) {
			m_componentData.DEAddValue(DETAG_NOSERVICE, "");
			continue;
		}
		if (service->local) {
			if (service->serviceType == SERVICETYPE_MULTICAST)
				m_componentData.DEAddValue(DETAG_MSERVICE, service->servicePath);
			else
				m_componentData.DEAddValue(DETAG_ESERVICE, service->servicePath);
		} else {
			m_componentData.DEAddValue(DETAG_NOSERVICE, "");
		}
	}

	m_componentData.DEComplete();
}

//----------------------------------------------------------

/*!
	\internal
*/

void Endpoint::linkCloseCleanup()
{
	SYNTRO_SERVICE_INFO *service = m_serviceInfo;

	for (int i = 0; i < SYNTRO_MAX_SERVICESPERCOMPONENT; i++, service++) {
		if (!service->inUse)
			continue;

		if (!service->enabled)
			continue;

		if (service->local)
			service->state = SYNTRO_LOCAL_SERVICE_STATE_INACTIVE;
		else
			service->state = SYNTRO_REMOTE_SERVICE_STATE_LOOK;
	}
}

/*!
	\internal
*/

bool Endpoint::syntroConnect()
{
	int	returnValue;
	int size = SYNTRO_MESSAGE_MAX * 3;
	int control;

	m_connectInProgress = false;
	m_connected = false;

	qint64 now = SyntroClock();

	if (!SyntroUtils::syntroTimerExpired(now, m_connWait, ENDPOINT_CONNWAIT))
		return false;				// not time yet

	m_connWait = now;

	// check if need to send beacon rather than check for SyntroControls

	if (!m_beaconDelay) {
		m_hello->sendHelloBeacon();							// request SyntroControl hellos
		m_beaconDelay = true;
		return false;
	}

	m_beaconDelay = false;									// beacon delay has now expired

	for (control = 0; control < ENDPOINT_MAX_SYNTROCONTROLS; control++) {
		if (m_controlName[control][0] == 0) {
			if (m_hello->findBestControl(&m_helloEntry)) {
				m_controlIndex = control;
				m_priorityMode = true;
				break;
			}
		} else {
			if (m_hello->findComponent(&m_helloEntry, m_controlName[control], (char *)COMPTYPE_CONTROL)) { 	
				m_controlIndex = control;
				m_priorityMode = false;
				break;
			}
		}
	}

	if (control == ENDPOINT_MAX_SYNTROCONTROLS) {
		// failed to find any usable SyntroControl
		if (strlen(m_controlName[0]) == 0)
			updateState("Waiting for any SyntroControl");
		else
			updateState(QString("Waiting for valid SyntroControl"));

		return false;
	}

	if (m_sock != NULL) {
		delete m_sock;
		delete m_syntroLink;
	}

	m_sock = new SyntroSocket(this, 0, m_encryptLink);
	m_syntroLink = new SyntroLink(m_logTag);

	returnValue = m_sock->sockCreate(0, SOCK_STREAM);

	if (returnValue == 0)
		return false;

	m_sock->sockSetConnectMsg(ENDPOINT_ONCONNECT_MESSAGE);
	m_sock->sockSetCloseMsg(ENDPOINT_ONCLOSE_MESSAGE);
	m_sock->sockSetReceiveMsg(ENDPOINT_ONRECEIVE_MESSAGE);
	m_sock->sockSetReceiveBufSize(size);

    if (m_encryptLink)
    	returnValue = m_sock->sockConnect(m_helloEntry.IPAddr, SYNTRO_SOCKET_LOCAL_ENCRYPT);
    else
    	returnValue = m_sock->sockConnect(m_helloEntry.IPAddr, SYNTRO_SOCKET_LOCAL);
	if (!returnValue) {
		TRACE0("Connect attempt failed");
		return false;
	}
	m_connectInProgress = true;

	QString str = QString("Trying connect to %1 %2").arg(m_helloEntry.IPAddr).arg(m_encryptLink ? "using SSL" : "unencrypted");
	updateState(str);
	logDebug(str);
	return	true;
}

/*!
	\internal
*/

void Endpoint::syntroClose()
{
	m_connected = false;
	m_connectInProgress = false;
	m_beaconDelay = false;

	if (m_sock != NULL) {
		delete m_sock;
		m_sock = NULL;
	}

	if (m_syntroLink != NULL) {
		delete m_syntroLink;
		m_syntroLink = NULL;
	}

	updateState("Connection closed");
}

/*!
	\internal
*/

bool Endpoint::syntroSendMessage(int cmd, SYNTRO_MESSAGE *syntroMessage, int len, int priority)
{
	if (!m_connected) {
			
		// can't send as not connected
		free(syntroMessage);
		return false;										
	}

	m_syntroLink->send(cmd, len, priority, syntroMessage);
	m_syntroLink->trySending(m_sock);
	return true;
}

/*!
	\internal
*/

Hello *Endpoint::getHelloObject()
{
	return m_hello;
}

//----------------------------------------------------------
//	These intermediate functions call appClient functions after pre-processing and checks

/*!
	\internal
*/

void Endpoint::endpointConnected()
{
	SYNTRO_SERVICE_INFO *service;

	m_connected = true;

	service = m_serviceInfo;

	for (int i = 0; i < SYNTRO_MAX_SERVICESPERCOMPONENT; i++, service++) {
		if (!service->inUse)
			continue;

		if (service->local)
			service->state = SYNTRO_LOCAL_SERVICE_STATE_INACTIVE;
		else
			service->state = SYNTRO_REMOTE_SERVICE_STATE_LOOK;

		service->lastReceivedSeqNo = -1;
		service->nextSendSeqNo = 0;
		service->lastReceivedAck = 0;
		service->lastSendTime = SyntroClock();
	}

	appClientConnected();
}

/*!
	\internal
*/

void Endpoint::endpointClosed()
{
	m_connected = false;
	m_connectInProgress = false;
	m_beaconDelay = false;
	appClientClosed();
}

/*!
	\internal
*/

void Endpoint::endpointHeartbeat(SYNTRO_HEARTBEAT *heartbeat, int length)
{
	m_connected = true;
	appClientHeartbeat(heartbeat, length);
}


/*!
	\internal
*/

void Endpoint::processMulticast(SYNTRO_EHEAD *message, int length, int destPort)
{
	SYNTRO_SERVICE_INFO *service;

	service = m_serviceInfo + destPort;

	if (!service->inUse) {
		logWarn(QString("Nulticast data received on not in use port %1").arg(destPort));
		free(message);
		return;								
	}

	if (service->serviceType != SERVICETYPE_MULTICAST) {
		logWarn(QString("Multicast data received on port %1 that is not a multicast service port").arg(destPort));
		free(message);
		return;								
	}

	if (service->lastReceivedSeqNo == message->seq)
		logWarn(QString("Received duplicate multicast seq number %1  source %2  dest %2")
			.arg(message->seq)
			.arg(SyntroUtils::displayUID(&message->sourceUID))
			.arg(SyntroUtils::displayUID(&message->destUID)));

	service->lastReceivedSeqNo = message->seq;

	appClientReceiveMulticast(destPort, message, length);
}

/*!
	\internal
*/

void Endpoint::processMulticastAck(SYNTRO_EHEAD *message, int length, int destPort)
{
	SYNTRO_SERVICE_INFO *service;

	service = m_serviceInfo + destPort;
	if (!service->inUse) {
		logWarn(QString("Nulticast data received on not in use port %1").arg(destPort));
		free(message);
		return;								
	}
	if (service->serviceType != SERVICETYPE_MULTICAST) {
		logWarn(QString("Multicast data received on port %1 that is not a multicast service port").arg(destPort));
		free(message);
		return;								
	}

	service->lastReceivedAck = message->seq;

	appClientReceiveMulticastAck(destPort, message, length);
}

/*!
	\internal
*/

void Endpoint::processE2E(SYNTRO_EHEAD *message, int length, int destPort)
{
	SYNTRO_SERVICE_INFO *service;

	service = m_serviceInfo + destPort;
	if (!service->inUse) {
		logWarn(QString("Multicast data received on not in use port %1").arg(destPort));
		free(message);
		return;								
	}
	if (service->serviceType != SERVICETYPE_E2E) {
		logWarn(QString("E2E data received on port %1 that is not an E2E service port").arg(destPort));
		free(message);
		return;								
	}

	appClientReceiveE2E(destPort, message, length);
}


//-------------------------------------------------------------------------------------------
//	SyntroCFS API functions
//

/*!
	\internal
*/

void Endpoint::CFSInit()
{
	int i;
	SYNTROCFS_CLIENT_EPINFO *EP;

	EP = cfsEPInfo;

	for (i = 0; i < SYNTRO_MAX_SERVICESPERCOMPONENT; i++, EP++) {
		EP->inUse = false;								// this meansEP is not in use for SyntroCFS
	}
}

/*!
	Allows a service port (returned from clientAddService or clientLoadServices) \a serviceEP to be 
	designated as a Cloud File System endpoint. Endpoint will trap message received from the 
	SyntroLink destined for a CFS endpoint and generate app client callbacks in response instead 
	of passing up the raw messages.
*/

void Endpoint::CFSAddEP(int serviceEP)
{
	SYNTROCFS_CLIENT_EPINFO *EP;
	int	i;

	if ((serviceEP < 0) || (serviceEP >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
		logWarn(QString("Tried to add out of range EP %1 for SyntroCFS processing").arg(serviceEP));
		return;
	}
	EP = cfsEPInfo + serviceEP;
	EP->dirInProgress = false;
	EP->inUse = true;
	for (i = 0; i < SYNTROCFS_MAX_CLIENT_FILES; i++) {
		EP->cfsFile[i].inUse = false;
		EP->cfsFile[i].clientHandle = i;
	}
}

/*!
	Removes the CFS designation from the service port \a serviceEP.
*/

void Endpoint::CFSDeleteEP(int serviceEP)
{
	SYNTROCFS_CLIENT_EPINFO *EP;

	if ((serviceEP < 0) || (serviceEP >= SYNTRO_MAX_SERVICESPERCOMPONENT)) {
		logWarn(QString("Tried to delete out of range EP %1 for SyntroCFS processing").arg(serviceEP));
		return;
	}
	EP = cfsEPInfo + serviceEP;
	EP->inUse = false;
}

/*!
	CFSDir can be called by the client app to request a directory of files from the SyntroCFS associated with 
	service port \a serviceEP. Only one request can be outstanding at a time. The function returns true if 
	the request was sent, false if an error occurred and indicates that the request was not sent. If the 
	function returns true, a call to CFSDirResponse will occur, either when a response is received or a timeout occurs.
*/

bool Endpoint::CFSDir(int serviceEP, int cfsDirParam)
{
	SYNTRO_EHEAD *requestE2E;
	SYNTRO_CFSHEADER *requestHdr;	

	if (cfsEPInfo[serviceEP].dirInProgress) {
		return false;
	}
	requestE2E = CFSBuildRequest(serviceEP, 0);
	if (requestE2E == NULL) {
		return false;
	}
	requestHdr = reinterpret_cast<SYNTRO_CFSHEADER *>(requestE2E+1);			// pointer to the new SyntroCFS header
	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_DIR_REQ, requestHdr->cfsType);
	SyntroUtils::convertIntToUC2(cfsDirParam, requestHdr->cfsParam);
	syntroSendMessage(SYNTROMSG_E2E, (SYNTRO_MESSAGE *)requestE2E, sizeof(SYNTRO_EHEAD) + sizeof(SYNTRO_CFSHEADER), SYNTROCFS_E2E_PRIORITY);
	cfsEPInfo[serviceEP].dirReqTime = SyntroClock();
	cfsEPInfo[serviceEP].dirInProgress = true;
	return true;
}

/*!
	CFSOpen is called to open a file identified by filePath on a SyntroCFS server connection 
	referenced by \a serviceEP. 

	If the \a filePath has an extension of “.srf” then the transfer mode is set to structured
	and the data transfer is in blocks as they were originally received by the Syntro store.
	The \a param argument is not used.

	If the extension is ".db", the transfer is database mode and query results are returned in
	result sets of varying length. The \a param argument specifies the table type.

	For any other extension, the transfer mode is assumed to be raw file mode and the data is 
	transferred in unstructured blocks. The \a param is the size of the blocks returned and 
	can be from 1 to 65535.

	The functions return a local handle that should be used for all future references to this open file 
	if the open request was sent to the server, it will return -1 if not. If the request was sent, a call to 
	CFSOpenResponse() will be made when either a response is received or the request it timed out.
*/

int Endpoint::CFSOpenDB(int serviceEP, QString databaseName)
{
	return CFSOpen(serviceEP, databaseName, SYNTROCFS_TYPE_DATABASE, 0);	
}

int Endpoint::CFSOpenStructuredFile(int serviceEP, QString filePath)
{
	return CFSOpen(serviceEP, filePath, SYNTROCFS_TYPE_STRUCTURED_FILE, 0);
}

int Endpoint::CFSOpenRawFile(int serviceEP, QString filePath, int blockSize)
{
	// max blockSize is legacy
	if (blockSize < 1 || blockSize > 0xffff) {
		logError(QString("CFSOpenRawFile: Invalid blockSize %1").arg(blockSize));
		return -1;
	}

	return CFSOpen(serviceEP, filePath, SYNTROCFS_TYPE_RAW_FILE, blockSize);
}

int Endpoint::CFSOpen(int serviceEP, QString filePath, int cfsMode, int blockSize)
{
	int handle;

	// Find a free stream slot
	for (handle = 0; handle < SYNTROCFS_MAX_CLIENT_FILES; handle++) {
		if (!cfsEPInfo[serviceEP].cfsFile[handle].inUse)
			break;
	}

	if (handle == SYNTROCFS_MAX_CLIENT_FILES) {
		logError(QString("Too many files open"));
		return -1;
	}

	SYNTRO_EHEAD *requestE2E = CFSBuildRequest(serviceEP, filePath.size() + 1);

	if (requestE2E == NULL) {
		logError(QString("CFSOpen on unavailable service %1 port %2").arg(filePath).arg(serviceEP));
		return -1;
	}

	SYNTRO_CFS_FILE *scf = cfsEPInfo[serviceEP].cfsFile + handle;

	scf->inUse = true;
	scf->open = false;
	scf->readInProgress = false;
	scf->writeInProgress = false;
	scf->queryInProgress = false;
	scf->fetchQueryInProgress = false;
	scf->cancelQueryInProgress = false;
	scf->closeInProgress = false;
	scf->structured = filePath.endsWith(SYNTRO_RECORD_SRF_RECORD_DOTEXT);

	SYNTRO_CFSHEADER *requestHdr = reinterpret_cast<SYNTRO_CFSHEADER *>(requestE2E+1);

	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_OPEN_REQ, requestHdr->cfsType);
	SyntroUtils::convertIntToUC2(scf->clientHandle, requestHdr->cfsClientHandle);
	SyntroUtils::convertIntToUC2(cfsMode, requestHdr->cfsParam);
	SyntroUtils::convertIntToUC4(blockSize, requestHdr->cfsIndex);	

	char *pData = reinterpret_cast<char *>(requestHdr + 1);

	strcpy(pData, qPrintable(filePath));

	int totalLength = sizeof(SYNTRO_EHEAD) + sizeof(SYNTRO_CFSHEADER) + (int)strlen(pData) + 1;

	syntroSendMessage(SYNTROMSG_E2E, (SYNTRO_MESSAGE *)requestE2E, totalLength, SYNTROCFS_E2E_PRIORITY);

	scf->openReqTime = SyntroClock();
	scf->openInProgress = true;
	
	return handle;
}

/*!
	CFSClose can be called to close an open file with handle \a handle on service port \a serviceEP. 
	The function will return true if the close was issued and there will be a subsequent call to 
	CFSClose() Response when a response is received or a timeout occurs. Returning false means 
	that an error occurred, the close was not sent and there will not be a call to CFSCloseResponse(). 
	The client app in this case should assume that the file is closed.
*/

bool Endpoint::CFSClose(int serviceEP, int handle)
{
	SYNTRO_CFS_FILE *scf;
	SYNTROCFS_CLIENT_EPINFO *EP;
	SYNTRO_EHEAD *requestE2E;
	SYNTRO_CFSHEADER *requestHdr;

	EP = cfsEPInfo + serviceEP;
	if (!EP->inUse) {
		logWarn(QString("CFSClose attempted on not in use port %1").arg(serviceEP));
		return false;												// the endpoint isn't a SyntroCFS one!
	}

	if (handle >= SYNTROCFS_MAX_CLIENT_FILES) {
		logWarn(QString("CFSClose attempted on out of range handle %1 on port %2").arg(handle).arg(serviceEP));
		return false;													
	}
	scf = EP->cfsFile + handle;
	if (!scf->inUse || !scf->open) {
		logWarn(QString("CFSClose attempted on not open handle %1 on port %2").arg(handle).arg(serviceEP));
		return false;													
	}
	
	if (scf->closeInProgress) {
		logWarn(QString("CFSClose attempted on already closing handle %1 on port %2").arg(handle).arg(serviceEP));
		return false;													
	}
	requestE2E = CFSBuildRequest(serviceEP, 0);
	if (requestE2E == NULL) {
		logWarn(QString("CFSClose attempted on unavailable service handle %1 on port %2").arg(handle).arg(serviceEP));
		return false;
	}

	requestHdr = reinterpret_cast<SYNTRO_CFSHEADER *>(requestE2E+1);	// pointer to the new SyntroCFS header
	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_CLOSE_REQ, requestHdr->cfsType);
	SyntroUtils::convertIntToUC2(scf->clientHandle, requestHdr->cfsClientHandle);
	SyntroUtils::convertIntToUC2(scf->storeHandle, requestHdr->cfsStoreHandle);
	syntroSendMessage(SYNTROMSG_E2E, 
		(SYNTRO_MESSAGE *)requestE2E, 
		sizeof(SYNTRO_EHEAD) + sizeof(SYNTRO_CFSHEADER), 
		SYNTROCFS_E2E_PRIORITY);
	scf->closeReqTime = SyntroClock();
	scf->closeInProgress = true;
	return true;
}

/*!
	CFSReadAtIndex can be called to read a record or block(s) starting at record or 
	block \a index from the file associated with \a handle on service port \a serviceEP. \a blockCount 
	can be used in raw mode to read more than one block. blockCount can be between 1 and 65535. 
	This parameter is ignored in structured mode.

	The function returns true if the read request was issued and a call to CFSReadAtIndexResponse() 
	will be made or false if the read was not issued and there will not be a subsequent call to CFSReadAtIndexResponse().
*/

bool Endpoint::CFSReadAtIndex(int serviceEP, int handle, unsigned int index, int blockCount)
{
	SYNTRO_CFS_FILE *scf;
	SYNTROCFS_CLIENT_EPINFO *EP;
	SYNTRO_EHEAD *requestE2E;
	SYNTRO_CFSHEADER *requestHdr;

	EP = cfsEPInfo + serviceEP;
	if (!EP->inUse) {
		logWarn(QString("CFSReadAtIndex attempted on not in use port %1").arg(serviceEP));
		return false;													// the endpoint isn't a SyntroCFS one!
	}

	if ((handle < 0) || (handle >= SYNTROCFS_MAX_CLIENT_FILES)) {
		logWarn(QString("CFSReadAtIndex attempted on out of range handle %1 on port %2").arg(handle).arg(serviceEP));
		return false;													
	}
	scf = EP->cfsFile + handle;
	if (!scf->inUse || !scf->open) {
		logWarn(QString("CFSReadAtIndex attempted on not open handle %1 on port %2").arg(handle).arg(serviceEP));
		return false;													
	}
	requestE2E = CFSBuildRequest(serviceEP, 0);
	if (requestE2E == NULL) {
		logWarn(QString("CFSReadAtIndex attempted on unavailable service handle %1 on port %2").arg(handle).arg(serviceEP));
		return false;
	}

	requestHdr = reinterpret_cast<SYNTRO_CFSHEADER *>(requestE2E+1);	// pointer to the new SyntroCFS header
	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_READ_INDEX_REQ, requestHdr->cfsType);
	SyntroUtils::convertIntToUC2(scf->clientHandle, requestHdr->cfsClientHandle);
	SyntroUtils::convertIntToUC2(scf->storeHandle, requestHdr->cfsStoreHandle);
	SyntroUtils::convertIntToUC4(index, requestHdr->cfsIndex);
	SyntroUtils::convertIntToUC2(blockCount, requestHdr->cfsParam);		// number of blocks to read if flat
	syntroSendMessage(SYNTROMSG_E2E, 
		(SYNTRO_MESSAGE *)requestE2E, 
		sizeof(SYNTRO_EHEAD) + sizeof(SYNTRO_CFSHEADER), 
		SYNTROCFS_E2E_PRIORITY);
	scf->readReqTime = SyntroClock();
	scf->readInProgress = true;
	return true;
}

/*!
	CFSWriteAtIndex can be called to write a record or block(s) starting at record or 
	block \a index to the file associated with \a handle on service port \a serviceEP. \a blockCount 
	can be used in raw mode to write more than one block. blockCount can be between 1 and 65535. 
	This parameter is ignored in structured mode.

	The function returns true if the write request was issued and a call to CFSWriteAtIndexResponse() 
	will be made or false if the write was not issued and there will not be a subsequent call to CFSWriteAtIndexResponse().
*/

bool Endpoint::CFSWriteAtIndex(int serviceEP, int handle, unsigned int index, unsigned char *fileData, int length)
{
	SYNTRO_CFS_FILE *scf;
	SYNTROCFS_CLIENT_EPINFO *EP;
	SYNTRO_EHEAD *requestE2E;
	SYNTRO_CFSHEADER *requestHdr;

	EP = cfsEPInfo + serviceEP;
	if (!EP->inUse) {
		logWarn(QString("CFSWriteAtIndex attempted on not in use port %1").arg(serviceEP));
		return false;													// the endpoint isn't a SyntroCFS one!
	}

	if ((handle < 0) || (handle >= SYNTROCFS_MAX_CLIENT_FILES)) {
		logWarn(QString("CFSWriteAtIndex attempted on out of range handle %1 on port %2").arg(handle).arg(serviceEP));
		return false;													
	}
	scf = EP->cfsFile + handle;
	if (!scf->inUse || !scf->open) {
		logWarn(QString("CFSWriteAtIndex attempted on not open handle %1 on port %2").arg(handle).arg(serviceEP));
		return false;													
	}
	requestE2E = CFSBuildRequest(serviceEP, length);
	if (requestE2E == NULL) {
		logWarn(QString("CFSWriteAtIndex attempted on unavailable service handle %1 on port %2").arg(handle).arg(serviceEP));
		return false;
	}

	requestHdr = reinterpret_cast<SYNTRO_CFSHEADER *>(requestE2E+1);	// pointer to the new SyntroCFS header
	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_WRITE_INDEX_REQ, requestHdr->cfsType);
	SyntroUtils::convertIntToUC2(scf->clientHandle, requestHdr->cfsClientHandle);
	SyntroUtils::convertIntToUC2(scf->storeHandle, requestHdr->cfsStoreHandle);
	SyntroUtils::convertIntToUC4(index, requestHdr->cfsIndex);
	memcpy((unsigned char *)(requestHdr + 1), fileData, length);
	syntroSendMessage(SYNTROMSG_E2E, 
		(SYNTRO_MESSAGE *)requestE2E, 
		sizeof(SYNTRO_EHEAD) + sizeof(SYNTRO_CFSHEADER) + length, 
		SYNTROCFS_E2E_PRIORITY);
	scf->writeReqTime = SyntroClock();
	scf->writeInProgress = true;
	return true;
}

bool Endpoint::CFSQuery(int serviceEP, int handle, QString sql)
{
	SYNTROCFS_CLIENT_EPINFO *EP = cfsEPInfo + serviceEP;

	if (!EP->inUse) {
		logWarn(QString("CFSQuery attempted on not in use port %1").arg(serviceEP));
		return false;
	}

	if ((handle < 0) || (handle >= SYNTROCFS_MAX_CLIENT_FILES)) {
		logWarn(QString("CFSQuery attempted on out of range handle %1 on port %2").arg(handle).arg(serviceEP));
		return false;													
	}

	SYNTRO_CFS_FILE *scf = EP->cfsFile + handle;

	if (!scf->inUse || !scf->open) {
		logWarn(QString("CFSQuery attempted on not open handle %1 on port %2").arg(handle).arg(serviceEP));
		return false;													
	}

	SYNTRO_EHEAD *requestE2E = CFSBuildRequest(serviceEP, sql.length() + 1);

	if (requestE2E == NULL) {
		logWarn(QString("CFSQuery attempted on unavailable service handle %1 on port %2").arg(handle).arg(serviceEP));
		return false;
	}

	SYNTRO_CFSHEADER *requestHdr = reinterpret_cast<SYNTRO_CFSHEADER *>(requestE2E + 1);

	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_QUERY_REQ, requestHdr->cfsType);
	SyntroUtils::convertIntToUC2(scf->clientHandle, requestHdr->cfsClientHandle);
	SyntroUtils::convertIntToUC2(scf->storeHandle, requestHdr->cfsStoreHandle);

	char *pData = reinterpret_cast<char *>(requestHdr + 1);
	strcpy(pData, qPrintable(sql));

	int totalLength = sizeof(SYNTRO_EHEAD) + sizeof(SYNTRO_CFSHEADER) + (int)strlen(pData) + 1;
	syntroSendMessage(SYNTROMSG_E2E, (SYNTRO_MESSAGE *)requestE2E, totalLength, SYNTROCFS_E2E_PRIORITY);

	scf->queryReqTime = SyntroClock();
	scf->queryInProgress = true;

	return true;
}

bool Endpoint::CFSCancelQuery(int serviceEP, int handle)
{
	SYNTROCFS_CLIENT_EPINFO *EP = cfsEPInfo + serviceEP;

	if (!EP->inUse) {
		logWarn(QString("CFSCancelQuery attempted on not in use port %1").arg(serviceEP));
		return false;
	}

	if ((handle < 0) || (handle >= SYNTROCFS_MAX_CLIENT_FILES)) {
		logWarn(QString("CFSCancelQuery attempted on out of range handle %1 on port %2").arg(handle).arg(serviceEP));
		return false;													
	}

	SYNTRO_CFS_FILE *scf = EP->cfsFile + handle;

	if (!scf->inUse || !scf->open) {
		logWarn(QString("CFSCancelQuery attempted on not open handle %1 on port %2").arg(handle).arg(serviceEP));
		return false;													
	}

	SYNTRO_EHEAD *requestE2E = CFSBuildRequest(serviceEP, 0);

	if (requestE2E == NULL) {
		logWarn(QString("CFSCancelQuery attempted on unavailable service handle %1 on port %2").arg(handle).arg(serviceEP));
		return false;
	}

	SYNTRO_CFSHEADER *requestHdr = reinterpret_cast<SYNTRO_CFSHEADER *>(requestE2E + 1);

	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_CANCEL_QUERY_REQ, requestHdr->cfsType);
	SyntroUtils::convertIntToUC2(scf->clientHandle, requestHdr->cfsClientHandle);
	SyntroUtils::convertIntToUC2(scf->storeHandle, requestHdr->cfsStoreHandle);

	int totalLength = sizeof(SYNTRO_EHEAD) + sizeof(SYNTRO_CFSHEADER);
	syntroSendMessage(SYNTROMSG_E2E, (SYNTRO_MESSAGE *)requestE2E, totalLength, SYNTROCFS_E2E_PRIORITY);

	scf->cancelQueryReqTime = SyntroClock();
	scf->cancelQueryInProgress = true;

	return true;
}

bool Endpoint::CFSFetchQuery(int serviceEP, int handle, int maxRows, int resultType)
{
	SYNTROCFS_CLIENT_EPINFO *EP = cfsEPInfo + serviceEP;

	if (!EP->inUse) {
		logWarn(QString("CFSFetchQuery attempted on not in use port %1").arg(serviceEP));
		return false;
	}

	if ((handle < 0) || (handle >= SYNTROCFS_MAX_CLIENT_FILES)) {
		logWarn(QString("CFSFetchQuery attempted on out of range handle %1 on port %2").arg(handle).arg(serviceEP));
		return false;													
	}

	SYNTRO_CFS_FILE *scf = EP->cfsFile + handle;

	if (!scf->inUse || !scf->open) {
		logWarn(QString("CFSFetchQuery attempted on not open handle %1 on port %2").arg(handle).arg(serviceEP));
		return false;													
	}

	SYNTRO_EHEAD *requestE2E = CFSBuildRequest(serviceEP, 0);

	if (requestE2E == NULL) {
		logWarn(QString("CFSFetchQuery attempted on unavailable service handle %1 on port %2").arg(handle).arg(serviceEP));
		return false;
	}

	SYNTRO_CFSHEADER *requestHdr = reinterpret_cast<SYNTRO_CFSHEADER *>(requestE2E + 1);

	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_FETCH_QUERY_REQ, requestHdr->cfsType);
	SyntroUtils::convertIntToUC2(scf->clientHandle, requestHdr->cfsClientHandle);
	SyntroUtils::convertIntToUC2(scf->storeHandle, requestHdr->cfsStoreHandle);
	SyntroUtils::convertIntToUC4(maxRows, requestHdr->cfsIndex);
	SyntroUtils::convertIntToUC2(resultType, requestHdr->cfsParam);

	int totalLength = sizeof(SYNTRO_EHEAD) + sizeof(SYNTRO_CFSHEADER);
	syntroSendMessage(SYNTROMSG_E2E, (SYNTRO_MESSAGE *)requestE2E, totalLength, SYNTROCFS_E2E_PRIORITY);

	scf->fetchQueryReqTime = SyntroClock();
	scf->fetchQueryInProgress = true;

	return true;
}

/*!
	\internal
*/


SYNTRO_EHEAD *Endpoint::CFSBuildRequest(int serviceEP, int length)
{
	if (!clientIsServiceActive(serviceEP))
		return NULL;										// can't send on port that isn't running!

	SYNTRO_UID uid = m_componentData.getMyUID();

	SYNTRO_EHEAD *requestE2E = SyntroUtils::createEHEAD(&uid,
					serviceEP, 
					clientGetRemoteServiceUID(serviceEP), 
					clientGetServiceDestPort(serviceEP), 
					0, 
					sizeof(SYNTRO_CFSHEADER) + length);					

    if (requestE2E) {
        SYNTRO_CFSHEADER *requestHdr = reinterpret_cast<SYNTRO_CFSHEADER *>(requestE2E + 1);
        memset(requestHdr, 0, sizeof(SYNTRO_CFSHEADER));
        SyntroUtils::convertIntToUC4(length, requestHdr->cfsLength);
    }

	return requestE2E;
}

/*!
	\internal
*/

void Endpoint::CFSBackground()
{
	int i, j;
	SYNTROCFS_CLIENT_EPINFO *EP;
	SYNTRO_CFS_FILE *scf;

	qint64 now = SyntroClock();

	EP = cfsEPInfo;
	for (i = 0; i < SYNTRO_MAX_SERVICESPERCOMPONENT; i++, EP++) {
		if (EP->inUse) {
			if (EP->dirInProgress) {
				if (SyntroUtils::syntroTimerExpired(now, EP->dirReqTime, SYNTROCFS_DIRREQ_TIMEOUT)) {
					CFSDirResponse(i, SYNTROCFS_ERROR_REQUEST_TIMEOUT, QStringList());
					EP->dirInProgress = false;
					return;
				}
			}
			scf = EP->cfsFile;
			for (j = 0; j < SYNTROCFS_MAX_CLIENT_FILES; j++, scf++) {
				if (!scf->inUse)
					continue;
				if (scf->openInProgress) {					// process open timeout
					if (SyntroUtils::syntroTimerExpired(now, scf->openReqTime, SYNTROCFS_OPENREQ_TIMEOUT)) {
						TRACE2("Timed out open request on port %d slot %d", i, j);
						scf->inUse = false;					// close it down
						CFSOpenResponse(i, SYNTROCFS_ERROR_REQUEST_TIMEOUT, j, 0);	// tell client
					}
				}
				if (scf->open) {
					if (SyntroUtils::syntroTimerExpired(now, scf->lastKeepAliveSent, SYNTROCFS_KEEPALIVE_INTERVAL))
						CFSSendKeepAlive(i, scf);

					if (SyntroUtils::syntroTimerExpired(now, scf->lastKeepAliveReceived, SYNTROCFS_KEEPALIVE_TIMEOUT))
						CFSTimeoutKeepAlive(i, scf);
				}
				if (scf->readInProgress) {
					if (SyntroUtils::syntroTimerExpired(now, scf->readReqTime, SYNTROCFS_READREQ_TIMEOUT)) {
						TRACE2("Timed out read request on port %d slot %d", i, j);
						CFSReadAtIndexResponse(i, j, 0, SYNTROCFS_ERROR_REQUEST_TIMEOUT, NULL, 0);	// tell client
						scf->readInProgress = false;
					}
				}
				if (scf->writeInProgress) {
					if (SyntroUtils::syntroTimerExpired(now, scf->writeReqTime, SYNTROCFS_WRITEREQ_TIMEOUT)) {
						TRACE2("Timed out write request on port %d slot %d", i, j);
						CFSWriteAtIndexResponse(i, j, 0, SYNTROCFS_ERROR_REQUEST_TIMEOUT);	// tell client
						scf->writeInProgress = false;
					}
				}
				if (scf->queryInProgress) {
					if (SyntroUtils::syntroTimerExpired(now, scf->queryReqTime, SYNTROCFS_QUERYREQ_TIMEOUT)) {
						TRACE2("Timed out query request on port %d slot %d", i, j);
						CFSWriteAtIndexResponse(i, j, 0, SYNTROCFS_ERROR_REQUEST_TIMEOUT);	// tell client
						scf->queryInProgress = false;
					}
				}
				if (scf->cancelQueryInProgress) {
					if (SyntroUtils::syntroTimerExpired(now, scf->cancelQueryReqTime, SYNTROCFS_QUERYREQ_TIMEOUT)) {
						TRACE2("Timed out cancel query request on port %d slot %d", i, j);
						CFSWriteAtIndexResponse(i, j, 0, SYNTROCFS_ERROR_REQUEST_TIMEOUT);	// tell client
						scf->cancelQueryInProgress = false;
					}
				}
				if (scf->fetchQueryInProgress) {
					if (SyntroUtils::syntroTimerExpired(now, scf->fetchQueryReqTime, SYNTROCFS_QUERYREQ_TIMEOUT)) {
						TRACE2("Timed out fetch query request on port %d slot %d", i, j);
						CFSWriteAtIndexResponse(i, j, 0, SYNTROCFS_ERROR_REQUEST_TIMEOUT);	// tell client
						scf->fetchQueryInProgress = false;
					}
				}
				if (scf->closeInProgress) {
					if (SyntroUtils::syntroTimerExpired(now, scf->closeReqTime, SYNTROCFS_CLOSEREQ_TIMEOUT)) {
						TRACE2("Timed out close request on port %d slot %d", i, j);
						scf->inUse = false;					// close it down
						CFSCloseResponse(i, SYNTROCFS_ERROR_REQUEST_TIMEOUT, j);	// tell client
					}
				}
			}
		}
	}
}


/*!
	\internal
*/

bool Endpoint::CFSProcessMessage(SYNTRO_EHEAD *pE2E, int nLen, int dstPort)
{
	SYNTROCFS_CLIENT_EPINFO *EP;
	SYNTRO_CFSHEADER	*cfsHdr;

	EP = cfsEPInfo + dstPort;
	if (!EP->inUse)
		return false;										// false indicates message was not trapped as not a SyntroCFS port

    if (nLen < (int)sizeof(SYNTRO_CFSHEADER)) {
		logWarn(QString("SyntroCFS message too short (%1) on port %2").arg(nLen).arg(dstPort));
		free(pE2E);
		return true;										// don't process any further
	}

	cfsHdr = reinterpret_cast<SYNTRO_CFSHEADER *>(pE2E + 1);

	unsigned int cfsType = SyntroUtils::convertUC2ToUInt(cfsHdr->cfsType);

	if (cfsType == SYNTROCFS_TYPE_FETCH_QUERY_RES)
		nLen -= sizeof(SYNTRO_QUERYRESULT_HEADER);
	else
		nLen -= sizeof(SYNTRO_CFSHEADER);

	if (nLen != SyntroUtils::convertUC4ToInt(cfsHdr->cfsLength)) {
		logWarn(QString("SyntroCFS message mismatch %1 %2 on port %3").arg(nLen).arg(SyntroUtils::convertUC2ToUInt(cfsHdr->cfsLength)).arg(dstPort));
		free(pE2E);
		return true;
	}

	//	Have trapped a SyntroCFS message

	switch (cfsType) {
		case SYNTROCFS_TYPE_DIR_RES:
			CFSProcessDirResponse(cfsHdr, dstPort);
			break;

		case SYNTROCFS_TYPE_OPEN_RES:
			CFSProcessOpenResponse(cfsHdr, dstPort);
			break;

		case SYNTROCFS_TYPE_CLOSE_RES:
			CFSProcessCloseResponse(cfsHdr, dstPort);
			break;

		case SYNTROCFS_TYPE_KEEPALIVE_RES:
			CFSProcessKeepAliveResponse(cfsHdr, dstPort);
			break;

		case SYNTROCFS_TYPE_READ_INDEX_RES:
			CFSProcessReadAtIndexResponse(cfsHdr, dstPort);
			break;

		case SYNTROCFS_TYPE_WRITE_INDEX_RES:
			CFSProcessWriteAtIndexResponse(cfsHdr, dstPort);
			break;

		case SYNTROCFS_TYPE_QUERY_RES:
			CFSProcessQueryResponse(cfsHdr, dstPort);
			break;

		case SYNTROCFS_TYPE_CANCEL_QUERY_RES:
			CFSProcessCancelQueryResponse(cfsHdr, dstPort);
			break;

		case SYNTROCFS_TYPE_FETCH_QUERY_RES:
			CFSProcessFetchQueryResponse(cfsHdr, dstPort);
			break;

		default:
			logWarn(QString("Unrecognized SyntroCFS message %1 on port %2").arg(SyntroUtils::convertUC2ToUInt(cfsHdr->cfsType)).arg(dstPort)); 
			break;
	}
	free (pE2E);
	return true;											// indicates message was trapped
}

/*!
	\internal
*/

void Endpoint::CFSProcessDirResponse(SYNTRO_CFSHEADER *cfsHdr, int dstPort)
{
	char	*pData;
	QStringList	filePaths;

	pData = reinterpret_cast<char *>(cfsHdr + 1);			// pointer to strings
	filePaths = QString(pData).split(SYNTROCFS_FILENAME_SEP);		// break up the file paths
	cfsEPInfo[dstPort].dirInProgress = false;				// dir process complete
	CFSDirResponse(dstPort, SyntroUtils::convertUC2ToUInt(cfsHdr->cfsParam), filePaths);
}

/*!
	CFSDirResponse is a client app override that is called when either a response is received to a 
	directory request or else a timeout has occurred on service port serviceEP. responseCode 
	indicates the result. SYNTROCFS_SUCCESS indicates that the request was successful and that 
	filePaths contains a list of file paths from the SyntroCFS. Any other value indicates an 
	error (see SyntroCFSDefs.h for details) and filePaths will be NULL.
*/

void Endpoint::CFSDirResponse(int, unsigned int, QStringList)
{
	logDebug(QString("Default CFSDirResponse called"));
}

/*!
	\internal
*/

void Endpoint::CFSProcessOpenResponse(SYNTRO_CFSHEADER *cfsHdr, int dstPort)
{
	SYNTRO_CFS_FILE *scf;
	SYNTROCFS_CLIENT_EPINFO *EP;
	int handle;
	int responseCode;

	EP = cfsEPInfo + dstPort;
	handle = SyntroUtils::convertUC2ToUInt(cfsHdr->cfsClientHandle);		// get the client handle

	if (handle >= SYNTROCFS_MAX_CLIENT_FILES) {
		logWarn(QString("Open response with invalid handle %1 on port %2").arg(handle).arg(dstPort));
		return;
	}
	scf = EP->cfsFile + handle;								// get the file slot pointer
	responseCode = SyntroUtils::convertUC2ToUInt(cfsHdr->cfsParam);		// get the response code
	if (!scf->inUse) {
		logWarn(QString("Open response with not in use handle %1 on port %2").arg(handle).arg(dstPort));
		return;
	}
	if (!scf->openInProgress) {
		logWarn(QString("Open response with not in progress handle %1 on port %2").arg(handle).arg(dstPort));
		return;
	}
	scf->openInProgress = false;
	if (responseCode == SYNTROCFS_SUCCESS) {
		scf->open = true;
		scf->storeHandle = SyntroUtils::convertUC2ToUInt(cfsHdr->cfsStoreHandle);// record SyntroCFS store handle
		scf->lastKeepAliveSent = SyntroClock();
		scf->lastKeepAliveReceived = scf->lastKeepAliveSent;
	} else {
		scf->inUse = false;									// close it down
	}
	CFSOpenResponse(dstPort, responseCode, handle, SyntroUtils::convertUC4ToInt(cfsHdr->cfsIndex));
}

/*!
	CFSOpenResponse is a client app override that is called when either a response to a file open 
	is received or the request is timed out on service port \a remoteServiceEP. \a responseCode indicates 
	the result. SYNTROCFS_SUCCESS indicates that the request was successful and that the file
	associated with handle handle is now open and the total length of the file in records 
	(structured) or blocks (raw) is fileLength. Any other value indicates an error (see SyntroCFSDefs.h for details).
*/

void Endpoint::CFSOpenResponse(int remoteServiceEP, unsigned int responseCode, int, unsigned int)
{
	logDebug(QString("Default CFSOpenResponse called %1 %2").arg(remoteServiceEP).arg(responseCode));
}

/*!
	\internal
*/

void Endpoint::CFSProcessCloseResponse(SYNTRO_CFSHEADER *cfsHdr, int dstPort)
{
	SYNTRO_CFS_FILE *scf;
	SYNTROCFS_CLIENT_EPINFO *EP;
	int handle;
	int responseCode;

	EP = cfsEPInfo + dstPort;
	handle = SyntroUtils::convertUC2ToUInt(cfsHdr->cfsClientHandle);		// get the client handle

	if (handle >= SYNTROCFS_MAX_CLIENT_FILES) {
		logWarn(QString("Close response with invalid handle %1 on port %2").arg(handle).arg(dstPort));
		return;
	}
	scf = EP->cfsFile + handle;							// get the stream slot pointer
	responseCode = SyntroUtils::convertUC2ToUInt(cfsHdr->cfsParam);		// get the response code
	if (!scf->inUse) {
		logWarn(QString("Close response with not in use handle %1 on port %2").arg(handle).arg(dstPort));
		return;
	}
	if (!scf->open) {
		logWarn(QString("Close response with not open handle %1 on port %2").arg(handle).arg(dstPort));
		return;
	}
	scf->open = false;
	scf->inUse = false;										// close it down
	CFSCloseResponse(dstPort, responseCode, handle);
}

/*!
	CFSCloseResponse is a client app override that is called when either a response to a file close 
	is received or the request is timed out on service port \a serviceEP. \a responseCode indicates the result. 
	SYNTROCFS_SUCCESS indicates that the request was successful and that the file associated with handle 
	handle is closed. Any other value indicates an error (see SyntroCFSDefs.h for details). 
	Regardless of the response code, the client app should assume that the file is closed. 
	The handle is no longer valid in all cases.
*/

void Endpoint::CFSCloseResponse(int serviceEP, unsigned int responseCode, int)
{
	logDebug(QString("Default CFSCloseResponse called %1 %2").arg(serviceEP).arg(responseCode));
}

/*!
	\internal
*/

void Endpoint::CFSProcessKeepAliveResponse(SYNTRO_CFSHEADER *cfsHdr, int dstPort)
{
	SYNTRO_CFS_FILE *scf;
	SYNTROCFS_CLIENT_EPINFO *EP;
	int handle;

	EP = cfsEPInfo + dstPort;
	handle = SyntroUtils::convertUC2ToUInt(cfsHdr->cfsClientHandle);		// get the client handle

	if (handle >= SYNTROCFS_MAX_CLIENT_FILES) {
		logWarn(QString("Keep alive response with invalid handle %1 on port %2").arg(handle).arg(dstPort));
		return;
	}
	scf = EP->cfsFile + handle;							// get the stream slot pointer
	if (!scf->open) {
		logWarn(QString("Keep alive response with not open handle %1 on port %2").arg(handle).arg(dstPort));
		return;
	}
	scf->lastKeepAliveReceived = SyntroClock();
#ifdef CFS_TRACE
	TRACE2("Got keep alive response on handle %d port %d", handle, dstPort);
#endif
}

/*!
	\internal
*/

void Endpoint::CFSSendKeepAlive(int serviceEP, SYNTRO_CFS_FILE *scf)
{
	SYNTRO_EHEAD *requestE2E;
	SYNTRO_CFSHEADER *requestHdr;

	scf->lastKeepAliveSent = SyntroClock();
	requestE2E = CFSBuildRequest(serviceEP, 0);
	if (requestE2E == NULL) {
		logWarn(QString("CFSSendKeepAlive on unavailable port %1 handle %2").arg(serviceEP).arg(scf->clientHandle));
		return;
	}

	requestHdr = reinterpret_cast<SYNTRO_CFSHEADER *>(requestE2E+1);	// pointer to the new SyntroCFS header
	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_KEEPALIVE_REQ, requestHdr->cfsType);
	SyntroUtils::convertIntToUC2(scf->clientHandle, requestHdr->cfsClientHandle);
	SyntroUtils::convertIntToUC2(scf->storeHandle, requestHdr->cfsStoreHandle);

	syntroSendMessage(SYNTROMSG_E2E, (SYNTRO_MESSAGE *)requestE2E, sizeof(SYNTRO_EHEAD) + sizeof(SYNTRO_CFSHEADER), SYNTROCFS_E2E_PRIORITY);
}

/*!
	\internal
*/

void Endpoint::CFSTimeoutKeepAlive(int serviceEP, SYNTRO_CFS_FILE *scf)
{
	scf->inUse = false;													// flag as not in use
	CFSKeepAliveTimeout(serviceEP, scf->clientHandle);
}

/*!
	This client app override is called is the CFS keep alive system has detected that the connection to 
	the CFS server for the active file associated with handle on service port \a serviceEP has broken. 
	The client app should handle this situation as though the file has been closed and the \a handle no longer valid.
*/

void Endpoint::CFSKeepAliveTimeout(int serviceEP, int handle)
{
	logDebug(QString("Default CFSKeepAliveTimeout called %1 %2").arg(serviceEP).arg(handle));
}

/*!
	\internal
*/

void Endpoint::CFSProcessReadAtIndexResponse(SYNTRO_CFSHEADER *cfsHdr, int dstPort)
{
	SYNTRO_CFS_FILE *scf;
	SYNTROCFS_CLIENT_EPINFO *EP;
	int handle;
	int responseCode;
	int length;
	unsigned char *fileData;

	EP = cfsEPInfo + dstPort;
	handle = SyntroUtils::convertUC2ToUInt(cfsHdr->cfsClientHandle);		// get the client handle

	if (handle >= SYNTROCFS_MAX_CLIENT_FILES) {
		logWarn(QString("ReadAtIndex response with invalid handle %1 on port %2").arg(handle).arg(dstPort));
		return;
	}
	scf = EP->cfsFile + handle;								// get the file slot pointer
	if (!scf->open) {
		logWarn(QString("ReadAtIndex response with not open handle %1 on port %2").arg(handle).arg(dstPort));
		return;
	}
	if (!scf->readInProgress) {
		logWarn(QString("ReadAtIndex response but no read in progress on handle %1 port %2").arg(handle).arg(dstPort));
		return;
	}
	scf->readInProgress = false;
	responseCode = SyntroUtils::convertUC2ToUInt(cfsHdr->cfsParam);		// get the response code
#ifdef CFS_TRACE
	TRACE3("Got ReadAtIndex response on handle %d port %d code %d", handle, dstPort, responseCode);
#endif
	if (responseCode == SYNTROCFS_SUCCESS) {
		length = SyntroUtils::convertUC4ToInt(cfsHdr->cfsLength);
		fileData = reinterpret_cast<unsigned char *>(malloc(length));
		memcpy(fileData, cfsHdr + 1, length);				// make a copy of the record to give to the client
		CFSReadAtIndexResponse(dstPort, handle, SyntroUtils::convertUC4ToInt(cfsHdr->cfsIndex), responseCode, fileData, length); 
	} else {
		CFSReadAtIndexResponse(dstPort, handle,SyntroUtils:: convertUC4ToInt(cfsHdr->cfsIndex), responseCode, NULL, 0); 
	}
}

/*!
	This client app override is called when a read response for the file associated with handle on service 
	port \a serviceEP has been received or else has timed out. \a responseCode indicates the result. 
	SYNTROCFS_SUCCESS indicates that the request was successful and fileData contains the file 
	data with length bytes. Any other value means that the read request failed.

	If the request was successful, the memory associated with fileData must be freed at some point by the client app.
*/

void Endpoint::CFSReadAtIndexResponse(int serviceEP, int handle, unsigned int, unsigned int, unsigned char *fileData, int)
{
	logDebug(QString("Default CFSReadAtIndexResponse called %1 %2").arg(serviceEP).arg(handle));
	if (fileData != NULL)
		free(fileData);
}

/*!
	\internal
*/

void Endpoint::CFSProcessWriteAtIndexResponse(SYNTRO_CFSHEADER *cfsHdr, int dstPort)
{
	SYNTRO_CFS_FILE *scf;
	SYNTROCFS_CLIENT_EPINFO *EP;
	int handle;
	int responseCode;

	EP = cfsEPInfo + dstPort;
	handle = SyntroUtils::convertUC2ToUInt(cfsHdr->cfsClientHandle);		// get the client handle

	if (handle >= SYNTROCFS_MAX_CLIENT_FILES) {
		logWarn(QString("WriteAtIndex response with invalid handle %1 on port %2").arg(handle).arg(dstPort));
		return;
	}
	scf = EP->cfsFile + handle;							// get the stream slot pointer
	if (!scf->open) {
		logWarn(QString("WriteAtIndex response with not open handle %1 on port %2").arg(handle).arg(dstPort));
		return;
	}
	if (!scf->writeInProgress) {
		logWarn(QString("WriteAtIndex response but no read in progress on handle %1 port %2").arg(handle).arg(dstPort));
		return;
	}
	scf->writeInProgress = false;
	responseCode = SyntroUtils::convertUC2ToUInt(cfsHdr->cfsParam);		// get the response code
#ifdef CFS_TRACE
	TRACE3("Got WriteAtIndex response on handle %d port %d code %d", handle, dstPort, responseCode);
#endif
	CFSWriteAtIndexResponse(dstPort, handle, SyntroUtils::convertUC4ToInt(cfsHdr->cfsIndex), responseCode); 
}

/*!
	This client app override is called when a write response for the file associated with handle 
	on service port \a serviceEP has been received or else has timed out. \a responseCode indicates the 
	result. SYNTROCFS_SUCCESS indicates that the request was successful. Any other value means that the write request failed.
*/

void Endpoint::CFSWriteAtIndexResponse(int serviceEP, int handle, unsigned int, unsigned int)
{
	logDebug(QString("Default CFSWriteAtIndexResponse called %1 %2").arg(serviceEP).arg(handle));
}

void Endpoint::CFSProcessQueryResponse(SYNTRO_CFSHEADER *cfsHdr, int dstPort)
{
	SYNTROCFS_CLIENT_EPINFO *EP = cfsEPInfo + dstPort;
	
	int handle = SyntroUtils::convertUC2ToUInt(cfsHdr->cfsClientHandle);

	if (handle >= SYNTROCFS_MAX_CLIENT_FILES) {
		logWarn(QString("Query response with invalid handle %1 on port %2").arg(handle).arg(dstPort));
		return;
	}

	SYNTRO_CFS_FILE *scf = EP->cfsFile + handle;

	if (!scf->open) {
		logWarn(QString("Query response on closed handle %1 on port %2").arg(handle).arg(dstPort));
		return;
	}

	if (!scf->queryInProgress) {
		logWarn(QString("Query response but no query in progress on handle %1 port %2").arg(handle).arg(dstPort));
		return;
	}

	scf->queryInProgress = false;
	
	int responseCode = SyntroUtils::convertUC2ToUInt(cfsHdr->cfsParam);

#ifdef CFS_TRACE
	TRACE3("Got query response on handle %d port %d code %d", handle, dstPort, responseCode);
#endif

	CFSQueryResponse(dstPort, handle, responseCode); 
}

void Endpoint::CFSQueryResponse(int serviceEP, int handle, unsigned int responseCode)
{
	logDebug(QString("Default CFSQueryResponse called %1 %2 responseCode %3")
		.arg(serviceEP).arg(handle).arg(responseCode));
}

void Endpoint::CFSProcessCancelQueryResponse(SYNTRO_CFSHEADER *cfsHdr, int dstPort)
{
	SYNTROCFS_CLIENT_EPINFO *EP = cfsEPInfo + dstPort;
	
	int handle = SyntroUtils::convertUC2ToUInt(cfsHdr->cfsClientHandle);

	if (handle >= SYNTROCFS_MAX_CLIENT_FILES) {
		logWarn(QString("Cancel query response with invalid handle %1 on port %2").arg(handle).arg(dstPort));
		return;
	}

	SYNTRO_CFS_FILE *scf = EP->cfsFile + handle;

	if (!scf->open) {
		logWarn(QString("Cancel query response on closed handle %1 on port %2").arg(handle).arg(dstPort));
		return;
	}

	if (!scf->cancelQueryInProgress) {
		logWarn(QString("Cancel query response but no cancel in progress on handle %1 port %2").arg(handle).arg(dstPort));
		return;
	}

	scf->cancelQueryInProgress = false;
	
	int responseCode = SyntroUtils::convertUC2ToUInt(cfsHdr->cfsParam);

#ifdef CFS_TRACE
	TRACE3("Got cancel query response on handle %d port %d code %d", handle, dstPort, responseCode);
#endif

	CFSCancelQueryResponse(dstPort, handle, responseCode); 
}

void Endpoint::CFSCancelQueryResponse(int serviceEP, int handle, unsigned int responseCode)
{
	logDebug(QString("Default CFSCancelQueryResponse called %1 %2 responseCode %3")
		.arg(serviceEP).arg(handle).arg(responseCode));
}

void Endpoint::CFSProcessFetchQueryResponse(SYNTRO_CFSHEADER *cfsHdr, int dstPort)
{
	SYNTROCFS_CLIENT_EPINFO *EP = cfsEPInfo + dstPort;
	
	int handle = SyntroUtils::convertUC2ToUInt(cfsHdr->cfsClientHandle);

	if (handle >= SYNTROCFS_MAX_CLIENT_FILES) {
		logWarn(QString("Fetch query response with invalid handle %1 on port %2").arg(handle).arg(dstPort));
		return;
	}

	SYNTRO_CFS_FILE *scf = EP->cfsFile + handle;

	if (!scf->open) {
		logWarn(QString("Fetch query response on closed handle %1 on port %2").arg(handle).arg(dstPort));
		return;
	}

	if (!scf->fetchQueryInProgress) {
		logWarn(QString("Fetch query response but no fetch in progress on handle %1 port %2").arg(handle).arg(dstPort));
		return;
	}

	scf->fetchQueryInProgress = false;
	
	int responseCode = SyntroUtils::convertUC2ToUInt(cfsHdr->cfsParam);
	int length = SyntroUtils::convertUC4ToInt(cfsHdr->cfsLength);

	SYNTRO_QUERYRESULT_HEADER *resultHdr = reinterpret_cast<SYNTRO_QUERYRESULT_HEADER *>(cfsHdr);

	int firstRow = SyntroUtils::convertUC4ToInt(resultHdr->firstRow);
	int lastRow = SyntroUtils::convertUC4ToInt(resultHdr->lastRow);
	int param1 = SyntroUtils::convertUC4ToInt(resultHdr->param1);
	int param2 = SyntroUtils::convertUC4ToInt(resultHdr->param2);

	unsigned char *data = reinterpret_cast<unsigned char *>(resultHdr + 1);

#ifdef CFS_TRACE
	TRACE3("Got fetch query response on handle %d port %d code %d", handle, dstPort, responseCode);
#endif

	CFSFetchQueryResponse(dstPort, handle, responseCode, firstRow, lastRow, param1, param2, length, data); 
}

void Endpoint::CFSFetchQueryResponse(int serviceEP, int handle, unsigned int responseCode, int, int, int, int, int, unsigned char *)
{
	logDebug(QString("Default CFSFetchQueryResponse called %1 %2 responseCode %3")
		.arg(serviceEP).arg(handle).arg(responseCode));
}
