//
//  Copyright (c) 2014 Scott Ellis and Richard Barnett
//	
//  This file is part of SyntroControlLib
//
//  SyntroControlLib is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  SyntroControlLib is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with SyntroControlLib.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef SYNTROSERVER_H
#define SYNTROSERVER_H

#include "SyntroLib.h"
#include "DirectoryManager.h"
#include "MulticastManager.h"
#include "FastUIDLookup.h"
#include "syntrocontrollib_global.h"
#include "SyntroComponentData.h"

#include <qstringlist.h>

//	SyntroServer settings

#define SYNTROCONTROL_PARAMS_GROUP						"SyntroControl" // group name for SyntroControl-specific settings
#define	SYNTROCONTROL_PARAMS_LISTEN_LOCAL_SOCKET		"LocalSocket"	// port number for local links
#define	SYNTROCONTROL_PARAMS_LISTEN_STATICTUNNEL_SOCKET	"StaticTunnelSocket"	// port number for static tunnels

#define	SYNTROCONTROL_PARAMS_LISTEN_LOCAL_SOCKET_ENCRYPT		"LocalSocketEncrypt"	// port number for encrypted local links
#define	SYNTROCONTROL_PARAMS_LISTEN_STATICTUNNEL_SOCKET_ENCRYPT	"StaticTunnelSocketEncrypt"	// port number for encrypted static tunnels

#define SYNTROCONTROL_PARAMS_ENCRYPT_LOCAL              "EncryptLocal"  // true if local connections using SSL
#define SYNTROCONTROL_PARAMS_ENCRYPT_STATICTUNNEL_SERVER "EncryptStaticTunnelServer" // true if static tunnel server connections using SSL

#define SYNTROCONTROL_PARAMS_HBINTERVAL					"controlHeartbeatInterval"	// interval between heartbeats in seconds
#define SYNTROCONTROL_PARAMS_HBTIMEOUT					"controlHeartbeatTimeout"	// heartbeat intervals before timeout

#define SYNTROCONTROL_PARAMS_VALID_TUNNEL_SOURCES   "ValidTunnelSources"    // UIDs of valid tunnel sources
#define SYNTROCONTROL_PARAMS_VALID_TUNNEL_UID       "ValidTunnelUID"        // the array entry

//	Static tunnel settings defs

#define	SYNTROCONTROL_PARAMS_STATIC_TUNNELS			"StaticTunnels"	// the group name
#define	SYNTROCONTROL_PARAMS_STATIC_NAME			"StaticName"	// name of the tunnel
#define	SYNTROCONTROL_PARAMS_STATIC_DESTIP_PRIMARY	"StaticPrimary"	// the primary destination IP address
#define	SYNTROCONTROL_PARAMS_STATIC_DESTIP_BACKUP	"StaticBackup"	// the backup IP port
#define SYNTROCONTROL_PARAMS_STATIC_ENCRYPT         "StaticEncrypt" // true if use SSL

//		SyntroControl SyntroServer Thread Messages

#define	SYNTROSERVER_ONCONNECT_MESSAGE		(SYNTRO_MSTART+0)
#define	SYNTROSERVER_ONACCEPT_MESSAGE		(SYNTRO_MSTART+1)
#define	SYNTROSERVER_ONCLOSE_MESSAGE		(SYNTRO_MSTART+2)
#define	SYNTROSERVER_ONRECEIVE_MESSAGE		(SYNTRO_MSTART+3)
#define	SYNTROSERVER_ONSEND_MESSAGE			(SYNTRO_MSTART+4)
#define	SYNTROSERVER_ONACCEPT_STATICTUNNEL_MESSAGE (SYNTRO_MSTART+5)

#define	SYNTROSERVER_SOCKET_RETRY			(2 * SYNTRO_CLOCKS_PER_SEC)
#define	SYNTROSERVER_STATS_INTERVAL			(2 * SYNTRO_CLOCKS_PER_SEC)

class SyntroTunnel;


enum ConnState
{
	ConnIdle,												// if not doing anything
	ConnWFHeartbeat,										// if waiting for first heartbeat
	ConnNormal,												// the normal state
};


//	SS_COMPONENT is the data structure used to hold information about a connection to a component

typedef struct
{
	bool inUse;												// true if being used
	bool tunnelSource;										// if the source end of a SyntroControl tunnel
	bool tunnelDest;										// if it is the dest end of a tunnel
	bool tunnelStatic;										// if it is a static tunnel
    bool tunnelEncrypt;                                     // is use SSL for tunnel
	QString tunnelStaticName;								// name of the static tunnel
	QString tunnelStaticPrimary;							// static tunnel primary IP address
	QString tunnelStaticBackup;								// static tunnel backup IP address

	SyntroThread *widgetThread;								// the thread pointer if a widget link
	int widgetMessageID;
	int index;												// position of entry in array
	SyntroLink *syntroLink;									// the link manager class
	SYNTRO_IPADDR compIPAddr;								// the component's IP address
	int compPort;											// the component's port
	SyntroSocket *sock;										// socket for SyntroLink connection
	int connectionID;										// its connection ID
  
	// Heartbeat system vars

	SYNTRO_HEARTBEAT heartbeat;								// a copy of the heartbeat message
	qint64 lastHeartbeatReceived;							// time last heartbeat was received
	qint64 lastHeartbeatSent;								// time last heartbeat was send (used by tunnel source)
	qint64 heartbeatInterval;								// expected receive interval (from heartbeat itself) or send interval if tunnel source
	qint64 heartbeatTimeoutPeriod;							// if no heartbeats received for this time, time out the SyntroLink

	int state;												// the connection's state
	SyntroTunnel *syntroTunnel;								// the tunnel class if it is a tunnel source
	char *dirEntry;											// this is the currently in use DE
	int dirEntryLength;										// and its length (can't use strlen as may have multiple components)
	DM_CONNECTEDCOMPONENT *dirManagerConnComp;				// this is the directory manager entry for this connection

	quint64 tempRXByteCount;								// for receive byte rate calculation
	quint64 tempTXByteCount;								// for transmit byte rate calculation
	quint64 RXByteCount;									// receive byte count
	quint64 TXByteCount;									// transmit byte count
	quint64 RXByteRate;										// receive byte rate
	quint64 TXByteRate;										// transmit byte rate

	quint32 tempRXPacketCount;								// for receive byte rate calculation
	quint32 tempTXPacketCount;								// for transmit byte rate calculation
	quint32 RXPacketCount;									// receive byte count
	quint32 TXPacketCount;									// transmit byte count
	quint32 RXPacketRate;									// receive byte rate
	quint32 TXPacketRate;									// transmit byte rate

	qint64 lastStatsTime;									// last time stats were updated

} SS_COMPONENT;

// SyntroServer

class SYNTROCONTROLLIB_EXPORT SyntroServer : public SyntroThread
{
	Q_OBJECT

    friend class SyntroTunnel;

public:
	SyntroServer();						
	virtual ~SyntroServer();

	Hello *getHelloObject();								// returns the hello object

	int m_socketNumber;										// the socket to listen on - defaults to SYNTRO_ACTIVE_SOCKET_LOCAL
	int m_staticTunnelSocketNumber;							// the socket for static tunnels - defaults to SYNTRO_ACTIVE_SOCKET_TUNNEL
	int m_socketNumberEncrypt;								// the SSL socket to listen on - defaults to SYNTRO_ACTIVE_SOCKET_LOCAL_ENCRYPT
	int m_staticTunnelSocketNumberEncrypt;					// the SSL socket for static tunnels - defaults to SYNTRO_ACTIVE_SOCKET_TUNNEL_ENCRYPT
    
    bool m_netLogEnabled;									// true if network logging enabled
	QString m_appName;										// this SyntroControl's app name

	SYNTRO_UID m_myUID;										// my UID - used for local service processing

	SS_COMPONENT m_components[SYNTRO_MAX_CONNECTEDCOMPONENTS]; // the Component array

	DirectoryManager m_dirManager;							// the directory manager object
	MulticastManager m_multicastManager;					// the multicast manager object
	FastUIDLookup m_fastUIDLookup;						// the fast UID lookup object

	bool sendSyntroMessage(SYNTRO_UID *uid, int cmd, SYNTRO_MESSAGE *message, int length, int priority);
	void setComponentSocket(SS_COMPONENT *syntroComponent, SyntroSocket *sock); // allocate a socket to this component

	qint64 m_multicastIn;									// total multicast in count
	unsigned m_multicastInRate;								// rate accumulator
	qint64 m_multicastOut;									// total multicast out count
	unsigned m_multicastOutRate;							// rate accumulator

	SyntroComponentData m_componentData;

	QString m_logTag;

//------------------------------------------------------------------

signals:
	void DMDisplay(DirectoryManager *dirManager);
	void UpdateSyntroStatusBox(int, QStringList);
	void UpdateSyntroDataBox(int, QStringList);
	void serverMulticastUpdate(qint64 in, unsigned inRate, qint64 out, unsigned outRate);
	void serverE2EUpdate(qint64 in, unsigned inRate, qint64 out, unsigned outRate);


protected:

	void initThread();
	void finishThread();
	void timerEvent(QTimerEvent *event);
	void loadStaticTunnels(QSettings *settings);
    void loadValidTunnelSources(QSettings *settings);
	bool processMessage(SyntroThreadMsg* msg);
	bool openSockets();										// open the sockets SyntroControl needs
    int getNextConnectionID();                              // gets the next free connection ID

	bool syConnected(SS_COMPONENT *syntroComponent);
	bool syAccept(bool staticTunnel);
	void syClose(SS_COMPONENT *syntroComponent);
	SyntroSocket *getNewSocket(bool staticTunnel);
	SS_COMPONENT *getFreeComponent();
	void processHelloBeacon(HELLO *hello);
	void processHelloUp(HELLOENTRY *helloEntry);
	void processHelloDown(HELLOENTRY *helloEntry);
	void syntroBackground();
	void sendHeartbeat(SS_COMPONENT *syntroComponent);
	void sendTunnelHeartbeat(SS_COMPONENT *syntroComponent);
	void setupComponentStatus();


	void setComponentDE(char *pDE, int nLen, SS_COMPONENT *pComp);
	void syCleanup(SS_COMPONENT *pSC);
	void updateSyntroStatus(SS_COMPONENT *syntroComponent);
	void updateSyntroData(SS_COMPONENT *syntroComponent);


//	forwardE2EMessage - forward an endpoint to endpoint message

	void forwardE2EMessage(SYNTRO_MESSAGE *message, int length);

//	forwardMulticastMessage - forwards a multicastmessage to the registered remote Components

	void forwardMulticastMessage(SS_COMPONENT *syntroComponent, int cmd, SYNTRO_MESSAGE *message, int length);

//	findComponent - maps an identifier to a component

	SS_COMPONENT *findComponent(SYNTRO_IPADDR compAddr, int compPort);

	SyntroSocket *m_listSyntroLinkSock;						// local listener socket

	SyntroSocket *m_listStaticTunnelSock;					// static tunnel listener socket

	QMutex m_lock;
	Hello *m_hello;

	SS_COMPONENT *getComponentFromConnectionID(int connectionID); // uses m_connectioIDMap to get a component pointer
	void processReceivedData(SS_COMPONENT *syntroComponent);
	void processReceivedDataDemux(SS_COMPONENT *syntroComponent, int cmd, int length, SYNTRO_MESSAGE *message);
	qint64 m_heartbeatSendInterval;							// the initial interval for apps and the send interval for tunnel sources
	int m_heartbeatTimeoutCount;							// number of heartbeat periods before SyntroLink timed out

	int m_connectionIDMap[SYNTRO_MAX_CONNECTIONIDS];		// maps connection IDs to component index
    int m_nextConnectionID;									// used to allocate unique IDs to socket connections

	qint64 m_E2EIn;											// total multicast in count
	unsigned m_E2EInRate;									// rate accumulator
	qint64 m_E2EOut;										// total multicast out count
	unsigned m_E2EOutRate;									// rate accumulator
	qint64 m_counterStart;									// rate counter start time

    bool m_encryptLocal;                                    // if use SSL for local connections
    bool m_encryptStaticTunnelServer;                       // if use SSL for tunnel service

	qint64 m_lastOpenSocketsTime;							// last time open sockets failed

    QList<SYNTRO_UID> m_validTunnelSources;                 // list of valid UIDs that can be tunnel sources

private:

	inline void updateTXStats(SS_COMPONENT *syntroComponent, int length) {
				syntroComponent->tempTXPacketCount++;
				syntroComponent->TXPacketCount++;
				syntroComponent->tempTXByteCount += length;
				syntroComponent->TXByteCount += length;
	}

	int m_timer;

};

#endif // SYNTROSERVER_H
