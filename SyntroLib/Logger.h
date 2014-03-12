//
//  Copyright (c) 2014 Scott Ellis and Richard Barnett
//	
//  This file is part of SyntroLib
//
//  SyntroLib is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  SyntroLib is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with SyntroLib.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef LOGGER_H
#define LOGGER_H

#include "syntrolib_global.h"
#include "SyntroUtils.h"
#include "Endpoint.h"

#include <qthread.h>
#include <qmutex.h>
#include <qfile.h>
#include <qwaitcondition.h>
#include <qtextstream.h>
#include <qqueue.h>
#include <qsettings.h>

#define LOG_FLUSH_INTERVAL_SECONDS 1

#define SYNTROLOG_BGND_INTERVAL		100

class SYNTROLIB_EXPORT LogMessage
{
public:
	LogMessage(QString type, QString &msg);
	LogMessage(const LogMessage &rhs);

	LogMessage& operator=(const LogMessage &rhs);

	QString m_type;
	QString m_msg;
    QString m_timeStamp;
};

class Logger : public Endpoint
{
public:
	Logger(const QString& appName, int level, bool diskLog, bool netLog, int logKeep, int maxSize);

	~Logger();

	void logWrite(QString level, QString msg);
	QQueue<LogMessage>* streamQueue();

protected:
	void appClientBackground();
	void appClientInit();

private:
    bool diskOpen();
    void diskFlush();
	bool rotateLogs(QString logName);

	QString m_logName;
	bool m_diskLog;
	bool m_netLog;
	int m_logKeep;
	qint64 m_maxDiskSize;
	QFile m_file;
	QTextStream m_stream;
	QWaitCondition m_stopCondition;
	QMutex m_stopMutex;
    QMutex m_flushMutex;
	QMutex m_streamMutex;
	QQueue<LogMessage> m_diskQ[2];
	QQueue<LogMessage> m_streamQ[2];
    int m_logInterval;
    int m_activeDiskQ;
	int m_activeStreamQ;

	qint64 m_lastFlushTime;
	int m_logPort;
};


#endif // LOGGER_H
