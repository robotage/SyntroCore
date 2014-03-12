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
#include "SyntroAboutDlg.h"
#include "StoreClient.h"
#include "CFSClient.h"
#include "CFSThread.h"
#include "BasicSetupDlg.h"
#include "ConfigurationDlg.h"

#define CELL_HEIGHT_PAD 6

SyntroDB::SyntroDB()
	: QMainWindow()
{
	m_startingUp = true;
	ui.setupUi(this);

	m_storeStreamDlg = NULL;

	connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));
	connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT(onAbout()));
	connect(ui.actionBasicSetup, SIGNAL(triggered()), this, SLOT(onBasicSetup()));
	connect(ui.actionConfiguration, SIGNAL(triggered()), this, SLOT(onConfiguration()));

	initStatusBar();
	initDisplayStats();
	restoreWindowState();

	m_timerId = startTimer(2000);

	SyntroUtils::syntroAppInit();

	setWindowTitle(QString("%1 - %2")
		.arg(SyntroUtils::getAppType())
		.arg(SyntroUtils::getAppName()));

	m_storeClient = new StoreClient(this);
	
    connect(this, SIGNAL(refreshStreamSource(int)),
        m_storeClient, SLOT(refreshStreamSource(int)), Qt::QueuedConnection);
    connect(m_storeClient, SIGNAL(running()),
        this, SLOT(storeRunning()), Qt::QueuedConnection);
    m_storeClient->resumeThread();

	m_CFSClient = new CFSClient(this);
	m_CFSClient->resumeThread();

	QSettings *settings = SyntroUtils::getSettings();

	settings->beginReadArray(SYNTRODB_PARAMS_STREAM_SOURCES);
	for (int index = 0; index < SYNTRODB_MAX_STREAMS; index++) {
		settings->setArrayIndex(index);

		m_rxStreamTable->item(index, SYNTRODB_COL_STREAM)->setText(settings->value(SYNTRODB_PARAMS_STREAM_SOURCE).toString());
		if (settings->value(SYNTRODB_PARAMS_INUSE).toString() == SYNTRO_PARAMS_TRUE)
			m_useBox[index]->setCheckState(Qt::Checked);
		else
			m_useBox[index]->setCheckState(Qt::Unchecked);
	}
	settings->endArray();


	m_startingUp = false;

	delete settings;
}

void SyntroDB::storeRunning()
{
    for (int index = 0; index < SYNTRODB_MAX_STREAMS; index++)
        emit refreshStreamSource(index);
}

void SyntroDB::closeEvent(QCloseEvent *)
{
	killTimer(m_timerId);

	if (m_storeClient) {
		m_storeClient->exitThread();
	}

	if (m_CFSClient) {
		m_CFSClient->exitThread();
	}

/*
	QSettings *settings = SyntroUtils::getSettings();

	settings->beginWriteArray(SYNTRODB_PARAMS_STREAM_SOURCES);
	for (int index = 0; index < SYNTRODB_MAX_STREAMS; index++) {
		settings->setArrayIndex(index);

		if (m_useBox[index]->checkState() == Qt::Checked)
			settings->setValue(SYNTRODB_PARAMS_INUSE, SYNTRO_PARAMS_TRUE);
		else
			settings->setValue(SYNTRODB_PARAMS_INUSE, SYNTRO_PARAMS_FALSE);
	}
	settings->endArray();

	delete settings;
*/
	saveWindowState();
	SyntroUtils::syntroAppExit();
}

void SyntroDB::displayStreamDetails(int index)
{
	QSettings *settings = SyntroUtils::getSettings();

	settings->beginReadArray(SYNTRODB_PARAMS_STREAM_SOURCES);
	settings->setArrayIndex(index);

	m_rxStreamTable->item(index, SYNTRODB_COL_STREAM)->setText(settings->value(SYNTRODB_PARAMS_STREAM_SOURCE).toString());

	settings->endArray();

	delete settings;
}

void SyntroDB::timerEvent(QTimerEvent *)
{
	StoreStream ss;

	for (int i = 0; i < SYNTRODB_MAX_STREAMS; i++) {
		if (!m_storeClient->getStoreStream(i, &ss))
			continue;

		m_rxStreamTable->item(i, SYNTRODB_COL_TOTALRECS)->setText(QString::number(ss.rxTotalRecords()));
		m_rxStreamTable->item(i, SYNTRODB_COL_TOTALBYTES)->setText(QString::number(ss.rxTotalBytes()));
		m_rxStreamTable->item(i, SYNTRODB_COL_FILERECS)->setText(QString::number(ss.rxRecords()));
		m_rxStreamTable->item(i, SYNTRODB_COL_FILEBYTES)->setText(QString::number(ss.rxBytes()));
		m_rxStreamTable->item(i, SYNTRODB_COL_FILE)->setText(ss.currentFile());
	}

	m_controlStatus->setText(m_storeClient->getLinkState());
}

void SyntroDB::initDisplayStats()
{
	QTableWidgetItem *item;

	int cellHeight = fontMetrics().lineSpacing() + CELL_HEIGHT_PAD;
	
	m_rxStreamTable = new QTableWidget(this);

	m_rxStreamTable->setColumnCount(SYNTRODB_COL_COUNT);
	m_rxStreamTable->setColumnWidth(SYNTRODB_COL_CONFIG, 80);
	m_rxStreamTable->setColumnWidth(SYNTRODB_COL_INUSE, 60);
	m_rxStreamTable->setColumnWidth(SYNTRODB_COL_STREAM, 140);
	m_rxStreamTable->setColumnWidth(SYNTRODB_COL_TOTALRECS, 80);
	m_rxStreamTable->setColumnWidth(SYNTRODB_COL_TOTALBYTES, 100);
	m_rxStreamTable->setColumnWidth(SYNTRODB_COL_FILERECS, 80);
	m_rxStreamTable->setColumnWidth(SYNTRODB_COL_FILEBYTES, 100);
	m_rxStreamTable->setColumnWidth(SYNTRODB_COL_FILE, 400);

	m_rxStreamTable->verticalHeader()->setDefaultSectionSize(cellHeight);

    m_rxStreamTable->setHorizontalHeaderLabels(QStringList() << tr("") << tr("In use") << tr("Stream") 
			<< tr("Total Recs") << tr("Total Bytes") 
			<< tr("File Recs") << tr("File Bytes") << tr("Current file path"));

    m_rxStreamTable->setSelectionMode(QAbstractItemView::NoSelection);
 
	for (int row = 0; row < SYNTRODB_MAX_STREAMS; row++) {
		m_rxStreamTable->insertRow(row);
		m_rxStreamTable->setRowHeight(row, 20);
		m_rxStreamTable->setContentsMargins(5, 5, 5, 5);

		StoreButton *button = new StoreButton("Configure", this, row);
		m_rxStreamTable->setCellWidget(row, SYNTRODB_COL_CONFIG, button);
		connect(button, SIGNAL(buttonClicked(int)), this, SLOT(buttonClicked(int)));

		m_useBox[row] = new StoreCheckBox(m_rxStreamTable, row);
		m_rxStreamTable->setCellWidget(row, SYNTRODB_COL_INUSE, m_useBox[row]);
		connect(m_useBox[row], SIGNAL(boxClicked(bool, int)), this, SLOT(boxClicked(bool, int)));
		
		for (int col = 2; col < SYNTRODB_COL_COUNT; col++) {
			item = new QTableWidgetItem();
			item->setTextAlignment(Qt::AlignLeft | Qt::AlignBottom);
			item->setFlags(Qt::ItemIsEnabled);
			item->setText("");
			m_rxStreamTable->setItem(row, col, item);
		}
	}
	setCentralWidget(m_rxStreamTable);
}

void SyntroDB::initStatusBar()
{
	m_controlStatus = new QLabel(this);
	m_controlStatus->setAlignment(Qt::AlignLeft);
	m_controlStatus->setText("");
	ui.statusBar->addWidget(m_controlStatus, 1);
}

void SyntroDB::saveWindowState()
{
	QSettings *settings = SyntroUtils::getSettings();

	settings->beginGroup("Window");
	settings->setValue("Geometry", saveGeometry());
	settings->setValue("State", saveState());

	settings->beginWriteArray("Grid");
	for (int i = 0; i < m_rxStreamTable->columnCount(); i++) {
		settings->setArrayIndex(i);
		settings->setValue("columnWidth", m_rxStreamTable->columnWidth(i));
	}
	settings->endArray();

	settings->endGroup();
	
	delete settings;
}

void SyntroDB::restoreWindowState()
{
	QSettings *settings = SyntroUtils::getSettings();

	settings->beginGroup("Window");
	restoreGeometry(settings->value("Geometry").toByteArray());
	restoreState(settings->value("State").toByteArray());

	int count = settings->beginReadArray("Grid");
	for (int i = 0; i < count && i < m_rxStreamTable->columnCount(); i++) {
		settings->setArrayIndex(i);
		int width = settings->value("columnWidth").toInt();

		if (width > 0)
			m_rxStreamTable->setColumnWidth(i, width);
	}
	settings->endArray();

	settings->endGroup();
	
	delete settings;
}

void SyntroDB::onAbout()
{
	SyntroAbout dlg;
	dlg.exec();
}

void SyntroDB::onBasicSetup()
{
	BasicSetupDlg dlg(this);
	dlg.exec();
}

void SyntroDB::buttonClicked(int buttonId)
{
	StoreStreamDlg dlg(this, buttonId);

	if (dlg.exec()) {										// need to update client with changes
		TRACE1("Refreshing source index %d", buttonId);
		displayStreamDetails(buttonId);
		emit refreshStreamSource(buttonId);
	}
}

void SyntroDB::onConfiguration()
{
	QMessageBox msgBox;
	ConfigurationDlg dlg(this);

	if (dlg.exec()) {										// need to update client with changes
		msgBox.setText("The component must be restarted for these changes to take effect");
		msgBox.exec();
	}
}

void SyntroDB::boxClicked(bool state, int boxId)
{
	if (m_startingUp)
		return;												// avoids thrashing when loading config

	QSettings *settings = SyntroUtils::getSettings();
	
	settings->beginWriteArray(SYNTRODB_PARAMS_STREAM_SOURCES);
	settings->setArrayIndex(boxId);

	if (m_useBox[boxId]->checkState() == Qt::Checked)
		settings->setValue(SYNTRODB_PARAMS_INUSE, SYNTRO_PARAMS_TRUE);
	else
		settings->setValue(SYNTRODB_PARAMS_INUSE, SYNTRO_PARAMS_FALSE);
	settings->endArray();

	delete settings;

	if (!state) {											// disabled so zero stats
		m_rxStreamTable->item(boxId, SYNTRODB_COL_TOTALRECS)->setText("");
		m_rxStreamTable->item(boxId, SYNTRODB_COL_TOTALBYTES)->setText("");
		m_rxStreamTable->item(boxId, SYNTRODB_COL_FILERECS)->setText("");
		m_rxStreamTable->item(boxId, SYNTRODB_COL_FILEBYTES)->setText("");
		m_rxStreamTable->item(boxId, SYNTRODB_COL_FILE)->setText("");
	}
	emit refreshStreamSource(boxId);
}


//----------------------------------------------------------
//
// StoreButton functions

StoreButton::StoreButton(const QString& text, QWidget *parent, int id)
	: QPushButton(text, parent) 
{
	m_id = id;
	connect(this, SIGNAL(clicked(bool)), this, SLOT(originalClicked(bool)));
};

void StoreButton::originalClicked(bool )
{
	emit buttonClicked(m_id);
}

//----------------------------------------------------------
//
// StoreCheckBox functions

StoreCheckBox::StoreCheckBox(QWidget *parent, int id)
	: QCheckBox(parent) 
{
	m_id = id;
	connect(this, SIGNAL(clicked(bool)), this, SLOT(originalClicked(bool)));
};

void StoreCheckBox::originalClicked(bool state)
{
	emit boxClicked(state, m_id);
}
