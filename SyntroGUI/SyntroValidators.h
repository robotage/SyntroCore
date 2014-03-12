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

#ifndef SYNTROVALIDATORS_H
#define SYNTROVALIDATORS_H

#include <qvalidator.h>
#include "syntrogui_global.h"
#include "SyntroDefs.h"


class SYNTROGUI_EXPORT ServiceNameValidator : public QValidator
{
	Q_OBJECT;

	QValidator::State validate(QString &input, int &pos) const;
};

class SYNTROGUI_EXPORT ServicePathValidator : public QValidator
{
	Q_OBJECT;

	QValidator::State validate(QString &input, int &pos) const;
};


#endif // SYNTROVALIDATORS_H
