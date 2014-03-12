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

#ifndef CONTROLCONSOLE_H
#define CONTROLCONSOLE_H

#include <QThread>

class	SyntroServer;

class ControlConsole : public QThread
{
	Q_OBJECT

public:
	ControlConsole(QObject *parent);
	~ControlConsole() {}

public slots:
	void aboutToQuit();

protected:
	void run();

private:
	void displayComponents();
	void displayDirectory();
	void displayMulticast();
	void showHelp();

	SyntroServer *m_client;
};

#endif // CONTROLCONSOLE_H
