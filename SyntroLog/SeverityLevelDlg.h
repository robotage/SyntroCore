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

#ifndef SEVERITYLEVELDLG_H
#define SEVERITYLEVELDLG_H

#include <qdialog.h>
#include <qradiobutton.h>

class SeverityLevelDlg : public QDialog
{
	Q_OBJECT

public:
    SeverityLevelDlg(QWidget *parent, int level);

    int level();

private:
    void layoutWindow(int level);

    QRadioButton *m_error;
    QRadioButton *m_warn;
    QRadioButton *m_info;
    QRadioButton *m_debug;
};

#endif // SEVERITYLEVELDLG_H
