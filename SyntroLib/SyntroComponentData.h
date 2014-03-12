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

#ifndef		_SYNTROCOMPONENTDATA_H_
#define		_SYNTROCOMPONENTDATA_H_

#include "SyntroDefs.h"
#include "syntrolib_global.h"
#include "HelloDefs.h"

#include <qstring.h>

class SyntroSocket;

class SYNTROLIB_EXPORT SyntroComponentData
{
public:
	SyntroComponentData();
	virtual ~SyntroComponentData();

	// sets up the initial data

	void init(const char *compType, int hbInterval, int priority = 0);

	// returns a pointer to the DE

	inline const char* getMyDE() {return m_myDE;};

	// sets up the initial part of the DE

	void DESetup();						

	// adds tag/value pairs to the DE

	bool DEAddValue(QString tag, QString value);

	// called to complete the DE

	void DEComplete();

	// returns the heartbeat

	inline SYNTRO_HEARTBEAT getMyHeartbeat() {return m_myHeartbeat;};

	// returns the component type

	inline const char *getMyComponentType() {return m_myComponentType;};

	// return the component UID

	inline SYNTRO_UID getMyUID() {return m_myUID;};

	// returns the component instance

	inline unsigned char getMyInstance() {return m_myInstance;};

	// return the hello socket pointer

	inline SyntroSocket *getMyHelloSocket() { return m_myHelloSocket;};


private:
	bool createHelloSocket();

	SYNTRO_HEARTBEAT m_myHeartbeat;
	SYNTRO_COMPTYPE m_myComponentType;
	SYNTRO_UID m_myUID;
	char m_myDE[SYNTRO_MAX_DELENGTH];	
	SyntroSocket *m_myHelloSocket;									
	unsigned char m_myInstance;

	QString m_logTag;
};

#endif		//_SYNTROCOMPONENTDATA_H_
