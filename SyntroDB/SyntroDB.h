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

#ifndef SYNTRODB_H
#define SYNTRODB_H

#include <QMainWindow>
#include <qtablewidget.h>
#include "ui_SyntroDB.h"
#include "SyntroLib.h"
#include "StoreStreamDlg.h"

#define	SYNTRODB_MAX_STREAMS		(SYNTRO_MAX_SERVICESPERCOMPONENT / 2)

// SyntroDB specific setting keys

#define	SYNTRODB_MAXAGE					"maxAge"

#define SYNTRODB_PARAMS_ROOT_DIRECTORY  "RootDirectory"

#define	SYNTRODB_PARAMS_STREAM_SOURCES	"Streams"
#define	SYNTRODB_PARAMS_INUSE			"inUse"
#define	SYNTRODB_PARAMS_STREAM_SOURCE	"stream"
#define SYNTRODB_PARAMS_CREATE_SUBFOLDER	"createSubFolder"
#define	SYNTRODB_PARAMS_FORMAT			"storeFormat"
#define SYNTRODB_PARAMS_ROTATION_POLICY	"rotationPolicy"
#define SYNTRODB_PARAMS_ROTATION_TIME_UNITS	"rotationTimeUnits"
#define SYNTRODB_PARAMS_ROTATION_TIME	"rotationTime"
#define SYNTRODB_PARAMS_ROTATION_SIZE	"rotationSize"
#define SYNTRODB_PARAMS_DELETION_POLICY	"deletionPolicy"
#define SYNTRODB_PARAMS_DELETION_TIME_UNITS	"deletionTimeUnits"
#define SYNTRODB_PARAMS_DELETION_TIME	"deletionTime"
#define SYNTRODB_PARAMS_DELETION_COUNT	"deletionCount"

//	magic strings used in the .ini file

#define	SYNTRODB_PARAMS_ROTATION_TIME_UNITS_HOURS	"hours"
#define	SYNTRODB_PARAMS_ROTATION_TIME_UNITS_MINUTES	"minutes"
#define	SYNTRODB_PARAMS_ROTATION_TIME_UNITS_DAYS	"days"
#define SYNTRODB_PARAMS_ROTATION_POLICY_TIME			"time"
#define SYNTRODB_PARAMS_ROTATION_POLICY_SIZE			"size"
#define SYNTRODB_PARAMS_ROTATION_POLICY_ANY			"any"

#define	SYNTRODB_PARAMS_DELETION_TIME_UNITS_HOURS	"hours"
#define	SYNTRODB_PARAMS_DELETION_TIME_UNITS_DAYS		"days"
#define SYNTRODB_PARAMS_DELETION_POLICY_TIME			"time"
#define SYNTRODB_PARAMS_DELETION_POLICY_COUNT		"count"
#define SYNTRODB_PARAMS_DELETION_POLICY_ANY			"any"

#define SYNTRODB_PARAMS_DELETION_TIME_DEFAULT		"2"
#define SYNTRODB_PARAMS_DELETION_COUNT_DEFAULT		"5"


//	The CFS message

#define	SYNTRO_CFS_MESSAGE					(SYNTRO_MSTART + 0)	// the message used to pass CFS messages around


//	Display columns

#define	SYNTRODB_COL_CONFIG			0					// configure entry
#define	SYNTRODB_COL_INUSE			1					// entry in use
#define	SYNTRODB_COL_STREAM			2					// stream name
#define	SYNTRODB_COL_TOTALRECS		3					// total records
#define	SYNTRODB_COL_TOTALBYTES		4					// total bytes
#define	SYNTRODB_COL_FILERECS		5					// file records
#define	SYNTRODB_COL_FILEBYTES		6					// file bytes
#define	SYNTRODB_COL_FILE			7					// name of current file

#define SYNTRODB_COL_COUNT			8					// number of columns

//	Timer intervals

#define	SYNTROSTORE_BGND_INTERVAL			(SYNTRO_CLOCKS_PER_SEC/10)
#define	SYNTROSTORE_DM_INTERVAL				(SYNTRO_CLOCKS_PER_SEC * 10)

#define	SYNTROCFS_BGND_INTERVAL				(SYNTRO_CLOCKS_PER_SEC / 100)
#define	SYNTROCFS_DM_INTERVAL				(SYNTRO_CLOCKS_PER_SEC * 10)


class StoreClient;
class CFSClient;

class StoreButton : public QPushButton
{
	Q_OBJECT

public:
	StoreButton(const QString& text, QWidget *parent, int id);

public slots:
	void originalClicked(bool);

signals:
	void buttonClicked(int);

private:
	int m_id;
};

class StoreCheckBox : public QCheckBox
{
	Q_OBJECT

public:
	StoreCheckBox(QWidget *parent, int id);

public slots:
	void originalClicked(bool);

signals:
	void boxClicked(bool, int);

private:
	int m_id;
};


class SyntroDB : public QMainWindow
{
	Q_OBJECT

public:
	SyntroDB();
    ~SyntroDB() {}

public slots:
	void onAbout();
	void onBasicSetup();
	void boxClicked(bool, int);
	void buttonClicked(int);
	void onConfiguration();
    void storeRunning();

signals:
	void refreshStreamSource(int index);

protected:
	void closeEvent(QCloseEvent *event);
	void timerEvent(QTimerEvent *event);

private:
	void saveWindowState();
	void restoreWindowState();
	void initStatusBar();
	void initDisplayStats();
	void displayStreamDetails(int index);
	
	Ui::CSyntroDBClass ui;
	QLabel *m_controlStatus;
	QTableWidget *m_rxStreamTable;
	StoreClient *m_storeClient;
	CFSClient *m_CFSClient;
	int	m_timerId;

	StoreCheckBox *m_useBox[SYNTRODB_MAX_STREAMS];

	StoreStreamDlg *m_storeStreamDlg;

	bool m_startingUp;
};

#endif // SYNTRODB_H
