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

#include <qdialogbuttonbox.h>
#include <qgroupbox.h>
#include <qboxlayout.h>

#include "SyntroLib.h"
#include "ViewFieldsDlg.h"

ViewFieldsDlg::ViewFieldsDlg(QWidget *parent, QList<int> fields)
    : QDialog(parent,  Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
    layoutWindow(fields);
}

QList<int> ViewFieldsDlg::viewFields()
{
    QList<int> fields;

    for (int i = 0; i < NUM_LOG_FIELDS; i++) {
        if (m_check[i]->isChecked())
            fields.append(i);
    }

    return fields;
}

void ViewFieldsDlg::layoutWindow(QList<int> fields)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QVBoxLayout *checkLayout = new QVBoxLayout();
    m_check[TYPE_FIELD] = new QCheckBox("Type");
    m_check[TIMESTAMP_FIELD] = new QCheckBox("TimeStamp");
    m_check[UID_FIELD] = new QCheckBox("UID");
    m_check[APP_TYPE_FIELD] = new QCheckBox("AppType");
    m_check[SOURCE_FIELD] = new QCheckBox("Source");
    m_check[MESSAGE_FIELD] = new QCheckBox("Message");

    for (int i = 0; i < fields.count(); i++) {
        if (fields.at(i) >= 0 && fields.at(i) < NUM_LOG_FIELDS)
            m_check[fields.at(i)]->setChecked(true);
    }

    m_check[TIMESTAMP_FIELD]->setChecked(true);
    m_check[TIMESTAMP_FIELD]->setEnabled(false);
    m_check[MESSAGE_FIELD]->setChecked(true);
    m_check[MESSAGE_FIELD]->setEnabled(false);

    for (int i = 0; i < NUM_LOG_FIELDS; i++)
        checkLayout->addWidget(m_check[i]);

    QGroupBox *group = new QGroupBox("Alert Fields");
    group->setLayout(checkLayout);
    mainLayout->addWidget(group);

    mainLayout->addStretch(1);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                         | QDialogButtonBox::Cancel);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    mainLayout->addWidget(buttonBox);
}
