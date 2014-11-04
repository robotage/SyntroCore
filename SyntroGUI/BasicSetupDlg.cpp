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

#include "BasicSetupDlg.h"
#include "SyntroUtils.h"
#include <qboxlayout.h>
#include <qformlayout.h>

BasicSetupDlg::BasicSetupDlg(QWidget *parent)
	: QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
	layoutWindow();
	setWindowTitle("Basic setup");
	connect(m_buttons, SIGNAL(accepted()), this, SLOT(onOk()));
    connect(m_buttons, SIGNAL(rejected()), this, SLOT(reject()));
}

BasicSetupDlg::~BasicSetupDlg()
{
	delete m_validator;
}

void BasicSetupDlg::onOk()
{
	QMessageBox msgBox;

	QSettings *settings = SyntroUtils::getSettings();

	// check to see if any setting has changed

	if (m_appName->text() != settings->value(SYNTRO_PARAMS_APPNAME).toString())
		goto goChanged;

	settings->beginReadArray(SYNTRO_PARAMS_CONTROL_NAMES);
	for (int control = 0; control < ENDPOINT_MAX_SYNTROCONTROLS; control++) {
		settings->setArrayIndex(control);
		if (m_controlName[control]->text() != settings->value(SYNTRO_PARAMS_CONTROL_NAME).toString()) {
			settings->endArray();
			goto goChanged;
		}
	}
	settings->endArray();

	if ((m_revert->checkState() == Qt::Checked) != settings->value(SYNTRO_PARAMS_CONTROLREVERT).toBool())
		goto goChanged;

	if ((m_localControl->checkState() == Qt::Checked) != settings->value(SYNTRO_PARAMS_LOCALCONTROL).toBool())
		goto goChanged;

	if (m_localControlPri->text() !=  settings->value(SYNTRO_PARAMS_LOCALCONTROL_PRI).toString())
		goto goChanged;

	if (m_adaptor->currentText() == "<any>") {
		if (settings->value(SYNTRO_RUNTIME_ADAPTER).toString() != "")
			goto goChanged;
	} else {
		if (m_adaptor->currentText() != settings->value(SYNTRO_RUNTIME_ADAPTER).toString())
			goto goChanged;
	}

    if ((m_encrypt->checkState() == Qt::Checked) != settings->value(SYNTRO_PARAMS_ENCRYPT_LINK).toBool())
		goto goChanged;

	
	delete settings;

	reject();
	return;

goChanged:
	// save changes to settings

	settings->setValue(SYNTRO_PARAMS_APPNAME, m_appName->text());

	settings->beginWriteArray(SYNTRO_PARAMS_CONTROL_NAMES);
	for (int control = 0; control < ENDPOINT_MAX_SYNTROCONTROLS; control++) {
		settings->setArrayIndex(control);
		settings->setValue(SYNTRO_PARAMS_CONTROL_NAME, m_controlName[control]->text());
	}
	settings->endArray();

	settings->setValue(SYNTRO_PARAMS_CONTROLREVERT, m_revert->checkState() == Qt::Checked);
	settings->setValue(SYNTRO_PARAMS_LOCALCONTROL, m_localControl->checkState() == Qt::Checked);
	settings->setValue(SYNTRO_PARAMS_LOCALCONTROL_PRI, m_localControlPri->text());
	settings->setValue(SYNTRO_PARAMS_ENCRYPT_LINK, m_encrypt->checkState() == Qt::Checked);

	if (m_adaptor->currentText() == "<any>")
		settings->setValue(SYNTRO_RUNTIME_ADAPTER, "");
	else
		settings->setValue(SYNTRO_RUNTIME_ADAPTER, m_adaptor->currentText());

    settings->sync();

	msgBox.setText("The component must be restarted for these changes to take effect");
	msgBox.setIcon(QMessageBox::Warning);
    msgBox.exec();

	delete settings;

	accept();
}

void BasicSetupDlg::layoutWindow()
{
	QSettings *settings = SyntroUtils::getSettings();

    setModal(true);

	QVBoxLayout *centralLayout = new QVBoxLayout(this);
	centralLayout->setSpacing(20);
	centralLayout->setContentsMargins(11, 11, 11, 11);
	
	QFormLayout *formLayout = new QFormLayout();
	formLayout->setSpacing(16);
	formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

	m_appName = new QLineEdit(this);
	m_appName->setToolTip("The name of this instance of the app.");
	m_appName->setMinimumWidth(200);
	formLayout->addRow(tr("App name:"), m_appName);
	m_validator = new ServiceNameValidator();
	m_appName->setValidator(m_validator);
	m_appName->setText(settings->value(SYNTRO_PARAMS_APPNAME).toString());

	settings->beginReadArray(SYNTRO_PARAMS_CONTROL_NAMES);
	for (int control = 0; control < ENDPOINT_MAX_SYNTROCONTROLS; control++) {
		settings->setArrayIndex(control);
		m_controlName[control] = new QLineEdit(this);
		m_controlName[control]->setToolTip("The component name of the SyntroControl to which the app should connect.\n If empty, use any available SyntroControl.");
		m_controlName[control]->setMinimumWidth(200);
		formLayout->addRow(tr("SyntroControl (priority %1):").arg(ENDPOINT_MAX_SYNTROCONTROLS - control), m_controlName[control]);
		m_controlName[control]->setText(settings->value(SYNTRO_PARAMS_CONTROL_NAME).toString());
		m_controlName[control]->setValidator(m_validator);
	}
	settings->endArray();

	m_revert = new QCheckBox();
	formLayout->addRow(tr("Revert to best SyntroControl:"), m_revert);
	if (settings->value(SYNTRO_PARAMS_CONTROLREVERT).toBool())
		m_revert->setCheckState(Qt::Checked);

	m_localControl = new QCheckBox();
	formLayout->addRow(tr("Enable internal SyntroControl:"), m_localControl);
	if (settings->value(SYNTRO_PARAMS_LOCALCONTROL).toBool())
		m_localControl->setCheckState(Qt::Checked);

	m_localControlPri = new QLineEdit();
	formLayout->addRow(tr("Internal SyntroControl priority (0 (lowest) - 255):"), m_localControlPri);
	m_localControlPri->setText(settings->value(SYNTRO_PARAMS_LOCALCONTROL_PRI).toString());
	m_localControlPri->setValidator(new QIntValidator(0, 255));

	m_adaptor = new QComboBox;
	m_adaptor->setEditable(false);
	populateAdaptors();
	QHBoxLayout *a = new QHBoxLayout;
	a->addWidget(m_adaptor);
	a->addStretch();
	formLayout->addRow(new QLabel("Ethernet adaptor:"), a);
	int findIndex = m_adaptor->findText(settings->value(SYNTRO_RUNTIME_ADAPTER).toString());
	if (findIndex != -1)
		m_adaptor->setCurrentIndex(findIndex);
	else
		m_adaptor->setCurrentIndex(0);

   	m_encrypt = new QCheckBox();
	formLayout->addRow(tr("Encrypt link:"), m_encrypt);
	if (settings->value(SYNTRO_PARAMS_ENCRYPT_LINK).toBool())
		m_encrypt->setCheckState(Qt::Checked);

	centralLayout->addLayout(formLayout);

	m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	m_buttons->setCenterButtons(true);

	centralLayout->addWidget(m_buttons);
}

void BasicSetupDlg::populateAdaptors()
{
	QNetworkInterface		cInterface;
	int index;

	m_adaptor->insertItem(0, "<any>");
	index = 1;
	QList<QNetworkInterface> ani = QNetworkInterface::allInterfaces();
	foreach (cInterface, ani)
		m_adaptor->insertItem(index++, cInterface.humanReadableName());
}
