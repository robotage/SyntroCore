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

#include "HelloDialog.h"
#include <qevent.h>

#define DEFAULT_ROW_HEIGHT 20

HelloDialog::HelloDialog(QWidget *parent)
	: QDialog(parent)
{
	hide();
	setWindowTitle("Hello table display");

	QVBoxLayout *vertLayout = new QVBoxLayout(this);
	m_table = new QTableWidget();
	vertLayout->addWidget(m_table);
	m_table->setRowCount(SYNTRO_MAX_CONNECTEDCOMPONENTS);
	m_table->setColumnCount(4);
	m_table->setColumnWidth(0, 120);
    m_table->setColumnWidth(1, 120);
    m_table->setColumnWidth(2, 160);
    m_table->setColumnWidth(3, 120);

    m_table->setHorizontalHeaderLabels(
                QStringList() << tr("App type") << tr("Component name")
                << tr("Unique ID") << tr("IP Address"));


    m_table->setSelectionMode(QAbstractItemView::NoSelection);

	for (int row = 0; row < SYNTRO_MAX_CONNECTEDCOMPONENTS; row++) {
		m_table->insertRow(row);
		m_table->setRowHeight(row, DEFAULT_ROW_HEIGHT);

		for (int col = 0; col < 5; col++) {
			QTableWidgetItem *item = new QTableWidgetItem();
			item->setTextAlignment(Qt::AlignLeft | Qt::AlignBottom);

			if (col == 0)
				item->setText("...");
			else
				item->setText("");

			m_table->setItem(row, col, item);
		}
	}
	resize(700, 500);
}

void HelloDialog::closeEvent(QCloseEvent *event)
{
	hide();
	event->ignore();
}

void HelloDialog::helloDisplayEvent(Hello *helloThread)
{
	HELLOENTRY *helloEntry = helloThread->m_helloArray;

	for (int i = 0, nEntry = 0; i < SYNTRO_MAX_CONNECTEDCOMPONENTS; i++, helloEntry++) {
		helloThread->m_lock.lock();
		if (helloEntry->inUse) {
			m_table->item(nEntry, 0)->setText(helloEntry->hello.appName);
			m_table->item(nEntry, 1)->setText(helloEntry->hello.componentType);
			m_table->item(nEntry, 2)->setText(SyntroUtils::displayUID(&helloEntry->hello.componentUID));
			m_table->item(nEntry, 3)->setText(SyntroUtils::displayIPAddr(helloEntry->hello.IPAddr));
			nEntry++;
		}
		else {
			m_table->item(nEntry, 0)->setText("...");
			m_table->item(nEntry, 1)->setText("");
			m_table->item(nEntry, 2)->setText("");
			m_table->item(nEntry, 3)->setText("");
			nEntry++;
		}
		helloThread->m_lock.unlock();
	}
}
