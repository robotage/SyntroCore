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

#ifndef STORESTREAMDLG_H
#define STORESTREAMDLG_H

#include "SyntroValidators.h"

#include <qdialog.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qsettings.h>
#include <qpushbutton.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qboxlayout.h>
#include <qformlayout.h>
#include <qfiledialog.h>

class StoreLabel : public QLabel
{
public:
	StoreLabel(const QString& text);

protected:
	void mousePressEvent (QMouseEvent *ev);

};

class StoreStreamDlg : public QDialog
{
	Q_OBJECT

friend class StoreLabel;

public:
	StoreStreamDlg(QWidget *parent, int index);

public slots:
	void cancelButtonClick();
	void okButtonClick();


private:
	void layoutWidgets();
	void loadCurrentValues();

	int m_index;

	QPushButton *m_okButton;
	QPushButton *m_cancelButton;
	QComboBox *m_formatCombo;
	QLineEdit *m_streamName;
	QCheckBox *m_subFolderCheck;
	QComboBox *m_rotationPolicy;
	QComboBox *m_rotationTimeUnits;
	QLineEdit *m_rotationTime;
	QLineEdit *m_rotationSize;
	QComboBox *m_deletionPolicy;
	QComboBox *m_deletionTimeUnits;
	QLineEdit *m_deletionTime;
	QLineEdit *m_deletionSize;
	QIntValidator *m_readOnlyValidator;
	QIntValidator *m_greaterThenZeroValidator;

	ServicePathValidator m_validator;
};

#endif // STORESTREAMDLG_H
