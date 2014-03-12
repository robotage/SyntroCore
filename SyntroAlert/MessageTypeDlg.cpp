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
#include <qboxlayout.h>
#include <qgroupbox.h>

#include "SyntroLib.h"
#include "MessageTypeDlg.h"

MessageTypeDlg::MessageTypeDlg(QWidget *parent, QStringList currentTypes)
    : QDialog(parent,  Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
    layoutWindow(currentTypes);
}

QStringList MessageTypeDlg::newTypes()
{
	QStringList types;

	if (m_error->isChecked())
		types.append(SYNTRO_LOG_ERROR);

	if (m_warn->isChecked())
		types.append(SYNTRO_LOG_WARN);
    
	if (m_info->isChecked())
		types.append(SYNTRO_LOG_INFO);

	if (m_debug->isChecked())
		types.append(SYNTRO_LOG_DEBUG);

	if (m_alert->isChecked())
		types.append("alert");

	return types;
}

void MessageTypeDlg::layoutWindow(QStringList currentTypes)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QVBoxLayout *checkLayout = new QVBoxLayout();

    m_error = new QCheckBox(SYNTRO_LOG_ERROR);

	if (currentTypes.contains(SYNTRO_LOG_ERROR))
		m_error->setChecked(true);

    checkLayout->addWidget(m_error);

    m_warn = new QCheckBox(SYNTRO_LOG_WARN);

	if (currentTypes.contains(SYNTRO_LOG_WARN))
		m_warn->setChecked(true);

    checkLayout->addWidget(m_warn);

    m_info = new QCheckBox(SYNTRO_LOG_INFO);

	if (currentTypes.contains(SYNTRO_LOG_INFO))
		m_info->setChecked(true);

    checkLayout->addWidget(m_info);

    m_debug = new QCheckBox(SYNTRO_LOG_DEBUG);

	if (currentTypes.contains(SYNTRO_LOG_DEBUG))
		m_debug->setChecked(true);

    checkLayout->addWidget(m_debug);

    m_alert = new QCheckBox("alert");

	if (currentTypes.contains("alert"))
		m_alert->setChecked(true);

    checkLayout->addWidget(m_alert);

    QGroupBox *group = new QGroupBox("Alert Types");
    group->setLayout(checkLayout);
    mainLayout->addWidget(group);

    mainLayout->addStretch(1);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                         | QDialogButtonBox::Cancel);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    mainLayout->addWidget(buttonBox);
}
