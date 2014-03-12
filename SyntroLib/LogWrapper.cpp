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

#include <qsettings.h>
#include "SyntroDefs.h"
#include "SyntroUtils.h"
#include "Logger.h"

#include "LogWrapper.h"

Logger *logSingleton = NULL;
int logLevel = SYNTRO_LOG_LEVEL_INFO;


bool logCreate()
{
	if (logSingleton)
		return false;

	QSettings *settings = SyntroUtils::getSettings();

	QString appName = settings->value(SYNTRO_PARAMS_APPNAME).toString();

	settings->beginGroup(SYNTRO_PARAMS_LOG_GROUP);

	bool diskLog = settings->value(SYNTRO_PARAMS_DISK_LOG, true).toBool();
	bool netLog = settings->value(SYNTRO_PARAMS_NET_LOG, true).toBool();

	int logKeep = settings->value(SYNTRO_PARAMS_LOG_KEEP, -1).toInt();

	if (logKeep < 0)
		logKeep = SYNTRO_DEFAULT_LOG_KEEP;
	else if (logKeep > 20)
		logKeep = 20;

	int maxSize = settings->value(SYNTRO_PARAMS_LOG_MAXDISKSIZE, -1).toInt();
	if (maxSize < 0)
		maxSize = SYNTRO_DEFAULT_MAX_DISK_LOG_SIZE;

	QString level = settings->value(SYNTRO_PARAMS_LOGLEVEL).toString().toLower();

	if (level == SYNTRO_LOG_ERROR)
		logLevel = SYNTRO_LOG_LEVEL_ERROR;
	else if (level == SYNTRO_LOG_WARN)
		logLevel = SYNTRO_LOG_LEVEL_WARN;
	else if (level == SYNTRO_LOG_INFO)
		logLevel = SYNTRO_LOG_LEVEL_INFO;
	else if (level == SYNTRO_LOG_DEBUG)
		logLevel = SYNTRO_LOG_LEVEL_DEBUG;
	else {
		logLevel = SYNTRO_LOG_LEVEL_INFO;
		level = SYNTRO_LOG_INFO;
	}

	// write out what we are using
	settings->setValue(SYNTRO_PARAMS_LOGLEVEL, level);
	settings->setValue(SYNTRO_PARAMS_DISK_LOG, diskLog);
	settings->setValue(SYNTRO_PARAMS_NET_LOG, netLog);
	settings->setValue(SYNTRO_PARAMS_LOG_KEEP, logKeep);
	settings->setValue(SYNTRO_PARAMS_LOG_MAXDISKSIZE, maxSize);

	settings->endGroup();

	logSingleton = new Logger(appName, logLevel, diskLog, netLog, logKeep, maxSize);
	logSingleton->setHeartbeatTimers(
			settings->value(SYNTRO_PARAMS_LOG_HBINTERVAL, SYNTRO_LOG_HEARTBEAT_INTERVAL).toInt(),
			settings->value(SYNTRO_PARAMS_LOG_HBTIMEOUT, SYNTRO_LOG_HEARTBEAT_TIMEOUT).toInt());

	logSingleton->resumeThread();

	delete settings;

	return true;
}

void logDestroy()
{
	if (logSingleton) {
		logSingleton->exitThread();
		logSingleton = NULL;
	}
}

void logAny(QString type, QString msg)
{
	if (logSingleton)
		logSingleton->logWrite(type, msg);
}

void _logDebug(QString msg)
{
	if (logLevel < SYNTRO_LOG_LEVEL_DEBUG)
		return;

	logAny(SYNTRO_LOG_DEBUG, msg);

	qDebug() << qPrintable(msg);
}

void _logInfo(QString msg)
{
	if (logLevel < SYNTRO_LOG_LEVEL_INFO)
		return;

	logAny(SYNTRO_LOG_INFO, msg);

	if (logLevel >= SYNTRO_LOG_LEVEL_DEBUG)
		qDebug() << qPrintable(msg);
}

void _logWarn(QString msg)
{
	if (logLevel < SYNTRO_LOG_LEVEL_WARN)
		return;

	logAny(SYNTRO_LOG_WARN, msg);

	qWarning() << qPrintable(msg);
}

void _logError(QString msg)
{
	if (logLevel < SYNTRO_LOG_LEVEL_ERROR)
		return;

	logAny(SYNTRO_LOG_ERROR, msg);

	qCritical() << qPrintable(msg);
}

void alert(QString type, QString msg)
{
	logAny(type, msg);
}

QQueue<LogMessage>* activeStreamQueue()
{
	if (!logSingleton)
		return NULL;

	return logSingleton->streamQueue();
}
