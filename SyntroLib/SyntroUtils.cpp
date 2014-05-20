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

#include "LogWrapper.h"
#include "SyntroDefs.h"
#include "SyntroUtils.h"
#include "SyntroSocket.h"
#include "Endpoint.h"
#include "SyntroClock.h"

#include <qfileinfo.h>
#include <qdir.h>
#include <qhostinfo.h>
#include <qthread.h>

/*!
    \class SyntroUtils
    \brief SyntroUtils provides a set of utility functions for Syntro apps.
	\inmodule SyntroLib
 
    SyntroUtils implements a range of static functions that simplify
	the task of writing Syntro apps.
*/

QString SyntroUtils::m_logTag = "Utils";
QString SyntroUtils::m_appName = "appName";
QString SyntroUtils::m_appType = "appType";
SYNTRO_IPADDR SyntroUtils::m_myIPAddr;						
SYNTRO_MACADDR SyntroUtils::m_myMacAddr;						

QString SyntroUtils::m_iniPath;								

SyntroClockObject *SyntroUtils::m_syntroClockGen;			

//	The address info for the adaptor being used

QHostAddress SyntroUtils::m_platformBroadcastAddress;					
QHostAddress SyntroUtils::m_platformSubnetAddress;					
QHostAddress SyntroUtils::m_platformNetMask;					

/*!
	Returns the IP address associated with the selected network interface.
*/

SYNTRO_IPADDR *SyntroUtils::getMyIPAddr()
{
	return &m_myIPAddr;
}

/*!
	Returns the MAC address associated with the selected network interface.
*/

SYNTRO_MACADDR *SyntroUtils::getMyMacAddr()
{
	return &m_myMacAddr;
}

/*!
	Returns the SyntroLib version string.
*/

const char *SyntroUtils::syntroLibVersion()
{
	return SYNTROLIB_VERSION;
}

/*!
	This function performs essential initialization required by all Syntro apps. In the
	case of windowed apps, it should be called in the constructor of the class derived from QMainWindow.
	Console or daemon apps should call this function in the constructor fo the class derived from QThread.

	Note that it assumes that loadStandardSettings() has been called.
*/

void SyntroUtils::syntroAppInit()
{
	m_logTag = "Utils";
#ifdef SYNTROCLOCK_ZEROBASED
	m_syntroClockGen = new SyntroClockObject();
	m_syntroClockGen->start();
#endif
	getMyIPAddress();
	logCreate();
}

/*!
	This function should be called whent he Syntro app is exiting. In a windowed app, this is normally
	the last thing called in the closeEvent() event handler. In a console app, this should be called before exiting the 
	app.
*/

void SyntroUtils::syntroAppExit()
{
	logDestroy();
#ifdef SYNTROCLOCK_ZEROBASED
	if (m_syntroClockGen) {
		m_syntroClockGen->m_run = false;
		m_syntroClockGen->wait(200);							// should never take more than this
		delete m_syntroClockGen;
		m_syntroClockGen = NULL;
	}
#endif
}

/*!
	Returns the name of the app. Default is the hostname unless it has been overridden in 
	the app's .ini file.
*/

const QString& SyntroUtils::getAppName()
{
	return m_appName;
}

/*!
	Returns the type of the app. This is hard-coded for any particular app.
*/

const QString& SyntroUtils::getAppType()
{
	return m_appType;
}


/*!
	Checks to see if the console mode flag "-c" is present in the runtime arguments supplied to the app
	on startup. \a argc and \a argv are the traditional argument count and string array form of the arguments.

	Returns true if it is, false otherwise.
*/

bool SyntroUtils::checkConsoleModeFlag(int argc, char *argv[])
{
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-c"))
			return true;
	}

	return false;
}

/*!
	Checks to see if the daemon mode flag "-d" is present in the runtime arguments supplied to the app
	on startup. \a argc and \a argv are the traditional argument count and string array form of the arguments.

	Returns true if it is, false otherwise.
*/


bool SyntroUtils::checkDaemonModeFlag(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-d"))
            return true;
    }

    return false;
}

/*!
	This function, typically called in the app's client thread, can be used to determine if the multicast
	service send window is currently open. \a seq is the current send sequence number, \a ack is the last received ack sequence
	number. Returns true if the window is open, false otherwise.
*/

bool SyntroUtils::isSendOK(unsigned char seq, unsigned char ack)
{
	return (seq - ack) < SYNTRO_MAX_WINDOW;
}

/*!
	Converts a string form of the current MAC address (in \a macAddress) and the app's current \a instance
	number into a UID string in \a UIDStr. This is in a form that can be displayed easily.
*/

void SyntroUtils::makeUIDSTR(SYNTRO_UIDSTR UIDStr, SYNTRO_MACADDRSTR macAddress, int instance)
{
	sprintf(UIDStr, "%s%04x", macAddress, instance);
}

/*!
	Converts a string form UID into a binary form UID as is actually used in Syntro messages.
	\a sourceStr is the string form UID, \a destUID is the binary UID.
*/

void SyntroUtils::UIDSTRtoUID(SYNTRO_UIDSTR sourceStr, SYNTRO_UID *destUID)
{
	int byteTemp;

	for (int i = 0; i < SYNTRO_MACADDR_LEN; i++) {
		sscanf(sourceStr + i * 2, "%02X", &byteTemp);
		destUID->macAddr[i] = byteTemp;
	}

	sscanf(sourceStr + SYNTRO_MACADDR_LEN * 2, "%04X", &byteTemp);
	convertIntToUC2(byteTemp, destUID->instance);
}

/*!
	Converts a binary form UID into a string form UID suitable for display.
	\a sourceUID is the binary form UID, \a destStr is the string form UID.
*/


void SyntroUtils::UIDtoUIDSTR(SYNTRO_UID *sourceUID, SYNTRO_UIDSTR destStr)
{
	for (int i = 0; i < SYNTRO_MACADDR_LEN; i++)
		sprintf(destStr + i * 2, "%02X", sourceUID->macAddr[i]);

	sprintf(destStr + SYNTRO_MACADDR_LEN * 2, "%04x", convertUC2ToUInt(sourceUID->instance));
}

/*!
	Returns a QString with the supplied binary form IP address in \a address. The QString is in the
	traditional dotted format - "xx.xx.xx.xx".
*/

QString SyntroUtils::displayIPAddr(SYNTRO_IPADDR address)
{
	return QString("%1.%2.%3.%4").arg(address[0]).arg(address[1]).arg(address[2]).arg(address[3]);
}

/*!
	Converts a dotted string form IP address in \a IPStr to a binary form IP address in \a IPAddr.
*/

void SyntroUtils::convertIPStringToIPAddr(char *IPStr, SYNTRO_IPADDR IPAddr)
{
	int	a[4];

	sscanf(IPStr, "%d.%d.%d.%d", a, a+1, a+2, a+3);
	
	for (int i = 0; i < 4; i++)
		IPAddr[i] = (unsigned char)a[i];
}

/*!
	Checks to see if the binary form IP address in \a addr is actually "0.0.0.0".

	Returns true if so, false otherwise.
*/

bool SyntroUtils::IPZero(SYNTRO_IPADDR addr)
{
	return (addr[0] == 0) && (addr[1] == 0) && (addr[2] == 0) && (addr[3] == 0);
}

/*!
	Checks to see if the binary form IP address in \a addr is actually "127.0.0.1".

	Returns true if so, false otherwise.
*/

bool SyntroUtils::IPLoopback(SYNTRO_IPADDR addr)
{
	return (addr[0] == 127) && (addr[1] == 0) && (addr[2] == 0) && (addr[3] == 1);
}


/*!
	Returns a QString with a printable version of the UID in \a uid.
*/

QString SyntroUtils::displayUID(SYNTRO_UID *uid)
{
	char temp[20];

	UIDtoUIDSTR(uid, temp);

	return QString(temp);
}

/*!
	Returns true if the binary form UIDs in \a a and \a b are equal, false otherwise.
*/

bool SyntroUtils::compareUID(SYNTRO_UID *a, SYNTRO_UID *b)
{
	return memcmp(a, b, sizeof(SYNTRO_UID)) == 0;
}

/*!
	Returns true if the binary form UID in \a a is numerically higher than the one in \a b, false otherwise.
*/

bool SyntroUtils::UIDHigher(SYNTRO_UID *a, SYNTRO_UID *b)
{
	return memcmp(a, b, sizeof(SYNTRO_UID)) > 0;
}

/*!
	This function swaps the source and destination UIDs and ports in the SYNTRO_EHEAD structure in \a ehead. Often used
	to return a Syntro message to its origin.
*/

void SyntroUtils::swapEHead(SYNTRO_EHEAD *ehead)
{
	swapUID(&(ehead->sourceUID), &(ehead->destUID));

	int i = convertUC2ToInt(ehead->sourcePort);

	copyUC2(ehead->sourcePort, ehead->destPort);

	convertIntToUC2(i, ehead->destPort);
}

/*!
	This function swaps the contents of the twp binary form UIDs \a a and \a b.
*/

void SyntroUtils::swapUID(SYNTRO_UID *a, SYNTRO_UID *b)
{
	SYNTRO_UID temp;

	memcpy(&temp, a, sizeof(SYNTRO_UID));
	memcpy(a, b, sizeof(SYNTRO_UID));
	memcpy(b, &temp, sizeof(SYNTRO_UID));
}

//	UC2, UC4 and UC8 Conversion routines
//
//	*** Note: 32 bit int assumed ***
//

/*!
	Returns the int64 value of the SYNTRO_UC8 variable \a uc8.
*/

qint64 SyntroUtils::convertUC8ToInt64(SYNTRO_UC8 uc8)
{
	return ((qint64)uc8[0] << 56) | ((qint64)uc8[1] << 48) | ((qint64)uc8[2] << 40) | ((qint64)uc8[3] << 32) |
		((qint64)uc8[4] << 24) | ((qint64)uc8[5] << 16) | ((qint64)uc8[6] << 8) | ((qint64)uc8[7] << 0);
}

/*!
	Converts an int64 value in \a val into a SYNTRO_UC8 variable \a uc8.
*/

void SyntroUtils::convertInt64ToUC8(qint64 val, SYNTRO_UC8 uc8)
{
	uc8[7] = val & 0xff;
	uc8[6] = (val >> 8) & 0xff;
	uc8[5] = (val >> 16) & 0xff;
	uc8[4] = (val >> 24) & 0xff;
	uc8[3] = (val >> 32) & 0xff;
	uc8[2] = (val >> 40) & 0xff;
	uc8[1] = (val >> 48) & 0xff;
	uc8[0] = (val >> 56) & 0xff;
}


/*!
	Returns the integer value of the SYNTRO_UC4 variable \a uc4.
*/

int	SyntroUtils::convertUC4ToInt(SYNTRO_UC4 uc4)
{
	return ((int)uc4[0] << 24) | ((int)uc4[1] << 16) | ((int)uc4[2] << 8) | uc4[3];
}

/*!
	Converts an integer value in \a val into a SYNTRO_UC4 variable \a uc4.
*/

void SyntroUtils::convertIntToUC4(int val, SYNTRO_UC4 uc4)
{
	uc4[3] = val & 0xff;
	uc4[2] = (val >> 8) & 0xff;
	uc4[1] = (val >> 16) & 0xff;
	uc4[0] = (val >> 24) & 0xff;
}

/*!
	Returns the integer value of the SYNTRO_UC2 variable \a uc2.
*/

int	SyntroUtils::convertUC2ToInt(SYNTRO_UC2 uc2)
{
	int val = ((int)uc2[0] << 8) | uc2[1];
	
	if (val & 0x8000)
		val |= 0xffff0000;
	
	return val;
}

/*!
	Returns the unsigned integer value of the SYNTRO_UC2 variable \a uc2.
*/

int SyntroUtils::convertUC2ToUInt(SYNTRO_UC2 uc2)
{
	return ((int)uc2[0] << 8) | uc2[1];
}

/*!
	Converts an integer value in \a val into a SYNTRO_UC2 variable \a uc2.
*/

void SyntroUtils::convertIntToUC2(int val, SYNTRO_UC2 uc2)
{
	uc2[1] = val & 0xff;
	uc2[0] = (val >> 8) & 0xff;
}

/*!
	Copies the contents of the SYNTRO_UC2 variable \a src into \a dst.
*/


void SyntroUtils::copyUC2(SYNTRO_UC2 dst, SYNTRO_UC2 src)
{
	dst[0] = src[0];
	dst[1] = src[1];
}

/*!
	Generates and returns a pointer to a SYNTRO_EHEAD structure with anough space for appended data. 
	\a sourceUID is the binary form
	UID source address, \a sourcePort is the source service port number, \a destUID is the binary
	form destination UID, \a destPort is the destination service port number, \a seq is the send
	sequence number to use and \a len is the length of the extra data to be allocated after the 
	SYNTRO_EHEAD structure.

	The length of extra allocated data can be 0 if just the SYNTRO_EHEAD structure itself is needed.

	The memory for the SYNTRO_EHEAD is malloced and therefore needs to be freed at some later date.
*/

SYNTRO_EHEAD *SyntroUtils::createEHEAD(SYNTRO_UID *sourceUID, int sourcePort, SYNTRO_UID *destUID, int destPort, 
										unsigned char seq, int len)
{
	SYNTRO_EHEAD *ehead;

	ehead = (SYNTRO_EHEAD *)malloc(len + sizeof(SYNTRO_EHEAD));
	memcpy(&(ehead->sourceUID), sourceUID, sizeof(SYNTRO_UID));
	convertIntToUC2(sourcePort, ehead->sourcePort);									
	memcpy(&(ehead->destUID), destUID, sizeof(SYNTRO_UID));
	convertIntToUC2(destPort, ehead->destPort);									
	ehead->seq = seq;

	return ehead;
}

/*!
	This function takes a service path in \a servicePath and breaks it up into its constituent components,
	\a regionName, \a componentName and \a serviceName.

	It returns true if the service path is valid, false otherwise.
*/

bool SyntroUtils::crackServicePath(QString servicePath, QString& regionName, QString& componentName, QString& serviceName)
{
	regionName.clear();
	componentName.clear();
	serviceName.clear();
 
	QStringList stringList = servicePath.split(SYNTRO_SERVICEPATH_SEP);

	if (stringList.count() > 3) {
		logWarn(QString("Service path received has invalid format ") + servicePath);
		return false;
	}
	switch (stringList.count()) {
		case 1:
			serviceName = stringList.at(0);
			break;

		case 2:
			serviceName = stringList.at(1);
			componentName = stringList.at(0);
			break;

		case 3:
			serviceName = stringList.at(2);
			componentName = stringList.at(1);
			regionName = stringList.at(0);
	}
	return true;
}

/*!
	This function is normally called from within main.cpp of a Syntro app before the main code begins.
	It makes sure that settings common to all applications are set up correctly in the .ini file and
	processes the runtime arguments. 

	Supported runtime arguments are:

	\list
	\li -a<adaptor>: adaptor. Can be used to specify a particular network adaptor for use. an example would be
	-aeth0.
	\li -c: console mode. If present, the app should run in console mode rather than windowed mode.
	\li -d: daemon mode. If present, the app should run in daemon mode
	\li -s<path>: settings file path. By default, the app will use the file <apptype>.ini in the working directory.
	The -s option can be used to override that and specify a path to anywhere in the file system.
	\endlist

	\a appType is the application type string, \a arglist is the list of arguments supplied to the app.
*/

void SyntroUtils::loadStandardSettings(const char *appType, QStringList arglist)
{
	QString args;
	QString adapter;									// IP adaptor name
	QSettings *settings = NULL;

	m_iniPath = "";
	adapter = "";

	for (int i = 1; i < arglist.size(); i++) {			// skip the first string as that is the program name
		QString opt = arglist.at(i);					// get the param

		if (opt.at(0).toLatin1() != '-')
			continue;									// doesn't start with '-'

		char optCode = opt.at(1).toLatin1();				// get the option code
		opt.remove(0, 2);								// remove the '-' and the code

		switch (optCode) {
		case 's':
			m_iniPath = opt;
			break;

		case 'a':
			adapter = opt;
			break;

		case 'c':
		// console app flag, handled elsewhere
			break;

        case 'd':
        // daemon mode flag, handled elsewhere
            break;

        default:
			logWarn(QString("Unrecognized argument option %1").arg(optCode));
		}
	}

	// user can override the default location for the ini file
	if (m_iniPath.size() > 0) {
		// make sure the file specified is usable
		QFileInfo fileInfo(m_iniPath);

		if (fileInfo.exists()) {
			if (!fileInfo.isWritable())
				logWarn(QString("Ini file is not writable: %1").arg(m_iniPath));

			if (!fileInfo.isReadable())
				logWarn(QString("Ini file is not readable: %1").arg(m_iniPath));
		}
		else {
			QFileInfo dirInfo(fileInfo.path());

			if (!dirInfo.exists()) {
				logWarn(QString("Directory for ini file does not exist: %1").arg(fileInfo.path()));
			}
			else {
				if (!dirInfo.isWritable())
					logWarn(QString("Directory for ini file is not writable: %1").arg(fileInfo.path()));

				if (!dirInfo.isReadable())
					logWarn(QString("Directory for ini file is not readable: %1").arg(fileInfo.path()));
			}
		}

		// regardless of how the checks turned out, we don't want to fall back to the
		// default as that might overwrite an existing good default configuration
		// i.e. the common reason for passing an ini file is to start a second instance
		// of the app
		settings = new QSettings(m_iniPath, QSettings::IniFormat);
	}
	else {
		settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "Syntro", appType);

		// need this for subsequent opens
		m_iniPath = settings->fileName();
	}

	// Save settings generated from command line arguments

	settings->setValue(SYNTRO_RUNTIME_PATH, m_iniPath);

	if (adapter.length() > 0) {
		settings->setValue(SYNTRO_RUNTIME_ADAPTER, adapter);
	} else {
		// we don't want to overwrite an existing entry so check first
		adapter = settings->value(SYNTRO_RUNTIME_ADAPTER).toString();

		// but we still want to put a default placeholder
		if (adapter.length() < 1)
			settings->setValue(SYNTRO_RUNTIME_ADAPTER, adapter);
	}

	//	Make sure common settings are present or else generate defaults

	if (!settings->contains(SYNTRO_PARAMS_APPNAME))
		settings->setValue(SYNTRO_PARAMS_APPNAME, QHostInfo::localHostName());		// use hostname for app name

	m_appName = settings->value(SYNTRO_PARAMS_APPNAME).toString();

	//	Force type to be mine always

	settings->setValue(SYNTRO_PARAMS_APPTYPE, appType);
	m_appType = appType;

	//	Add in SyntroControl array

 	int	size = settings->beginReadArray(SYNTRO_PARAMS_CONTROL_NAMES);
	settings->endArray();

	if (size == 0) {
		settings->beginWriteArray(SYNTRO_PARAMS_CONTROL_NAMES);

		for (int control = 0; control < ENDPOINT_MAX_SYNTROCONTROLS; control++) {
			settings->setArrayIndex(control);
			settings->setValue(SYNTRO_PARAMS_CONTROL_NAME, "");
		}
		settings->endArray();
	}

	if (!settings->contains(SYNTRO_PARAMS_CONTROLREVERT))
		settings->setValue(SYNTRO_PARAMS_CONTROLREVERT, 0);		
	
	if (!settings->contains(SYNTRO_PARAMS_LOCALCONTROL))
        settings->setValue(SYNTRO_PARAMS_LOCALCONTROL, true);
	
	if (!settings->contains(SYNTRO_PARAMS_LOCALCONTROL_PRI))
		settings->setValue(SYNTRO_PARAMS_LOCALCONTROL_PRI, 0);		
	
	if (!settings->contains(SYNTRO_PARAMS_HBINTERVAL))
		settings->setValue(SYNTRO_PARAMS_HBINTERVAL, SYNTRO_HEARTBEAT_INTERVAL);

	if (!settings->contains(SYNTRO_PARAMS_HBTIMEOUT))
		settings->setValue(SYNTRO_PARAMS_HBTIMEOUT, SYNTRO_HEARTBEAT_TIMEOUT);

	if (!settings->contains(SYNTRO_PARAMS_LOG_HBINTERVAL))
		settings->setValue(SYNTRO_PARAMS_LOG_HBINTERVAL, SYNTRO_LOG_HEARTBEAT_INTERVAL);

	if (!settings->contains(SYNTRO_PARAMS_LOG_HBTIMEOUT))
		settings->setValue(SYNTRO_PARAMS_LOG_HBTIMEOUT, SYNTRO_LOG_HEARTBEAT_TIMEOUT);

    settings->sync();
	delete settings;
}

/*!
	Returns a pointer to QSettings object for the app's .ini file. Note: the caller must
	delete the QSettiongs object at some point and must not pass it to another thread.
*/

QSettings *SyntroUtils::getSettings()
{
	return new QSettings(m_iniPath, QSettings::IniFormat);
}

/*!
	Returns true if the character in \a value is a character that is not permitted in service names, false otherwise.
*/

bool SyntroUtils::isReservedNameCharacter(char value)
{
	switch (value) {
		case SYNTROCFS_FILENAME_SEP:
		case SYNTRO_SERVICEPATH_SEP:
		case SYNTRO_LOG_COMPONENT_SEP:
		case ' ' :
		case '\\':
			return true;

		default:
			return false;
	}
}

/*!
	Returns true if the character in \a value is a character that is not permitted in service paths, false otherwise.
*/

bool SyntroUtils::isReservedPathCharacter(char value)
{
	switch (value) {
		case SYNTROCFS_FILENAME_SEP:
		case SYNTRO_LOG_COMPONENT_SEP:
		case ' ' :
		case '\\':
			return true;

		default:
			return false;
	}
}


/*!
	This funtion takes \a streamSource, a QString containing the app name of a stream source possibly with
	a concententated type qualifier (eg :lr for low rate video) and inserts the \a streamName at the
	coorrect point and returns the result as a new QString.

	For example "Ubuntu:lr" with a stream name of "video" would result in "Ubuntu/video:lr".

	In many apps, users may specify the app name and type qualifier but the stream name is fixed. This
	function provides a convenient way of integrating the two ready to activate a remote stream for example.
*/

QString SyntroUtils::insertStreamNameInPath(const QString& streamSource, const QString& streamName)
{
	 int index;
	 QString result;

	 index = streamSource.indexOf(SYNTRO_STREAM_TYPE_SEP);
	 if (index == -1) {
		 // there is no extension - just add stream name to the end
		 result = streamSource + "/" + streamName;
	 } else {
		 // there is an extension - insert stream name before extension
		 result = streamSource;
		 result.insert(index, QString("/") + streamName);
	 }
	 return result;
}

/*!
	This funtion takes \a servicePath, and return a \a streamSource that is the servicePath with
	the stream name removed. The stream name is returned in \a streamName. 

	For example "Ubuntu/video:lr" would return a streamSource of "ubuntu:lr" and a streamName of "video".

*/

void SyntroUtils::removeStreamNameFromPath(const QString& servicePath,
			QString& streamSource, QString& streamName)
{
	 int start, end;

	 start = servicePath.indexOf(SYNTRO_SERVICEPATH_SEP);	// find the "/"
	 if (start == -1) {										// not found
		 streamSource = servicePath;
		 streamName = "";
		 return;
	 }
	 end = servicePath.indexOf(SYNTRO_STREAM_TYPE_SEP);	// find the ":" if there is one

	 if (end == -1) {										// there isn't
		 streamSource = servicePath.left(start);
		 streamName = servicePath;
		 streamName.remove(0, start + 1);
		 return;
	 }

	 // We have all parts present

	 streamSource = servicePath;
	 streamSource.remove(start, end - start);				// take out stream name
	 streamName = servicePath;
	 streamName.remove(end, streamName.length());			// make sure everything at end removed
	 streamName.remove(0, start + 1);
}


/*!
	Returns the broadcast address of the selected network interface.
*/

QHostAddress SyntroUtils::getMyBroadcastAddress()
{
	return m_platformBroadcastAddress;
}


/*!
	Returns the subnet address of the selected network interface.
*/

QHostAddress SyntroUtils::getMySubnetAddress()
{
	return m_platformSubnetAddress;
}

/*!
	Returns the netmask of the selected network interface.
*/

QHostAddress SyntroUtils::getMyNetMask()
{
	return m_platformNetMask;
}

/*!
	Returns true if the supplied binary form IP address \a IPAddr is in the same subnet as this app.
*/

bool SyntroUtils::isInMySubnet(SYNTRO_IPADDR IPAddr)
{
	quint32 intaddr;

	if ((IPAddr[0] == 0) && (IPAddr[1] == 0) && (IPAddr[2] == 0) && (IPAddr[3] == 0))
		return true;
	if ((IPAddr[0] == 0xff) && (IPAddr[1] == 0xff) && (IPAddr[2] == 0xff) && (IPAddr[3] == 0xff))
		return true;
	intaddr = (IPAddr[0] << 24) + (IPAddr[1] << 16) + (IPAddr[2] << 8) + IPAddr[3];
	return (intaddr & m_platformNetMask.toIPv4Address()) == m_platformSubnetAddress.toIPv4Address(); 
}


/*!
	Provides a convenient way of checking to see if a timer has expired. \a now is the current
	time, usually the output of SyntroClock(). \a start is the start time of the timer, usually
	the value of SyntroClock when the timer was started. \a interval is the length of the timer.

	Returns true if the timer has expired, false otherwise. If \a interval is 0, the function always returns false,
	providing an easy way of disabling timers.
*/

bool SyntroUtils::syntroTimerExpired(qint64 now, qint64 start, qint64 interval)
{
	if (interval == 0)
		return false;										// can never expire
	return ((now - start) >= interval);
}

/*!
	\internal
*/

void SyntroUtils::getMyIPAddress()
{
	QNetworkInterface		cInterface;
	QNetworkAddressEntry	cEntry;
	QList<QNetworkAddressEntry>	cList;
	QHostAddress			haddr;
	quint32 intaddr;
	quint32 intmask;
	int addr[SYNTRO_MACADDR_LEN];

	QSettings *settings = SyntroUtils::getSettings();

	while (1) {
		QList<QNetworkInterface> ani = QNetworkInterface::allInterfaces();
		foreach (cInterface, ani) {
			QString name = cInterface.humanReadableName();
			logDebug(QString("Found IP adaptor %1").arg(name));

			if ((strcmp(qPrintable(name), qPrintable(settings->value(SYNTRO_RUNTIME_ADAPTER).toString())) == 0) || 
				(strlen(qPrintable(settings->value(SYNTRO_RUNTIME_ADAPTER).toString())) == 0)) {
				cList = cInterface.addressEntries();				// check to see if there's an address
				if (cList.size() > 0) {
					foreach (cEntry, cList) {
						haddr = cEntry.ip();
						intaddr = haddr.toIPv4Address();			// get it as four bytes
						if (intaddr == 0)
							continue;								// not real
	
						m_myIPAddr[0] = intaddr >> 24;
						m_myIPAddr[1] = intaddr >> 16;
						m_myIPAddr[2] = intaddr >> 8;
						m_myIPAddr[3] = intaddr;
						if (IPLoopback(m_myIPAddr) || IPZero(m_myIPAddr))
							continue;
						QString macaddr = cInterface.hardwareAddress();					// get the MAC address
						sscanf(macaddr.toLocal8Bit().constData(), "%x:%x:%x:%x:%x:%x", 
							addr, addr+1, addr+2, addr+3, addr+4, addr+5);

						for (int i = 0; i < SYNTRO_MACADDR_LEN; i++)
							m_myMacAddr[i] = addr[i];

						logInfo(QString("Using IP adaptor %1").arg(displayIPAddr(m_myIPAddr)));

						m_platformNetMask = cEntry.netmask();
						intmask = m_platformNetMask.toIPv4Address();
						intaddr &= intmask;
						m_platformSubnetAddress = QHostAddress(intaddr);
						intaddr |= ~intmask;
						m_platformBroadcastAddress = QHostAddress(intaddr);
						logInfo(QString("Subnet = %1, netmask = %2, bcast = %3")
							.arg(m_platformSubnetAddress.toString())
							.arg(m_platformNetMask.toString())
							.arg(m_platformBroadcastAddress.toString()));

						delete settings;

						return;
					}
				}
			}
		}

		TRACE1("Waiting for adapter %s", qPrintable(settings->value(SYNTRO_RUNTIME_ADAPTER).toString()));
		QThread::yieldCurrentThread();
	}
	delete settings;
}

/*!
	Sets the current mS resolution timestamp into \a timestamp.
*/

void SyntroUtils::setTimestamp(SYNTRO_UC8 timestamp)
{
	convertInt64ToUC8(QDateTime::currentMSecsSinceEpoch(), timestamp);
}

/*!
	Returns the timestamp in \a timestamp as a qint64.
*/

qint64 SyntroUtils::getTimestamp(SYNTRO_UC8 timestamp)
{
	return convertUC8ToInt64(timestamp);
}

//	Multimedia functions

void SyntroUtils::avmuxHeaderInit(SYNTRO_RECORD_AVMUX *avmuxHead, SYNTRO_AVPARAMS *avParams,
		int param, int recordIndex, int muxSize, int videoSize,int audioSize)
{
	memset(avmuxHead, 0, sizeof(SYNTRO_RECORD_AVMUX));

	// do the generic record header stuff

	convertIntToUC2(SYNTRO_RECORD_TYPE_AVMUX, avmuxHead->recordHeader.type);
	convertIntToUC2(avParams->avmuxSubtype, avmuxHead->recordHeader.subType);
	convertIntToUC2(sizeof(SYNTRO_RECORD_AVMUX), avmuxHead->recordHeader.headerLength);
	convertIntToUC2(param, avmuxHead->recordHeader.param);
	convertIntToUC4(recordIndex, avmuxHead->recordHeader.recordIndex);
	setTimestamp(avmuxHead->recordHeader.timestamp);

	// and the rest

	avmuxHead->videoSubtype = avParams->videoSubtype;
	avmuxHead->audioSubtype = avParams->audioSubtype;
	convertIntToUC4(muxSize, avmuxHead->muxSize);
	convertIntToUC4(videoSize, avmuxHead->videoSize);
	convertIntToUC4(audioSize, avmuxHead->audioSize);

	convertIntToUC2(avParams->videoWidth, avmuxHead->videoWidth);
	convertIntToUC2(avParams->videoHeight, avmuxHead->videoHeight);
	convertIntToUC2(avParams->videoFramerate, avmuxHead->videoFramerate);

	convertIntToUC4(avParams->audioSampleRate, avmuxHead->audioSampleRate);
	convertIntToUC2(avParams->audioChannels, avmuxHead->audioChannels);
	convertIntToUC2(avParams->audioSampleSize, avmuxHead->audioSampleSize);
}

void SyntroUtils::avmuxHeaderToAVParams(SYNTRO_RECORD_AVMUX *avmuxHead, SYNTRO_AVPARAMS *avParams)
{
	avParams->avmuxSubtype = convertUC2ToInt(avmuxHead->recordHeader.subType);

	avParams->videoSubtype = avmuxHead->videoSubtype;
	avParams->audioSubtype = avmuxHead->audioSubtype;

	avParams->videoWidth = convertUC2ToInt(avmuxHead->videoWidth);
	avParams->videoHeight = convertUC2ToInt(avmuxHead->videoHeight);
	avParams->videoFramerate = convertUC2ToInt(avmuxHead->videoFramerate);

	avParams->audioSampleRate = convertUC4ToInt(avmuxHead->audioSampleRate);
	avParams->audioChannels = convertUC2ToInt(avmuxHead->audioChannels);
	avParams->audioSampleSize = convertUC2ToInt(avmuxHead->audioSampleSize);
}

/*!
	Performs validation of an avmux record. \a avmuxHead is a pointer to the avmux record,
	\a length is its total length (header plus data). The function returns true if the header passes
	validation. This means that the lengths of the component parts
	correctly add up to the total length. If the function returns false
	then the avmux record should not be processed further.

	Additionally, the function can return useful data extracted form the header. \a muxPtr is a
	pointer to a pointer to the mux data part, \a muxLength is a reference to its length with
	the same for \a videoPtr, \a videoLength, \a audioPtr and \a audioLength. If NULL is passed as
	any of these pointers, that variable is not set.
*/

bool SyntroUtils::avmuxHeaderValidate(SYNTRO_RECORD_AVMUX *avmuxHead, int length,
				unsigned char **muxPtr, int& muxLength,
				unsigned char **videoPtr, int& videoLength,
				unsigned char **audioPtr, int& audioLength)
{
	muxLength = SyntroUtils::convertUC4ToInt(avmuxHead->muxSize);
	videoLength = SyntroUtils::convertUC4ToInt(avmuxHead->videoSize);
	audioLength = SyntroUtils::convertUC4ToInt(avmuxHead->audioSize);

    if (length != (sizeof(SYNTRO_RECORD_AVMUX) + muxLength + videoLength + audioLength))
		return false;

	if (muxPtr != NULL)
		*muxPtr = (unsigned char *)(avmuxHead + 1);

	if (videoPtr != NULL)
		*videoPtr = (unsigned char *)(avmuxHead + 1) + muxLength;

	if (audioPtr != NULL)
		*audioPtr = (unsigned char *)(avmuxHead + 1) + muxLength + videoLength;

	return true;
}


void SyntroUtils::videoHeaderInit(SYNTRO_RECORD_VIDEO *videoHead, int width, int height, int size)
{
	convertIntToUC2(SYNTRO_RECORD_TYPE_VIDEO, videoHead->recordHeader.type);
	convertIntToUC2(SYNTRO_RECORD_TYPE_VIDEO_MJPEG, videoHead->recordHeader.subType);
	convertIntToUC2(sizeof(SYNTRO_RECORD_VIDEO), videoHead->recordHeader.headerLength);

	convertIntToUC2(width, videoHead->width);
	convertIntToUC2(height, videoHead->height);
	setTimestamp(videoHead->recordHeader.timestamp);
	convertIntToUC4(size, videoHead->size);
}
