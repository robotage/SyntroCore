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

#ifndef SYNTROLOG_H
#define SYNTROLOG_H

#include <QMainWindow>
#include <qstringlist.h>
#include "ui_syntrolog.h"
#include "LogClient.h"

class SyntroLog : public QMainWindow
{
	Q_OBJECT

public:
	SyntroLog(QWidget *parent = 0);
	
public slots:
	void newLogMsg(QByteArray bulkMsg);
	void activeClientUpdate(int count);
	void onSave();
	void onBasicSetup();
	void onAbout();
    void onSeverityLevel();
    void onFields();

protected:
	void closeEvent(QCloseEvent *event);
	void timerEvent(QTimerEvent *event);
	
private:
	void saveWindowState();
	void restoreWindowState();
	void initStatusBar();
	void initGrid();
	void parseMsgQueue();
    void addMessage(QString packedMsg);
    void updateTable(QStringList msg);
    int findRowInsertPosition(QStringList msg);
    int msgLogLevel(QString level);
    void onLevelChange();
    void onFieldsChange();
    void validateViewFields();
    void setDefaultViewFields();

	Ui::SyntroLogClass ui;

	LogClient *m_client;
	int m_timer;
	QQueue<QByteArray> m_logQ;
	QMutex m_logMutex;

	int m_activeClientCount;
	QMutex m_activeClientMutex;

	QLabel *m_controlStatus;
    QLabel *m_severityLevelStatus;
	QLabel *m_activeClientStatus;

    QList<QStringList> m_entries;
    QList<int> m_viewFields;
    QStringList m_fieldLabels;
    QList<int> m_colWidths;
    int m_severityLevel;
    int m_timestampCol;

	QString m_savePath;
};

#endif // SYNTROLOG_H
