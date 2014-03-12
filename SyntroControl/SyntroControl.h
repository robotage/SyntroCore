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

#ifndef SYNTROCONTROL_H
#define SYNTROCONTROL_H

#include <QMainWindow>
#include "ui_SyntroControl.h"
#include "SyntroLib.h"
#include "SyntroServer.h"
#include "DirectoryDialog.h"
#include "MulticastDialog.h"
#include "HelloDialog.h"

class SyntroControl : public QMainWindow
{
	Q_OBJECT

public:
	SyntroControl();
	~SyntroControl() {}

public slots:
	void onAbout();
	void onDirectory();
	void onBasicSetup();
	void onHello();
	void onMulticast();
	void UpdateSyntroStatusBox(int, QStringList);
	void UpdateSyntroDataBox(int, QStringList);
	void serverMulticastUpdate(qint64 in, unsigned inRate, qint64 out, unsigned outRate);
	void serverE2EUpdate(qint64 in, unsigned inRate, qint64 out, unsigned outRate);

protected:
	DirectoryDialog *m_directoryDlg;
	HelloDialog *m_helloDlg;
	MulticastDialog *m_multicastDlg;
	QLabel *m_serverE2EStatus;
	QLabel *m_serverMulticastStatus;
	void timerEvent(QTimerEvent *event);

private:
	void closeEvent(QCloseEvent * event);
	void saveWindowState();
	void restoreWindowState();

	Ui::SyntroControlClass ui;
	SyntroServer *m_server;

	int m_timer;
	bool m_helloConnected;
};

#endif // SYNTROCONTROL_H
