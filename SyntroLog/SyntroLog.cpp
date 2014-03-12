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
#include "SyntroLog.h"
#include "SyntroAboutDlg.h"
#include "BasicSetupDlg.h"
#include "SeverityLevelDlg.h"
#include "ViewLogFieldsDlg.h"

SyntroLog::SyntroLog(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);	

	m_activeClientCount = 0;

	connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));
	connect(ui.actionSave, SIGNAL(triggered()), this, SLOT(onSave()));
	connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT(onAbout()));
	connect(ui.actionBasicSetup, SIGNAL(triggered()), this, SLOT(onBasicSetup()));
    connect(ui.actionSeverityLevel, SIGNAL(triggered()), this, SLOT(onSeverityLevel()));
    connect(ui.actionFields, SIGNAL(triggered()), this, SLOT(onFields()));

	SyntroUtils::syntroAppInit();

    m_client = new LogClient();
	connect(m_client, SIGNAL(newLogMsg(QByteArray)), this, SLOT(newLogMsg(QByteArray)), Qt::DirectConnection);
	connect(m_client, SIGNAL(activeClientUpdate(int)), this, SLOT(activeClientUpdate(int)), Qt::DirectConnection);
	m_client->resumeThread();

    m_fieldLabels << "Level" << "Timestamp" << "UID" << "AppType" << "AppName" << "Message";
	restoreWindowState();

    initGrid();
    initStatusBar();
    onLevelChange();

	setWindowTitle(QString("%1 - %2")
		.arg(SyntroUtils::getAppType())
		.arg(SyntroUtils::getAppName()));

	m_timer = startTimer(20);
}

void SyntroLog::closeEvent(QCloseEvent *)
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

	saveWindowState();
	SyntroUtils::syntroAppExit();
}

void SyntroLog::newLogMsg(QByteArray bulkMsg)
{
	QMutexLocker lock(&m_logMutex);
	m_logQ.enqueue(bulkMsg);
}

void SyntroLog::activeClientUpdate(int count)
{
	QMutexLocker lock(&m_activeClientMutex);
	m_activeClientCount = count;
}

void SyntroLog::timerEvent(QTimerEvent *)
{
	m_controlStatus->setText(m_client->getLinkState());
	m_activeClientMutex.lock();
	m_activeClientStatus->setText(QString("Active clients - %1").arg(m_activeClientCount));
	m_activeClientMutex.unlock();
	parseMsgQueue();
}

void SyntroLog::parseMsgQueue()
{
	QMutexLocker lock(&m_logMutex);

	if (!m_logQ.empty()) {
		QByteArray bulkMsg = m_logQ.dequeue();
		QString s(bulkMsg);
		QStringList list = s.split('\n');

		for (int i = 0; i < list.count(); i++) {
			addMessage(list[i]);
		}
	}
}

void SyntroLog::addMessage(QString packedMsg)
{
    QStringList msg = packedMsg.split(SYNTRO_LOG_COMPONENT_SEP);

    if (msg.count() != NUM_LOG_FIELDS)
		return;

    msg[LOGLEVEL_FIELD] = msg[LOGLEVEL_FIELD].toLower();
    msg[TIMESTAMP_FIELD].replace('T', ' ');
    msg[APP_TYPE_FIELD].remove(SYNTRO_LOG_SERVICE_TAG);

    m_entries.append(msg);

    updateTable(msg);
}

void SyntroLog::updateTable(QStringList msg)
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

        if (field == LOGLEVEL_FIELD) {
            if (msg.at(LOGLEVEL_FIELD) == SYNTRO_LOG_ERROR)
                item->setBackgroundColor(Qt::red);
            else if (msg.at(LOGLEVEL_FIELD) == SYNTRO_LOG_WARN)
                item->setBackgroundColor(Qt::yellow);
            else if (msg.at(LOGLEVEL_FIELD) == SYNTRO_LOG_DEBUG)
                item->setBackgroundColor(Qt::green);
        }

        ui.logTable->setItem(row, i, item);
    }

    ui.logTable->scrollToBottom();
}

int SyntroLog::findRowInsertPosition(QStringList msg)
{
    int level = msgLogLevel(msg.at(LOGLEVEL_FIELD));

    if (level > m_severityLevel)
        return -1;

    QString timestamp = msg.at(TIMESTAMP_FIELD);

	for (int i = ui.logTable->rowCount() - 1; i >= 0; i--) {
        QTableWidgetItem *item = ui.logTable->item(i, m_timestampCol);

		if (timestamp >= item->text())
			return i + 1;
	}

	return 0;
}

int SyntroLog::msgLogLevel(QString level)
{
    if (level == SYNTRO_LOG_ERROR)
        return SYNTRO_LOG_LEVEL_ERROR;
    else if (level == SYNTRO_LOG_WARN)
        return SYNTRO_LOG_LEVEL_WARN;
    else if (level == SYNTRO_LOG_INFO)
        return SYNTRO_LOG_LEVEL_INFO;
    else if (level == SYNTRO_LOG_DEBUG)
        return SYNTRO_LOG_LEVEL_DEBUG;
    else
        return 100;
}

void SyntroLog::onSave()
{
	QFile file;
	QString fileName = QFileDialog::getSaveFileName(this, "SyntroLog Save", m_savePath, QString("Logs (*.log)"));

	if (fileName.isEmpty())
		return;

	file.setFileName(fileName);

	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		QMessageBox::warning(this, "SyntroLog Save", "File save failed");
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

void SyntroLog::onSeverityLevel()
{
    SeverityLevelDlg dlg(this, m_severityLevel);

    if (dlg.exec() == QDialog::Accepted) {
        int newLevel = dlg.level();

        if (newLevel != m_severityLevel) {
            m_severityLevel = newLevel;
            onLevelChange();
        }
    }
}

void SyntroLog::onLevelChange()
{
    switch (m_severityLevel) {
    case SYNTRO_LOG_LEVEL_ERROR:
        m_severityLevelStatus->setText("Severity level - error");
        break;
    case SYNTRO_LOG_LEVEL_WARN:
        m_severityLevelStatus->setText("Severity level - warn");
        break;
    case SYNTRO_LOG_LEVEL_INFO:
        m_severityLevelStatus->setText("Severity level - info");
        break;
    case SYNTRO_LOG_LEVEL_DEBUG:
        m_severityLevelStatus->setText("Severity level - debug");
        break;
    }

    ui.logTable->setRowCount(0);

    for (int i = 0; i < m_entries.count(); i++)
        updateTable(m_entries.at(i));
}

void SyntroLog::onFields()
{
    ViewLogFieldsDlg dlg(this, m_viewFields);

    if (dlg.exec() == QDialog::Accepted) {
        QList<int> newFields = dlg.viewFields();

        if (newFields != m_viewFields) {
            m_viewFields = newFields;
            validateViewFields();
            ui.logTable->setRowCount(0);
            onFieldsChange();
            onLevelChange();
        }
    }
}

void SyntroLog::onFieldsChange()
{
    ui.logTable->setColumnCount(m_viewFields.count());
    ui.logTable->horizontalHeader()->setStretchLastSection(true);

    QStringList labels;

    for (int i = 0; i < m_viewFields.count(); i++)
        labels << m_fieldLabels.at(m_viewFields.at(i));

    ui.logTable->setHorizontalHeaderLabels(labels);
}

void SyntroLog::initGrid()
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

void SyntroLog::initStatusBar()
{
	m_controlStatus = new QLabel(this);
	m_controlStatus->setAlignment(Qt::AlignLeft);
	ui.statusBar->addWidget(m_controlStatus, 1);

    m_severityLevelStatus = new QLabel(this);
    m_severityLevelStatus->setAlignment(Qt::AlignLeft);

    ui.statusBar->addWidget(m_severityLevelStatus);

	m_activeClientStatus = new QLabel(this);
	m_activeClientStatus->setAlignment(Qt::AlignRight);
	ui.statusBar->addWidget(m_activeClientStatus);
}

void SyntroLog::onAbout()
{
	SyntroAbout dlg;
	dlg.exec();
}

void SyntroLog::onBasicSetup()
{
	BasicSetupDlg dlg(this);
	dlg.exec();
}

void SyntroLog::saveWindowState()
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

    settings->setValue("SeverityLevel", m_severityLevel);

	settings->endGroup();

	delete settings;
}

void SyntroLog::restoreWindowState()
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

    validateViewFields();

    m_severityLevel = settings->value("SeverityLevel", SYNTRO_LOG_LEVEL_WARN).toInt();

    if (m_severityLevel < SYNTRO_LOG_LEVEL_ERROR && m_severityLevel > SYNTRO_LOG_LEVEL_DEBUG)
        m_severityLevel = SYNTRO_LOG_LEVEL_WARN;

    settings->endGroup();

	delete settings;
}

void SyntroLog::validateViewFields()
{
    if (m_viewFields.count() < 2) {
        setDefaultViewFields();
        return;
    }

    if (m_viewFields.at(m_viewFields.count() - 1) != MESSAGE_FIELD) {
        setDefaultViewFields();
        return;
    }

    if (m_viewFields.at(0) == LOGLEVEL_FIELD) {
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

void SyntroLog::setDefaultViewFields()
{
    m_viewFields.clear();

    m_viewFields.append(LOGLEVEL_FIELD);
    m_viewFields.append(TIMESTAMP_FIELD);
    m_viewFields.append(APP_TYPE_FIELD);
    m_viewFields.append(APP_NAME_FIELD);
    m_viewFields.append(MESSAGE_FIELD);

    m_timestampCol = 1;
}
