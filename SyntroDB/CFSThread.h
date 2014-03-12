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

#ifndef CFSTHREAD_H
#define CFSTHREAD_H

#include <qlist.h>

#include "SyntroLib.h"
#include "SyntroRecord.h"
#include "SyntroDB.h"

#define	SYNTROCFS_STATUS_INTERVAL	(SYNTRO_CLOCKS_PER_SEC)	// this is the min interval between status emits

#define	SYNTROCFS_MAX_FILES		1024						// max files open at one time

#define CFS_TYPE_DATABASE   0
#define CFS_TYPE_STRUCTURED 1
#define CFS_TYPE_RAW        2

//	The SyntroCFS state information referenced by a stream handle

class SyntroCFS;

typedef struct
{
public:
	bool inUse;												// true if this represents an open stream
	int	storeHandle;										// the index of this entry
	int clientHandle;										// the client's handle
	SYNTRO_UID clientUID;									// the uid of the client app
	int clientPort;											// the client port
	qint64 lastKeepalive;									// last time a keepalive was received
	qint64 rxBytes;											// total bytes received for this file
	qint64 txBytes;											// total bytes sent for this file
	qint64 lastStatusEmit;									// the time that the last status was emitted
	SyntroCFS *agent;
} SYNTROCFS_STATE;

class CFSClient;
class DirThread;

class CFSThread : public SyntroThread
{
	Q_OBJECT

public:
	CFSThread(CFSClient *parent);
	~CFSThread();

	QMutex m_lock;
	SYNTROCFS_STATE m_cfsState[SYNTROCFS_MAX_FILES];		// the open file state cache

signals:
	void newStatus(int handle, SYNTROCFS_STATE *CFSState);

protected:
	void initThread();
	bool processMessage(SyntroThreadMsg *msg);
	void timerEvent(QTimerEvent *event);
	void finishThread();

private:
	SyntroCFS* factory(int cfsMode, CFSClient *client, QString filePath);

	CFSClient *m_parent;
	QString m_storePath;
	int m_timer;

	DirThread *m_DirThread;

	void CFSInit();
	void CFSBackground();

	void CFSProcessMessage(SyntroThreadMsg *msg);

	void CFSDir(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg);
	void CFSOpen(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg);
	void CFSClose(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg);
	void CFSKeepAlive(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg);
	void CFSReadIndex(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg);
	void CFSWriteIndex(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg);
	void CFSQuery(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg);
	void CFSCancelQuery(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg);
	void CFSFetchQuery(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg);

	SYNTRO_EHEAD *CFSBuildResponse(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg, int length);
	bool CFSSanityCheck(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg);
	void CFSReturnError(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg, int responseCode);
};

#endif // CFSTHREAD_H

