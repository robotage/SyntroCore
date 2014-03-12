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

#ifndef CONTROLSETUPDLG_H
#define CONTROLSETUPDLG_H

#include "SyntroLib.h"
#include "SyntroValidators.h"
#include <qdialog.h>
#include <qlineedit.h>
#include <qdialogbuttonbox.h>
#include <qmessagebox.h>
#include <qformlayout.h>
#include <qcombobox.h>

class ControlSetupDlg : public QDialog
{
	Q_OBJECT

public:
	ControlSetupDlg(QWidget *parent);
	~ControlSetupDlg();

public slots:
	void onOk();

private:
	void layoutWindow();
	void populateAdaptors();

	QLineEdit *m_channels;
	QLineEdit *m_appName;
	QDialogButtonBox *m_buttons;
	ServiceNameValidator *m_validator;
	QLineEdit *m_priority;
	QComboBox *m_adaptor;
};

#endif // CONSTROLSETUPDLG_H
