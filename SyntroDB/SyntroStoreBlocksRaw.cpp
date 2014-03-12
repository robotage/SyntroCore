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

#include <qfile.h>
#include "SyntroUtils.h"
#include "SyntroStoreBlocksRaw.h"

SyntroStoreBlocksRaw::SyntroStoreBlocksRaw(StoreStream *stream)
	: SyntroStore(stream)
{	
}

SyntroStoreBlocksRaw::~SyntroStoreBlocksRaw()
{
}

void SyntroStoreBlocksRaw::processQueue()
{
	if (m_stream->needRotation())
		m_stream->doRotation();

	writeBlocks();	
}

void SyntroStoreBlocksRaw::writeBlocks()
{
	SYNTRO_RECORD_HEADER *record;
	int headerLen;

	m_blockMutex.lock();
	int blockCount = m_blocks.count();
	m_blockMutex.unlock();

	if (blockCount < 1)
		return;

	QString dataFilename = m_stream->rawFileFullPath();

	QFile rf(dataFilename);

	if (!rf.open(QIODevice::Append)) {
		appLogWarn(QString("SyntroStoreBlocksRaw::writeBlocks - Failed opening file %1").arg(dataFilename));
		return;
	}

	for (int i = 0; i < blockCount; i++) {
		m_blockMutex.lock();
		QByteArray block = m_blocks.dequeue();
		m_blockMutex.unlock();

		if (block.length() < (int)sizeof(SYNTRO_RECORD_HEADER))
			continue;

		record = reinterpret_cast<SYNTRO_RECORD_HEADER *>(block.data());
		headerLen = SyntroUtils::convertUC2ToInt(record->headerLength);

		if (headerLen < 0 || headerLen > block.size()) {
			appLogWarn(QString("SyntroStoreBlocksRaw::writeBlocks - invalid header size %1").arg(headerLen));
			continue;
		}

		rf.write((char *)record + headerLen, block.size() - headerLen);
	}

	rf.close();
}
