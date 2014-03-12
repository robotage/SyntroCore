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

#include "SyntroComponentData.h"
#include <qglobal.h>
#include "SyntroUtils.h"
#include "SyntroSocket.h"

SyntroComponentData::SyntroComponentData() 
{
	m_myHelloSocket = NULL;
	m_myInstance = -1;
}

SyntroComponentData::~SyntroComponentData()
{
}

void SyntroComponentData::init(const char *compType, int hbInterval, int priority)
{
	HELLO *hello;

	m_logTag = compType;

	hello = &(m_myHeartbeat.hello);
	hello->helloSync[0] = HELLO_SYNC0;
	hello->helloSync[1] = HELLO_SYNC1;
	hello->helloSync[2] = HELLO_SYNC2;
	hello->helloSync[3] = HELLO_SYNC3;

	strncpy(hello->appName, qPrintable(SyntroUtils::getAppName()), SYNTRO_MAX_APPNAME - 1);
	strncpy(hello->componentType, compType, SYNTRO_MAX_COMPTYPE - 1);

	strcpy(m_myComponentType, compType);
	
	// set up instance values and create hello socket using dynamic instance if a normal component
	if (strcmp(compType, COMPTYPE_CONTROL) == 0)
		m_myInstance = INSTANCE_CONTROL;
	else
		createHelloSocket();
	memcpy(hello->IPAddr, SyntroUtils::getMyIPAddr(), sizeof(SYNTRO_IPADDR));
	memcpy(hello->componentUID.macAddr, SyntroUtils::getMyMacAddr(), sizeof(SYNTRO_MACADDR));
	SyntroUtils::convertIntToUC2(m_myInstance, hello->componentUID.instance);

	memcpy(&(m_myUID), &(hello->componentUID), sizeof(SYNTRO_UID));

	SyntroUtils::convertIntToUC2(hbInterval, hello->interval);

	hello->priority = priority;							
	hello->unused1 = 1;		

	// generate empty DE
	DESetup();
	DEComplete();

}

void SyntroComponentData::DESetup()
{
	m_myDE[0] = 0;
	sprintf(m_myDE + (int)strlen(m_myDE), "<%s>", DETAG_COMP);
	DEAddValue(DETAG_UID, qPrintable(SyntroUtils::displayUID(&(m_myUID))));
	DEAddValue(DETAG_APPNAME, (char *)m_myHeartbeat.hello.appName);
	DEAddValue(DETAG_COMPTYPE, (char *)m_myHeartbeat.hello.componentType);
}

void SyntroComponentData::DEComplete()
{
	sprintf(m_myDE + (int)strlen(m_myDE), "</%s>", DETAG_COMP);
}

bool SyntroComponentData::DEAddValue(QString tag, QString value)
{
	int len = (int)strlen(m_myDE) + (2 * tag.length()) + value.length() + 8;

	if (len >= SYNTRO_MAX_DELENGTH) {
		logError(QString("DE too long"));
		return false;
	}

	sprintf(m_myDE + (int)strlen(m_myDE), "<%s>%s</%s>", qPrintable(tag), qPrintable(value), qPrintable(tag));

	return true;
}


bool SyntroComponentData::createHelloSocket()
{
	int i;

	m_myHelloSocket = new SyntroSocket(m_logTag);

	if (!m_myHelloSocket) {
		logError(QString("Failed to create socket got %s").arg(m_myComponentType));
		return false;
	}
	for (i = INSTANCE_COMPONENT; i < SYNTRO_MAX_COMPONENTSPERDEVICE; i++) {
		if (m_myHelloSocket->sockCreate(SOCKET_HELLO + i, SOCK_DGRAM) != 0) {
			m_myInstance = i;
			return true;
		}
	}

	if (i == SYNTRO_MAX_COMPONENTSPERDEVICE) {
		logError(QString("Dynamic instance allocation failed for %1").arg(m_myComponentType));
		return false;
	}
	return true;
}
