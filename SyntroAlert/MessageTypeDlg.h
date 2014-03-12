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

#ifndef MESSAGETYPEDLG_H
#define MESSAGETYPEDLG_H

#include <qdialog.h>
#include <qstringlist.h>
#include <qcheckbox.h>

class MessageTypeDlg : public QDialog
{
	Q_OBJECT

public:
    MessageTypeDlg(QWidget *parent, QStringList currentTypes);

    QStringList newTypes();

private:
    void layoutWindow(QStringList currentTypes);

    QCheckBox *m_error;
    QCheckBox *m_warn;
    QCheckBox *m_info;
    QCheckBox *m_debug;
	QCheckBox *m_alert;
};

#endif // MESSAGETYPEDLG_H
