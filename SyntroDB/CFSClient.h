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

#ifndef CFSCLIENT_H
#define CFSCLIENT_H

#include "SyntroLib.h"

class CFSThread;
class SyntroCFS;

class CFSClient : public Endpoint
{
	Q_OBJECT

friend class CFSThread;
friend class SyntroCFS;

public:
	CFSClient(QObject *parent);
	~CFSClient() {}

	CFSThread *getCFSThread();

	void sendMessage(SYNTRO_EHEAD *message, int length);

protected:
	void appClientInit();
	void appClientExit();
	void appClientReceiveE2E(int servicePort, SYNTRO_EHEAD *message, int length);
	void appClientBackground();

	bool appClientProcessThreadMessage(SyntroThreadMsg *msg);

	int m_CFSPort;										// the local service port assigned to CFS

private:

	QObject	*m_parent;
	CFSThread *m_CFSThread;								// the worker thread
};

#endif // CFSCLIENT_H


