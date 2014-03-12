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

#include <qfile.h>

#include "CFSClient.h"
#include "SyntroCFSStructured.h"

SyntroCFSStructured::SyntroCFSStructured(CFSClient *client, QString filePath)
	: SyntroCFS(client, filePath)
{
}

SyntroCFSStructured::~SyntroCFSStructured()
{
}

bool SyntroCFSStructured::cfsOpen(SYNTRO_CFSHEADER *)
{
	m_indexPath = m_filePath;
	m_indexPath.truncate(m_indexPath.length() - (int)strlen(SYNTRO_RECORD_SRF_RECORD_DOTEXT));
	m_indexPath += SYNTRO_RECORD_SRF_INDEX_DOTEXT;

	return true;
}

void SyntroCFSStructured::cfsRead(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *, SYNTROCFS_STATE *scs, unsigned int requestedIndex)
{
	qint64 rpos;
	SYNTRO_STORE_RECORD_HEADER	cHead;
	int responseCode = SYNTROCFS_SUCCESS;
	int recordLength = 0;
	SYNTRO_CFSHEADER *responseHdr = NULL;
	SYNTRO_EHEAD *responseE2E = NULL;
	qint64 fileLength = 0;
	qint64 now = 0;
	char *data = NULL;

	QFile xf(m_indexPath);
	QFile rf(m_filePath);

	if (!xf.open(QIODevice::ReadOnly)) {
		responseCode = SYNTROCFS_ERROR_INDEX_FILE_NOT_FOUND;
		goto sendResponse;
	}

	fileLength = xf.size() / (sizeof (qint64));

	if (requestedIndex >= fileLength) {
		responseCode = SYNTROCFS_ERROR_INVALID_RECORD_INDEX;
		goto sendResponse;
	}

	xf.seek(requestedIndex * sizeof (qint64));

	if ((int)xf.read((char *)(&rpos), sizeof(qint64)) != sizeof(qint64)) {
		responseCode = SYNTROCFS_ERROR_READING_INDEX_FILE;
		goto sendResponse;
	}

	xf.close();

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
		//emit newStatus(scs->storeHandle, scs);
		scs->lastStatusEmit = now;
	}

	responseE2E = cfsBuildResponse(ehead, recordLength);		

	responseHdr = reinterpret_cast<SYNTRO_CFSHEADER *>(responseE2E + 1);

	data = reinterpret_cast<char *>(responseHdr + 1);				

	if (rf.read(data, recordLength) != recordLength) {
		responseCode = SYNTROCFS_ERROR_RECORD_READ;
		goto sendResponse;
	}

	rf.close();
	
sendResponse:

	if (responseE2E == NULL) {
		responseE2E = cfsBuildResponse(ehead, 0);
		recordLength = 0;
		responseHdr = reinterpret_cast<SYNTRO_CFSHEADER *>(responseE2E + 1);
	}

	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_READ_INDEX_RES, responseHdr->cfsType);
	SyntroUtils::convertIntToUC2(responseCode, responseHdr->cfsParam);
	SyntroUtils::convertIntToUC4(requestedIndex, responseHdr->cfsIndex);
	
	int totalLength = sizeof(SYNTRO_CFSHEADER) + recordLength;
	m_parent->sendMessage(responseE2E, totalLength);

#ifdef CFS_THREAD_TRACE
	TRACE2("Sent record to %s, length %d", qPrintable(SyntroUtils::displayUID(&ehead->sourceUID)), totalLength);
#endif

	free(ehead);
}

void SyntroCFSStructured::cfsWrite(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg, SYNTROCFS_STATE *scs, unsigned int requestedIndex)
{
	SYNTRO_STORE_RECORD_HEADER cHeadV0;
	unsigned int responseCode = SYNTROCFS_SUCCESS;
	int length = 0;
	SYNTRO_CFSHEADER *responseHdr = NULL;
	SYNTRO_EHEAD *responseE2E = NULL;
	qint64 pos = 0;

	QFile xf(m_indexPath);
	QFile rf(m_filePath);

	// delete first if starting at zero
	if (requestedIndex == 0) {
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

	responseE2E = cfsBuildResponse(ehead, 0);		
	
	responseHdr = reinterpret_cast<SYNTRO_CFSHEADER *>(responseE2E + 1);
	
	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_WRITE_INDEX_RES, responseHdr->cfsType);
	SyntroUtils::convertIntToUC2(responseCode, responseHdr->cfsParam);
	SyntroUtils::convertIntToUC4(requestedIndex, responseHdr->cfsIndex);
	
	int totalLength = sizeof(SYNTRO_CFSHEADER);
	m_parent->sendMessage(responseE2E, totalLength);

#ifdef CFS_THREAD_TRACE
	TRACE2("Wrote record from %s, length %d", qPrintable(SyntroUtils::displayUID(&ehead->sourceUID)), length);
#endif

	free(ehead);
}

unsigned int SyntroCFSStructured::cfsGetRecordCount()
{
	QFile xf(m_indexPath);

	if (!xf.open(QIODevice::ReadOnly))
		return 0;

	unsigned int length = (unsigned int)(xf.size() / (sizeof (qint64)));

	xf.close();

	return length;
}
