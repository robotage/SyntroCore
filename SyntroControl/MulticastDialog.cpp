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
#include "SyntroLib.h"
#include "MulticastDialog.h"
#include "MulticastManager.h"


MulticastDialog::MulticastDialog(QWidget *parent)
	: QDialog(parent)
{
	m_logTag = "MulticastDialog";
	hide();
	layoutWindow();
	setWindowTitle("SyntroControl multicast maps");
	m_currentMapIndex = -1;
	m_currentItem = NULL;
}

void MulticastDialog::closeEvent(QCloseEvent *event)
{
	hide();
	event->ignore();
}

void MulticastDialog::layoutWindow()
{
	QList<QTreeWidgetItem *> items;

	setMinimumSize(500, 250);
	QSplitter *splitter = new QSplitter(this);
	m_tree = new QTreeWidget(splitter);
	m_tree->setColumnCount(1);
	m_tree->setHeaderLabel("Multicast source");
	QVBoxLayout *vertLayout = new QVBoxLayout(this);
	vertLayout->addWidget(splitter);

	m_table = new QTableWidget(splitter);
	m_table->setColumnCount(4);

    m_table->setColumnWidth(0, 120);
    m_table->setColumnWidth(1, 80);
    m_table->setColumnWidth(2, 80);
    m_table->setColumnWidth(3, 80);

	m_table->setHorizontalHeaderLabels(
                QStringList() << tr("UID") << tr("Port")
                << tr("Seq") << tr("Ack"));

    m_table->setSelectionMode(QAbstractItemView::NoSelection);

	splitter->addWidget(m_tree);
	splitter->addWidget(m_table);
	splitter->setHandleWidth(1);
	setLayout(vertLayout);
	QList <int> sizeList;
	sizeList.append(250);
	sizeList.append(300);
	splitter->setSizes(sizeList);

	vertLayout->setContentsMargins(0, 0, 0, 0);

	connect(m_tree, SIGNAL(itemSelectionChanged()), this, SLOT(selectionChanged()));
}


void MulticastDialog::MMNewEntry(int index)
{
	MM_MMAP *map;
	QMutexLocker locker(&(m_multicastMgr->m_lock));

	map = &(m_multicastMgr->m_multicastMap[index]);
	if (!map->valid)
		return;												// must be historic

	QTreeWidgetItem *twi = new QTreeWidgetItem((QTreeWidget*)0, QStringList(map->serviceLookup.servicePath));
	new QTreeWidgetItem(twi, QStringList(QString(SyntroUtils::displayUID(&(map->sourceUID)))));
	new QTreeWidgetItem(twi, QStringList(QString("Local port: %1").arg(SyntroUtils::convertUC2ToInt(map->serviceLookup.localPort))));
	new QTreeWidgetItem(twi, QStringList(QString("Remote port: %1").arg(SyntroUtils::convertUC2ToInt(map->serviceLookup.remotePort))));
	twi->setData(0, Qt::UserRole, SyntroUtils::convertUC2ToInt(map->serviceLookup.localPort));	// so we can identify the entry later
	m_treeItems.append(twi);
	m_tree->insertTopLevelItem(0, twi);
	m_tree->sortItems(0, Qt::AscendingOrder);
}


void MulticastDialog::MMDeleteEntry(int index)
{
	int i;
	QTreeWidgetItem *twi;
	int treePos;

	for (i = 0; i < m_treeItems.count(); i++) {
		twi = m_treeItems.at(i);
		if (twi->data(0, Qt::UserRole).toInt() == index) {	// found the entry
			treePos = m_tree->indexOfTopLevelItem(twi);
			if (treePos != -1)
				m_tree->takeTopLevelItem(treePos);			// remove from tree
			m_treeItems.removeAt(i);
			if (index == m_currentMapIndex) {
				m_currentMapIndex = -1;
				clearTable();
			}
			return;
		}
	}
	logError(QString("Got delete map display entry request on %d but not in table").arg(index));
}

void MulticastDialog::MMRegistrationChanged(int index)
{
	if (index != m_currentMapIndex)
		return;												// not interested if not displaying
	clearTable();
	MMDisplay();
}


void MulticastDialog::MMDisplay()
{
	int rowIndex;

	QMutexLocker locker(&(m_multicastMgr->m_lock));

	if (m_currentMapIndex == -1)
		return;												// this means there is no selection

	MM_MMAP	*map = &(m_multicastMgr->m_multicastMap[m_currentMapIndex]);

	if (!map->valid) {
		clearTable();
		return;
	}

	MM_REGISTEREDCOMPONENT *registeredComponent = map->head;
	rowIndex = 0;

	while (registeredComponent != NULL) {
		if (rowIndex >= m_table->rowCount())
			addTableRow(rowIndex);
		m_table->item(rowIndex, 0)->setText(SyntroUtils::displayUID(&(registeredComponent->registeredUID)));
		m_table->item(rowIndex, 1)->setText(QString::number(registeredComponent->port));
		m_table->item(rowIndex, 2)->setText(QString::number(registeredComponent->sendSeq));
		m_table->item(rowIndex, 3)->setText(QString::number(registeredComponent->lastAckSeq));
		registeredComponent = registeredComponent->next;
		rowIndex++;
	}
}

void MulticastDialog::addTableRow(int index)
{
	m_table->insertRow(index);
	m_table->setRowHeight(index, 20);

	for (int col = 0; col < 4; col++) {
		QTableWidgetItem *item = new QTableWidgetItem();
		item->setTextAlignment(Qt::AlignLeft | Qt::AlignBottom);
		item->setFlags(Qt::ItemIsEnabled);
		item->setText("");
		m_table->setItem(index, col, item);
	}
}

void MulticastDialog::clearTable()
{
	while (m_table->rowCount() > 0)
		m_table->removeRow(0);
}

void MulticastDialog::selectionChanged()
{
	clearTable();
	m_currentItem = m_tree->currentItem();
	if (m_currentItem == NULL) {
		m_currentMapIndex = -1;
		return;
	}
	if (m_currentItem->parent() != NULL)
		m_currentItem = m_currentItem->parent();			// was second level item
	if (m_currentItem == NULL) {
		m_currentMapIndex = -1;
		return;
	}
	m_currentMapIndex = m_currentItem->data(0, Qt::UserRole).toInt();
	MMDisplay();
}
