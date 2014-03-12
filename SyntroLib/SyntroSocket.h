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

#ifndef _SYNTROSOCKET_H
#define _SYNTROSOCKET_H

#include "SyntroUtils.h"
#include "SyntroThread.h"

//	Define the standard socket types if necessary

#ifndef	SOCK_STREAM
#define	SOCK_STREAM		0
#define	SOCK_DGRAM		1
#define	SOCK_SERVER		2
#endif

class SYNTROLIB_EXPORT SyntroSocket : public QObject
{
	Q_OBJECT

public:
	SyntroSocket(const QString& logTag);
	SyntroSocket(SyntroThread	*thread);
	virtual ~SyntroSocket();

	void sockSetThread(SyntroThread *thread);
	int sockGetConnectionID();								// returns the allocated connection ID
	void sockSetConnectMsg(int msg);
	void sockSetAcceptMsg(int msg);
	void sockSetCloseMsg(int msg);
	void sockSetReceiveMsg(int msg);
	void sockSetSendMsg(int msg);
    bool sockEnableBroadcast(int flag);
	bool sockSetReceiveBufSize(int size);
	bool sockSetSendBufSize(int size);
	int sockSendTo(const void *buf, int bufLen, int hostPort, char *host = NULL);
	int sockReceiveFrom(void *buf, int bufLen, char *IpAddr, unsigned int *port, int flags = 0);
	int sockCreate(int socketPort, int socketType, int flags = 0);
	bool sockConnect(const char *addr, int port);
	bool sockAccept(SyntroSocket& sock, char *IpAddr, int *port);
	bool sockClose();
	int sockListen();
	int sockReceive(void *buf, int bufLen);
	int sockSend(void *buf, int bufLen);
	int sockPendingDatagramSize();

public slots:
	void onConnect();
	void onAccept();
	void onClose();
	void onReceive();
	void onSend(qint64 bytes);
	void onError(QAbstractSocket::SocketError socketError);
	void onState(QAbstractSocket::SocketState socketState);

protected:
	SyntroThread	*m_ownerThread;
	int m_connectionID;
	int m_sockType;											// the socket type
	int m_sockPort;											// the socket port number
	QUdpSocket *m_UDPSocket;
	QTcpSocket *m_TCPSocket;
	QTcpServer *m_server;

	void clearSocket();										// clear up all socket fields
	int m_onConnectMsg;
	int m_onAcceptMsg;
	int m_onCloseMsg;
	int m_onReceiveMsg;
	int m_onSendMsg;

	int m_state;											// last reported socket state

	QString m_logTag;
};

#endif // _SYNTROSOCKET_H


