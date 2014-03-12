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

#include "ConfigurationDlg.h"
#include "SyntroDB.h"


ConfigurationDlg::ConfigurationDlg(QWidget *parent)
	: QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
	layoutWidgets();
	connect(m_cancelButton, SIGNAL(clicked()), this, SLOT(cancelButtonClick()));
	connect(m_okButton, SIGNAL(clicked()), this, SLOT(okButtonClick()));
	loadCurrentValues();
}


void ConfigurationDlg::okButtonClick()
{
	QSettings *settings = SyntroUtils::getSettings();

	if (settings->value(SYNTRODB_PARAMS_ROOT_DIRECTORY).toString() != m_storePath->text()) {

		settings->setValue(SYNTRODB_PARAMS_ROOT_DIRECTORY, m_storePath->text());
		delete settings;
		accept();
		return;
	}
	delete settings;
	reject();
}

void ConfigurationDlg::cancelButtonClick()
{
	reject();
}

void ConfigurationDlg::loadCurrentValues()
{
	QSettings *settings = SyntroUtils::getSettings();
	m_storePath->setText(settings->value(SYNTRODB_PARAMS_ROOT_DIRECTORY).toString());
	delete settings;
}

void ConfigurationDlg::layoutWidgets()
{
	QFormLayout *formLayout = new QFormLayout;

	m_storePath = new CFSLabel("");
	m_storePath->setMinimumWidth(320);
	formLayout->addRow(new QLabel("Root directory:"), m_storePath);

	QHBoxLayout *buttonLayout = new QHBoxLayout;

	m_okButton = new QPushButton("Ok");
	m_cancelButton = new QPushButton("Cancel");

	buttonLayout->addStretch();
	buttonLayout->addWidget(m_okButton);
	buttonLayout->addWidget(m_cancelButton);
	buttonLayout->addStretch();

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addLayout(formLayout);
	mainLayout->addSpacing(20);
	mainLayout->addLayout(buttonLayout);
	setLayout(mainLayout);

	setWindowTitle("Configure SyntroDB Root Directory");
}


//----------------------------------------------------------
//
//	CFSLabel functions

CFSLabel::CFSLabel(const QString& text) : QLabel(text)
{
	setFrameStyle(QFrame::Sunken | QFrame::Panel);
}

void CFSLabel::mousePressEvent (QMouseEvent *)
{
	QFileDialog *fileDialog;

	fileDialog = new QFileDialog(this, "Root directory");
	fileDialog->setFileMode(QFileDialog::DirectoryOnly);
	fileDialog->selectFile(text());
	if (fileDialog->exec())
		setText(fileDialog->selectedFiles().at(0));
}
