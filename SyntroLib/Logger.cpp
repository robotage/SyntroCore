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

#include <QtGlobal>
#include <qdatetime.h>
#include <qfileinfo.h>
#include <qdebug.h>

#include "Logger.h"
#include "SyntroClock.h"

#define MAX_STREAM_QUEUE_MESSAGES 64

Logger::Logger(const QString& appName, int level, bool diskLog, bool netLog, int logKeep, int maxSize)
	: Endpoint(SYNTROLOG_BGND_INTERVAL, COMPTYPE_LOGSOURCE)
{
	m_lastFlushTime = SyntroClock();
    m_activeDiskQ = 0;
	m_activeStreamQ = 0;
    m_logInterval = LOG_FLUSH_INTERVAL_SECONDS;
	m_logKeep = logKeep;

	m_diskLog = diskLog;
	m_netLog = netLog;
	
	m_logName = appName + "-" + SyntroUtils::getAppType() + ".log";

	m_logName.replace(' ', '_');

 	if (m_diskLog) {
		if (!diskOpen())
			m_diskLog = false;
	}

	if (level >= SYNTRO_LOG_LEVEL_INFO) {
        logWrite(SYNTRO_LOG_INFO, QString("Log start: %1").arg(m_logName.left(m_logName.length() - 4)));

		if (level == SYNTRO_LOG_LEVEL_DEBUG) {
			logWrite(SYNTRO_LOG_DEBUG, QString("Qt  runtime %1  build %2").arg(qVersion()).arg(QT_VERSION_STR));
			logWrite(SYNTRO_LOG_DEBUG, QString("SyntroLib version %1").arg(SYNTROLIB_VERSION));
		}
	}

	m_maxDiskSize = maxSize * 1024;
}

Logger::~Logger()
{
    if (m_file.isOpen())
		m_file.close();
}

void Logger::appClientInit()
{
	if (!m_netLog)
		return;

	m_logPort = clientAddService(SyntroUtils::getAppType() + SYNTRO_LOG_SERVICE_TAG, SERVICETYPE_MULTICAST, true);
}

void Logger::appClientBackground()
{
	QString packedMsg;
	qint64 now = SyntroClock();

	if (m_diskLog && SyntroUtils::syntroTimerExpired(now, m_lastFlushTime, m_logInterval)) {
		diskFlush();
		m_lastFlushTime = now;
	}

	if (!m_netLog)
		return;

	if (!clientIsServiceActive(m_logPort) || !clientClearToSend(m_logPort))
		return;

	QQueue<LogMessage> *log = streamQueue();

	if (!log)
		return;

	m_streamMutex.lock();

	int count = log->count();

	if (count < 1) {
		m_streamMutex.unlock();
		return;
	}

	for (int i = 0; i < count; i++) {
		LogMessage m = log->dequeue();
		packedMsg += m.m_type + SYNTRO_LOG_COMPONENT_SEP
			+ m.m_timeStamp + SYNTRO_LOG_COMPONENT_SEP
			+ SyntroUtils::displayUID(&m_UID) + SYNTRO_LOG_COMPONENT_SEP
			+ SyntroUtils::getAppType() + SYNTRO_LOG_SERVICE_TAG + SYNTRO_LOG_COMPONENT_SEP
			+ SyntroUtils::getAppName() + SYNTRO_LOG_COMPONENT_SEP
			+ m.m_msg + "\n";
	}

	m_streamMutex.unlock();

	QByteArray data = packedMsg.toLatin1();

	int length = sizeof(SYNTRO_RECORD_HEADER) + data.length();

	SYNTRO_EHEAD *multiCast = clientBuildMessage(m_logPort, length);

	if (multiCast) {
		SYNTRO_RECORD_HEADER *recordHead = (SYNTRO_RECORD_HEADER *)(multiCast + 1);
		SyntroUtils::convertIntToUC2(SYNTRO_RECORD_TYPE_LOG, recordHead->type);
		SyntroUtils::convertIntToUC2(0, recordHead->subType);
		SyntroUtils::convertIntToUC2(0, recordHead->param);
		SyntroUtils::convertIntToUC2(sizeof(SYNTRO_RECORD_HEADER), recordHead->headerLength);
		SyntroUtils::setTimestamp(recordHead->timestamp);
		memcpy((char *)(recordHead + 1), data.constData(), data.length());
		clientSendMessage(m_logPort, multiCast, sizeof(SYNTRO_EHEAD) + length, SYNTROLINK_LOWPRI);
	}
}

void Logger::logWrite(QString type, QString msg)
{
	LogMessage m(type, msg);

	if (m_diskLog) {
		if (m_flushMutex.tryLock()) {
			m_diskQ[m_activeDiskQ].enqueue(m);
			m_flushMutex.unlock();
		}
	}

	if (m_netLog) {
		if (m_streamMutex.tryLock()) {
			// throw away older messages
			// TODO: be smarter about this, maybe xxx repeated n times...
			if (m_streamQ[m_activeStreamQ].count() > MAX_STREAM_QUEUE_MESSAGES)
				m_streamQ[m_activeStreamQ].dequeue();

			m_streamQ[m_activeStreamQ].enqueue(m);
			m_streamMutex.unlock();
		}
	}
}

bool Logger::diskOpen()
{
    QString f;

	if (m_file.isOpen())
        return true;

	if (m_logKeep > 0) {
		if (!rotateLogs(m_logName))
			return false;
	}

	m_file.setFileName(m_logName);

	if (!m_file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return false;

	m_stream.setDevice(&m_file);

    return true;
}

// Remove logName.<m_logKeep>
// Rename logName.<m_logKeep - 1> to logName.<m_logKeep>
// etc...down to
// Rename logName to logName.1
// So that we can then open logName for use.
bool Logger::rotateLogs(QString logName)
{
	QFileInfo info;

	QString dst = logName + '.' + QString::number(m_logKeep);

	info.setFile(dst);

	if (info.exists()) {
		if (!QFile::remove(dst)) {
			qDebug() << "QFile.remove(" << dst << ") failed";
			return false;
		}
	}

	for (int i = m_logKeep; i > 1; i--) {
		QString src = logName + '.' + QString::number(i - 1);

		info.setFile(src);

		if (info.exists()) {
			dst = logName + '.' + QString::number(i);

			if (!QFile::rename(src, dst)) {
				qDebug() << "QFile.rename(" << src << ", " << dst << ") failed";
				return false;
			}
		}
	}

	info.setFile(logName);

	if (info.exists()) {
		dst = logName + ".1";

		if (!QFile::rename(logName, dst)) {
			qDebug() << "QFile.rename(" << logName << ", " << dst << ") failed";
			return false;
		}
	}

	return true;
}

// Switch the active queue and write out the non-active queue.
void Logger::diskFlush()
{
    int write_queue = m_activeDiskQ;

    if (!m_file.isOpen()) {
        if (!diskOpen())
            return;
    }

    m_flushMutex.lock();
    m_activeDiskQ = (m_activeDiskQ == 0 ? 1 : 0);
    m_flushMutex.unlock();

 	while (!m_diskQ[write_queue].empty()) {
 		LogMessage next = m_diskQ[write_queue].dequeue();

#ifdef WIN32
		m_stream << next.m_type << " " << next.m_timeStamp << " " << next.m_msg << '\r' << endl;
#else
		m_stream << next.m_type << " " << next.m_timeStamp << " " << next.m_msg << endl;
#endif
    }

	if (m_maxDiskSize > 0) {
		QFileInfo fi(m_logName);

		if (fi.size() >= m_maxDiskSize)
			m_file.close();										// this will cause a rotation
	}
}

QQueue<LogMessage>* Logger::streamQueue()
{
	m_streamMutex.lock();

	int n = m_activeStreamQ;

	m_activeStreamQ = m_activeStreamQ ? 0 : 1;

	m_streamQ[m_activeStreamQ].clear();

	m_streamMutex.unlock();

	return &m_streamQ[n];
}

LogMessage::LogMessage(QString type, QString &msg)
{
	m_type = type;
	QDateTime dt = QDateTime::currentDateTime();
	m_timeStamp = QString("%1.%2").arg(dt.toString(Qt::ISODate)).arg(dt.time().msec(), 3, 10, QChar('0'));
	m_msg = msg;
}

LogMessage::LogMessage(const LogMessage &rhs)
{
	m_type = rhs.m_type;
	m_msg = rhs.m_msg;
	m_timeStamp = rhs.m_timeStamp;
}

LogMessage& LogMessage::operator=(const LogMessage &rhs)
{
	if (this != &rhs) {
		m_type = rhs.m_type;
		m_msg = rhs.m_msg;
		m_timeStamp = rhs.m_timeStamp;
	}

	return *this;
}
