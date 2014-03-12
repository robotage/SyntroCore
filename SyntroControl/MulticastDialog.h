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

#ifndef MULTICASTDIALOG_H
#define MULTICASTDIALOG_H

#include <QDialog>
#include <QPlainTextEdit>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTableWidget>

class MulticastManager;

class MulticastDialog : public QDialog
{
	Q_OBJECT

public:
	MulticastDialog(QWidget *parent = 0);
	~MulticastDialog() {}

	MulticastManager *m_multicastMgr;

public slots:
	void MMDisplay();
	void MMNewEntry(int index);
	void MMDeleteEntry(int index);
	void MMRegistrationChanged(int index);
	void selectionChanged();

protected:
	void closeEvent(QCloseEvent * event);

private:
	void layoutWindow();
	void clearTable();
	void addTableRow(int index);

	QTableWidget *m_table;
	QTreeWidget *m_tree;
	QList <QTreeWidgetItem *> m_treeItems;
	int m_currentMapIndex;									// this is the multicast manager's index of the current service	
	QTreeWidgetItem *m_currentItem;							// this is the list index of the current service	

	QString m_logTag;
};

#endif // MULTICASTDIALOG_H
