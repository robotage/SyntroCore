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

#ifndef _SYNTROUTILS_H_
#define _SYNTROUTILS_H_

#include "syntrolib_global.h"
#include "SyntroDefs.h"
#include "HelloDefs.h"
#include "LogWrapper.h"
#include "SyntroAV/SyntroAVDefs.h"

//	Standard Qt includes for SyntroLib

#include <qmutex.h>
#include <qabstractsocket.h>
#include <qtcpserver.h>
#include <qudpsocket.h>
#include <qtcpsocket.h>
#include <qthread.h>
#include <qapplication.h>
#include <qstringlist.h>
#include <qqueue.h>
#include <qnetworkinterface.h>
#include <qdatetime.h>
#include <qsettings.h>
#include <qlabel.h>

class SyntroSocket;
class SyntroClockObject;

//	Debug message and error display macros

#ifdef _DEBUG
#define	TRACE0(x) qDebug("%s", x)

#define	TRACE1(x, y) {qDebug(x, y);}

#define	TRACE2(x, y, z) {qDebug(x, y, z);}

#define	TRACE3(x, y, z, a) {qDebug(x, y, z, a);}
#else
#define	TRACE0(x) 

#define	TRACE1(x, y) 

#define	TRACE2(x, y, z) 

#define	TRACE3(x, y, z, a)
#endif

//		Syntro standard settings entries

#define	SYNTRO_RUNTIME_PATH				"runtimePath"		// runtime path
#define	SYNTRO_RUNTIME_ADAPTER			"runtimeAdapter"	// ethernet adapter

//	Standard operating parameter entries

#define	SYNTRO_PARAMS_APPNAME			"appName"			// name of the application
#define	SYNTRO_PARAMS_APPTYPE			"appType"			// type of the application
#define SYNTRO_PARAMS_CONTROLREVERT		"controlRevert"		// true if implement control priority with active connection
#define SYNTRO_PARAMS_LOCALCONTROL		"localControl"		// true if want to run a local SyntroControl
#define SYNTRO_PARAMS_LOCALCONTROL_PRI	"localControlPriority"	// local SyntroControl priority
#define	SYNTRO_PARAMS_HBINTERVAL		"heartbeatInterval"	// time between sent heartbeats
#define	SYNTRO_PARAMS_HBTIMEOUT			"heartbeatTimeout"	// number of hb intervals without hb before timeout
#define	SYNTRO_PARAMS_LOG_HBINTERVAL	"logHeartbeatInterval"	// time between sent heartbeats for log
#define	SYNTRO_PARAMS_LOG_HBTIMEOUT		"logHeartbeatTimeout"	// number of hb intervals without hb before timeout for log
#define SYNTRO_PARAMS_ENCRYPT_LINK      "encryptLink"       // true if use SSL for links

#define	SYNTRO_PARAMS_CONTROL_NAMES		"controlNames"		// ordered list of SyntroControls as an array
#define	SYNTRO_PARAMS_CONTROL_NAME		"controlName"		// an entry in the array

//	Standard logging entries

#define SYNTRO_PARAMS_LOG_GROUP			"Logging"			// log settings group tag
#define SYNTRO_PARAMS_LOGLEVEL			"logLevel"			// error|warn|info|debug, default is info
#define SYNTRO_PARAMS_DISK_LOG			"diskLog"			// implement local disk logging, true|false, default true
#define SYNTRO_PARAMS_NET_LOG			"netLog"			// implement network logging, true|false, default true
#define SYNTRO_PARAMS_LOG_KEEP			"logKeep"			// the number of log files to keep, default is SYNTRO_DEFAULT_LOG_KEEP
#define SYNTRO_PARAMS_LOG_MAXDISKSIZE 	"maxDiskSize"		// the max size in kbytes any log file can grow to or else cause a rotation

#define	SYNTRO_PARAMS_TRUE				"true"				// for true/false settings
#define	SYNTRO_PARAMS_FALSE				"false"

#define SYNTRO_DEFAULT_LOG_KEEP 1							// default o keeping one log file
#define SYNTRO_DEFAULT_MAX_DISK_LOG_SIZE 1000				// default is to rotate after 1Mb of log

//	Syntro message log level defs

#define SYNTRO_LOG_LEVEL_ERROR 0
#define SYNTRO_LOG_LEVEL_WARN 1
#define SYNTRO_LOG_LEVEL_INFO 2
#define SYNTRO_LOG_LEVEL_DEBUG 3

#define SYNTRO_LOG_SERVICE_TAG			":log"

//	Standard stream entries

#define SYNTRO_PARAMS_STREAM_SOURCES	"streamSources"
#define SYNTRO_PARAMS_STREAM_SOURCE		"source"

//	Standard entries for SyntroClient
//
//	"local" services are those published by this client.
//	"remote" services are those to which this client subscribes
//	Each array entry consists of a name, local or remote and type for a service

#define	SYNTRO_PARAMS_CLIENT_SERVICES		"clientServices"	// the array containing the service names, location and types
#define	SYNTRO_PARAMS_CLIENT_SERVICE_NAME	"clientServiceName"	// the name of a service
#define	SYNTRO_PARAMS_CLIENT_SERVICE_TYPE	"clientServiceType"	// multicast or E2E
#define	SYNTRO_PARAMS_CLIENT_SERVICE_LOCATION "clientServiceLocation" // local or remote

#define SYNTRO_PARAMS_SERVICE_LOCATION_LOCAL  "local"		// local service
#define SYNTRO_PARAMS_SERVICE_LOCATION_REMOTE "remote"		// remote service

#define SYNTRO_PARAMS_SERVICE_TYPE_MULTICAST	"multicast"	// the multicast service type string
#define SYNTRO_PARAMS_SERVICE_TYPE_E2E			"E2E"		// the E2E service type string

//	Standard SyntroCFS entries

#define	SYNTRO_PARAMS_CFS_STORES		"CFSStores"			// SyntroCFS stores array
#define	SYNTRO_PARAMS_CFS_STORE			"CFSStore"			// the entries

class SYNTROLIB_EXPORT SyntroUtils
{
public:
	static const char *syntroLibVersion();			// returns a string containing the SyntroLib version as x.y.z

//	Settings functions
	
	static void loadStandardSettings(const char *appType, QStringList arglist);
	static QSettings *getSettings();

//	app init and exit

	static void syntroAppInit();
	static void syntroAppExit();

	static const QString& getAppName();
	static const QString& getAppType();

	static SYNTRO_IPADDR *getMyIPAddr();
	static SYNTRO_MACADDR *getMyMacAddr();

	static bool checkConsoleModeFlag(int argc, char *argv[]);	// checks if console mode
	static bool checkDaemonModeFlag(int argc, char *argv[]);
    static bool isSendOK(unsigned char sendSeq, unsigned char ackSeq);
    static SYNTRO_EHEAD *createEHEAD(SYNTRO_UID *sourceUID, int sourcePort, 
					SYNTRO_UID *destUID, int destPort, unsigned char seq, int len); 
    static void swapEHead(SYNTRO_EHEAD *ehead);		// swaps UIDs and port numbers
    static bool crackServicePath(QString servicePath, QString &regionName, QString& componentName, QString& serviceName); // breaks a service path into its constituent bits
    static bool syntroTimerExpired(qint64 now, qint64 start, qint64 interval);

//	Multimedia stream functions

	static void avmuxHeaderInit(SYNTRO_RECORD_AVMUX *avmuxHead, SYNTRO_AVPARAMS *avParams,
		int param, int recordIndex, int muxSize, int videoSize,int audioSize);
	static void avmuxHeaderToAVParams(SYNTRO_RECORD_AVMUX *avmuxHead, SYNTRO_AVPARAMS *avParams);
	static bool avmuxHeaderValidate(SYNTRO_RECORD_AVMUX *avmuxHead, int length,
				unsigned char **muxPtr, int& muxLength,
				unsigned char **videoPtr, int& videoLength,
				unsigned char **audioPtr, int& audioLength);
	static void videoHeaderInit(SYNTRO_RECORD_VIDEO *videoHead, int width, int height, int size);


//	Service path functions

    static bool isReservedNameCharacter(char value);	// returns true if value is not allowed in names (components of paths)
    static bool isReservedPathCharacter(char value);	// returns true if value is not allowed in paths
    static QString insertStreamNameInPath(const QString& streamSource, const QString& streamName); // adds in a stream name before any extension 
    static void removeStreamNameFromPath(const QString& servicePath,
			QString& streamSource, QString& streamName); // extracts a stream name before any extension 

//	IP Address functions

    static void getMyIPAddress();						// gets my IP address from first or known IP adaptor
    static QString displayIPAddr(SYNTRO_IPADDR addr);	// returns string version of IP
    static void convertIPStringToIPAddr(char *IPStr, SYNTRO_IPADDR IPAddr);	// converts a string IP address to internal
    static bool IPZero(SYNTRO_IPADDR addr);			// returns true if address all zeroes
    static bool IPLoopback(SYNTRO_IPADDR addr);		// returns true if address is 127.0.0.1

//	UID manipulation functions

    static void UIDSTRtoUID(SYNTRO_UIDSTR sourceStr, SYNTRO_UID *destUID);// converts a string UID to a binary
    static void UIDtoUIDSTR(SYNTRO_UID *sourceUID, SYNTRO_UIDSTR destStr);// converts a binary UID to a string
    static void makeUIDSTR(SYNTRO_UIDSTR UIDStr, SYNTRO_MACADDRSTR macAddress, int instance); // Fills in the UID
    static QString displayUID(SYNTRO_UID *UID);			// return string version of UID
    static bool compareUID(SYNTRO_UID *a, SYNTRO_UID *b);	// return true if UIDs are equal
    static bool UIDHigher(SYNTRO_UID *a, SYNTRO_UID *b);	// returns true if a > b numerically
    static void swapUID(SYNTRO_UID *a, SYNTRO_UID *b);

//	Syntro type conversion functions

    static qint64 convertUC8ToInt64(SYNTRO_UC8 uc8);
    static void convertInt64ToUC8(qint64 val, SYNTRO_UC8 uc8);
    static int convertUC4ToInt(SYNTRO_UC4 uc4);
    static void convertIntToUC4(int val, SYNTRO_UC4 uc4);
    static int convertUC2ToInt(SYNTRO_UC2 uc2);
    static int convertUC2ToUInt(SYNTRO_UC2 uc2);
    static void convertIntToUC2(int val, SYNTRO_UC2 uc2);
    static void copyUC2(SYNTRO_UC2 dst, SYNTRO_UC2 src);

//	Syntro timestamp functions

	static void setTimestamp(SYNTRO_UC8 timestamp);
	static qint64 getTimestamp(SYNTRO_UC8 timestamp);

//	Information for the network address in use

    static QHostAddress getMyBroadcastAddress();
    static QHostAddress getMySubnetAddress();
    static QHostAddress getMyNetMask();
    static bool isInMySubnet(SYNTRO_IPADDR IPAddr);

private:
	static QString m_logTag;
	static SYNTRO_IPADDR m_myIPAddr;						
	static SYNTRO_MACADDR m_myMacAddr;						

	static QString m_iniPath;								
	static QString m_appName;
	static QString m_appType;

	static SyntroClockObject *m_syntroClockGen;			

	//	The address info for the adaptor being used

	static QHostAddress m_platformBroadcastAddress;					
	static QHostAddress m_platformSubnetAddress;					
	static QHostAddress m_platformNetMask;	
};

#endif		//_SYNTROUTILS_H_
