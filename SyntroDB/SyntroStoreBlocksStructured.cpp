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
#include "SyntroStoreBlocksStructured.h"

SyntroStoreBlocksStructured::SyntroStoreBlocksStructured(StoreStream *stream)
	: SyntroStore(stream)
{	
}

SyntroStoreBlocksStructured::~SyntroStoreBlocksStructured()
{
}

void SyntroStoreBlocksStructured::processQueue()
{
	if (m_stream->needRotation())
		m_stream->doRotation();

	writeBlocks();	
}

void SyntroStoreBlocksStructured::writeBlocks()
{
	SYNTRO_RECORD_HEADER *record;
	SYNTRO_STORE_RECORD_HEADER storeRecHeader;
	qint64 pos, headerLength;
	QList<qint64> posList;

	m_blockMutex.lock();
	int blockCount = m_blocks.count();
	m_blockMutex.unlock();

	if (blockCount < 1)
		return;

	QString dataFilename = m_stream->srfFileFullPath();
	QString indexFilename = m_stream->srfIndexFullPath();

	QFile dataFile(dataFilename);

	if (!dataFile.open(QIODevice::Append))
		return;

	QFile indexFile(indexFilename);

	if (!indexFile.open(QIODevice::Append)) {
		dataFile.close();
		return;
	}

	strncpy(storeRecHeader.sync, SYNC_STRINGV0, SYNC_LENGTH);
	SyntroUtils::convertIntToUC4(0, storeRecHeader.data);

	for (int i = 0; i < blockCount; i++) {
		m_blockMutex.lock();
		QByteArray block = m_blocks.dequeue();
		m_blockMutex.unlock();

		if (block.length() < (int)sizeof(SYNTRO_RECORD_HEADER))
			continue;

		SyntroUtils::convertIntToUC4(block.size(), storeRecHeader.size);
		
		record = reinterpret_cast<SYNTRO_RECORD_HEADER *>(block.data());

		dataFile.write((char *)(&storeRecHeader), sizeof(SYNTRO_STORE_RECORD_HEADER));
		headerLength = (qint64)sizeof(SYNTRO_STORE_RECORD_HEADER);

		pos = dataFile.pos();
		pos -= headerLength;

		dataFile.write((char *)record, block.size());
		posList.append(pos);
	}

	for (int i = 0; i < posList.count(); i++) {
		pos = posList[i];
		indexFile.write((char *)&pos, sizeof(qint64));
	}
	
	indexFile.close();
	dataFile.close();
}
