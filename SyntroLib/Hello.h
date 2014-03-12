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

#ifndef _HELLO_H_
#define _HELLO_H_

#include "syntrolib_global.h"
#include "SyntroUtils.h"
#include "SyntroThread.h"
#include"SyntroComponentData.h"

#define HELLO_CONTROL_HELLO_INTERVAL	2000				// send hello every two seconds if control

typedef struct
{
	bool inUse;												// true if entry being used
	HELLO hello;											// received hello
	char IPAddr[SYNTRO_IPSTR_LEN];							// a convenient string form of address
	qint64 lastHello;										// time of last hello
} HELLOENTRY;


class	SyntroSocket;

class SYNTROLIB_EXPORT Hello : public SyntroThread
{
	Q_OBJECT

public:
	Hello(SyntroComponentData *data, const QString& logTag);
	~Hello();

	void sendHelloBeacon();									// sends a hello to SyntroControls to elicit a response
	void sendHelloBeaconResponse(HELLO *hello);				// responds to a beacon
	char *getHelloEntry(int index);							// gets a formatted string version of the hello entry
	void copyHelloEntry(int index, HELLOENTRY *dest);		// gets a copy of the HELLOENTRY data structure at index

//--------------------------------------------
//	These variables should be changed on startup to get the desired behaviour

	SyntroThread *m_parentThread;							// this is set with the parent thred's ID if messages are required

	int m_socketFlags;										// set to 1 for use socket number

	int m_priority;											// priority to be advertised

//--------------------------------------------

//	The lock should always be used when processing the HelloArray

	HELLOENTRY m_helloArray[SYNTRO_MAX_CONNECTEDCOMPONENTS];
	QMutex m_lock;											// for access to the hello array


	void initThread();
	bool processMessage(SyntroThreadMsg* msg);
	bool findComponent(HELLOENTRY *foundHelloEntry, SYNTRO_UID *UID);
	bool findComponent(HELLOENTRY *foundHelloEntry, char *appName, char *componentType);
	bool findBestControl(HELLOENTRY *foundHelloEntry);

signals:
	void helloDisplayEvent(Hello *hello);

protected:
	void timerEvent(QTimerEvent *event);
	void processHello();
	void deleteEntry(HELLOENTRY *pH);
	bool matchHello(HELLO *a, HELLO *b);
	void processTimers();

	HELLO m_RXHello;
	char m_IpAddr[SYNTRO_MAX_NONTAG];				
	unsigned int m_port;
	SyntroSocket *m_helloSocket;
	SyntroComponentData *m_componentData;
	bool m_control;
	qint64 m_controlTimer;

private:
	int m_timer;
	QString m_logTag;
};

#endif // _HELLO_H_
