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
#include "SeverityLevelDlg.h"

SeverityLevelDlg::SeverityLevelDlg(QWidget *parent, int level)
    : QDialog(parent,  Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
    layoutWindow(level);
}

int SeverityLevelDlg::level()
{
    if (m_error->isChecked())
        return SYNTRO_LOG_LEVEL_ERROR;
    else if (m_warn->isChecked())
        return SYNTRO_LOG_LEVEL_WARN;
    else if (m_info->isChecked())
        return SYNTRO_LOG_LEVEL_INFO;
    else
        return SYNTRO_LOG_LEVEL_DEBUG;
}

void SeverityLevelDlg::layoutWindow(int level)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QVBoxLayout *checkLayout = new QVBoxLayout();
    m_error = new QRadioButton("Error");
    checkLayout->addWidget(m_error);
    m_warn = new QRadioButton("Warn");
    checkLayout->addWidget(m_warn);
    m_info = new QRadioButton("Info");
    checkLayout->addWidget(m_info);
    m_debug = new QRadioButton("Debug");
    checkLayout->addWidget(m_debug);

    switch (level) {
    case SYNTRO_LOG_LEVEL_ERROR:
        m_error->setChecked(true);
        break;
    case SYNTRO_LOG_LEVEL_WARN:
        m_warn->setChecked(true);
        break;
    case SYNTRO_LOG_LEVEL_DEBUG:
        m_debug->setChecked(true);
        break;
    case SYNTRO_LOG_LEVEL_INFO:
    default:
        m_info->setChecked(true);
        break;
    }

    QGroupBox *group = new QGroupBox("Severity Level");
    group->setLayout(checkLayout);
    mainLayout->addWidget(group);

    mainLayout->addStretch(1);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                         | QDialogButtonBox::Cancel);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    mainLayout->addWidget(buttonBox);
}
