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

#include "SyntroDB.h"
#include <QApplication>

#include <QtDebug>

#include "SyntroUtils.h"
#include "SyntroDBConsole.h"
#include "SyntroRecord.h"

int runGuiApp(int argc, char *argv[]);
int runConsoleApp(int argc, char *argv[]);
void loadSettings(QStringList arglist);


int main(int argc, char *argv[])
{
	if (SyntroUtils::checkConsoleModeFlag(argc, argv))
		return runConsoleApp(argc, argv);
	else
		return runGuiApp(argc, argv);
}

// look but do not modify argv

int runGuiApp(int argc, char *argv[])
{
	QApplication a(argc, argv);

	loadSettings(a.arguments());

	SyntroDB *w = new SyntroDB();

	w->show();

	return a.exec();
}

int runConsoleApp(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	loadSettings(a.arguments());

	SyntroDBConsole sc(&a);

	return a.exec();
}

void loadSettings(QStringList arglist)
{
	SyntroUtils::loadStandardSettings(APPTYPE_DB, arglist);

	// app-specific part

	QSettings *settings = SyntroUtils::getSettings();

	if (!settings->contains(SYNTRODB_PARAMS_ROOT_DIRECTORY))
		settings->setValue(SYNTRODB_PARAMS_ROOT_DIRECTORY, "./");		

	// Max age of files in days afterwhich they will be deleted
	// A value of 0 turns off the delete behavior
	if (!settings->contains(SYNTRODB_MAXAGE))
		settings->setValue(SYNTRODB_MAXAGE, 0);		

	// The SyntroStore component can save any type of stream.
	// Here you can list the Syntro streams by name that it should look for.
	int	nSize = settings->beginReadArray(SYNTRODB_PARAMS_STREAM_SOURCES);
	settings->endArray();

	if (nSize == 0) {
		settings->beginWriteArray(SYNTRODB_PARAMS_STREAM_SOURCES);
	
		for (int index = 0; index < SYNTRODB_MAX_STREAMS; index++) {
			settings->setArrayIndex(index);
			if (index == 0) {
				settings->setValue(SYNTRODB_PARAMS_INUSE, SYNTRO_PARAMS_TRUE);
				settings->setValue(SYNTRODB_PARAMS_STREAM_SOURCE, "source/video");
			} else {
				settings->setValue(SYNTRODB_PARAMS_INUSE, SYNTRO_PARAMS_FALSE);
				settings->setValue(SYNTRODB_PARAMS_STREAM_SOURCE, "");
			}
		}
		settings->endArray();
	}

	delete settings;

	return;
}
