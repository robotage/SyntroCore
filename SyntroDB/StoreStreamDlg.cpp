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

#include "StoreStreamDlg.h"
#include "StoreStream.h"
#include "SyntroDB.h"

StoreStreamDlg::StoreStreamDlg(QWidget *parent, int index)
	: QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
{
	m_index = index;
	layoutWidgets();
	connect(m_cancelButton, SIGNAL(clicked()), this, SLOT(cancelButtonClick()));
	connect(m_okButton, SIGNAL(clicked()), this, SLOT(okButtonClick()));
	loadCurrentValues();
}

void StoreStreamDlg::okButtonClick()
{
	QSettings *settings = SyntroUtils::getSettings();

	settings->beginWriteArray(SYNTRODB_PARAMS_STREAM_SOURCES);
	settings->setArrayIndex(m_index);

	if (m_formatCombo->currentIndex() == 1)
		settings->setValue(SYNTRODB_PARAMS_FORMAT, SYNTRO_RECORD_STORE_FORMAT_RAW);
	else //if (m_formatCombo->currentIndex() == 0)
		settings->setValue(SYNTRODB_PARAMS_FORMAT, SYNTRO_RECORD_STORE_FORMAT_SRF);

	settings->setValue(SYNTRODB_PARAMS_CREATE_SUBFOLDER, 
		m_subFolderCheck->checkState() == Qt::Checked ? SYNTRO_PARAMS_TRUE : SYNTRO_PARAMS_FALSE);

	settings->setValue(SYNTRODB_PARAMS_STREAM_SOURCE, m_streamName->text());

	//	Rotation policy

	switch (m_rotationPolicy->currentIndex()) {
		case 0:
			settings->setValue(SYNTRODB_PARAMS_ROTATION_POLICY, SYNTRODB_PARAMS_ROTATION_POLICY_TIME);
			break;

		case 1:
			settings->setValue(SYNTRODB_PARAMS_ROTATION_POLICY, SYNTRODB_PARAMS_ROTATION_POLICY_SIZE);
			break;

		default:
			settings->setValue(SYNTRODB_PARAMS_ROTATION_POLICY, SYNTRODB_PARAMS_ROTATION_POLICY_ANY);
			break;
	}

	switch (m_rotationTimeUnits->currentIndex()) {
	case 1:
		settings->setValue(SYNTRODB_PARAMS_ROTATION_TIME_UNITS, SYNTRODB_PARAMS_ROTATION_TIME_UNITS_HOURS);
		break;

	case 2:
		settings->setValue(SYNTRODB_PARAMS_ROTATION_TIME_UNITS, SYNTRODB_PARAMS_ROTATION_TIME_UNITS_DAYS);
		break;

	default:
		settings->setValue(SYNTRODB_PARAMS_ROTATION_TIME_UNITS, SYNTRODB_PARAMS_ROTATION_TIME_UNITS_MINUTES);
		break;
	}
	
	settings->setValue(SYNTRODB_PARAMS_ROTATION_TIME, m_rotationTime->text());
	settings->setValue(SYNTRODB_PARAMS_ROTATION_SIZE, m_rotationSize->text());

	//	Deletion policy

	switch (m_deletionPolicy->currentIndex()) {
		case 0:
			settings->setValue(SYNTRODB_PARAMS_DELETION_POLICY, SYNTRODB_PARAMS_DELETION_POLICY_TIME);
			break;

		case 1:
			settings->setValue(SYNTRODB_PARAMS_DELETION_POLICY, SYNTRODB_PARAMS_DELETION_POLICY_COUNT);
			break;

		default:
			settings->setValue(SYNTRODB_PARAMS_DELETION_POLICY, SYNTRODB_PARAMS_DELETION_POLICY_ANY);
			break;
	}

	if (m_deletionTimeUnits->currentIndex() == 1)
		settings->setValue(SYNTRODB_PARAMS_DELETION_TIME_UNITS, SYNTRODB_PARAMS_DELETION_TIME_UNITS_DAYS);
	else	
		settings->setValue(SYNTRODB_PARAMS_DELETION_TIME_UNITS, SYNTRODB_PARAMS_DELETION_TIME_UNITS_HOURS);
			
	settings->setValue(SYNTRODB_PARAMS_DELETION_TIME, m_deletionTime->text());
	settings->setValue(SYNTRODB_PARAMS_DELETION_COUNT, m_deletionSize->text());


	settings->endArray();

	delete settings;

	accept();
}

void StoreStreamDlg::cancelButtonClick()
{
	reject();
}

void StoreStreamDlg::loadCurrentValues()
{
	QSettings *settings = SyntroUtils::getSettings();

	settings->beginReadArray(SYNTRODB_PARAMS_STREAM_SOURCES);
	settings->setArrayIndex(m_index);

	m_streamName->setText(settings->value(SYNTRODB_PARAMS_STREAM_SOURCE).toString());

	if (settings->value(SYNTRODB_PARAMS_FORMAT) == SYNTRO_RECORD_STORE_FORMAT_RAW)
		m_formatCombo->setCurrentIndex(1);
	else // if (settings->value(SYNTRODB_PARAMS_FORMAT) == SYNTRO_RECORD_STORE_FORMAT_SRF)
		m_formatCombo->setCurrentIndex(0);

	if (settings->value(SYNTRODB_PARAMS_CREATE_SUBFOLDER) == SYNTRO_PARAMS_TRUE)
		m_subFolderCheck->setCheckState(Qt::Checked);
	else
		m_subFolderCheck->setCheckState(Qt::Unchecked);

	//	Rotation policy

	QString rotate = settings->value(SYNTRODB_PARAMS_ROTATION_POLICY).toString();
	if (rotate == SYNTRODB_PARAMS_ROTATION_POLICY_TIME)
		m_rotationPolicy->setCurrentIndex(0);
	else if (rotate == SYNTRODB_PARAMS_ROTATION_POLICY_SIZE)
		m_rotationPolicy->setCurrentIndex(1);
	else
		m_rotationPolicy->setCurrentIndex(2);

	m_rotationTimeUnits->setCurrentIndex(0);
	if (settings->value(SYNTRODB_PARAMS_ROTATION_TIME_UNITS).toString() == SYNTRODB_PARAMS_ROTATION_TIME_UNITS_HOURS)
		m_rotationTimeUnits->setCurrentIndex(1);
	if (settings->value(SYNTRODB_PARAMS_ROTATION_TIME_UNITS).toString() == SYNTRODB_PARAMS_ROTATION_TIME_UNITS_DAYS)
		m_rotationTimeUnits->setCurrentIndex(2);

	m_rotationTime->setText(settings->value(SYNTRODB_PARAMS_ROTATION_TIME).toString());
	m_rotationSize->setText(settings->value(SYNTRODB_PARAMS_ROTATION_SIZE).toString());

	//	Deletion policy

	QString deletion = settings->value(SYNTRODB_PARAMS_DELETION_POLICY).toString();
	if (deletion == SYNTRODB_PARAMS_DELETION_POLICY_TIME)
		m_deletionPolicy->setCurrentIndex(0);
	else if (deletion == SYNTRODB_PARAMS_DELETION_POLICY_COUNT)
		m_deletionPolicy->setCurrentIndex(1);
	else
		m_deletionPolicy->setCurrentIndex(2);

	if (settings->value(SYNTRODB_PARAMS_DELETION_TIME_UNITS).toString() == SYNTRODB_PARAMS_DELETION_TIME_UNITS_HOURS)
		m_deletionTimeUnits->setCurrentIndex(1);
	else
		m_deletionTimeUnits->setCurrentIndex(0);

	m_deletionTime->setText(settings->value(SYNTRODB_PARAMS_DELETION_TIME).toString());
	m_deletionSize->setText(settings->value(SYNTRODB_PARAMS_DELETION_COUNT).toString());

	settings->endArray();

	delete settings;
}

void StoreStreamDlg::layoutWidgets()
{
	QFormLayout *formLayout = new QFormLayout;

	QHBoxLayout *e = new QHBoxLayout;
	e->addStretch();
	formLayout->addRow(new QLabel("Store stream"), e);

	m_formatCombo = new QComboBox;
	m_formatCombo->addItem("Structured file [srf])");
	m_formatCombo->addItem("Raw [raw]");
	m_formatCombo->setEditable(false);
	QHBoxLayout *a = new QHBoxLayout;
	a->addWidget(m_formatCombo);
	a->addStretch();
	formLayout->addRow(new QLabel("Store format"), a);

	m_streamName = new QLineEdit;
	m_streamName->setMinimumWidth(200);
	m_streamName->setValidator(&m_validator);
	formLayout->addRow(new QLabel("Stream path"), m_streamName);

	m_subFolderCheck = new QCheckBox;
	formLayout->addRow(new QLabel("Create subfolder"), m_subFolderCheck);

	//	Rotation policy

	m_rotationPolicy = new QComboBox;
	m_rotationPolicy->addItem(SYNTRODB_PARAMS_ROTATION_POLICY_TIME);
	m_rotationPolicy->addItem(SYNTRODB_PARAMS_ROTATION_POLICY_SIZE);
	m_rotationPolicy->addItem(SYNTRODB_PARAMS_ROTATION_POLICY_ANY);
	m_rotationPolicy->setEditable(false);
	QHBoxLayout *b = new QHBoxLayout;
	b->addWidget(m_rotationPolicy);
	b->addStretch();
	formLayout->addRow(new QLabel("Rotation policy"), b);

	m_rotationTimeUnits = new QComboBox;
	m_rotationTimeUnits->addItem(SYNTRODB_PARAMS_ROTATION_TIME_UNITS_MINUTES);
	m_rotationTimeUnits->addItem(SYNTRODB_PARAMS_ROTATION_TIME_UNITS_HOURS);
	m_rotationTimeUnits->addItem(SYNTRODB_PARAMS_ROTATION_TIME_UNITS_DAYS);
	m_rotationTimeUnits->setEditable(false);
	QHBoxLayout *c = new QHBoxLayout;
	c->addWidget(m_rotationTimeUnits);
	c->addStretch();
	formLayout->addRow(new QLabel("Rotation time units"), c);
		
	m_rotationTime = new QLineEdit;
	m_rotationTime->setMaximumWidth(100);
	formLayout->addRow(new QLabel("Rotation time"), m_rotationTime);

	m_rotationSize = new QLineEdit;
	m_rotationSize->setMaximumWidth(100);
	formLayout->addRow(new QLabel("Rotation size (MB)"), m_rotationSize);

	//	Deletion policy

	m_deletionPolicy = new QComboBox;
	m_deletionPolicy->addItem(SYNTRODB_PARAMS_DELETION_POLICY_TIME);
	m_deletionPolicy->addItem(SYNTRODB_PARAMS_DELETION_POLICY_COUNT);
	m_deletionPolicy->addItem(SYNTRODB_PARAMS_DELETION_POLICY_ANY);
	m_deletionPolicy->setEditable(false);
	QHBoxLayout *d = new QHBoxLayout;
	d->addWidget(m_deletionPolicy);
	d->addStretch();
	formLayout->addRow(new QLabel("Deletion policy"), d);

	m_deletionTimeUnits = new QComboBox;
	m_deletionTimeUnits->addItem(SYNTRODB_PARAMS_DELETION_TIME_UNITS_HOURS);
	m_deletionTimeUnits->addItem(SYNTRODB_PARAMS_DELETION_TIME_UNITS_DAYS);
	m_deletionTimeUnits->setEditable(false);
	QHBoxLayout *f = new QHBoxLayout;
	f->addWidget(m_deletionTimeUnits);
	f->addStretch();
	formLayout->addRow(new QLabel("Max age time units"), f);
		
	m_deletionTime = new QLineEdit;
	m_deletionTime->setMaximumWidth(100);
	formLayout->addRow(new QLabel("Max age of file"), m_deletionTime);

	m_deletionSize = new QLineEdit;
	m_deletionSize->setMaximumWidth(100);
	formLayout->addRow(new QLabel("Max files to keep"), m_deletionSize);


	QHBoxLayout *buttonLayout = new QHBoxLayout;

	m_okButton = new QPushButton("Ok");
	m_cancelButton = new QPushButton("Cancel");

	buttonLayout->addStretch();
	buttonLayout->addWidget(m_okButton);
	buttonLayout->addWidget(m_cancelButton);
	buttonLayout->addStretch();

	QVBoxLayout *mainLayout = new QVBoxLayout(this);
	mainLayout->addLayout(formLayout);
	mainLayout->addSpacing(20);
	mainLayout->addLayout(buttonLayout);
	setLayout(mainLayout);

	setWindowTitle("Configure entry");
}

//----------------------------------------------------------
//
//	StoreLabel functions

StoreLabel::StoreLabel(const QString& text) : QLabel(text)
{
	setFrameStyle(QFrame::Sunken | QFrame::Panel);
}

void StoreLabel::mousePressEvent (QMouseEvent *)
{
	QFileDialog *fileDialog;

	fileDialog = new QFileDialog(this, "Store folder path");
	fileDialog->setFileMode(QFileDialog::DirectoryOnly);
	fileDialog->selectFile(text());
	if (fileDialog->exec())
		setText(fileDialog->selectedFiles().at(0));
}
