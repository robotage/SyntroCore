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

#include "SyntroSocket.h"

// SyntroSocket

//  This constructor only used by the Hello system

SyntroSocket::SyntroSocket(const QString& logTag)
{
	m_logTag = logTag;
	clearSocket();
}

//  This is what everyone else uses

SyntroSocket::SyntroSocket(SyntroThread *thread, int connectionID, bool encrypt)
{
	clearSocket();
	m_ownerThread = thread;
	m_connectionID = connectionID;
    m_encrypt = encrypt;
  	TRACE1("Allocated new socket with connection ID %d", m_connectionID);
}

void SyntroSocket::clearSocket()
{
	m_ownerThread = NULL;
	m_onConnectMsg = -1;
	m_onCloseMsg = -1;
	m_onReceiveMsg = -1;
	m_onAcceptMsg = -1;
	m_onSendMsg = -1;
	m_TCPSocket = NULL;
	m_UDPSocket = NULL;
	m_server = NULL;
	m_state = -1;
}

//	Set nFlags = true for reuseaddr

int	SyntroSocket::sockCreate(int nSocketPort, int nSocketType, int nFlags)
{
	bool	ret;

	m_sockType = nSocketType;
	m_sockPort = nSocketPort;
	switch (m_sockType) {
		case SOCK_DGRAM:
			m_UDPSocket = new QUdpSocket(this);
			connect(m_UDPSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));
			connect(m_UDPSocket, SIGNAL(stateChanged ( QAbstractSocket::SocketState) ), this, SLOT(onState( QAbstractSocket::SocketState)));
			if (nFlags)
				ret = m_UDPSocket->bind(nSocketPort, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
			else
				ret = m_UDPSocket->bind(nSocketPort);
			return ret;

		case SOCK_STREAM:
            if (m_encrypt) {
		        m_TCPSocket = new QSslSocket(this);
                connect((QSslSocket *)m_TCPSocket, SIGNAL(sslErrors(const QList<QSslError>&)),
                    this, SLOT(sslErrors(const QList<QSslError>&)));
                connect((QSslSocket *)m_TCPSocket, SIGNAL(peerVerifyError(const QSslError&)),
                    this, SLOT(peerVerifyError(const QSslError&)));
                ((QSslSocket *)m_TCPSocket)->setPeerVerifyMode(QSslSocket::VerifyNone);
           } else {
		        m_TCPSocket = new QTcpSocket(this);
		        connect(m_TCPSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));
            }
		    connect(m_TCPSocket, SIGNAL(stateChanged( QAbstractSocket::SocketState) ), this, SLOT(onState( QAbstractSocket::SocketState)));
			return 1;

		case SOCK_SERVER:
            if (m_encrypt)
			    m_server = new SSLServer(this);
            else
			    m_server = new TCPServer(this);
			return 1;

		default:
			logError(QString("Socket create on illegal type %1").arg(nSocketType));
			return 0;
	}
}

bool SyntroSocket::sockConnect(const char *addr, int port)
{
	if (m_sockType != SOCK_STREAM) {
		logError(QString("Incorrect socket type for create %1").arg(m_sockType));
		return false;
	}
    if (m_encrypt)
        ((QSslSocket *)m_TCPSocket)->connectToHostEncrypted(addr, port);
    else
        m_TCPSocket->connectToHost(addr, port);
	return true;
}

bool SyntroSocket::sockAccept(SyntroSocket& sock, char *IpAddr, int *port)
{
    sock.m_TCPSocket = m_server->nextPendingConnection();
   	strcpy(IpAddr, (char *)sock.m_TCPSocket->peerAddress().toString().toLocal8Bit().constData());
    *port = (int)sock.m_TCPSocket->peerPort();
	sock.m_sockType = SOCK_STREAM;
	sock.m_ownerThread = m_ownerThread;
	sock.m_state = QAbstractSocket::ConnectedState;
    sock.m_encrypt = m_server->usingSSL();
	return true;
}

bool SyntroSocket::sockClose()
{
	switch (m_sockType) {
		case SOCK_DGRAM:
			disconnect(m_UDPSocket, 0, 0, 0);
			m_UDPSocket->close();
			delete m_UDPSocket;
			m_UDPSocket = NULL;
			break;

		case SOCK_STREAM:
		    disconnect(m_TCPSocket, 0, 0, 0);
		    m_TCPSocket->close();
		    delete m_TCPSocket;
		    m_TCPSocket = NULL;
		    break;

		case SOCK_SERVER:
		    disconnect(m_server, 0, 0, 0);
		    m_server->close();
		    delete m_server;
		    m_server = NULL;
			break;
	}
	clearSocket();
	return true;
}

int	SyntroSocket::sockListen()
{
	if (m_sockType != SOCK_SERVER) {
		logError(QString("Incorrect socket type for listen %1").arg(m_sockType));
		return false;
	}
    return m_server->listen(QHostAddress::Any, m_sockPort);
}


int	SyntroSocket::sockReceive(void *lpBuf, int nBufLen)
{
	switch (m_sockType) {
		case SOCK_DGRAM:
			return m_UDPSocket->read((char *)lpBuf, nBufLen);

		case SOCK_STREAM:
			if (m_state != QAbstractSocket::ConnectedState)
				return 0;
		    return m_TCPSocket->read((char *)lpBuf, nBufLen);

		default:
			logError(QString("Incorrect socket type for receive %1").arg(m_sockType));
			return -1;
	}
}

int	SyntroSocket::sockSend(void *lpBuf, int nBufLen)
{
	if (m_sockType != SOCK_STREAM) {
		logError(QString("Incorrect socket type for send %1").arg(m_sockType));
		return false;
	}
	if (m_state != QAbstractSocket::ConnectedState)
		return 0;
  	return m_TCPSocket->write((char *)lpBuf, nBufLen); 
}

bool SyntroSocket::sockEnableBroadcast(int)
{
    return true;
}

bool SyntroSocket::sockSetReceiveBufSize(int nSize)
{
	if (m_sockType != SOCK_STREAM) {
		logError(QString("Incorrect socket type for SetReceiveBufferSize %1").arg(m_sockType));
		return false;
	}
    m_TCPSocket->setReadBufferSize(nSize);
	return true;
}

bool SyntroSocket::sockSetSendBufSize(int)
{
	if (m_sockType != SOCK_STREAM) {
		logError(QString("Incorrect socket type for SetSendBufferSize %1").arg(m_sockType));
		return false;
	}

//	logDebug(QString("SetSendBufferSize not implemented"));
	return true;
}


int SyntroSocket::sockSendTo(const void *buf, int bufLen, int hostPort, char *host)
{
	if (m_sockType != SOCK_DGRAM) {
		logError(QString("Incorrect socket type for SendTo %1").arg(m_sockType));
		return false;
	}
	if (host == NULL)
		return m_UDPSocket->writeDatagram((const char *)buf, bufLen, SyntroUtils::getMyBroadcastAddress(), hostPort);	
	else
		return m_UDPSocket->writeDatagram((const char *)buf, bufLen, QHostAddress(host), hostPort);	
}

int	SyntroSocket::sockReceiveFrom(void *buf, int bufLen, char *IpAddr, unsigned int *port, int)
{
	QHostAddress ha;
	quint16 qport;

	if (m_sockType != SOCK_DGRAM) {
		logError(QString("Incorrect socket type for ReceiveFrom %1").arg(m_sockType));
		return false;
	}
	int	nRet = m_UDPSocket->readDatagram((char *)buf, bufLen, &ha, &qport);
	*port = qport;
	strcpy(IpAddr, qPrintable(ha.toString()));
	return nRet;
}

int		SyntroSocket::sockPendingDatagramSize()
{
	if (m_sockType != SOCK_DGRAM) {
		logError(QString("Incorrect socket type for PendingDatagramSize %1").arg(m_sockType));
		return false;
	}
	return m_UDPSocket->pendingDatagramSize();
}


SyntroSocket::~SyntroSocket()
{
	if (m_sockType != -1)
		sockClose();
}

// SyntroSocket member functions

int SyntroSocket::sockGetConnectionID()
{
	return m_connectionID;
}

void SyntroSocket::sockSetThread(SyntroThread *thread)
{
	m_ownerThread = thread;
}

void SyntroSocket::sockSetConnectMsg(int msg)
{
	if (m_sockType != SOCK_STREAM) {
		logError(QString("Incorrect socket type for SetConnectMsg %1").arg(m_sockType));
		return;
	}
	m_onConnectMsg = msg;
    connect(m_TCPSocket, SIGNAL(connected()), this, SLOT(onConnect()));
}


void SyntroSocket::sockSetAcceptMsg(int msg)
{
	if (m_sockType != SOCK_SERVER) {
		logError(QString("Incorrect socket type for SetAcceptMsg %1").arg(m_sockType));
		return;
	}
	m_onAcceptMsg = msg;
    connect(m_server, SIGNAL(newConnection()), this, SLOT(onAccept()));
}

void SyntroSocket::sockSetCloseMsg(int msg)
{
	if (m_sockType != SOCK_STREAM) {
		logError(QString("Incorrect socket type for SetCloseMsg %1").arg(m_sockType));
		return;
	}
	m_onCloseMsg = msg;
    connect(m_TCPSocket, SIGNAL(disconnected()), this, SLOT(onClose()));
}

void SyntroSocket::sockSetReceiveMsg(int msg)
{
	m_onReceiveMsg = msg;

	switch (m_sockType) {
		case SOCK_DGRAM:
			connect(m_UDPSocket, SIGNAL(readyRead()), this, SLOT(onReceive()));
			break;

		case SOCK_STREAM:
		    connect(m_TCPSocket, SIGNAL(readyRead()), this, SLOT(onReceive()), Qt::DirectConnection);
			break;

		default:
			logError(QString("Incorrect socket type for SetReceiveMsg %1").arg(m_sockType));
			break;
	}
}

void SyntroSocket::sockSetSendMsg(int msg)
{
	m_onSendMsg = msg;
	switch (m_sockType)
	{
		case SOCK_DGRAM:
			connect(m_UDPSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(onSend(qint64)));
			break;

		case SOCK_STREAM:
		    connect(m_TCPSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(onSend(qint64)), Qt::DirectConnection);
			break;

		default:
			logError(QString("Incorrect socket type for SetSendMsg %1").arg(m_sockType));
			break;
	}
}

void SyntroSocket::onConnect()
{
	if (m_onConnectMsg != -1)
		m_ownerThread->postThreadMessage(m_onConnectMsg, m_connectionID, NULL);
}

void SyntroSocket::onAccept()
{
	if (m_onAcceptMsg != -1)
		m_ownerThread->postThreadMessage(m_onAcceptMsg, m_connectionID, NULL);
}

void SyntroSocket::onClose()
{
	if (m_onCloseMsg != -1)
		m_ownerThread->postThreadMessage(m_onCloseMsg, m_connectionID, NULL);
}

void SyntroSocket::onReceive()
{
	if (m_onReceiveMsg != -1)
		m_ownerThread->postThreadMessage(m_onReceiveMsg, m_connectionID, NULL);
}

void	SyntroSocket::onSend(qint64)
{
	if (m_onSendMsg != -1)
		m_ownerThread->postThreadMessage(m_onSendMsg, m_connectionID, NULL);
}

void SyntroSocket::onError(QAbstractSocket::SocketError errnum)
{
	switch (m_sockType) {
		case SOCK_DGRAM:
			if (errnum != QAbstractSocket::AddressInUseError)
				logDebug(QString("UDP socket error %1").arg(m_UDPSocket->errorString()));

			break;

		case SOCK_STREAM:
		    logDebug(QString("TCP socket error %1").arg(m_TCPSocket->errorString()));
			break;
	}
}

void SyntroSocket::onState(QAbstractSocket::SocketState socketState)
{
	switch (m_sockType)
	{
		case SOCK_DGRAM:
			logDebug(QString("UDP socket state %1").arg(socketState));
			break;

		case SOCK_STREAM:
			logDebug(QString("TCP socket state %1").arg(socketState));
			break;
	}
	if ((socketState == QAbstractSocket::UnconnectedState) && (m_state < QAbstractSocket::ConnectedState)) {
        logDebug("onClose generated by onState"); 
		onClose();									// no signal generated in this situation
    }
	m_state = socketState;
}


void SyntroSocket::peerVerifyError(const QSslError & error)
{
    QString msg = "Peer verify error from " + m_TCPSocket->peerAddress().toString() + ": " 
        + error.errorString() /* + "\n" + error.certificate().toText() */ ;
    logWarn(msg);
}

void SyntroSocket::sslErrors(const QList<QSslError> & errors) {

    foreach(QSslError err, errors) {
        QString msg = "SSL handshake error from " + m_TCPSocket->peerAddress().toString() 
            + ": " + err.errorString() /* + "\n" + err.certificate().toText() */ ;
        logWarn(msg);
    }

    QSslSocket *sslSocket = qobject_cast<QSslSocket*>(sender());
    sslSocket->ignoreSslErrors();

}

//----------------------------------------------------------
//
//  TCPServer

TCPServer::TCPServer(QObject *parent) : QTcpServer(parent)
{
    m_encrypt = false;
    m_logTag = "TCPServer";
}

//----------------------------------------------------------
//
//  SSLServer

SSLServer::SSLServer(QObject *parent) : TCPServer(parent)
{
    m_encrypt = true;
    m_logTag = "SSLServer";
}

#if QT_VERSION < 0x050000
void SSLServer::incomingConnection(int socket)
#else
void SSLServer::incomingConnection(qintptr socket)
#endif
{

    Q_UNUSED(socket);

    //
    // Prepare a new server socket for the incoming connection
    //
    QSslSocket *sslSocket = new QSslSocket();

    sslSocket->setProtocol( QSsl::AnyProtocol );
    sslSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
    sslSocket->setLocalCertificate("./server.crt", QSsl::Pem);
    sslSocket->setPrivateKey("./server.key", QSsl::Rsa, QSsl::Pem, "");
    sslSocket->setProtocol(QSsl::TlsV1SslV3);

    if (sslSocket->setSocketDescriptor(socket)) {
       connect(sslSocket, SIGNAL(encrypted()), this, SLOT(ready()));
       connect(sslSocket, SIGNAL(sslErrors(const QList<QSslError>&)), this, SLOT(sslErrors(const QList<QSslError>&)));
       connect(sslSocket, SIGNAL(peerVerifyError(const QSslError&)), this, SLOT(peerVerifyError(const QSslError&)));
       addPendingConnection(sslSocket);

       sslSocket->startServerEncryption();

    }
    else {
       delete sslSocket;
    }
}

void SSLServer::ready() {

    QSslSocket *sslSocket = qobject_cast<QSslSocket*>(sender());

    QSslCipher ciph = sslSocket->sessionCipher();
    QString cipher = QString("%1, %2 (%3/%4)").arg(ciph.authenticationMethod())
                          .arg(ciph.name()).arg(ciph.usedBits()).arg(ciph.supportedBits());

    QString msg = "SSL session from " + sslSocket->peerAddress().toString() + " established with cipher: " + cipher;
    logDebug(msg);
}

void SSLServer::peerVerifyError(const QSslError & error)
{
    QString msg = "Peer verify error: " + error.errorString() /* + "\n" + error.certificate().toText() */;
    logWarn(msg);
}

void SSLServer::sslErrors(const QList<QSslError> & errors) {

    foreach(QSslError err, errors) {
        QString msg = "SSL handshake error: " + err.errorString() /* + "\n" + err.certificate().toText() */;
        logWarn(msg);
    }

    QSslSocket *sslSocket = qobject_cast<QSslSocket*>(sender());
    sslSocket->ignoreSslErrors();

}