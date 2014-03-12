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

#include "SyntroUtils.h"

#include "SyntroDB.h"
#include "StoreManager.h"
#include "SyntroStoreBlocksRaw.h"
#include "SyntroStoreBlocksStructured.h"

StoreManager::StoreManager(StoreStream *stream) 
	: SyntroThread(QString("StoreManager"), QString(COMPTYPE_STORE))
{	
	m_stream = stream;
	m_store = NULL;
	m_timerID = -1;
}

StoreManager::~StoreManager()
{
	if (m_store) {
		delete m_store;
		m_store = NULL;
	}
}

void StoreManager::initThread()
{
	QMutexLocker lock(&m_workMutex);

	switch (m_stream->storeFormat()) {
	case structuredFileFormat:
		m_store = new SyntroStoreBlocksStructured(m_stream);
		break;

	case rawFileFormat:
		m_store = new SyntroStoreBlocksRaw(m_stream);
		break;

	default:
		logWarn(QString("Unhandled storeFormat requested: %1").arg(m_stream->storeFormat()));
		break;
	}

	if (m_store)
		m_timerID = startTimer(SYNTRO_CLOCKS_PER_SEC);
}

void StoreManager::finishThread()
{
	QMutexLocker lock(&m_workMutex);

	if (m_timerID > 0) {
		killTimer(m_timerID);
		m_timerID = -1;
	}

	if (m_store) {
		delete m_store;
		m_store = NULL;
	}

	m_stream = NULL;	
}

void StoreManager::stop()
{
	QMutexLocker lock(&m_workMutex);

	if (m_store) {
		delete m_store;
		m_store = NULL;
	}

	m_stream = NULL;
}

void StoreManager::queueBlock(QByteArray block)
{
	if (m_store)
		m_store->queueBlock(block);

	if (m_stream)
		m_stream->updateStats(block.length());
}

void StoreManager::timerEvent(QTimerEvent *)
{
	QMutexLocker lock(&m_workMutex);

	if (m_store)
		m_store->processQueue();
}
