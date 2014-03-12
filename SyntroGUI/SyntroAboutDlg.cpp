//
//  Copyright (c) 2014 Scott Ellis and Richard Barnett
//	
//  This file is part of SyntroGUI
//
//  SyntroGUI is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  SyntroGUI is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with SyntroGUI.  If not, see <http://www.gnu.org/licenses/>.
//

#include <qboxlayout.h>
#include <qformlayout.h>

#include "SyntroAboutDlg.h"
#include "SyntroUtils.h"

SyntroAbout::SyntroAbout(QWidget *parent)
	: QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
	layoutWindow();

	setWindowTitle("About " + SyntroUtils::getAppType());
	m_appType->setText(SyntroUtils::getAppType());
	m_appName->setText(SyntroUtils::getAppName());
	m_buildDate->setText(QString("%1 %2").arg(__DATE__).arg(__TIME__));
	m_syntroLibVersion->setText(SyntroUtils::syntroLibVersion());
	m_qtRuntime->setText(qVersion());
	
	connect(m_actionOk, SIGNAL(clicked()), this, SLOT(close()));
}

void SyntroAbout::layoutWindow()
{
	resize(300, 240);
    setModal(true);

	QVBoxLayout *centralLayout = new QVBoxLayout(this);
	centralLayout->setSpacing(6);
	centralLayout->setContentsMargins(11, 11, 11, 11);
	
	QVBoxLayout *verticalLayout = new QVBoxLayout();
	verticalLayout->setSpacing(6);
	
	m_appType = new QLabel(this);
	QFont font;
	font.setPointSize(12);
	m_appType->setFont(font);
	m_appType->setAlignment(Qt::AlignCenter);

	verticalLayout->addWidget(m_appType);

	QLabel *label_1 = new QLabel("A SyntroNet Application", this);
	QFont font1;
	font1.setPointSize(10);
	label_1->setFont(font1);
	label_1->setAlignment(Qt::AlignCenter);

	verticalLayout->addWidget(label_1);

	QLabel *label_2 = new QLabel("Copyright (c) 2014 Scott Ellis and Richard Barnett", this);
	label_2->setFont(font1);
	label_2->setAlignment(Qt::AlignCenter);

	verticalLayout->addWidget(label_2);

	QFrame *line = new QFrame(this);
	line->setFrameShape(QFrame::HLine);
	line->setFrameShadow(QFrame::Sunken);

	verticalLayout->addWidget(line);

	centralLayout->addLayout(verticalLayout);

	// vertical spacer
	QSpacerItem *verticalSpacer_2 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Expanding);
	centralLayout->addItem(verticalSpacer_2);


	QFormLayout *formLayout = new QFormLayout();
	formLayout->setSpacing(6);
	formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	
	QLabel *label_3 = new QLabel("Name:", this);
	formLayout->setWidget(0, QFormLayout::LabelRole, label_3);
	m_appName = new QLabel(this);
	formLayout->setWidget(0, QFormLayout::FieldRole, m_appName);

	QLabel *label_5 = new QLabel("Build Date:", this);
	formLayout->setWidget(2, QFormLayout::LabelRole, label_5);
	m_buildDate = new QLabel(this);
	formLayout->setWidget(2, QFormLayout::FieldRole, m_buildDate);

	QLabel *label_6 = new QLabel("SyntroLib Version:", this);
	formLayout->setWidget(3, QFormLayout::LabelRole, label_6);
	m_syntroLibVersion = new QLabel(this);
	formLayout->setWidget(3, QFormLayout::FieldRole, m_syntroLibVersion);

	QLabel *label_7 = new QLabel("Qt Version:", this);
	formLayout->setWidget(4, QFormLayout::LabelRole, label_7);
	m_qtRuntime = new QLabel(this);
	formLayout->setWidget(4, QFormLayout::FieldRole, m_qtRuntime);

	centralLayout->addLayout(formLayout);


	// vertical spacer
	QSpacerItem *verticalSpacer = new QSpacerItem(20, 8, QSizePolicy::Minimum, QSizePolicy::Expanding);
	centralLayout->addItem(verticalSpacer);


	// OK button, centered at bottom by two spacers
	QHBoxLayout *horizontalLayout = new QHBoxLayout();
	horizontalLayout->setSpacing(6);

	QSpacerItem *horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
	horizontalLayout->addItem(horizontalSpacer);

	m_actionOk = new QPushButton("Ok", this);
	QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	sizePolicy.setHorizontalStretch(0);
	sizePolicy.setVerticalStretch(0);
	sizePolicy.setHeightForWidth(m_actionOk->sizePolicy().hasHeightForWidth());
	m_actionOk->setSizePolicy(sizePolicy);
	horizontalLayout->addWidget(m_actionOk);

	QSpacerItem *horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
	horizontalLayout->addItem(horizontalSpacer_2);

	centralLayout->addLayout(horizontalLayout);
}