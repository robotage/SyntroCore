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

#ifndef BASICSETUPDLG_H
#define BASICSETUPDLG_H

#include <QDialog>
#include <qsettings.h>
#include <qlineedit.h>
#include <qdialogbuttonbox.h>
#include <qmessagebox.h>
#include "syntrogui_global.h"
#include "SyntroValidators.h"
#include <qcombobox.h>
#include <qcheckbox.h>

#include "SyntroDefs.h"
#include "Endpoint.h"

class SYNTROGUI_EXPORT BasicSetupDlg : public QDialog
{
	Q_OBJECT

public:
	BasicSetupDlg(QWidget *parent = 0);
	~BasicSetupDlg();

public slots:
	void onOk();

private:
	void layoutWindow();
	void populateAdaptors();

	QLineEdit *m_controlName[ENDPOINT_MAX_SYNTROCONTROLS];
	QLineEdit *m_appName;
	QDialogButtonBox *m_buttons;
	ServiceNameValidator *m_validator;

	QComboBox *m_adaptor;
	QCheckBox *m_revert;
	QCheckBox *m_localControl;
	QLineEdit *m_localControlPri;
    QCheckBox *m_encrypt;
};

#endif // BASICSETUPDLG_H
