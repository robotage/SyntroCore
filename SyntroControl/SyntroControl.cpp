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

#include "SyntroControl.h"
#include "SyntroAboutDlg.h"
#include "ControlSetupDlg.h"

#define DEFAULT_ROW_HEIGHT 20


SyntroControl::SyntroControl()
	: QMainWindow()
{
	ui.setupUi(this);

	connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));
	connect(ui.actionDirectory, SIGNAL(triggered()), this, SLOT(onDirectory()));
	connect(ui.actionHello, SIGNAL(triggered()), this, SLOT(onHello()));
	connect(ui.actionMulticast, SIGNAL(triggered()), this, SLOT(onMulticast()));
	connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT(onAbout()));
	connect(ui.actionBasicSetup, SIGNAL(triggered()), this, SLOT(onBasicSetup()));

    ui.m_table->setColumnCount(9);

    ui.m_table->setColumnWidth(0, 120);
    ui.m_table->setColumnWidth(1, 120);
     ui.m_table->setColumnWidth(2, 120);
    ui.m_table->setColumnWidth(3, 100);
	ui.m_table->setColumnWidth(4, 80);
	ui.m_table->setColumnWidth(5, 100);
	ui.m_table->setColumnWidth(6, 100);
	ui.m_table->setColumnWidth(7, 100);
	ui.m_table->setColumnWidth(8, 100);

    ui.m_table->setHorizontalHeaderLabels(
                QStringList() << tr("App name") << tr("Component type")
                << tr("Unique ID") << tr("IP Address") << tr("HB interval")
				<< tr("RX bytes") << tr("TX bytes") << tr("RX rate") << tr("TX rate"));


    ui.m_table->setSelectionMode(QAbstractItemView::NoSelection);

	for (int row = 0; row < SYNTRO_MAX_CONNECTEDCOMPONENTS; row++) {
		ui.m_table->insertRow(row);
		ui.m_table->setRowHeight(row, DEFAULT_ROW_HEIGHT);

		for (int col = 0; col < 9; col++) {
			QTableWidgetItem *item = new QTableWidgetItem();
			item->setTextAlignment(Qt::AlignLeft | Qt::AlignBottom);
			item->setFlags(Qt::ItemIsEnabled);
			if (col == 0)
				item->setText("...");
			else
				item->setText("");

			ui.m_table->setItem(row, col, item);
		}
	}
 
    setCentralWidget(ui.m_table);

	m_serverMulticastStatus = new QLabel(this);
	m_serverMulticastStatus->setAlignment(Qt::AlignLeft);
	m_serverMulticastStatus->setText("");
	ui.statusBar->addWidget(m_serverMulticastStatus, 1);

 	m_serverE2EStatus = new QLabel(this);
	m_serverE2EStatus->setAlignment(Qt::AlignLeft);
	m_serverE2EStatus->setText("");
	ui.statusBar->addWidget(m_serverE2EStatus, 1);

  //  setMinimumSize(600, 120);

	restoreWindowState();

	m_directoryDlg = new DirectoryDialog(this);
	m_directoryDlg->setModal(false);

	m_helloDlg = new HelloDialog(this);
	m_helloDlg->setModal(false);

	m_multicastDlg = new MulticastDialog(this);
	m_multicastDlg->setModal(false);


	SyntroUtils::syntroAppInit();

	setWindowTitle(QString("%1 - %2")
		.arg(SyntroUtils::getAppType())
		.arg(SyntroUtils::getAppName()));

	m_server = new SyntroServer();

	m_multicastDlg->m_multicastMgr = &(m_server->m_multicastManager);
	m_directoryDlg->m_directoryMgr = &(m_server->m_dirManager);

    connect(m_server, SIGNAL(UpdateSyntroStatusBox(int, QStringList)), 
		this, SLOT(UpdateSyntroStatusBox(int, QStringList)), Qt::QueuedConnection);
    connect(m_server, SIGNAL(UpdateSyntroDataBox(int, QStringList)), 
		this, SLOT(UpdateSyntroDataBox(int, QStringList)), Qt::QueuedConnection);

	connect(&(m_server->m_multicastManager), SIGNAL(MMDisplay()), 
		m_multicastDlg, SLOT(MMDisplay()), Qt::QueuedConnection);
	connect(&(m_server->m_multicastManager), SIGNAL(MMRegistrationChanged(int)), 
		m_multicastDlg, SLOT(MMRegistrationChanged(int)), Qt::QueuedConnection);
	connect(&(m_server->m_multicastManager), SIGNAL(MMNewEntry(int)), 
		m_multicastDlg, SLOT(MMNewEntry(int)), Qt::QueuedConnection);
	connect(&(m_server->m_multicastManager), SIGNAL(MMDeleteEntry(int)), 
		m_multicastDlg, SLOT(MMDeleteEntry(int)), Qt::QueuedConnection);

	connect(&(m_server->m_dirManager), SIGNAL(DMNewDirectory(int)), 
		m_directoryDlg, SLOT(DMNewDirectory(int)), Qt::QueuedConnection);

	// This next connection has to be direct - ugly but necessary

	connect(&(m_server->m_dirManager), SIGNAL(DMRemoveDirectory(DM_COMPONENT *)), 
		m_directoryDlg, SLOT(DMRemoveDirectory(DM_COMPONENT *)), Qt::DirectConnection);

	m_server->resumeThread();

	// can't connect new hello emits to dialog yet so just start timer and do it when the time is right

	m_helloConnected = false;
	m_timer = startTimer(1000);

	connect(m_server, SIGNAL(serverMulticastUpdate(qint64, unsigned, qint64, unsigned)), 
		this, SLOT(serverMulticastUpdate(qint64, unsigned, qint64, unsigned)), Qt::QueuedConnection);
	connect(m_server, SIGNAL(serverE2EUpdate(qint64, unsigned, qint64, unsigned)), 
		this, SLOT(serverE2EUpdate(qint64, unsigned, qint64, unsigned)), Qt::QueuedConnection);
}

void SyntroControl::closeEvent(QCloseEvent *)
{
	killTimer(m_timer);
	m_server->exitThread();
	SyntroUtils::syntroAppExit();
	saveWindowState();
}

void SyntroControl::timerEvent(QTimerEvent *)
{
	if (!m_helloConnected && (m_server->getHelloObject() != NULL)) {
		connect(m_server->getHelloObject(), SIGNAL(helloDisplayEvent(Hello *)), m_helloDlg, SLOT(helloDisplayEvent(Hello *)), Qt::QueuedConnection); 
		m_helloConnected = true;
	}
}

void SyntroControl::onDirectory()
{
	m_directoryDlg->activateWindow();
	m_directoryDlg->show();
}

void SyntroControl::onHello()
{
	m_helloDlg->activateWindow();
	m_helloDlg->show();
}

void SyntroControl::onMulticast()
{
	m_multicastDlg->activateWindow();
	m_multicastDlg->show();
}

void SyntroControl::UpdateSyntroStatusBox(int index, QStringList list)
{
	if (index >= ui.m_table->rowCount())
		return;
	ui.m_table->item(index, 0)->setText(list.at(0));
	ui.m_table->item(index, 1)->setText(list.at(1));
	ui.m_table->item(index, 2)->setText(list.at(2));
	ui.m_table->item(index, 3)->setText(list.at(3));
	ui.m_table->item(index, 4)->setText(list.at(4));
}

void SyntroControl::UpdateSyntroDataBox(int index, QStringList list)
{
	if (index >= ui.m_table->rowCount())
		return;
	ui.m_table->item(index, 5)->setText(list.at(0));
	ui.m_table->item(index, 6)->setText(list.at(1));
	ui.m_table->item(index, 7)->setText(list.at(2));
	ui.m_table->item(index, 8)->setText(list.at(3));
}

void SyntroControl::saveWindowState()
{
	QSettings *settings = SyntroUtils::getSettings();

	settings->beginGroup("ControlWindow");
	settings->setValue("Geometry", saveGeometry());
	settings->setValue("State", saveState());

	settings->beginWriteArray("Grid");
	for (int i = 0; i < ui.m_table->columnCount(); i++) {
		settings->setArrayIndex(i);
		settings->setValue("columnWidth", ui.m_table->columnWidth(i));
	}
	settings->endArray();

	settings->endGroup();

	delete settings;
}

void SyntroControl::restoreWindowState()
{
	QSettings *settings = SyntroUtils::getSettings();
	
	settings->beginGroup("ControlWindow");
	restoreGeometry(settings->value("Geometry").toByteArray());
	restoreState(settings->value("State").toByteArray());

	int count = settings->beginReadArray("Grid");
	for (int i = 0; i < count && i < ui.m_table->columnCount(); i++) {
		settings->setArrayIndex(i);
		int width = settings->value("columnWidth").toInt();

		if (width > 0)
			ui.m_table->setColumnWidth(i, width);
	}
	settings->endArray();

	settings->endGroup();

	delete settings;
}

void SyntroControl::onAbout()
{
	SyntroAbout dlg;
	dlg.exec();
}

void SyntroControl::onBasicSetup()
{
	ControlSetupDlg dlg(this);
	dlg.exec();
}

void SyntroControl::serverMulticastUpdate(qint64 in, unsigned inRate, qint64 out, unsigned outRate)
{
	m_serverMulticastStatus->setText(	QString("Mcast: in=") + QString::number(in) +
										QString(" out=") + QString::number(out) + 
										QString("  Mcast rate: in=") + QString::number(inRate) +
										QString(" out=") + QString::number(outRate));
}

void SyntroControl::serverE2EUpdate(qint64 in, unsigned inRate, qint64 out, unsigned outRate)
{
	m_serverE2EStatus->setText(			QString("E2E: in=") + QString::number(in) +
										QString(" out=") + QString::number(out) + 
										QString("  E2E rate: in=") + QString::number(inRate) +
										QString(" out=") + QString::number(outRate));
}
