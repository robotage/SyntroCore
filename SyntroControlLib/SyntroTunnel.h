//
//  Copyright (c) 2014 Scott Ellis and Richard Barnett
//	
//  This file is part of SyntroControlLib
//
//  SyntroControlLib is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  SyntroControlLib is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with SyntroControlLib.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef SYNTROTUNNEL_H
#define SYNTROTUNNEL_H

#include "SyntroLib.h"
#include "SyntroServer.h"

// SyntroTunnel

class SyntroTunnel
{

public:
	SyntroTunnel(SyntroServer *server, SS_COMPONENT *syntroComponent, Hello *helloTask, HELLOENTRY *helloEntry);           
	virtual ~SyntroTunnel();

	void tunnelBackground();
	void connected();										// called when onconnect message received
	void close();											// close a tunnel

	bool m_connected;										// true if connection active
	bool m_connectInProgress;								// true if in middle of connecting the SyntroLink
	HELLOENTRY m_helloEntry;								// the hello entry for the other end of this tunnel

protected:

	bool connect();											// try to connect to target SyntroControl

	Hello *m_helloTask;										// this is to record SyntroServer's Hello task
	SyntroServer *m_server;									// the server task
	SS_COMPONENT *m_comp;									// the component to which this tunnel belongs

	qint64 m_connWait;										// timer between connection attempts

	QString m_logTag;
};

#endif // SYNTROTUNNEL_H


