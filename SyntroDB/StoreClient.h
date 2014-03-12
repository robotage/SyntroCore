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

#ifndef STORECLIENT_H
#define STORECLIENT_H

#include "SyntroLib.h"
#include "StoreStream.h"
#include "SyntroDB.h"

class StoreManager;

class StoreClient : public Endpoint
{
	Q_OBJECT

public:
	StoreClient(QObject *parent);
	~StoreClient() {}

	bool getStoreStream(int index, StoreStream *ss);

public slots:
	void refreshStreamSource(int index);

protected:
	void appClientInit();
	void appClientExit();
	void appClientReceiveMulticast(int servicePort, SYNTRO_EHEAD *message, int len);

private:
	void deleteStreamSource(int index);
	
	QObject	*m_parent;
	QMutex m_lock;

	StoreStream *m_sources[SYNTRODB_MAX_STREAMS];
	StoreManager *m_storeManagers[SYNTRODB_MAX_STREAMS];
};

#endif // STORECLIENT_H


