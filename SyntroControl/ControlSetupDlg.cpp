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

#include "ControlSetupDlg.h"
#include "SyntroControl.h"

ControlSetupDlg::ControlSetupDlg(QWidget *parent)
	: QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
	layoutWindow();
	setWindowTitle("Basic setup");
	connect(m_buttons, SIGNAL(accepted()), this, SLOT(onOk()));
    connect(m_buttons, SIGNAL(rejected()), this, SLOT(reject()));
}

ControlSetupDlg::~ControlSetupDlg()
{
	delete m_validator;
}

void ControlSetupDlg::onOk()
{
	QMessageBox msgBox;

	bool changed = false;

	QSettings *settings = SyntroUtils::getSettings();

	changed = m_appName->text() != settings->value(SYNTRO_PARAMS_APPNAME).toString();
	changed |= m_priority->text() != settings->value(SYNTRO_PARAMS_LOCALCONTROL_PRI).toString();

	if (m_adaptor->currentText() == "<any>") {
		if (settings->value(SYNTRO_RUNTIME_ADAPTER).toString() != "")
			changed = true;
	} 
	else {
		if (m_adaptor->currentText() != settings->value(SYNTRO_RUNTIME_ADAPTER).toString())
			changed = true;
	}

	if (changed) {
		msgBox.setText("The component must be restarted for these changes to take effect");
		msgBox.exec();

		settings->setValue(SYNTRO_PARAMS_APPNAME, m_appName->text());
		settings->setValue(SYNTRO_PARAMS_LOCALCONTROL_PRI, m_priority->text());
		
		if (m_adaptor->currentText() == "<any>")
			settings->setValue(SYNTRO_RUNTIME_ADAPTER, "");
		else
			settings->setValue(SYNTRO_RUNTIME_ADAPTER, m_adaptor->currentText());
		
		delete settings;
		accept();
	}
	else {
		delete settings;
		reject();
	}
}

void ControlSetupDlg::layoutWindow()
{
	QSettings *settings = SyntroUtils::getSettings();

	QVBoxLayout *centralLayout = new QVBoxLayout(this);
	
	QFormLayout *formLayout = new QFormLayout();

	m_appName = new QLineEdit(settings->value(SYNTRO_PARAMS_APPNAME).toString());
	m_appName->setMinimumWidth(200);
	formLayout->addRow(tr("App name:"), m_appName);

	m_validator = new ServiceNameValidator();
	m_appName->setValidator(m_validator);

	m_priority = new QLineEdit(settings->value(SYNTRO_PARAMS_LOCALCONTROL_PRI).toString());
	formLayout->addRow(tr("Priority (0 (lowest) - 255):"), m_priority);
	m_priority->setValidator(new QIntValidator(0, 255));

	m_adaptor = new QComboBox;
	m_adaptor->setEditable(false);

	populateAdaptors();

	formLayout->addRow(new QLabel("Ethernet adaptor:"), m_adaptor);

	int findIndex = m_adaptor->findText(settings->value(SYNTRO_RUNTIME_ADAPTER).toString());

	if (findIndex != -1)
		m_adaptor->setCurrentIndex(findIndex);
	else
		m_adaptor->setCurrentIndex(0);

	centralLayout->addLayout(formLayout);
	
	centralLayout->addSpacerItem(new QSpacerItem(20, 20));

	m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	m_buttons->setCenterButtons(true);

	centralLayout->addWidget(m_buttons);
}

void ControlSetupDlg::populateAdaptors()
{
	QNetworkInterface cInterface;

	m_adaptor->insertItem(0, "<any>");

	int index = 1;

	QList<QNetworkInterface> ani = QNetworkInterface::allInterfaces();

	foreach (cInterface, ani)
		m_adaptor->insertItem(index++, cInterface.humanReadableName());
}
