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

#ifndef LOGCLIENT_H
#define LOGCLIENT_H

#include "SyntroLib.h"

class LogStreamEntry
{
public:
	LogStreamEntry(QString name, bool active, int port);
	QString m_name;
	bool m_active;
	int m_port;
};

class LogClient : public Endpoint
{
	Q_OBJECT

public:
    LogClient();

signals:
	void newLogMsg(QByteArray bulkMsg);
	void activeClientUpdate(int count);

protected:
	void appClientInit();
	void appClientReceiveMulticast(int servicePort, SYNTRO_EHEAD *multiCast, int len);
	void appClientBackground();
	void appClientReceiveDirectory(QStringList dirList);
	void appClientConnected();
	void appClientClosed();

private:
	void handleDirEntry(QString item);
	int findEntry(QString name);
    void logLocal(QString logLevel, QString msg);

	bool m_waitingOnDirRefresh;
	int m_dirRefreshCounter;
	
	QList<LogStreamEntry> m_sources;
};

#endif // LOGCLIENT_H

