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

#include "SyntroDB.h"
#include "SyntroDBConsole.h"
#include "StoreClient.h"
#include "SyntroLib.h"
#include "StoreManager.h"

StoreClient::StoreClient(QObject *parent)
	: Endpoint(SYNTROSTORE_BGND_INTERVAL, COMPTYPE_STORE),  m_parent(parent)
{
	for (int i = 0; i < SYNTRODB_MAX_STREAMS; i++) {
		m_sources[i] = NULL;
		m_storeManagers[i] = NULL;
	}
}

bool StoreClient::getStoreStream(int index, StoreStream *ss)
{
	QMutexLocker lock(&m_lock);

	if (index < 0 || index >= SYNTRODB_MAX_STREAMS)
		return false;

	if (!ss || !m_sources[index])
		return false;

	*ss = *m_sources[index];

	return true;
}

void StoreClient::appClientInit()
{
}

void StoreClient::appClientExit()
{
	for (int i = 0; i < SYNTRODB_MAX_STREAMS; i++) {
		deleteStreamSource(i);
	}	
}

void StoreClient::appClientReceiveMulticast(int servicePort, SYNTRO_EHEAD *message, int len)
{
	int sourceIndex;

	sourceIndex = clientGetServiceData(servicePort);

	if ((sourceIndex >= SYNTRODB_MAX_STREAMS) || (sourceIndex < 0)) {
		logWarn(QString("Multicast received to out of range port %1").arg(servicePort));
		free(message);
		return;
	}

	if (m_storeManagers[sourceIndex] != NULL) {
		m_storeManagers[sourceIndex]->queueBlock(QByteArray(reinterpret_cast<char *>(message + 1), len));
		clientSendMulticastAck(servicePort);
	}
	free(message);
}

void StoreClient::refreshStreamSource(int index)
{
	QMutexLocker locker(&m_lock);

	if (!StoreStream::streamIndexValid(index))
		return;

	bool inUse = StoreStream::streamIndexInUse(index);

	if (m_storeManagers[index] != NULL)
		deleteStreamSource(index);

	if (!inUse)
		return;

	m_sources[index] = new StoreStream(m_logTag);
	m_sources[index]->load(index);

	if (!m_sources[index]->folderWritable()) {
		logError(QString("Folder is not writable: %1").arg(m_sources[index]->pathOnly()));
		delete m_sources[index];
		m_sources[index] = NULL;
	}

	m_storeManagers[index] = new StoreManager(m_sources[index]);

	m_sources[index]->port = clientAddService(m_sources[index]->streamName(), SERVICETYPE_MULTICAST, false);

	// record index in service entry
	clientSetServiceData(m_sources[index]->port, index);

	m_storeManagers[index]->resumeThread();
}

void StoreClient::deleteStreamSource(int index)
{
	if ((index < 0) || (index >= SYNTRODB_MAX_STREAMS)) {
		logWarn(QString("Got deleteStreamSource with out of range index %1").arg(index));
		return;
	}

	if (m_storeManagers[index] == NULL)
		return;

	clientRemoveService(m_sources[index]->port);
	StoreManager *dm = m_storeManagers[index];
	dm->stop();

	// deletes automatically, but don't want to reference it again
	m_storeManagers[index] = NULL;
	
	dm->exitThread();

	delete m_sources[index];
	m_sources[index] = NULL;
}
