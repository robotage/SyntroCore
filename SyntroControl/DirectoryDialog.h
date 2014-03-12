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

#ifndef DIRECTORYDIALOG_H
#define DIRECTORYDIALOG_H

#include <QDialog>
#include <qplaintextedit.h>
#include <QTreeWidgetItem>
#include <QTreeWidget>
#include <QTableWidget>
#include "DirectoryManager.h"

class DirectoryManager;

class DirectoryDialog : public QDialog
{
	Q_OBJECT

public:
	DirectoryDialog(QWidget *parent = 0);
	~DirectoryDialog() {}

	DirectoryManager *m_directoryMgr;

public slots:
	void DMNewDirectory(int index);
	void selectionChanged();
	void DMRemoveDirectory(DM_COMPONENT *component);

protected:
	void closeEvent(QCloseEvent *event);

private:
	void layoutWindow();
	void updateTable();
	void addTableRow(DM_SERVICE *service);
	QTreeWidgetItem *findTreeItem(DM_COMPONENT *component);
	void addTreeItem(DM_CONNECTEDCOMPONENT *connectedComponent, DM_COMPONENT *component);
	void updateTreeItem(QTreeWidgetItem *twi, DM_COMPONENT *component);
	QString entryPath(DM_COMPONENT *component);

	QTableWidget *m_table;
	QTreeWidget *m_tree;
	QList <QTreeWidgetItem *> m_treeItems;
	QTreeWidgetItem *m_currentItem;							// this is the list index of the current selected entry	
	int m_currentDirIndex;									// the currently selected directory index

	QString m_logTag;
};

#endif // DIRECTORYDIALOG_H
