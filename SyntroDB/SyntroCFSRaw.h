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

#ifndef SYNTROCFSRAW_H
#define SYNTROCFSRAW_H

#include "SyntroCFS.h"

class SyntroCFSRaw : public SyntroCFS
{
public:
	SyntroCFSRaw(CFSClient *client, QString filePath);
	virtual ~SyntroCFSRaw();

	virtual bool cfsOpen(SYNTRO_CFSHEADER *cfsMsg);
	virtual void cfsRead(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg, SYNTROCFS_STATE *scs, unsigned int requestedIndex);
	virtual void cfsWrite(SYNTRO_EHEAD *ehead, SYNTRO_CFSHEADER *cfsMsg, SYNTROCFS_STATE *scs, unsigned int requestedIndex);

	virtual unsigned int cfsGetRecordCount();

private:
	int m_blockSize;
	unsigned int m_recordCount;
};

#endif // SYNTROCFSRAW_H
