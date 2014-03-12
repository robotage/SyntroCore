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

#ifndef CFSDIRTHREAD_H
#define CFSDIRTHREAD_H

#include <qlist.h>
#include <qdir.h>

#include "SyntroLib.h"

class CFSDirThread : public SyntroThread
{
	Q_OBJECT

public:
	CFSDirThread(QObject *parent, QSettings *settings);
	~CFSDirThread();
	
	void exitThread();
	QString getDirectory();

protected:
	void initThread();
	bool processMessage(SyntroThreadMsg *msg);

private:
	void buildDirString();
	void processDir(QDir dir, QString& dirString, QString relativePath);

	QSettings *m_settings;
	QString m_storePath;
	QString m_directory;
	QMutex m_lock;

	int m_timer;
};

#endif // CFSDIRTHREAD_H

