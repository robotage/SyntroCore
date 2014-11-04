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

    settings->beginGroup(SYNTROCONTROL_PARAMS_GROUP);

	changed |= m_localSocket->text() != settings->value(SYNTROCONTROL_PARAMS_LISTEN_LOCAL_SOCKET).toString();
	changed |= m_encryptLocalSocket->text() != settings->value(SYNTROCONTROL_PARAMS_LISTEN_LOCAL_SOCKET_ENCRYPT).toString();
	changed |= m_staticTunnelSocket->text() != settings->value(SYNTROCONTROL_PARAMS_LISTEN_STATICTUNNEL_SOCKET).toString();
	changed |= m_encryptStaticTunnelSocket->text() != settings->value(SYNTROCONTROL_PARAMS_LISTEN_STATICTUNNEL_SOCKET_ENCRYPT).toString();

    changed |= (m_encryptLocal->checkState() == Qt::Checked) != settings->value(SYNTROCONTROL_PARAMS_ENCRYPT_LOCAL).toBool();
    changed |= (m_encryptStaticTunnelServer->checkState() == Qt::Checked) != settings->value(SYNTROCONTROL_PARAMS_ENCRYPT_STATICTUNNEL_SERVER).toBool();
    
    changed |= m_heartbeatInterval->text() != settings->value(SYNTROCONTROL_PARAMS_HBINTERVAL).toString();
	changed |= m_heartbeatRetries->text() != settings->value(SYNTROCONTROL_PARAMS_HBTIMEOUT).toString();

    settings->endGroup();

    if (changed) {
		msgBox.setText("The component must be restarted for these changes to take effect");
		msgBox.exec();

		settings->setValue(SYNTRO_PARAMS_APPNAME, m_appName->text());
		settings->setValue(SYNTRO_PARAMS_LOCALCONTROL_PRI, m_priority->text());
		
		if (m_adaptor->currentText() == "<any>")
			settings->setValue(SYNTRO_RUNTIME_ADAPTER, "");
		else
			settings->setValue(SYNTRO_RUNTIME_ADAPTER, m_adaptor->currentText());

        settings->beginGroup(SYNTROCONTROL_PARAMS_GROUP);

	    settings->setValue(SYNTROCONTROL_PARAMS_LISTEN_LOCAL_SOCKET, m_localSocket->text());
	    settings->setValue(SYNTROCONTROL_PARAMS_LISTEN_LOCAL_SOCKET_ENCRYPT, m_encryptLocalSocket->text());
	    settings->setValue(SYNTROCONTROL_PARAMS_LISTEN_STATICTUNNEL_SOCKET, m_staticTunnelSocket->text());
	    settings->setValue(SYNTROCONTROL_PARAMS_LISTEN_STATICTUNNEL_SOCKET_ENCRYPT, m_encryptStaticTunnelSocket->text());

	    settings->setValue(SYNTROCONTROL_PARAMS_ENCRYPT_LOCAL, m_encryptLocal->checkState() == Qt::Checked);
	    settings->setValue(SYNTROCONTROL_PARAMS_ENCRYPT_STATICTUNNEL_SERVER, m_encryptStaticTunnelServer->checkState() == Qt::Checked);

     	settings->setValue(SYNTROCONTROL_PARAMS_HBINTERVAL, m_heartbeatInterval->text());
    	settings->setValue(SYNTROCONTROL_PARAMS_HBTIMEOUT, m_heartbeatRetries->text());

        settings->endGroup();

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
    QFrame *line;

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

   	settings->beginGroup(SYNTROCONTROL_PARAMS_GROUP);

    //  local link config

  	line = new QFrame(this);
	line->setFrameShape(QFrame::HLine);
	line->setFrameShadow(QFrame::Sunken);
    line->setMinimumHeight(20);
    formLayout->addRow(line);
    formLayout->addRow(new QLabel("Local link configuration:"));

    m_localSocket = new QLineEdit(settings->value(SYNTROCONTROL_PARAMS_LISTEN_LOCAL_SOCKET).toString());
	formLayout->addRow(tr("Local link socket (encryption off):" ), m_localSocket);
	m_localSocket->setValidator(new QIntValidator(0, 65535));

    m_encryptLocal = new QCheckBox();
	if (settings->value(SYNTROCONTROL_PARAMS_ENCRYPT_LOCAL).toBool())
	    m_encryptLocal->setCheckState(Qt::Checked);
	formLayout->addRow(tr("Encrypt local links:"), m_encryptLocal);

	m_encryptLocalSocket = new QLineEdit(settings->value(SYNTROCONTROL_PARAMS_LISTEN_LOCAL_SOCKET_ENCRYPT).toString());
	formLayout->addRow(tr("Local link socket (encryption on):" ), m_encryptLocalSocket);
	m_encryptLocalSocket->setValidator(new QIntValidator(0, 65535));

    //  static tunnel server config

  	line = new QFrame(this);
	line->setFrameShape(QFrame::HLine);
	line->setFrameShadow(QFrame::Sunken);
    line->setMinimumHeight(20);
    formLayout->addRow(line);
    formLayout->addRow(new QLabel("Static tunnel server configuration:"));

    m_staticTunnelSocket = new QLineEdit(settings->value(SYNTROCONTROL_PARAMS_LISTEN_STATICTUNNEL_SOCKET).toString());
	formLayout->addRow(tr("Static tunnel service socket (encryption off):" ), m_staticTunnelSocket);
	m_staticTunnelSocket->setValidator(new QIntValidator(0, 65535));

    m_encryptStaticTunnelServer = new QCheckBox();
	if (settings->value(SYNTROCONTROL_PARAMS_ENCRYPT_STATICTUNNEL_SERVER).toBool())
	    m_encryptStaticTunnelServer->setCheckState(Qt::Checked);
	formLayout->addRow(tr("Encrypt static tunnel server:"), m_encryptStaticTunnelServer);

	m_encryptStaticTunnelSocket = new QLineEdit(settings->value(SYNTROCONTROL_PARAMS_LISTEN_STATICTUNNEL_SOCKET_ENCRYPT).toString());
	formLayout->addRow(tr("Static tunnel service socket (encryption on):" ), m_encryptStaticTunnelSocket);
	m_encryptStaticTunnelSocket->setValidator(new QIntValidator(0, 65535));

    //  heartbeat config

  	line = new QFrame(this);
	line->setFrameShape(QFrame::HLine);
	line->setFrameShadow(QFrame::Sunken);
    line->setMinimumHeight(20);
    formLayout->addRow(line);
    formLayout->addRow(new QLabel("Heartbeat configuration:"));

    m_heartbeatInterval = new QLineEdit(settings->value(SYNTROCONTROL_PARAMS_HBINTERVAL).toString());
	formLayout->addRow(tr("Heartbeat interval (1 - 120 seconds):" ), m_heartbeatInterval);
	m_heartbeatInterval->setValidator(new QIntValidator(1, 120));

    m_heartbeatRetries = new QLineEdit(settings->value(SYNTROCONTROL_PARAMS_HBTIMEOUT).toString());
	formLayout->addRow(tr("Heartbeat intervals before timeout (1 - 10):" ), m_heartbeatRetries);
	m_heartbeatRetries->setValidator(new QIntValidator(1, 10));

    settings->endGroup();

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
