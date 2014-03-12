//
//  Copyright (c) 2014 Scott Ellis and Richard Barnett.
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
#include "SyntroCFS.h"

#define	SYNTROCFS_STATUS_INTERVAL	(SYNTRO_CLOCKS_PER_SEC)	// this is the min interval between status emits


class SyntroCFSClient;
class CFSDirThread;

class CFSThread : public SyntroThread
{
	Q_OBJECT

public:
	CFSThread(SyntroCFSClient *parent, QSettings *settings);
	~CFSThread();
	void exitThread();

	QMutex m_lock;
	SYNTROCFS_STATE m_cfsState[SYNTROCFS_MAX_FILES];		// the open file state cache

signals:
	void newStatus(int handle, SYNTROCFS_STATE *CFSState);

protected:
	void initThread();
	bool processMessage(SyntroThreadMsg *msg);

private:
	QSettings *m_settings;
	SyntroCFSClient *m_parent;
	QString m_storePath;
	int m_timer;

	CFSDirThread *m_DirThread;

	void CFSInit();
	void CFSBackground();

	void CFSProcessMessage(SyntroThreadMsg *msg);

	void CFSDir(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg);
	void CFSOpen(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg);
	void CFSClose(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg);
	void CFSKeepAlive(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg);
	void CFSReadIndex(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg);
	void CFSWriteIndex(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg);

	SYNTRO_EHEAD *CFSBuildResponse(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg, int length); // builds a reponse from a request
	bool CFSSanityCheck(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg);
	void CFSReturnError(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg, int responseCode);
	bool CFSFindFileIndex(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg, SYNTROCFS_STATE *scs, unsigned int requestedIndex);
	bool CFSStructuredFileRead(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg, SYNTROCFS_STATE *scs, unsigned int requestedIndex);
	bool CFSFlatFileRead(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg, SYNTROCFS_STATE *scs, unsigned int requestedIndex);
	bool CFSStructuredFileWrite(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg, SYNTROCFS_STATE *scs, unsigned int requestedIndex);
	bool CFSFlatFileWrite(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg, SYNTROCFS_STATE *scs, unsigned int requestedIndex);

	unsigned int getFileSize(SYNTROCFS_STATE *scs);			// returns file size in units of blocks or records
	unsigned int getStructuredFileSize(SYNTROCFS_STATE *scs);// returns file size in units of records
	unsigned int getFlatFileSize(SYNTROCFS_STATE *scs);		// returns file size in units of blocks

};

#endif // CFSTHREAD_H

