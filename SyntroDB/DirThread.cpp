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

#include "DirThread.h"
#include "SyntroRecord.h"
#include "SyntroDB.h"

DirThread::DirThread(const QString& storePath)
    : SyntroThread(QString("DirThread"), QString(COMPTYPE_CFS))
{
	m_storePath = storePath;
}

DirThread::~DirThread()
{
}

void DirThread::initThread()
{
	m_timer = startTimer(SYNTROCFS_DM_INTERVAL);
	buildDirString();
}

void DirThread::finishThread()
{
	killTimer(m_timer);
}

QString DirThread::getDirectory()
{
	QMutexLocker locker(&m_lock);
	return m_directory;
}

void DirThread::timerEvent(QTimerEvent *)
{
	buildDirString();
}

void DirThread::buildDirString()
{
	QDir dir;
	QString relativePath;

	QMutexLocker locker(&m_lock);

	m_directory = "";
	relativePath = "";

	if (!dir.cd(m_storePath)) {
		logError(QString("Failed to open store path %1").arg(m_storePath));
		return;
	}

	dir.setFilter(QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot);

	processDir(dir, m_directory, "");
}

void DirThread::processDir(QDir dir, QString& dirString, QString relativePath)
{
	QFileInfoList list = dir.entryInfoList();

	for (int i = 0; i < list.size(); i++) {
		QFileInfo fileInfo = list.at(i);

		if (fileInfo.isDir()) {
			dir.cd(fileInfo.fileName());

			if (relativePath == "")
				processDir(dir, dirString, fileInfo.fileName());
			else
				processDir(dir, dirString, relativePath + SYNTRO_SERVICEPATH_SEP + fileInfo.fileName());

			dir.cdUp();
		}
		else {
			if (dirString.length() > 0)
				dirString += SYNTROCFS_FILENAME_SEP;

			if (relativePath == "")
				dirString += fileInfo.fileName();
			else
				dirString += relativePath + SYNTRO_SERVICEPATH_SEP + fileInfo.fileName();
		}
	}
}
