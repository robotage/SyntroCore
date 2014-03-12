//
//  Copyright (c) 2014 Scott Ellis and Richard Barnett.
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
#include "CFSDirThread.h"


CFSThread::CFSThread(SyntroCFSClient *parent, QSettings *settings)
	: m_settings(settings), m_parent(parent)
{
	m_storePath = m_settings->value(SYNTRO_CFS_STORE_PATH).toString();
}

CFSThread::~CFSThread()
{
}

void CFSThread::initThread()
{
	CFSInit();
	m_DirThread = new CFSDirThread(this, m_settings);
	m_DirThread->resumeThread();

	m_timer = startTimer(SYNTRO_CLOCKS_PER_SEC);
}

void CFSThread::exitThread()
{
	m_run = false;
	killTimer(m_timer);
	exit();

	if (m_DirThread)
		m_DirThread->exitThread();
}

bool CFSThread::processMessage(SyntroThreadMsg *msg)
{
	switch (msg->message) {
		case SYNTRO_CFS_MESSAGE:
			CFSProcessMessage(msg);
			return true;

		case SYNTROTHREAD_TIMER_MESSAGE:
			CFSBackground();
			return true;

		default:
			return true;
	}
}

void CFSThread::CFSInit()
{
	int i;

	for (i = 0; i < SYNTROCFS_MAX_FILES; i++) {
		m_cfsState[i].inUse = false;
		m_cfsState[i].storeHandle = i;
	}
}


void CFSThread::CFSBackground()
{
	qint64 now = SyntroClock();
	SYNTROCFS_STATE *scs = m_cfsState;

	for (int i = 0; i < SYNTROCFS_MAX_FILES; i++, scs++) {
		if (scs->inUse) {
			if (SyntroUtils::syntroTimerExpired(now, scs->lastKeepalive, SYNTROCFS_KEEPALIVE_TIMEOUT)) {
				TRACE2("Timed out slot %d connected to %s", i, qPrintable(SyntroUtils::displayUID(&scs->clientUID)));
				scs->inUse = false;								// just indicate entry is unused
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
	SYNTRO_CFSHEADER *cfsMsg;
	int length;
	SYNTRO_EHEAD *message;

	message = reinterpret_cast<SYNTRO_EHEAD *>(msg->ptrParam);// this is the message itself
	length = msg->intParam;									// this is the message length (not including the SYNTRO_EHEAD)

	if (length < (int)sizeof(SYNTRO_CFSHEADER)) {
		logWarn(QString("CFS message received is too short %1").arg(length));
		free(message);
		return;
	}

	cfsMsg = reinterpret_cast<SYNTRO_CFSHEADER *>(message+1);		// get the SyntroCFS header pointer

    if (length != (int)(sizeof(SYNTRO_CFSHEADER) + SyntroUtils::convertUC4ToInt(cfsMsg->cfsLength))) {
		logWarn(QString("CFS received message of length %1 but header said length was %2")
			.arg(length).arg(sizeof(SYNTRO_CFSHEADER) + SyntroUtils::convertUC4ToInt(cfsMsg->cfsLength)));

		free (message);
		return;
	}

	QMutexLocker locker(&m_lock);							// avoids issues with status display

	switch (SyntroUtils::convertUC2ToUInt(cfsMsg->cfsType)) {
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
	default:
		logWarn(QString("CFS message received with unrecognized type %1").arg(SyntroUtils::convertUC2ToUInt(cfsMsg->cfsType)));
		free(message);
		break;
	}
}

void CFSThread::CFSDir(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg)
{
	SYNTRO_EHEAD *responseE2E;
	SYNTRO_CFSHEADER *responseHdr;
	char *data;
	int totalLength;

	QString dirString = m_DirThread->getDirectory();							// get the directory info
	
	responseE2E = CFSBuildResponse(ehead, cfsMsg, dirString.length() + 1);		// allow for zero at end of the directory

	responseHdr = reinterpret_cast<SYNTRO_CFSHEADER *>(responseE2E + 1);		// pointer to the new SyntroCFS header

	data = reinterpret_cast<char *>(responseHdr+1);							// where the stream names go
	strcpy(data, qPrintable(dirString));										// copy them in
	
	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_DIR_RES, responseHdr->cfsType);
	SyntroUtils::convertIntToUC2(SYNTROCFS_SUCCESS, responseHdr->cfsParam);
	totalLength = sizeof(SYNTRO_CFSHEADER) + dirString.length() + 1;
	m_parent->clientSendMessage(m_parent->m_CFSPort, responseE2E, totalLength, SYNTROCFS_E2E_PRIORITY);
	TRACE2("Sent directory to %s, length %d", qPrintable(SyntroUtils::displayUID(&ehead->sourceUID)), totalLength);
	free(ehead);
}

void CFSThread::CFSOpen(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg)
{
	char *data;
	SYNTROCFS_STATE *scs;
	int handle;
	int nameLength;
	int totalLength;


	scs = m_cfsState;
	for (handle = 0; handle < SYNTROCFS_MAX_FILES; handle++, scs++) {
		if (!scs->inUse)
			break;
	}
	if (handle < SYNTROCFS_MAX_FILES) {						// found a free slot
		data = reinterpret_cast<char *>(cfsMsg + 1);		// get pointer to stream name
		nameLength = SyntroUtils::convertUC4ToInt(cfsMsg->cfsLength);	// get the length of the name
		if (nameLength > 0) {								// it's non-zero in length
			scs->inUse = true;								// flags as in use
			scs->rxBytes = 0;
			scs->txBytes = 0;
			scs->lastStatusEmit = SyntroClock();
			scs->lastKeepalive = SyntroClock();
			data[nameLength-1] = 0;							// make sure it's zero terminated
			scs->filePath.clear();
			scs->filePath = m_storePath + "/" + data;		// set the filepath
			scs->structured = scs->filePath.endsWith(SYNTRO_RECORD_SRF_RECORD_DOTEXT);
			if (scs->structured) {
				scs->indexPath = scs->filePath;
				scs->indexPath.truncate(scs->indexPath.length() - (int)strlen(SYNTRO_RECORD_SRF_RECORD_DOTEXT)); // take off original extension
				scs->indexPath += SYNTRO_RECORD_SRF_INDEX_DOTEXT;
			}
			scs->clientUID = ehead->sourceUID;					// save the source UID
			scs->clientPort = SyntroUtils::convertUC2ToUInt(ehead->sourcePort); // and the port
			scs->clientHandle = SyntroUtils::convertUC2ToUInt(cfsMsg->cfsClientHandle); // save the client's handle
			scs->blockSize = SyntroUtils::convertUC2ToInt(cfsMsg->cfsParam);	// record the block size
			SyntroUtils::convertIntToUC2(scs->storeHandle, cfsMsg->cfsStoreHandle);
			SyntroUtils::convertIntToUC2(SYNTROCFS_SUCCESS, cfsMsg->cfsParam);
			scs->fileLength = getFileSize(scs);
			SyntroUtils::convertIntToUC4(scs->fileLength, cfsMsg->cfsIndex);	// set the file size
			emit newStatus(handle, scs);
		} else {
			SyntroUtils::convertIntToUC2(SYNTROCFS_ERROR_FILE_NOT_FOUND, cfsMsg->cfsParam);
		}

	} else {
		SyntroUtils::convertIntToUC2(SYNTROCFS_ERROR_MAX_STORE_FILES, cfsMsg->cfsParam);
	}
	SyntroUtils::swapEHead(ehead);										// get ready to return a response
	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_OPEN_RES, cfsMsg->cfsType); // set response type
	SyntroUtils::convertIntToUC4(0, cfsMsg->cfsLength);

	totalLength = sizeof(SYNTRO_CFSHEADER);
	TRACE2("Sent open response to %s, response %d", qPrintable(SyntroUtils::displayUID(&ehead->sourceUID)), 
		SyntroUtils::convertUC2ToInt(cfsMsg->cfsParam));
	m_parent->clientSendMessage(m_parent->m_CFSPort, ehead, totalLength, SYNTROCFS_E2E_PRIORITY);
}

void CFSThread::CFSClose(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg)
{
	int handle;
	SYNTROCFS_STATE *scs;

	if (!CFSSanityCheck(ehead, cfsMsg)) {
		free(ehead);												// failed sanity check - just discard
		return;
	}
	handle = SyntroUtils::convertUC2ToUInt(cfsMsg->cfsStoreHandle);			// get my handle
	scs = m_cfsState + handle;									// get the stream data
	SyntroUtils::swapEHead(ehead);											// get ready to return a response
	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_CLOSE_RES, cfsMsg->cfsType); // set response type
	SyntroUtils::convertIntToUC4(0, cfsMsg->cfsLength);
	SyntroUtils::convertIntToUC2(SYNTROCFS_SUCCESS, cfsMsg->cfsParam);
	TRACE2("Sent close response to %s, response %d", 
		qPrintable(SyntroUtils::displayUID(&ehead->sourceUID)), 
		SyntroUtils::convertUC2ToInt(cfsMsg->cfsParam));
	m_parent->clientSendMessage(m_parent->m_CFSPort, ehead, sizeof(SYNTRO_CFSHEADER), SYNTROCFS_E2E_PRIORITY);
	scs->inUse = false;
	emit newStatus(handle, scs);
}

void CFSThread::CFSKeepAlive(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg)
{
	int handle;
	SYNTROCFS_STATE *scs;

	if (!CFSSanityCheck(ehead, cfsMsg)) {
		free(ehead);												// failed sanity check - just discard
		return;
	}
	handle = SyntroUtils::convertUC2ToUInt(cfsMsg->cfsStoreHandle);			// get my handle
	scs = m_cfsState + handle;									// get the stream data

	scs->lastKeepalive = SyntroClock();							// refresh timer

	SyntroUtils::swapEHead(ehead);											// get ready to return a response
	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_KEEPALIVE_RES, cfsMsg->cfsType); // set response type
	SyntroUtils::convertIntToUC4(0, cfsMsg->cfsLength);

	TRACE2("Sent keep alive response to %s, response %d", qPrintable(SyntroUtils::displayUID(&ehead->destUID)), 
		SyntroUtils::convertUC2ToInt(cfsMsg->cfsParam));
	m_parent->clientSendMessage(m_parent->m_CFSPort, ehead, sizeof(SYNTRO_CFSHEADER), SYNTROCFS_E2E_PRIORITY);
}

void CFSThread::CFSReadIndex(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg)
{
	int handle;
	int requestedIndex;
	SYNTROCFS_STATE *scs;

	if (!CFSSanityCheck(ehead, cfsMsg)) {
		free(ehead);												// failed sanity check - just discard
		return;
	}
	handle = SyntroUtils::convertUC2ToUInt(cfsMsg->cfsStoreHandle);			// get my handle
	scs = m_cfsState + handle;									// get the stream data
	requestedIndex = SyntroUtils::convertUC4ToInt(cfsMsg->cfsIndex);			// get the requested index
	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_READ_INDEX_RES, cfsMsg->cfsType); // preset type for error response
	if (scs->structured)
		CFSStructuredFileRead(ehead, cfsMsg, scs, requestedIndex);			
	else
		CFSFlatFileRead(ehead, cfsMsg, scs, requestedIndex);			
	free(ehead);
}

void CFSThread::CFSWriteIndex(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg)
{
	int handle;
	int requestedIndex;
	SYNTROCFS_STATE *scs;

	if (!CFSSanityCheck(ehead, cfsMsg)) {
		free(ehead);												// failed sanity check - just discard
		return;
	}
	handle = SyntroUtils::convertUC2ToUInt(cfsMsg->cfsStoreHandle);			// get my handle
	scs = m_cfsState + handle;									// get the stream data
	requestedIndex = SyntroUtils::convertUC4ToInt(cfsMsg->cfsIndex);			// get the requested index
	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_READ_INDEX_RES, cfsMsg->cfsType); // preset type for error response
	if (scs->structured)
		CFSStructuredFileWrite(ehead, cfsMsg, scs, requestedIndex);			
	else
		CFSFlatFileWrite(ehead, cfsMsg, scs, requestedIndex);			
	free(ehead);
}

SYNTRO_EHEAD *CFSThread::CFSBuildResponse(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *, int length)
{
	SYNTRO_EHEAD *responseE2E;
	SYNTRO_CFSHEADER *responseHdr;	


	responseE2E = m_parent->clientBuildLocalE2EMessage(m_parent->m_CFSPort, &(ehead->sourceUID), 
		SyntroUtils::convertUC2ToUInt(ehead->sourcePort), sizeof(SYNTRO_CFSHEADER) + length);
	responseHdr = (SYNTRO_CFSHEADER *)(responseE2E+1);			// pointer to the new SyntroCFS header
	memset(responseHdr, 0, sizeof(SYNTRO_CFSHEADER));
	SyntroUtils::convertIntToUC4(length, responseHdr->cfsLength);
	return responseE2E;
}

bool CFSThread::CFSSanityCheck(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg)
{
	int handle;
	SYNTROCFS_STATE *scs;

	handle = SyntroUtils::convertUC2ToUInt(cfsMsg->cfsStoreHandle);			// get my handle
	if (handle >= SYNTROCFS_MAX_FILES) {
		logWarn(QString("Request from %1 on out of range handle %2")
			.arg(SyntroUtils::displayUID(&ehead->sourceUID)).arg(handle));
		
		return false;
	}
	scs = m_cfsState + handle;									// get the stream data
	if (!scs->inUse) {
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
	SyntroUtils::swapEHead(ehead);							// swap fields for return
	SyntroUtils::convertIntToUC4(0, cfsMsg->cfsLength);
	SyntroUtils::convertIntToUC2(responseCode, cfsMsg->cfsParam);
	TRACE2("Sent error response to %s, response %d", qPrintable(SyntroUtils::displayUID(&ehead->destUID)), 
		SyntroUtils::convertUC2ToInt(cfsMsg->cfsParam));
	m_parent->clientSendMessage(m_parent->m_CFSPort, ehead, sizeof(SYNTRO_CFSHEADER), SYNTROCFS_E2E_PRIORITY);
}

bool CFSThread::CFSStructuredFileRead(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg, SYNTROCFS_STATE *scs, unsigned int requestedIndex)
{
	qint64 rpos;
	qint64 fileLength;
	SYNTRO_STORE_RECORD_HEADER	cHead;
	char *data;
	int totalLength;
	qint64 now;

	int responseCode = SYNTROCFS_SUCCESS;
	int recordLength = 0;
	SYNTRO_CFSHEADER *responseHdr = NULL;
	SYNTRO_EHEAD *responseE2E = NULL;

	QFile	xf(scs->indexPath);
	QFile	rf(scs->filePath);

	if (!xf.open(QIODevice::ReadOnly)) {
		responseCode = SYNTROCFS_ERROR_INDEX_FILE_NOT_FOUND;
		goto sendResponse;
	}
	fileLength = xf.size() / (sizeof (qint64));
	if (requestedIndex >= fileLength) {
		responseCode = SYNTROCFS_ERROR_INVALID_RECORD_INDEX;
		goto sendResponse;
	}
	xf.seek(requestedIndex * sizeof (qint64));	// go to correct place in index file

	if ((int)xf.read((char *)(&rpos), sizeof(qint64)) != sizeof(qint64)) {
		responseCode = SYNTROCFS_ERROR_READING_INDEX_FILE;
		goto sendResponse;
	}
	xf.close();											// don't need index file
	if (!rf.open(QIODevice::ReadOnly)) {
		responseCode = SYNTROCFS_ERROR_FILE_NOT_FOUND;
		goto sendResponse;
	}
	if (!rf.seek(rpos))	{
		responseCode = SYNTROCFS_ERROR_RECORD_SEEK;
		goto sendResponse;
	}

	if (rf.read((char *)&cHead, sizeof (SYNTRO_STORE_RECORD_HEADER)) != sizeof (SYNTRO_STORE_RECORD_HEADER)) {
		responseCode = SYNTROCFS_ERROR_RECORD_READ;
		goto sendResponse;
	}
	if (strncmp(SYNC_STRINGV0, cHead.sync, SYNC_LENGTH) != 0) {
		responseCode = SYNTROCFS_ERROR_INVALID_HEADER;
		goto sendResponse;
	}

	recordLength = SyntroUtils::convertUC4ToInt(cHead.size);
	scs->txBytes += recordLength;

	now = SyntroClock();
	if (SyntroUtils::syntroTimerExpired(now, scs->lastStatusEmit, SYNTROCFS_STATUS_INTERVAL)) {
		emit newStatus(scs->storeHandle, scs);
		scs->lastStatusEmit = now;
	}

	responseE2E = CFSBuildResponse(ehead, cfsMsg, recordLength);		

	responseHdr = reinterpret_cast<SYNTRO_CFSHEADER *>(responseE2E + 1);		// pointer to the new SyntroCFS header

	data = reinterpret_cast<char *>(responseHdr+1);				
	if (rf.read(data, recordLength) != recordLength) {
		responseCode = SYNTROCFS_ERROR_RECORD_READ;
		goto sendResponse;
	}
	rf.close();
	
sendResponse:
	if (responseE2E == NULL) {
		responseE2E = CFSBuildResponse(ehead, cfsMsg, 0);	// error response
		recordLength = 0;
		responseHdr = reinterpret_cast<SYNTRO_CFSHEADER *>(responseE2E + 1);		// pointer to the new SyntroCFS header
	}

	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_READ_INDEX_RES, responseHdr->cfsType);
	SyntroUtils::convertIntToUC2(responseCode, responseHdr->cfsParam);
	SyntroUtils::convertIntToUC4(requestedIndex, responseHdr->cfsIndex);
	totalLength = sizeof(SYNTRO_CFSHEADER) + recordLength;
	m_parent->clientSendMessage(m_parent->m_CFSPort, responseE2E, totalLength, SYNTROCFS_E2E_PRIORITY);
	TRACE2("Sent record to %s, length %d", qPrintable(SyntroUtils::displayUID(&ehead->sourceUID)), totalLength);
	return true;
}

bool CFSThread::CFSStructuredFileWrite(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg, SYNTROCFS_STATE *scs, unsigned int requestedIndex)
{
	SYNTRO_STORE_RECORD_HEADER cHeadV0;
	qint64 pos;
	unsigned int responseCode = SYNTROCFS_SUCCESS;
	int length;
	SYNTRO_CFSHEADER *responseHdr = NULL;
	SYNTRO_EHEAD *responseE2E = NULL;
	int totalLength;

	QFile	xf(scs->indexPath);
	QFile	rf(scs->filePath);

	if (requestedIndex == 0) {								// must reset files first
		xf.remove();
		rf.remove();
	}

	if (!xf.open(QIODevice::Append)) {
		responseCode = SYNTROCFS_ERROR_INDEX_FILE_NOT_FOUND;
		goto sendResponse;
	}

	if (!rf.open(QIODevice::Append)) {
		responseCode = SYNTROCFS_ERROR_FILE_NOT_FOUND;
		xf.close();
		goto sendResponse;
	}
	length = SyntroUtils::convertUC4ToInt(cfsMsg->cfsLength);
	scs->rxBytes += length;
	strncpy(cHeadV0.sync, SYNC_STRINGV0, SYNC_LENGTH);
	SyntroUtils::convertIntToUC4(0, cHeadV0.data);
	SyntroUtils::convertIntToUC4(length, cHeadV0.size);

	pos = rf.pos();
	if (xf.write((char *)&pos, sizeof(qint64)) != sizeof(qint64)) {
		xf.close();
		rf.close();
		responseCode = SYNTROCFS_ERROR_INDEX_WRITE;
		goto sendResponse;
	}
	xf.close();

	if (rf.write((char *)(&cHeadV0), sizeof(SYNTRO_STORE_RECORD_HEADER)) != sizeof(SYNTRO_STORE_RECORD_HEADER)) {
		rf.close();
		responseCode = SYNTROCFS_ERROR_WRITE;
		goto sendResponse;
	}

	if (rf.write(reinterpret_cast<char *>(cfsMsg + 1), length) != length) { 
		rf.close();
		responseCode = SYNTROCFS_ERROR_WRITE;
		goto sendResponse;
	}
	rf.close();

sendResponse:
	responseE2E = CFSBuildResponse(ehead, cfsMsg, 0);		
	responseHdr = reinterpret_cast<SYNTRO_CFSHEADER *>(responseE2E + 1);		// pointer to the new SyntroCFS header
	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_WRITE_INDEX_RES, responseHdr->cfsType);
	SyntroUtils::convertIntToUC2(responseCode, responseHdr->cfsParam);
	SyntroUtils::convertIntToUC4(requestedIndex, responseHdr->cfsIndex);
	totalLength = sizeof(SYNTRO_CFSHEADER);
	m_parent->clientSendMessage(m_parent->m_CFSPort, responseE2E, totalLength, SYNTROCFS_E2E_PRIORITY);
	TRACE2("Wrote record from %s, length %d", qPrintable(SyntroUtils::displayUID(&ehead->sourceUID)), length);
	return true;
}

bool CFSThread::CFSFlatFileRead(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg, SYNTROCFS_STATE *scs, unsigned int requestedIndex)
{
	int responseCode = SYNTROCFS_SUCCESS;
	int totalLength;
	int length = 0;
	SYNTRO_CFSHEADER *responseHdr = NULL;
	SYNTRO_EHEAD *responseE2E = NULL;
	qint64 bpos;
	char *fileData;

	QFile	ff(scs->filePath);

	if (!ff.open(QIODevice::ReadOnly)) {
		responseCode = SYNTROCFS_ERROR_FILE_NOT_FOUND;
		goto sendResponse;
	}

	bpos = (qint64)scs->blockSize * (qint64)SyntroUtils::convertUC4ToInt(cfsMsg->cfsIndex);	// byte position in file

	if (!ff.seek(bpos))	{
		responseCode = SYNTROCFS_ERROR_RECORD_SEEK;
		ff.close();
		goto sendResponse;
	}

	length = scs->blockSize * SyntroUtils::convertUC2ToInt(cfsMsg->cfsParam);
	scs->txBytes += length;

	if (length == 0)
		goto sendResponse;

	if (length > (SYNTRO_MESSAGE_MAX - (int)sizeof(SYNTRO_EHEAD) - (int)sizeof(SYNTRO_CFSHEADER))) {
		responseCode = SYNTROCFS_ERROR_TRANSFER_TOO_LONG;
		ff.close();
		goto sendResponse;

	}

	if (((qint64)length + bpos) > ((qint64)scs->fileLength * scs->blockSize))
		length = (qint64)scs->fileLength * scs->blockSize - bpos;				// adjust for EOF

	responseE2E = CFSBuildResponse(ehead, cfsMsg, length);		

	responseHdr = reinterpret_cast<SYNTRO_CFSHEADER *>(responseE2E + 1);	// pointer to the new SyntroCFS header

	fileData = reinterpret_cast<char *>(responseHdr+1);				
	if (ff.read(fileData, length) != length) {
		responseCode = SYNTROCFS_ERROR_READ;
		ff.close();
		goto sendResponse;
	}
	ff.close();

sendResponse:
	if (responseE2E == NULL) {
		responseE2E = CFSBuildResponse(ehead, cfsMsg, 0);					// error response
		length = 0;
		responseHdr = reinterpret_cast<SYNTRO_CFSHEADER *>(responseE2E + 1);// pointer to the new SyntroCFS header
	}

	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_READ_INDEX_RES, responseHdr->cfsType);
	SyntroUtils::convertIntToUC2(responseCode, responseHdr->cfsParam);
	SyntroUtils::convertIntToUC4(requestedIndex, responseHdr->cfsIndex);
	totalLength = sizeof(SYNTRO_CFSHEADER) + length;
	m_parent->clientSendMessage(m_parent->m_CFSPort, responseE2E, totalLength, SYNTROCFS_E2E_PRIORITY);
	TRACE2("Sent record to %s, length %d", qPrintable(SyntroUtils::displayUID(&ehead->sourceUID)), totalLength);
	return true;
}

bool CFSThread::CFSFlatFileWrite(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg, SYNTROCFS_STATE *scs, unsigned int requestedIndex)
{
	unsigned int responseCode = SYNTROCFS_SUCCESS;
	SYNTRO_CFSHEADER *responseHdr = NULL;
	SYNTRO_EHEAD *responseE2E = NULL;
	int totalLength;
	int length;

	QFile	ff(scs->filePath);

	if (requestedIndex == 0)
		ff.remove();										// index 0 means delete old file as new one starting

	if (!ff.open(QIODevice::Append)) {
		responseCode = SYNTROCFS_ERROR_FILE_NOT_FOUND;
		ff.close();
		goto sendResponse;
	}
	length = SyntroUtils::convertUC4ToInt(cfsMsg->cfsLength);
	scs->rxBytes += length;
	if (ff.write(reinterpret_cast<char *>(cfsMsg + 1), length) != length) { 
		ff.close();
		responseCode = SYNTROCFS_ERROR_WRITE;
		goto sendResponse;
	}
	ff.close();

sendResponse:
	responseE2E = CFSBuildResponse(ehead, cfsMsg, 0);		
	responseHdr = reinterpret_cast<SYNTRO_CFSHEADER *>(responseE2E + 1);		// pointer to the new SyntroCFS header
	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_WRITE_INDEX_RES, responseHdr->cfsType);
	SyntroUtils::convertIntToUC2(responseCode, responseHdr->cfsParam);
	SyntroUtils::convertIntToUC4(requestedIndex, responseHdr->cfsIndex);
	totalLength = sizeof(SYNTRO_CFSHEADER);
	m_parent->clientSendMessage(m_parent->m_CFSPort, responseE2E, totalLength, SYNTROCFS_E2E_PRIORITY);
	TRACE2("Wrote record from %s, length %d", qPrintable(SyntroUtils::displayUID(&ehead->sourceUID)), length);
	return true;
}



unsigned int CFSThread::getFileSize(SYNTROCFS_STATE *scs)
{
	if (scs->structured)
		return getStructuredFileSize(scs);
	else
		return getFlatFileSize(scs);
}

unsigned int CFSThread::getStructuredFileSize(SYNTROCFS_STATE *scs)
{
	QFile	xf(scs->indexPath);
	unsigned int length;

	if (!xf.open(QIODevice::ReadOnly))
		return 0;
	length = (unsigned int)(xf.size() / (sizeof (qint64)));
	xf.close();
	return length;
}

unsigned int CFSThread::getFlatFileSize(SYNTROCFS_STATE *scs)
{
	QFile	xf(scs->filePath);

	if (scs->blockSize != 0)
		return (unsigned int)(xf.size() / scs->blockSize);
	else
		return 0;
}
