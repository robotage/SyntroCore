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

#include <qfiledialog.h>
#include <qmessagebox.h>

#include "SyntroDefs.h"
#include "SyntroAlert.h"
#include "SyntroAboutDlg.h"
#include "BasicSetupDlg.h"
#include "ViewFieldsDlg.h"
#include "MessageTypeDlg.h"

SyntroAlert::SyntroAlert(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);	

	m_activeClientCount = 0;

	connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));
	connect(ui.actionSave, SIGNAL(triggered()), this, SLOT(onSave()));
	connect(ui.actionClear, SIGNAL(triggered()), this, SLOT(onClear()));
	connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT(onAbout()));
	connect(ui.actionBasicSetup, SIGNAL(triggered()), this, SLOT(onBasicSetup()));
	connect(ui.actionTypes, SIGNAL(triggered()), this, SLOT(onTypes()));
	connect(ui.actionFields, SIGNAL(triggered()), this, SLOT(onFields()));

	SyntroUtils::syntroAppInit();
	startControlServer();

 	m_client = new AlertClient();
	connect(m_client, SIGNAL(newLogMsg(QByteArray)), this, SLOT(newLogMsg(QByteArray)), Qt::DirectConnection);
	connect(m_client, SIGNAL(activeClientUpdate(int)), this, SLOT(activeClientUpdate(int)), Qt::DirectConnection);
	m_client->resumeThread();

	m_fieldLabels << "Type" << "Timestamp" << "UID" << "AppType" << "Source" << "Message";
	restoreWindowState();

	initGrid();
	initStatusBar();
	onTypesChange();

	setWindowTitle(QString("%1 - %2")
		.arg(SyntroUtils::getAppType())
		.arg(SyntroUtils::getAppName()));

	m_timer = startTimer(20);
}

void SyntroAlert::closeEvent(QCloseEvent *)
{
	if (m_timer) {
		killTimer(m_timer);
		m_timer = 0;
	}

	if (m_client) {
		disconnect(m_client, SIGNAL(newLogMsg(QByteArray)), this, SLOT(newLogMsg(QByteArray)));
		disconnect(m_client, SIGNAL(activeClientUpdate(int)), this, SLOT(activeClientUpdate(int)));
		m_client->exitThread();
	}

	if (m_controlServer) {
		m_controlServer->exitThread();
		m_controlServer = NULL;
	}

	saveWindowState();

	SyntroUtils::syntroAppExit();
}

void SyntroAlert::startControlServer()
{
	QSettings *settings = SyntroUtils::getSettings();

	if (settings->value(SYNTRO_PARAMS_LOCALCONTROL).toBool()) {
		m_controlServer = new SyntroServer();
		m_controlServer->resumeThread();
	} 
	else {
		m_controlServer = NULL;
	}

	delete settings;
}

void SyntroAlert::newLogMsg(QByteArray bulkMsg)
{
	QMutexLocker lock(&m_logMutex);
	m_logQ.enqueue(bulkMsg);
}

void SyntroAlert::activeClientUpdate(int count)
{
	QMutexLocker lock(&m_activeClientMutex);
	m_activeClientCount = count;
}

void SyntroAlert::timerEvent(QTimerEvent *)
{
	m_controlStatus->setText(m_client->getLinkState());
	m_activeClientMutex.lock();
	m_activeClientStatus->setText(QString("Active clients - %1").arg(m_activeClientCount));
	m_activeClientMutex.unlock();
	parseMsgQueue();
}

void SyntroAlert::parseMsgQueue()
{
	QMutexLocker lock(&m_logMutex);

	if (!m_logQ.empty()) {
		QByteArray bulkMsg = m_logQ.dequeue();
		QString s(bulkMsg);
		QStringList list = s.split('\n');

		for (int i = 0; i < list.count(); i++)
			addMessage(list[i]);
	}
}

void SyntroAlert::addMessage(QString packedMsg)
{
    QStringList msg = packedMsg.split(SYNTRO_LOG_COMPONENT_SEP);

    if (msg.count() != NUM_LOG_FIELDS)
		return;

    msg[TYPE_FIELD] = msg[TYPE_FIELD].toLower();
    msg[TIMESTAMP_FIELD].replace('T', ' ');
    msg[APP_TYPE_FIELD].remove(SYNTRO_LOG_SERVICE_TAG);

    m_entries.append(msg);

    updateTable(msg);
}

void SyntroAlert::updateTable(QStringList msg)
{
    int row = findRowInsertPosition(msg);

    if (row < 0)
        return;

	ui.logTable->insertRow(row);

    for (int i = 0; i < m_viewFields.count(); i++) {
        int field = m_viewFields.at(i);

        QTableWidgetItem *item = new QTableWidgetItem();
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        item->setText(msg.at(field));

        if (field == TYPE_FIELD) {
            if (msg.at(TYPE_FIELD) == SYNTRO_LOG_ERROR)
                item->setBackgroundColor(Qt::red);
            else if (msg.at(TYPE_FIELD) == SYNTRO_LOG_WARN)
                item->setBackgroundColor(Qt::yellow);
            else if (msg.at(TYPE_FIELD) == SYNTRO_LOG_DEBUG)
                item->setBackgroundColor(Qt::green);
        }

        ui.logTable->setItem(row, i, item);
    }

    ui.logTable->scrollToBottom();
}

int SyntroAlert::findRowInsertPosition(QStringList msg)
{
	if (filtered(msg.at(TYPE_FIELD)))
		return -1;

    QString timestamp = msg.at(TIMESTAMP_FIELD);

	for (int i = ui.logTable->rowCount() - 1; i >= 0; i--) {
        QTableWidgetItem *item = ui.logTable->item(i, m_timestampCol);

		if (timestamp >= item->text())
			return i + 1;
	}

	return 0;
}

bool SyntroAlert::filtered(QString type)
{
	if (m_currentTypes.contains(type))
		return false;

	if (m_currentTypes.contains("alert")) {
		if (type != SYNTRO_LOG_ERROR && type != SYNTRO_LOG_WARN && type != SYNTRO_LOG_INFO  && type != SYNTRO_LOG_DEBUG)
			return false;
	}

	return true;
}

void SyntroAlert::onClear()
{
	m_entries.clear();
	onTypesChange();
}

void SyntroAlert::onSave()
{
	QFile file;
	QString fileName = QFileDialog::getSaveFileName(this, "SyntroAlert Save", m_savePath, QString("Logs (*.log)"));

	if (fileName.isEmpty())
		return;

	file.setFileName(fileName);

	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		QMessageBox::warning(this, "SyntroAlert Save", "File save failed");
		return;
	}

	QTextStream textStream(&file);

	m_logMutex.lock();

    for (int i = 0; i < m_entries.count(); i++) {
        QStringList msg = m_entries.at(i);

        for (int j = 0; j < msg.count(); j++) {
            textStream << msg.at(j);

            if (j == msg.count() - 1)
                textStream << endl;
            else
                textStream << SYNTRO_LOG_COMPONENT_SEP;
        }
    }

	m_logMutex.unlock();

	// for next time
	m_savePath = fileName;
}

void SyntroAlert::onTypes()
{
	MessageTypeDlg dlg(this, m_currentTypes);

	if (dlg.exec() == QDialog::Accepted) {
		m_currentTypes = dlg.newTypes();
		onTypesChange();
	}		
}

void SyntroAlert::onTypesChange()
{
    ui.logTable->setRowCount(0);

    for (int i = 0; i < m_entries.count(); i++)
        updateTable(m_entries.at(i));
}

void SyntroAlert::onFields()
{
    ViewFieldsDlg dlg(this, m_viewFields);

    if (dlg.exec() == QDialog::Accepted) {
        QList<int> newFields = dlg.viewFields();

        if (newFields != m_viewFields) {
            m_viewFields = newFields;
            validateViewFields();
            ui.logTable->setRowCount(0);
            onFieldsChange();
            onTypesChange();
        }
    }
}

void SyntroAlert::onFieldsChange()
{
    ui.logTable->setColumnCount(m_viewFields.count());
    ui.logTable->horizontalHeader()->setStretchLastSection(true);

    QStringList labels;

    for (int i = 0; i < m_viewFields.count(); i++)
        labels << m_fieldLabels.at(m_viewFields.at(i));

    ui.logTable->setHorizontalHeaderLabels(labels);
}

void SyntroAlert::initGrid()
{
	ui.logTable->verticalHeader()->setDefaultSectionSize(fontMetrics().height() + 4);
	ui.logTable->setSelectionMode(QAbstractItemView::ContiguousSelection);
	ui.logTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    onFieldsChange();

    for (int i = 0; i < m_viewFields.count() && i < m_colWidths.count(); i++) {
        if (m_colWidths.at(i) > 0)
            ui.logTable->setColumnWidth(i, m_colWidths.at(i));
    }

    ui.logTable->setShowGrid(true);
}

void SyntroAlert::initStatusBar()
{
	m_controlStatus = new QLabel(this);
	m_controlStatus->setAlignment(Qt::AlignLeft);
	ui.statusBar->addWidget(m_controlStatus, 1);

	m_activeClientStatus = new QLabel(this);
	m_activeClientStatus->setAlignment(Qt::AlignRight);
	ui.statusBar->addWidget(m_activeClientStatus);
}

void SyntroAlert::onAbout()
{
	SyntroAbout dlg;
	dlg.exec();
}

void SyntroAlert::onBasicSetup()
{
	BasicSetupDlg dlg(this);
	dlg.exec();
}

void SyntroAlert::saveWindowState()
{
	QSettings *settings = SyntroUtils::getSettings();

	settings->beginGroup("Window");
	settings->setValue("Geometry", saveGeometry());
	settings->setValue("State", saveState());
    settings->endGroup();

    settings->beginGroup("View");

	settings->beginWriteArray("Grid");
	for (int i = 0; i < ui.logTable->columnCount() - 1; i++) {
		settings->setArrayIndex(i);
		settings->setValue("columnWidth", ui.logTable->columnWidth(i));
	}
	settings->endArray();

    settings->beginWriteArray("Fields");
    for (int i = 0; i < m_viewFields.count(); i++) {
        settings->setArrayIndex(i);
        settings->setValue("Field", m_viewFields.at(i));
    }
    settings->endArray();

	settings->endGroup();

	settings->beginGroup("Types");
	settings->setValue(SYNTRO_LOG_ERROR, m_currentTypes.contains(SYNTRO_LOG_ERROR) ? true : false);
	settings->setValue(SYNTRO_LOG_WARN, m_currentTypes.contains(SYNTRO_LOG_WARN) ? true : false);
	settings->setValue(SYNTRO_LOG_INFO, m_currentTypes.contains(SYNTRO_LOG_INFO) ? true : false);
	settings->setValue(SYNTRO_LOG_DEBUG, m_currentTypes.contains(SYNTRO_LOG_DEBUG) ? true : false);
	settings->setValue("alert", m_currentTypes.contains("alert") ? true : false);
	settings->endGroup();

	delete settings;
}

void SyntroAlert::restoreWindowState()
{
	QSettings *settings = SyntroUtils::getSettings();

	settings->beginGroup("Window");
	restoreGeometry(settings->value("Geometry").toByteArray());
	restoreState(settings->value("State").toByteArray());
    settings->endGroup();

    settings->beginGroup("View");
    m_colWidths.clear();

	int count = settings->beginReadArray("Grid");
    for (int i = 0; i < count; i++) {
		settings->setArrayIndex(i);
        m_colWidths.append(settings->value("columnWidth").toInt());
	}
    settings->endArray();

    m_viewFields.clear();

    count = settings->beginReadArray("Fields");
    for (int i = 0; i < count; i++) {
        settings->setArrayIndex(i);
        int field = settings->value("Field").toInt();

        if (field >= 0 && field < NUM_LOG_FIELDS)
            m_viewFields.append(field);
    }
    settings->endArray();
	
	settings->endGroup();

    validateViewFields();

	m_currentTypes.clear();

	settings->beginGroup("Types");

	if (settings->value(SYNTRO_LOG_ERROR, true).toBool())
		m_currentTypes.append(SYNTRO_LOG_ERROR);

	if (settings->value(SYNTRO_LOG_WARN, true).toBool())
		m_currentTypes.append(SYNTRO_LOG_WARN);

	if (settings->value(SYNTRO_LOG_INFO, true).toBool())
		m_currentTypes.append(SYNTRO_LOG_INFO);

	if (settings->value(SYNTRO_LOG_DEBUG, true).toBool())
		m_currentTypes.append(SYNTRO_LOG_DEBUG);

	if (settings->value("alert", true).toBool())
		m_currentTypes.append("alert");

    settings->endGroup();

	if (m_currentTypes.count() == 0)
		m_currentTypes << SYNTRO_LOG_ERROR << SYNTRO_LOG_WARN << "alert";

	delete settings;
}

void SyntroAlert::validateViewFields()
{
    if (m_viewFields.count() < 2) {
        setDefaultViewFields();
        return;
    }

    if (m_viewFields.at(m_viewFields.count() - 1) != MESSAGE_FIELD) {
        setDefaultViewFields();
        return;
    }

    if (m_viewFields.at(0) == TYPE_FIELD) {
        if (m_viewFields.at(1) != TIMESTAMP_FIELD) {
            setDefaultViewFields();
            return;
        }
    }
    else if (m_viewFields.at(0) != TIMESTAMP_FIELD) {
        setDefaultViewFields();
        return;
    }

    for (int i = 1; i < m_viewFields.count(); i++) {
        if (m_viewFields.at(i) <= m_viewFields.at(i-1)) {
            setDefaultViewFields();
            return;
        }
    }

    if (m_viewFields.at(0) == TIMESTAMP_FIELD)
        m_timestampCol = 0;
    else
        m_timestampCol = 1;
}

void SyntroAlert::setDefaultViewFields()
{
    m_viewFields.clear();

    m_viewFields.append(TYPE_FIELD);
    m_viewFields.append(TIMESTAMP_FIELD);
    m_viewFields.append(APP_TYPE_FIELD);
    m_viewFields.append(SOURCE_FIELD);
    m_viewFields.append(MESSAGE_FIELD);

    m_timestampCol = 1;
}
