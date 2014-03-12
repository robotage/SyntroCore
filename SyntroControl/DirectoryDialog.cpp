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

#include <QSplitter>
#include <QBoxLayout>
#include "DirectoryDialog.h"
#include "SyntroLib.h"
#include "SyntroServer.h"


DirectoryDialog::DirectoryDialog(QWidget *parent)
	: QDialog(parent)
{
	m_logTag = "DirectoryDialog";
	hide();
	layoutWindow();
	setWindowTitle("SyntroControl directory");
	m_currentDirIndex = -1;
	m_currentItem = NULL;
}

void DirectoryDialog::closeEvent(QCloseEvent *event)
{
	hide();
	event->ignore();
}

void DirectoryDialog::layoutWindow()
{
	QList<QTreeWidgetItem *> items;

	setMinimumSize(500, 250);
	QSplitter *splitter = new QSplitter(this);
	m_tree = new QTreeWidget(splitter);
	m_tree->setColumnCount(1);
	m_tree->setHeaderLabel("Component");
	QVBoxLayout *vertLayout = new QVBoxLayout(this);
	vertLayout->addWidget(splitter);

	m_table = new QTableWidget(splitter);
	m_table->setColumnCount(3);

    m_table->setColumnWidth(0, 120);
    m_table->setColumnWidth(1, 60);
    m_table->setColumnWidth(2, 100);
 
	m_table->setHorizontalHeaderLabels(
                QStringList() << tr("Service name") << tr("Port")
                << tr("Type"));

    m_table->setSelectionMode(QAbstractItemView::NoSelection);

	splitter->addWidget(m_tree);
	splitter->addWidget(m_table);
	splitter->setHandleWidth(1);
	setLayout(vertLayout);
	QList <int> sizeList;
	sizeList.append(180);
	sizeList.append(250);
	splitter->setSizes(sizeList);

	vertLayout->setContentsMargins(0, 0, 0, 0);

	//	This connection MUST be queued therwise deadlock will occur when a directory entry is removed

	connect(m_tree, SIGNAL(itemSelectionChanged()), this, SLOT(selectionChanged()), Qt::QueuedConnection);
}

void DirectoryDialog::DMNewDirectory(int index)
{
	QMutexLocker locker(&(m_directoryMgr->m_lock));
	QTreeWidgetItem *twi;

	DM_CONNECTEDCOMPONENT *connectedComponent = m_directoryMgr->m_directory + index;
	if (!connectedComponent->valid)
		return;												// not a valid entry
	TRACE1("Displaying new directory from %s", qPrintable(SyntroUtils::displayUID(&(connectedComponent->connectedComponentUID))));

	DM_COMPONENT *component = connectedComponent->componentDE;

	while (component != NULL) {
		if ((twi = findTreeItem(component)) == NULL)
			addTreeItem(connectedComponent, component);
		else
			updateTreeItem(twi, component);
		component = component->next;
	}
}

void DirectoryDialog::DMRemoveDirectory(DM_COMPONENT *component)
{
	QTreeWidgetItem *twi;
	int treePos;

	if ((twi = findTreeItem(component)) == NULL)
		return;												// can't find it anyway
	if (m_currentItem != NULL) {
		if (m_currentDirIndex == m_currentItem->data(0, Qt::UserRole).toInt()) {
			m_currentDirIndex = -1;
			while (m_table->rowCount() > 0)
				m_table->removeRow(0);							// clear table
		}
	}
	treePos = m_tree->indexOfTopLevelItem(twi);
	if (treePos == -1)
		return;												// should never happen
	m_tree->takeTopLevelItem(treePos);
}

void DirectoryDialog::updateTable()
{
	QMutexLocker locker(&(m_directoryMgr->m_lock));

	while (m_table->rowCount() > 0)
		m_table->removeRow(0);								// clear table

	if (m_currentDirIndex == -1)
		return;												// nothing else to do

	DM_CONNECTEDCOMPONENT *connectedComponent = m_directoryMgr->m_directory + m_currentDirIndex;
	if (!connectedComponent->valid)
		return;												// not a valid entry
	DM_COMPONENT *component = connectedComponent->componentDE;

	while (component != NULL) {
		if (entryPath(component) == m_currentItem->text(0)) {			// found the correct component
			DM_SERVICE *service = component->services;
			for (int serviceIndex = 0; serviceIndex < component->serviceCount; serviceIndex++, service++) {
				if (!service->valid)
					continue;		// can this happen?
				if (service->serviceType != SERVICETYPE_NOSERVICE) {
					addTableRow(service);
				}
			}
		}
		component = component->next;
	}
}

void DirectoryDialog::addTableRow(DM_SERVICE *service)
{
	int index = m_table->rowCount();
	m_table->insertRow(index);
	m_table->setRowHeight(index, 20);

	for (int col = 0; col < 3; col++) {
		QTableWidgetItem *item = new QTableWidgetItem();
		item->setTextAlignment(Qt::AlignLeft | Qt::AlignBottom);
		item->setFlags(Qt::ItemIsEnabled);
		item->setText("");
		m_table->setItem(index, col, item);
	}
	(m_table->item(index, 0))->setText(service->serviceName);
	(m_table->item(index, 1))->setText(QString::number(service->port));
	(m_table->item(index, 2))->setText(service->serviceType == SERVICETYPE_MULTICAST ? "Multicast" : "E2E");
}

QTreeWidgetItem *DirectoryDialog::findTreeItem(DM_COMPONENT *component)
{
	QString entryPath;

	entryPath = SyntroUtils::displayUID(&(component->componentUID)) + ", " + QString(component->appName);
	
	QList <QTreeWidgetItem *> matchItems = m_tree->findItems(entryPath, Qt::MatchExactly);
	if (matchItems.count() == 0)
		return NULL;

	if (matchItems.count() != 1)
		logError(QString("More than one match for " + entryPath));
	return matchItems.at(0);							// just use first one whatever
}


void DirectoryDialog::addTreeItem(DM_CONNECTEDCOMPONENT *connectedComponent, DM_COMPONENT *component)
{
	QTreeWidgetItem *twi;
	bool control;
	QString controlName;
	QString entryPath;

	entryPath = SyntroUtils::displayUID(&(component->componentUID)) + ", " + QString(component->appName);

	control = connectedComponent->connectedComponentUID.instance == INSTANCE_CONTROL;
	if (control) {
		controlName = ((SS_COMPONENT *)connectedComponent->data)->heartbeat.hello.appName;
	}

	twi = new QTreeWidgetItem((QTreeWidget*)0, QStringList(entryPath));
	twi->setData(0, Qt::UserRole, connectedComponent->index);		// so we can identify the entry later
	if (control)
		new QTreeWidgetItem(twi, QStringList(QString("Via SyntroControl ") + controlName));
	else
		new QTreeWidgetItem(twi, QStringList(QString("Directly connected")));
	new QTreeWidgetItem(twi, QStringList(QString("Component type: %1").arg(component->componentType)));
	new QTreeWidgetItem(twi, QStringList(QString("Sequence ID: %1").arg(component->sequenceID)));
	m_treeItems.append(twi);
	m_tree->insertTopLevelItem(0, twi);
}

void DirectoryDialog::updateTreeItem(QTreeWidgetItem *twi, DM_COMPONENT *component)
{
	(twi->child(1))->setText(0, QString("Component type: %1").arg(component->componentType));
	(twi->child(2))->setText(0, QString("Sequence ID: %1").arg(component->sequenceID));
}

QString DirectoryDialog::entryPath(DM_COMPONENT *component)
{
	QString path = SyntroUtils::displayUID(&(component->componentUID)) + ", " + QString(component->appName);
	return path;
}


void DirectoryDialog::selectionChanged()
{
	m_currentItem = m_tree->currentItem();
	if (m_currentItem == NULL) {
		m_currentDirIndex = -1;
		return;
	}
	if (m_currentItem->parent() != NULL)
		m_currentItem = m_currentItem->parent();			// was second level item
	if (m_currentItem == NULL) {
		m_currentDirIndex = -1;
		return;
	}
	m_currentDirIndex = m_currentItem->data(0, Qt::UserRole).toInt();
	updateTable();
}
