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
#include "SyntroCFSRaw.h"

SyntroCFSRaw::SyntroCFSRaw(CFSClient *client, QString filePath)
	: SyntroCFS(client, filePath)
{
}

SyntroCFSRaw::~SyntroCFSRaw()
{
}

bool SyntroCFSRaw::cfsOpen(SYNTRO_CFSHEADER *cfsMsg)
{
	m_blockSize = SyntroUtils::convertUC4ToInt(cfsMsg->cfsIndex);

	// we are going to use this as a divisor to get the number of blocks
	if (m_blockSize <= 0) {
		SyntroUtils::convertIntToUC2(SYNTROCFS_ERROR_BAD_BLOCKSIZE_REQUEST, cfsMsg->cfsParam);
		return false;
	}

	return true;
}

void SyntroCFSRaw::cfsRead(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg, SYNTROCFS_STATE *scs, unsigned int requestedIndex)
{
	int responseCode = SYNTROCFS_SUCCESS;
	int length = 0;
	SYNTRO_CFSHEADER *responseHdr = NULL;
	SYNTRO_EHEAD *responseE2E = NULL;
	qint64 bpos = 0;
	char *fileData = NULL;

	QFile ff(m_filePath);

	if (!ff.open(QIODevice::ReadOnly)) {
		responseCode = SYNTROCFS_ERROR_FILE_NOT_FOUND;
		goto sendResponse;
	}

	// byte position in file
	bpos = (qint64)m_blockSize * (qint64)SyntroUtils::convertUC4ToInt(cfsMsg->cfsIndex);

	if (!ff.seek(bpos))	{
		responseCode = SYNTROCFS_ERROR_RECORD_SEEK;
		ff.close();
		goto sendResponse;
	}

	length = m_blockSize * SyntroUtils::convertUC2ToInt(cfsMsg->cfsParam);
	scs->txBytes += length;

	if (length == 0)
		goto sendResponse;

	if (length > (SYNTRO_MESSAGE_MAX - (int)sizeof(SYNTRO_EHEAD) - (int)sizeof(SYNTRO_CFSHEADER))) {
		responseCode = SYNTROCFS_ERROR_TRANSFER_TOO_LONG;
		ff.close();
		goto sendResponse;
	}

	// adjust for EOF if necessary
	if (((qint64)length + bpos) > ((qint64)m_recordCount * m_blockSize))
		length = (qint64)m_recordCount * m_blockSize - bpos;

	responseE2E = cfsBuildResponse(ehead, length);		

	responseHdr = reinterpret_cast<SYNTRO_CFSHEADER *>(responseE2E + 1);

	fileData = reinterpret_cast<char *>(responseHdr + 1);				

	if (ff.read(fileData, length) != length) {
		responseCode = SYNTROCFS_ERROR_READ;
		ff.close();
		goto sendResponse;
	}

	ff.close();

sendResponse:

	if (responseE2E == NULL) {
		responseE2E = cfsBuildResponse(ehead, 0);
		length = 0;
		responseHdr = reinterpret_cast<SYNTRO_CFSHEADER *>(responseE2E + 1);
	}

	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_READ_INDEX_RES, responseHdr->cfsType);
	SyntroUtils::convertIntToUC2(responseCode, responseHdr->cfsParam);
	SyntroUtils::convertIntToUC4(requestedIndex, responseHdr->cfsIndex);

	int totalLength = sizeof(SYNTRO_CFSHEADER) + length;
	m_parent->sendMessage(responseE2E, totalLength);

#ifdef CFS_THREAD_TRACE
	TRACE2("Sent record to %s, length %d", qPrintable(SyntroUtils::displayUID(&ehead->sourceUID)), totalLength);
#endif

	free(ehead);
}

void SyntroCFSRaw::cfsWrite(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg, SYNTROCFS_STATE *scs, unsigned int requestedIndex)
{
	int length = 0;
	unsigned int responseCode = SYNTROCFS_SUCCESS;

	QFile ff(m_filePath);

	// delete first if starting at zero
	if (requestedIndex == 0)
		ff.remove();

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

	SYNTRO_EHEAD *responseE2E = cfsBuildResponse(ehead, 0);		

	SYNTRO_CFSHEADER *responseHdr = reinterpret_cast<SYNTRO_CFSHEADER *>(responseE2E + 1);

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

unsigned int SyntroCFSRaw::cfsGetRecordCount()
{
	QFile xf(m_filePath);

	m_recordCount = 0;

	if (m_blockSize != 0)
		m_recordCount = (unsigned int)(xf.size() / m_blockSize);

	return m_recordCount;
}
