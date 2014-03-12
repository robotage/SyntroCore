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

#include "AlertClient.h"
#include "SyntroLib.h"
#include "DirectoryEntry.h"

#define	ALERTCLIENT_BACKGROUND_INTERVAL (SYNTRO_CLOCKS_PER_SEC / 10)

// in ALERTCLIENT_BACKGROUND_INTERVAL units, so 5 seconds
#define ALERTCLIENT_DIR_REFRESH_INTERVAL 50

AlertClient::AlertClient()
	: Endpoint(ALERTCLIENT_BACKGROUND_INTERVAL, COMPTYPE_LOGSINK)
{
	// force a dir refresh as soon as we connect
	m_dirRefreshCounter = ALERTCLIENT_DIR_REFRESH_INTERVAL;
	m_waitingOnDirRefresh = false;
}

void AlertClient::appClientInit()
{
}

void AlertClient::appClientConnected()
{
    logLocal(SYNTRO_LOG_INFO, "SyntroLink established");
}

void AlertClient::appClientClosed()
{
    logLocal(SYNTRO_LOG_INFO, "SyntroLink closed");
}

void AlertClient::appClientReceiveMulticast(int servicePort, SYNTRO_EHEAD *multiCast, int len)
{
	SYNTRO_RECORD_HEADER *recordHead = reinterpret_cast<SYNTRO_RECORD_HEADER *>(multiCast + 1);

	if (SyntroUtils::convertUC2ToInt(recordHead->type) == SYNTRO_RECORD_TYPE_LOG) {
		int dataLength = len - SyntroUtils::convertUC2ToInt(recordHead->headerLength);

		if (dataLength > 0) {
			QByteArray bulkMsg(reinterpret_cast<const char *>(recordHead + 1), dataLength);
			emit newLogMsg(bulkMsg);
		}
	}
		
	clientSendMulticastAck(servicePort);

	free(multiCast);
}

void AlertClient::appClientBackground()
{
	if (clientIsConnected()) {
		m_dirRefreshCounter++;

		if (m_dirRefreshCounter > ALERTCLIENT_DIR_REFRESH_INTERVAL) {
			if (m_waitingOnDirRefresh) {
                logLocal(SYNTRO_LOG_ERROR, QString("Failed to get directory response after %1 seconds").arg((ALERTCLIENT_BACKGROUND_INTERVAL * ALERTCLIENT_DIR_REFRESH_INTERVAL) / 1000));
				m_waitingOnDirRefresh = false;
			}
			else {
				requestDirectory();
				m_waitingOnDirRefresh = true;
				m_dirRefreshCounter = 0;
			}
		}
	}
}

void AlertClient::appClientReceiveDirectory(QStringList dirList)
{
	int oldCount = m_sources.count();

	// mark all inactive
	for (int i = 0; i < m_sources.count(); i++)
		m_sources[i].m_active = false;

	for (int i = 0; i < dirList.count(); i++)
		handleDirEntry(QString(dirList[i]));

	// any entries not active now should be removed
	for (int i = 0; i < m_sources.count(); i++) {
		if (!m_sources[i].m_active) {
			clientRemoveService(m_sources[i].m_port);
            logLocal(SYNTRO_LOG_INFO, QString("Removed client %1").arg(m_sources[i].m_name));
			m_sources.removeAt(i);
		}
	}

	if (m_sources.count() != oldCount)
		emit activeClientUpdate(m_sources.count());

	m_waitingOnDirRefresh = false;
	m_dirRefreshCounter = 0;
}

void AlertClient::handleDirEntry(QString item)
{
	QStringList services;

	DirectoryEntry dirEntry(item);

	if (!dirEntry.isValid())
		return;

	if (dirEntry.componentType() != COMPTYPE_LOGSOURCE)
		return;

	services = dirEntry.multicastServices();

	for (int j = 0; j < services.count(); j++) {
		if (!services.at(j).endsWith(SYNTRO_LOG_SERVICE_TAG))
			continue;

		QString entry = dirEntry.appName() + SYNTRO_SERVICEPATH_SEP + services.at(j);

		int i = findEntry(entry);

		if (i >= 0) {
			m_sources[i].m_active = true;
		}
		else {
			int port = clientAddService(entry, SERVICETYPE_MULTICAST, false, true);

			if (port >= 0) {
				logLocal(SYNTRO_LOG_INFO, QString("New client %1").arg(entry));
				m_sources.append(LogStreamEntry(entry, true, port));
			}
		}
	}
}

int AlertClient::findEntry(QString name)
{
	for (int i = 0; i < m_sources.count(); i++) {
		if (m_sources[i].m_name == name)
			return i;
	}

	return -1;
}

void AlertClient::logLocal(QString logLevel, QString s)
{
	QByteArray msg;

	QDateTime dt = QDateTime::currentDateTime();
	QString timestamp = QString("%1.%2").arg(dt.toString(Qt::ISODate)).arg(dt.time().msec(), 3, 10, QChar('0'));

    msg.append(logLevel);
    msg.append(SYNTRO_LOG_COMPONENT_SEP);
	msg.append(timestamp);
    msg.append(SYNTRO_LOG_COMPONENT_SEP);
    msg.append("");
    msg.append(SYNTRO_LOG_COMPONENT_SEP);
    msg.append(SyntroUtils::getAppType());
    msg.append(SYNTRO_LOG_COMPONENT_SEP);
    msg.append("this");
    msg.append(SYNTRO_LOG_COMPONENT_SEP);
	msg.append(s);

	emit newLogMsg(msg);
}

LogStreamEntry::LogStreamEntry(QString name, bool active, int port)
{
	m_name = name;
	m_active = active;
	m_port = port;
}
