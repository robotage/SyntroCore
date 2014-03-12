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

#include "SyntroDBConsole.h"
#include "SyntroLib.h"
#include "SyntroDB.h"
#include "StoreClient.h"
#include "CFSClient.h"

#ifdef WIN32
#include <conio.h>
#else
#include <termios.h>
#endif


SyntroDBConsole::SyntroDBConsole(QObject *parent)
	: QThread(parent)
{
	SyntroUtils::syntroAppInit();

    m_storeClient = new StoreClient(this);

    connect(this, SIGNAL(refreshStreamSource(int)),
        m_storeClient, SLOT(refreshStreamSource(int)), Qt::QueuedConnection);
    connect(m_storeClient, SIGNAL(running()),
        this, SLOT(storeRunning()), Qt::QueuedConnection);
    m_storeClient->resumeThread();

    m_CFSClient = new CFSClient(this);
    m_CFSClient->resumeThread();

	start();
}

SyntroDBConsole::~SyntroDBConsole()
{
}

void SyntroDBConsole::storeRunning()
{
    for (int index = 0; index < SYNTRODB_MAX_STREAMS; index++)
        emit refreshStreamSource(index);
}

void SyntroDBConsole::showHelp()
{
	printf("\n\nOptions are:\n\n");
	printf("  C - Display record and byte counts\n");
	printf("  S - Display status\n");
	printf("  X - Exit\n");
}

void SyntroDBConsole::showStatus()
{
	printf("\n\nStore SyntroControl link status is: %s\n", qPrintable(m_storeClient->getLinkState()));
	printf("CFS SyntroControl link status is: %s\n", qPrintable(m_CFSClient->getLinkState()));
}

void SyntroDBConsole::showCounts()
{
	printf("\n\n%-15s %-12s %-16s %s", "Service", "RX records", "RX bytes", "Active File");
	printf("\n%-15s %-12s %-16s %s", "-------", "----------", "--------", "------------------------");

	StoreStream ss;

	for (int i = 0; i < SYNTRODB_MAX_STREAMS; i++) {
		if (!m_storeClient->getStoreStream(i, &ss))
			continue;

		printf("\n%-15s %-12s %-16s %s",
			qPrintable(ss.streamName()),
			qPrintable(QString::number(ss.rxTotalRecords())),
			qPrintable(QString::number(ss.rxTotalBytes())),
			qPrintable(ss.currentFile()));			
	}
	printf("\n");
}

void SyntroDBConsole::run()
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
			showCounts();
			break;

		case 'S':
			showStatus();
			break;

		case 'H':
			showHelp();
			break;

		case 'X':
			printf("\nExiting\n");
			m_storeClient->exitThread();
			m_CFSClient->exitThread();
			mustExit = true;
			SyntroUtils::syntroAppExit();
			((QCoreApplication *)parent())->exit();
			break;

		default:
            break;
		}
	}
}

