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
#include "CFSClient.h"
#include "SyntroLib.h"
#include "SyntroRecord.h"
#include "CFSThread.h"

CFSClient::CFSClient(QObject *parent)
	: Endpoint(SYNTROCFS_BGND_INTERVAL, COMPTYPE_CFS),  m_parent(parent)
{
    m_CFSThread = NULL;
}

void CFSClient::appClientInit()
{
	m_CFSPort = clientAddService(SYNTRO_STREAMNAME_CFS, SERVICETYPE_E2E, true);
    m_CFSThread = new CFSThread(this);
    m_CFSThread->resumeThread();
}

void CFSClient::appClientExit()
{
    if (m_CFSThread != NULL)
        m_CFSThread->exitThread();

    m_CFSThread = NULL;
}

void CFSClient::appClientBackground()
{
}

CFSThread *CFSClient::getCFSThread()
{
	return m_CFSThread;
}

void CFSClient::appClientReceiveE2E(int servicePort, SYNTRO_EHEAD *message, int length)
{
	if (servicePort != m_CFSPort) {
		logWarn(QString("Message received with incorrect service port %1").arg(servicePort));
		free(message);
		return;
	}

	m_CFSThread->postThreadMessage(SYNTRO_CFS_MESSAGE, length, message);
}

bool CFSClient::appClientProcessThreadMessage(SyntroThreadMsg *msg)
{
	if (msg->message == SYNTRO_CFS_MESSAGE) {
		clientSendMessage(m_CFSPort, (SYNTRO_EHEAD *)msg->ptrParam, msg->intParam, SYNTROCFS_E2E_PRIORITY);
		return true;
	}

	return false;
}


void CFSClient::sendMessage(SYNTRO_EHEAD *message, int length)
{
	postThreadMessage(SYNTRO_CFS_MESSAGE, length, message); 
}
