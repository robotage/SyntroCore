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

#ifndef VIEWFIELDSDLG_H
#define VIEWFIELDSDLG_H

#include <qobject.h>
#include <qdialog.h>
#include <qcheckbox.h>
#include <qlist.h>

#define TYPE_FIELD  0
#define TIMESTAMP_FIELD 1
#define UID_FIELD       2
#define APP_TYPE_FIELD  3
#define SOURCE_FIELD	4
#define MESSAGE_FIELD   5
#define NUM_LOG_FIELDS  6


class ViewFieldsDlg : public QDialog
{
	Q_OBJECT

public:
    ViewFieldsDlg(QWidget *parent, QList<int> fields);

    QList<int> viewFields();

private:
    void layoutWindow(QList<int> fields);

    QCheckBox *m_check[NUM_LOG_FIELDS];
};

#endif // VIEWFIELDSDLG_H
