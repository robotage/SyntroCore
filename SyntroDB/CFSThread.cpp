//
//  Copyright (c) 2014 Scott Ellis and Richard Barnett
//	
//  This file is part of SyntroNet
//
//  SyntroNet is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  SyntroNet is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with SyntroNet.  If not, see <http://www.gnu.org/licenses/>.
//

#include "CFSThread.h"
#include "DirThread.h"
#include "SyntroDB.h"
#include "CFSClient.h"
#include "SyntroCFS.h"
#include "SyntroCFSRaw.h"
#include "SyntroCFSStructured.h"

//#define CFS_THREAD_TRACE

CFSThread::CFSThread(CFSClient *parent)
    : SyntroThread(QString("CFSThread"), QString(COMPTYPE_CFS)), m_parent(parent)
{
}

CFSThread::~CFSThread()
{
}

void CFSThread::initThread()
{
	QSettings *settings = SyntroUtils::getSettings();

	m_storePath = settings->value(SYNTRODB_PARAMS_ROOT_DIRECTORY).toString();

	delete settings;

	CFSInit();

	m_DirThread = new DirThread(m_storePath);
	m_DirThread->resumeThread();

	m_timer = startTimer(SYNTRO_CLOCKS_PER_SEC);
}

void CFSThread::finishThread()
{
	killTimer(m_timer);
	
	if (m_DirThread)
		m_DirThread->exitThread();
}

void CFSThread::timerEvent(QTimerEvent *)
{
	CFSBackground();
}

bool CFSThread::processMessage(SyntroThreadMsg *msg)
{
	if (msg->message == SYNTRO_CFS_MESSAGE)
		CFSProcessMessage(msg);

	return true;
}

void CFSThread::CFSInit()
{
	for (int i = 0; i < SYNTROCFS_MAX_FILES; i++) {
		m_cfsState[i].inUse = false;
		m_cfsState[i].storeHandle = i;
		m_cfsState[i].agent = NULL;
	}
}

void CFSThread::CFSBackground()
{
	qint64 now = SyntroClock();
	SYNTROCFS_STATE *scs = m_cfsState;

	for (int i = 0; i < SYNTROCFS_MAX_FILES; i++, scs++) {
		if (scs->inUse) {
			if (SyntroUtils::syntroTimerExpired(now, scs->lastKeepalive, SYNTROCFS_KEEPALIVE_TIMEOUT)) {
#ifdef CFS_THREAD_TRACE
				TRACE2("Timed out slot %d connected to %s", i, qPrintable(SyntroUtils::displayUID(&scs->clientUID)));
#endif
				scs->inUse = false;
				
				if (scs->agent) {
					delete scs->agent;
					scs->agent = NULL;
				}

				emit newStatus(scs->storeHandle, scs);
			}

			if (SyntroUtils::syntroTimerExpired(now, scs->lastStatusEmit, SYNTROCFS_STATUS_INTERVAL)) {
				emit newStatus(scs->storeHandle, scs);
				scs->lastStatusEmit = now;
			}
		}
	}
}

void CFSThread::CFSProcessMessage(SyntroThreadMsg *msg)
{
	// this is the message itself
	SYNTRO_EHEAD *message = reinterpret_cast<SYNTRO_EHEAD *>(msg->ptrParam);
	
	// this is the message length (not including the SYNTRO_EHEAD)
	int length = msg->intParam;

	if (length < (int)sizeof(SYNTRO_CFSHEADER)) {
		logWarn(QString("CFS message received is too short %1").arg(length));
		free(message);
		return;
	}

	SYNTRO_CFSHEADER *cfsMsg = reinterpret_cast<SYNTRO_CFSHEADER *>(message + 1);

    if (length != (int)(sizeof(SYNTRO_CFSHEADER) + SyntroUtils::convertUC4ToInt(cfsMsg->cfsLength))) {
		logWarn(QString("CFS received message of length %1 but header said length was %2")
			.arg(length).arg(sizeof(SYNTRO_CFSHEADER) + SyntroUtils::convertUC4ToInt(cfsMsg->cfsLength)));

		free (message);
		return;
	}

	int cfsType = SyntroUtils::convertUC2ToUInt(cfsMsg->cfsType);

	switch (cfsType) {
	case SYNTROCFS_TYPE_DIR_REQ:
	case SYNTROCFS_TYPE_OPEN_REQ:
		break;

	case SYNTROCFS_TYPE_CLOSE_REQ:
	case SYNTROCFS_TYPE_KEEPALIVE_REQ:
	case SYNTROCFS_TYPE_READ_INDEX_REQ:
	case SYNTROCFS_TYPE_WRITE_INDEX_REQ:
	case SYNTROCFS_TYPE_QUERY_REQ:
	case SYNTROCFS_TYPE_CANCEL_QUERY_REQ:
	case SYNTROCFS_TYPE_FETCH_QUERY_REQ:
		if (!CFSSanityCheck(message, cfsMsg)) {
			free(message);
			return;
		}

		break;

	default:
		logWarn(QString("CFS message received with unrecognized type %1").arg(cfsType));
		free(message);
		return;
	}

	// avoids issues with status display
	QMutexLocker locker(&m_lock);

	switch (cfsType) {
	case SYNTROCFS_TYPE_DIR_REQ:
		CFSDir(message, cfsMsg);
		break;
	case SYNTROCFS_TYPE_OPEN_REQ:
		CFSOpen(message, cfsMsg);
		break;
	case SYNTROCFS_TYPE_CLOSE_REQ:
		CFSClose(message, cfsMsg);
		break;
	case SYNTROCFS_TYPE_KEEPALIVE_REQ:
		CFSKeepAlive(message, cfsMsg);
		break;
	case SYNTROCFS_TYPE_READ_INDEX_REQ:
		CFSReadIndex(message, cfsMsg);
		break;
	case SYNTROCFS_TYPE_WRITE_INDEX_REQ:
		CFSWriteIndex(message, cfsMsg);
		break;
	case SYNTROCFS_TYPE_QUERY_REQ:
		CFSQuery(message, cfsMsg);
		break;
	case SYNTROCFS_TYPE_CANCEL_QUERY_REQ:
		CFSCancelQuery(message, cfsMsg);
		break;
	case SYNTROCFS_TYPE_FETCH_QUERY_REQ:
		CFSFetchQuery(message, cfsMsg);
		break;
	}
}

void CFSThread::CFSDir(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg)
{
	QString dirString;

	int cfsParam = SyntroUtils::convertUC2ToInt(cfsMsg->cfsParam);

	if (cfsParam == SYNTROCFS_DIR_PARAM_LIST_ALL)
		dirString = m_DirThread->getDirectory();
	else if (cfsParam == SYNTROCFS_DIR_PARAM_LIST_DB_ONLY)
		dirString = "syntro";

#ifdef CFS_THREAD_TRACE	
	qDebug() << "dirString: " << dirString;
#endif

	// allow for zero at end of the directory
	SYNTRO_EHEAD *responseE2E = CFSBuildResponse(ehead, cfsMsg, dirString.length() + 1);

	SYNTRO_CFSHEADER *responseHdr = reinterpret_cast<SYNTRO_CFSHEADER *>(responseE2E + 1);

	char *data = reinterpret_cast<char *>(responseHdr + 1);
	strcpy(data, qPrintable(dirString));
	
	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_DIR_RES, responseHdr->cfsType);
	SyntroUtils::convertIntToUC2(SYNTROCFS_SUCCESS, responseHdr->cfsParam);
	
	int totalLength = sizeof(SYNTRO_CFSHEADER) + dirString.length() + 1;
	m_parent->sendMessage(responseE2E, totalLength);

#ifdef CFS_THREAD_TRACE
	TRACE2("Sent directory to %s, length %d", qPrintable(SyntroUtils::displayUID(&ehead->sourceUID)), totalLength);
#endif

	free(ehead);
}

void CFSThread::CFSOpen(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg)
{
	int handle;
	unsigned int recordCount = 0;
	char *data = NULL;
	int nameLength = 0;
	int cfsMode = SYNTROCFS_TYPE_STRUCTURED_FILE;

	SYNTROCFS_STATE *scs = m_cfsState;

	for (handle = 0; handle < SYNTROCFS_MAX_FILES; handle++, scs++) {
		if (!scs->inUse)
			break;
	}

	if (handle >= SYNTROCFS_MAX_FILES) {
		SyntroUtils::convertIntToUC2(SYNTROCFS_ERROR_MAX_STORE_FILES, cfsMsg->cfsParam);
		goto CFSOPEN_DONE;
	}

	nameLength = SyntroUtils::convertUC4ToInt(cfsMsg->cfsLength);

	if (nameLength <= 2) {
		SyntroUtils::convertIntToUC2(SYNTROCFS_ERROR_FILE_NOT_FOUND, cfsMsg->cfsParam);
		goto CFSOPEN_DONE;
	}

	data = reinterpret_cast<char *>(cfsMsg + 1);
	data[nameLength - 1] = 0;

	cfsMode = SyntroUtils::convertUC2ToInt(cfsMsg->cfsParam);

	scs->agent = factory(cfsMode, m_parent, m_storePath + "/" + data);

	if (!scs->agent) {
		SyntroUtils::convertIntToUC2(SYNTROCFS_ERROR_MEMORY_ALLOC_FAIL, cfsMsg->cfsParam);
		goto CFSOPEN_DONE;
	}

	if (!scs->agent->cfsOpen(cfsMsg)) {
		delete scs->agent;
		scs->agent = NULL;
		goto CFSOPEN_DONE;
	}

	// save some client info
	scs->rxBytes = 0;
	scs->txBytes = 0;
	scs->lastKeepalive = SyntroClock();
	scs->lastStatusEmit = scs->lastKeepalive;
	scs->clientUID = ehead->sourceUID;					
	scs->clientPort = SyntroUtils::convertUC2ToUInt(ehead->sourcePort);
	scs->clientHandle = SyntroUtils::convertUC2ToUInt(cfsMsg->cfsClientHandle);

	// use the cfsIndex field to return the number of records in the requested store
	// for raw files this is the number of read blocks
	// NA for databases since we don't know the table
	recordCount = scs->agent->cfsGetRecordCount();
	SyntroUtils::convertIntToUC4(recordCount, cfsMsg->cfsIndex);
	qDebug() << "recordCount =" << recordCount;

	// give the client our handle for this connection
	SyntroUtils::convertIntToUC2(scs->storeHandle, cfsMsg->cfsStoreHandle);

	SyntroUtils::convertIntToUC2(SYNTROCFS_SUCCESS, cfsMsg->cfsParam);
	scs->inUse = true;

	emit newStatus(handle, scs);

CFSOPEN_DONE:

	SyntroUtils::swapEHead(ehead);
	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_OPEN_RES, cfsMsg->cfsType);
	SyntroUtils::convertIntToUC4(0, cfsMsg->cfsLength);

	int totalLength = sizeof(SYNTRO_CFSHEADER);
	m_parent->sendMessage(ehead, totalLength);

#ifdef CFS_THREAD_TRACE
	TRACE2("Sent open response to %s, response %d", qPrintable(SyntroUtils::displayUID(&ehead->sourceUID)), 
		SyntroUtils::convertUC2ToInt(cfsMsg->cfsParam));
#endif
}

void CFSThread::CFSClose(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg)
{
	int handle = SyntroUtils::convertUC2ToUInt(cfsMsg->cfsStoreHandle);

	SYNTROCFS_STATE *scs = m_cfsState + handle;

	SyntroUtils::swapEHead(ehead);
	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_CLOSE_RES, cfsMsg->cfsType);
	SyntroUtils::convertIntToUC4(0, cfsMsg->cfsLength);
	SyntroUtils::convertIntToUC2(SYNTROCFS_SUCCESS, cfsMsg->cfsParam);

#ifdef CFS_THREAD_TRACE
	TRACE2("Sent close response to %s, response %d", 
		qPrintable(SyntroUtils::displayUID(&ehead->sourceUID)), 
		SyntroUtils::convertUC2ToInt(cfsMsg->cfsParam));
#endif
	
	m_parent->sendMessage(ehead, sizeof(SYNTRO_CFSHEADER));

	scs->inUse = false;

	if (scs->agent) {
		scs->agent->cfsClose();
		delete scs->agent;
		scs->agent = NULL;
	}

	emit newStatus(handle, scs);
}

void CFSThread::CFSKeepAlive(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg)
{
	int handle = SyntroUtils::convertUC2ToUInt(cfsMsg->cfsStoreHandle);

	SYNTROCFS_STATE *scs = m_cfsState + handle;

	scs->lastKeepalive = SyntroClock();

	SyntroUtils::swapEHead(ehead);
	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_KEEPALIVE_RES, cfsMsg->cfsType);
	SyntroUtils::convertIntToUC4(0, cfsMsg->cfsLength);

#ifdef CFS_THREAD_TRACE
	TRACE2("Sent keep alive response to %s, response %d", qPrintable(SyntroUtils::displayUID(&ehead->destUID)), 
		SyntroUtils::convertUC2ToInt(cfsMsg->cfsParam));
#endif

	m_parent->sendMessage(ehead, sizeof(SYNTRO_CFSHEADER));
}

void CFSThread::CFSReadIndex(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg)
{
	int handle = SyntroUtils::convertUC2ToUInt(cfsMsg->cfsStoreHandle);
	
	SYNTROCFS_STATE *scs = m_cfsState + handle;

	int requestedIndex = SyntroUtils::convertUC4ToInt(cfsMsg->cfsIndex);

	scs->agent->cfsRead(ehead, cfsMsg, scs, requestedIndex);
}

void CFSThread::CFSWriteIndex(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg)
{
	int handle = SyntroUtils::convertUC2ToUInt(cfsMsg->cfsStoreHandle);

	SYNTROCFS_STATE *scs = m_cfsState + handle;

	int requestedIndex = SyntroUtils::convertUC4ToInt(cfsMsg->cfsIndex);

	scs->agent->cfsWrite(ehead, cfsMsg, scs, requestedIndex);
}

void CFSThread::CFSQuery(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg)
{
	int handle = SyntroUtils::convertUC2ToUInt(cfsMsg->cfsStoreHandle);

	SYNTROCFS_STATE *scs = m_cfsState + handle;
	
	scs->agent->cfsQuery(ehead, cfsMsg);
}

void CFSThread::CFSCancelQuery(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg)
{
	int handle = SyntroUtils::convertUC2ToUInt(cfsMsg->cfsStoreHandle);

	SYNTROCFS_STATE *scs = m_cfsState + handle;
	
	scs->agent->cfsCancelQuery(ehead, cfsMsg);
}

void CFSThread::CFSFetchQuery(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg)
{
	int handle = SyntroUtils::convertUC2ToUInt(cfsMsg->cfsStoreHandle);

	SYNTROCFS_STATE *scs = m_cfsState + handle;
	
	scs->agent->cfsFetchQuery(ehead, cfsMsg);
}

SYNTRO_EHEAD *CFSThread::CFSBuildResponse(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *, int length)
{
	SYNTRO_EHEAD *responseE2E = m_parent->clientBuildLocalE2EMessage(m_parent->m_CFSPort, 
                                                                     &(ehead->sourceUID), 
                                                                     SyntroUtils::convertUC2ToUInt(ehead->sourcePort), 
                                                                     sizeof(SYNTRO_CFSHEADER) + length);

	SYNTRO_CFSHEADER *responseHdr = (SYNTRO_CFSHEADER *)(responseE2E + 1);
	
	memset(responseHdr, 0, sizeof(SYNTRO_CFSHEADER));

	SyntroUtils::convertIntToUC4(length, responseHdr->cfsLength);

	return responseE2E;
}

bool CFSThread::CFSSanityCheck(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg)
{
	int handle = SyntroUtils::convertUC2ToUInt(cfsMsg->cfsStoreHandle);

	if (handle >= SYNTROCFS_MAX_FILES) {
		logWarn(QString("Request from %1 on out of range handle %2")
			.arg(SyntroUtils::displayUID(&ehead->sourceUID)).arg(handle));
		
		return false;
	}

	SYNTROCFS_STATE *scs = m_cfsState + handle;

	if (!scs->inUse || !scs->agent) {
		logWarn(QString("Request from %1 on not in use handle %2")
			.arg(SyntroUtils::displayUID(&ehead->sourceUID)).arg(handle));
		
		return false;
	}

	if (!SyntroUtils::compareUID(&(scs->clientUID), &(ehead->sourceUID))) {
		logWarn(QString("Request from %1 doesn't match UID on slot %2") 
			.arg(SyntroUtils::displayUID(&ehead->sourceUID)) 
			.arg(SyntroUtils::displayUID(&scs->clientUID)));

		return false;
	}

	if (scs->clientPort != SyntroUtils::convertUC2ToUInt(ehead->sourcePort)) {
		logWarn(QString("Request from %1 doesn't match port on slot %2 port %3") 
			.arg(SyntroUtils::displayUID(&ehead->sourceUID)) 
			.arg(SyntroUtils::displayUID(&scs->clientUID))  
			.arg(SyntroUtils::convertUC2ToUInt(ehead->sourcePort)));

		return false;
	}

	if (scs->clientHandle != SyntroUtils::convertUC2ToInt(cfsMsg->cfsClientHandle)) {
		logWarn(QString("Request from %1 doesn't match client handle on slot %2 port %3") 
			.arg(SyntroUtils::displayUID(&ehead->sourceUID)) 
			.arg(SyntroUtils::displayUID(&scs->clientUID))
			.arg(SyntroUtils::convertUC2ToUInt(ehead->sourcePort)));

		return false;
	}

	return true;
}

void CFSThread::CFSReturnError(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg, int responseCode)
{
	SyntroUtils::swapEHead(ehead);
	SyntroUtils::convertIntToUC4(0, cfsMsg->cfsLength);
	SyntroUtils::convertIntToUC2(responseCode, cfsMsg->cfsParam);

#ifdef CFS_THREAD_TRACE
	TRACE2("Sent error response to %s, response %d", qPrintable(SyntroUtils::displayUID(&ehead->destUID)), 
		SyntroUtils::convertUC2ToInt(cfsMsg->cfsParam));
#endif

	m_parent->sendMessage(ehead, sizeof(SYNTRO_CFSHEADER));
}

SyntroCFS* CFSThread::factory(int cfsMode, CFSClient *client, QString filePath)
{
	switch (cfsMode) {
	case SYNTROCFS_TYPE_STRUCTURED_FILE:
		return new SyntroCFSStructured(client, filePath);

	case SYNTROCFS_TYPE_RAW_FILE:
		return new SyntroCFSRaw(client, filePath);
	}

	return NULL;	
}
