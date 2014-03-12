//
//  Copyright (c) 2014 Scott Ellis and Richard Barnett
//	
//  This file is part of SyntroLib
//
//  SyntroLib is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  SyntroLib is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with SyntroLib.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef DIRECTORYENTRY_H
#define DIRECTORYENTRY_H

#include "syntrolib_global.h"
#include <qstring.h>

class SYNTROLIB_EXPORT DirectoryEntry
{
public:
	DirectoryEntry(QString dirLine = "");
	
	void setLine(QString dirLine);

	bool isValid();

	QString uid();
	QString appName();
	QString componentType();
	QStringList multicastServices();
	QStringList e2eServices();

private:
	void parseLine();
	QString element(QString name);
	QStringList elements(QString name);

	QString m_raw;
	QString m_uid;
	QString m_name;
	QString m_type;
	QStringList m_multicastServices;
	QStringList m_e2eServices;
};

#endif // DIRECTORYENTRY_H
