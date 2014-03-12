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

#include "CFSClient.h"
#include "SyntroCFS.h"
#include "SyntroCFSDefs.h"


SyntroCFS::SyntroCFS(CFSClient *client, QString filePath)
	: m_parent(client), m_filePath(filePath)
{
}

SyntroCFS::~SyntroCFS()
{
}

bool SyntroCFS::cfsOpen(SYNTRO_CFSHEADER *)
{
	return true;
}

void SyntroCFS::cfsClose()
{
}

void SyntroCFS::cfsRead(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg, SYNTROCFS_STATE *, unsigned int)
{
	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_READ_INDEX_RES, cfsMsg->cfsType);
	cfsReturnError(ehead, cfsMsg, SYNTROCFS_ERROR_INVALID_REQUEST_TYPE);
}

void SyntroCFS::cfsWrite(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg, SYNTROCFS_STATE *, unsigned int)
{
	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_WRITE_INDEX_RES, cfsMsg->cfsType);
	cfsReturnError(ehead, cfsMsg, SYNTROCFS_ERROR_INVALID_REQUEST_TYPE);
}

void SyntroCFS::cfsQuery(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg)
{
	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_QUERY_RES, cfsMsg->cfsType);
	cfsReturnError(ehead, cfsMsg, SYNTROCFS_ERROR_INVALID_REQUEST_TYPE);
}

void SyntroCFS::cfsCancelQuery(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg)
{
	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_CANCEL_QUERY_RES, cfsMsg->cfsType);
	cfsReturnError(ehead, cfsMsg, SYNTROCFS_ERROR_INVALID_REQUEST_TYPE);
}

void SyntroCFS::cfsFetchQuery(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg)
{
	SyntroUtils::convertIntToUC2(SYNTROCFS_TYPE_FETCH_QUERY_RES, cfsMsg->cfsType);
	cfsReturnError(ehead, cfsMsg, SYNTROCFS_ERROR_INVALID_REQUEST_TYPE);
}

unsigned int SyntroCFS::cfsGetRecordCount()
{
	return 0;
}

SYNTRO_EHEAD *SyntroCFS::cfsBuildResponse(SYNTRO_EHEAD *ehead, int length)
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

SYNTRO_EHEAD *SyntroCFS::cfsBuildQueryResponse(SYNTRO_EHEAD *ehead, int length)
{
	SYNTRO_EHEAD *responseE2E = m_parent->clientBuildLocalE2EMessage(m_parent->m_CFSPort, 
                                                                     &(ehead->sourceUID), 
                                                                     SyntroUtils::convertUC2ToUInt(ehead->sourcePort), 
                                                                     sizeof(SYNTRO_QUERYRESULT_HEADER) + length);

	SYNTRO_QUERYRESULT_HEADER *responseHdr = (SYNTRO_QUERYRESULT_HEADER *)(responseE2E + 1);
	
	memset(responseHdr, 0, sizeof(SYNTRO_QUERYRESULT_HEADER));

	SyntroUtils::convertIntToUC4(length, responseHdr->cfsHeader.cfsLength);

	return responseE2E;
}

void SyntroCFS::cfsReturnError(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg, int responseCode)
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
