//
//  Copyright (c) 2014 Scott Ellis and Richard Barnett
//	
//  This file is part of SyntroNet
//
//  SyntroNet is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  SyntroNet is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with SyntroNet.  If not, see <http://www.gnu.org/licenses/>.
//

#include "ControlConsole.h"
#include "SyntroLib.h"
#include "SyntroServer.h"

#ifdef WIN32
#include <conio.h>
#else
#include <termios.h>
#endif


ControlConsole::ControlConsole(QObject *parent)
	: QThread(parent)
{
	SyntroUtils::syntroAppInit();

	connect((QCoreApplication *)parent, SIGNAL(aboutToQuit()), this, SLOT(aboutToQuit()));

	m_client = new SyntroServer();
	m_client->resumeThread();
	start();
}

void ControlConsole::aboutToQuit()
{
	m_client->exitThread();
	SyntroUtils::syntroAppExit();
	for (int i = 0; i < 5; i++) {
		if (wait(1000))
			break;

		printf("Waiting for console thread to exit...\n");
	}
}

void ControlConsole::displayComponents()
{
	bool first = true;

	printf("\n\n%-16s %-16s %-16s %-15s\n", "Component", "Name", "UID", "IP address");
    printf("%-16s %-16s %-16s %-15s\n", "---------", "----", "----", "----------");

	SS_COMPONENT *syntroComponent = m_client->m_components;

	for (int i = 0; i < SYNTRO_MAX_CONNECTEDCOMPONENTS; i++, syntroComponent++) {
		if (!syntroComponent->inUse)
			continue;

		first = false;

		HELLO *hello = &(syntroComponent->heartbeat.hello);

		printf("%-16s %-16s %-16s %-15s\n", hello->componentType, hello->appName,
				qPrintable(SyntroUtils::displayUID(&hello->componentUID)),
				qPrintable(SyntroUtils::displayIPAddr(hello->IPAddr)));
	}

	if (first)
		printf("\nNo active components\n");
}

void ControlConsole::displayDirectory()
{
	bool first = true;

	DM_CONNECTEDCOMPONENT *connectedComponent = m_client->m_dirManager.m_directory;

	m_client->m_dirManager.m_lock.lock();

	for (int i = 0; i < SYNTRO_MAX_CONNECTEDCOMPONENTS; i++, connectedComponent++) {
		if (!connectedComponent->valid)
			continue;						// a not in use entry

		DM_COMPONENT *component = connectedComponent->componentDE;

		while (component != NULL) {
			if (!first)
				printf("------------------------");

			first = false;

			printf("\n\nApp name: %s\n", component->appName);
			printf("Comp type: %s\n", component->componentType);
			printf("UID: %s\n", component->UIDStr);
			printf("Sequence ID: %d\n", component->sequenceID);

			DM_SERVICE *service = component->services;

			for (int j = 0; j < component->serviceCount; j++, service++) {
				if (!service->valid)
					continue;					// can this happen?

				if (service->serviceType == SERVICETYPE_NOSERVICE)
					continue;

				printf("Service: %s, port %d, type %d\n", service->serviceName, service->port, service->serviceType);
			}

			component = component->next;
		}
	}

	m_client->m_dirManager.m_lock.unlock();

	if (first)
		printf("\n\nNo active directories\n");
}

void ControlConsole::displayMulticast()
{
	bool first = true;

	QMutexLocker locker(&(m_client->m_multicastManager.m_lock));

	MM_MMAP *multicastMap = m_client->m_multicastManager.m_multicastMap;

	for (int i = 0; i < SYNTROSERVER_MAX_MMAPS; i++, multicastMap++) {
		if (!multicastMap->valid)
			continue;

		if (!first)
			printf("------------------------");

		first = false;

		printf("\n\nMap: Srv=%s, UID=%s, \n", multicastMap->serviceLookup.servicePath, 
					qPrintable(SyntroUtils::displayUID(&multicastMap->sourceUID)));
		printf("     LPort=%d, RPort=%d\n", 
				SyntroUtils::convertUC2ToUInt(multicastMap->serviceLookup.localPort), 
				SyntroUtils::convertUC2ToUInt(multicastMap->serviceLookup.remotePort));

		MM_REGISTEREDCOMPONENT *registeredComponent = multicastMap->head;

		while (registeredComponent != NULL) {
			printf("          RC: UID=%s, port=%d, seq=%d, ack=%d\n", 
				qPrintable(SyntroUtils::displayUID(&registeredComponent->registeredUID)), registeredComponent->port, 
				registeredComponent->sendSeq, registeredComponent->lastAckSeq);
			registeredComponent = registeredComponent->next;
		}
	}

	if (first)
		printf("\n\nNo active multicasts\n");
}

void ControlConsole::showHelp()
{
	printf("\nOptions are:\n\n");
	printf("  C - Display Components\n");
	printf("  D - Display directory\n");
	printf("  M - Display multicast map table\n");
	printf("  X - Exit\n");
}

void ControlConsole::run()
{
	bool mustExit;

#ifndef WIN32
        struct termios	ctty;

        tcgetattr(fileno(stdout), &ctty);
        ctty.c_lflag &= ~(ICANON);
        tcsetattr(fileno(stdout), TCSANOW, &ctty);
#endif

	mustExit = false;

	while (!mustExit) {

		printf("\nEnter option: ");

#ifdef WIN32
		switch (toupper(_getch()))
#else
        switch (toupper(getchar()))
#endif		
		{
		case 'C':
			displayComponents();
			break;

		case 'D':
			displayDirectory();
			break;

		case 'H':
			showHelp();
			break;

		case 'M':
			displayMulticast();
			break;

		case 'X':
			printf("\nExiting\n");
			mustExit = true;
			((QCoreApplication *)parent())->exit();
			break;

		default:
			break;
		}
	}
}

