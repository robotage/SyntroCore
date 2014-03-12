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

#ifndef _HELLODEFS_H
#define _HELLODEFS_H

#include "SyntroDefs.h"

//--------------------------------------------------------------------------------
//	The Hello data structure

#define	INSTANCE_CONTROL			0						// the instance for SyntroControl

#define	INSTANCE_DYNAMIC			-1						// for a non core app, this means that instance is dynamic
#define	INSTANCE_COMPONENT			2						// instance of first component

#define	SOCKET_HELLO				8040					// socket to use is this plus the instance

#define	HELLO_SYNC0		0xff								// the sync bytes
#define	HELLO_SYNC1		0xa5
#define	HELLO_SYNC2		0x5a
#define	HELLO_SYNC3		0x11

#define	HELLO_SYNC_LEN	4									// 4 bytes in sync

#define	HELLO_BEACON	2									// a beacon event (SyntroControl only)
#define	HELLO_UP		1									// state in hello state message
#define	HELLO_DOWN		0									// as above

typedef struct
{
	unsigned char helloSync[HELLO_SYNC_LEN];				// a 4 byte code to check synchronisation (HELLO_SYNC)
	SYNTRO_IPADDR IPAddr;									// device IP address
	SYNTRO_UID componentUID;								// Component UID
	SYNTRO_APPNAME appName;									// the app name of the sender
	SYNTRO_COMPTYPE componentType;							// the component type of the sender
	unsigned char priority;									// priority of SyntroControl
	unsigned char unused1;									// was operating mode
	SYNTRO_UC2 interval;									// heartbeat send interval
} HELLO;

//	SYNTRO_HEARTBEAT is the type sent on the SyntroLink. It is the hello but with the SYNTRO_MESSAGE header

typedef struct
{
	SYNTRO_MESSAGE syntroMessage;							// the message header
	HELLO hello;											// the hello itself
} SYNTRO_HEARTBEAT;

#define	HELLO_INTERVAL		(2 * SYNTRO_CLOCKS_PER_SEC)		// send a hello every 2 seconds
#define	HELLO_LIFETIME		(4 * HELLO_INTERVAL)			// HELLO entry timeout = 4 retries

#endif	// _HELLODEFS_H
