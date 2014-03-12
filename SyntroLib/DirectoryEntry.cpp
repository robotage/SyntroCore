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

#include <qstringlist.h>

#include "SyntroDefs.h"
#include "DirectoryEntry.h"

DirectoryEntry::DirectoryEntry(QString dirLine)
{
	m_raw = dirLine;
	parseLine();
}
	
void DirectoryEntry::setLine(QString dirLine)
{
	m_uid.clear();
	m_name.clear();
	m_type.clear();
	m_multicastServices.clear();
	m_e2eServices.clear();

	m_raw = dirLine;

	parseLine();
}

bool DirectoryEntry::isValid()
{
	if (m_uid.length() > 0 && m_name.length() > 0 && m_type.length() > 0)
		return true;

	return false;
}

QString DirectoryEntry::uid()
{
	return m_uid;
}

QString DirectoryEntry::appName()
{
	return m_name;
}

QString DirectoryEntry::componentType()
{
	return m_type;
}

QStringList DirectoryEntry::multicastServices()
{
	return m_multicastServices;
}

QStringList DirectoryEntry::e2eServices()
{
	return m_e2eServices;
}

void DirectoryEntry::parseLine()
{
	if (m_raw.length() == 0)
		return;

	m_uid = element(DETAG_UID);
	m_name = element(DETAG_APPNAME);
	m_type = element(DETAG_COMPTYPE);
	m_multicastServices = elements(DETAG_MSERVICE);
	m_e2eServices = elements(DETAG_ESERVICE);
}

QString DirectoryEntry::element(QString name)
{
	QString element;

	QString start= QString("<%1>").arg(name);
	QString end = QString("</%1>").arg(name);

	int i = m_raw.indexOf(start);
	int j = m_raw.indexOf(end);

	if (i >= 0 && j > i + start.length())
		element = m_raw.mid(i + start.length(), j - (i + start.length()));

	return element;
}

QStringList DirectoryEntry::elements(QString name)
{
	QStringList elements;
	int pos = 0;

	QString start = QString("<%1>").arg(name);
	QString end = QString("</%1>").arg(name);

	int i = m_raw.indexOf(start, pos);

	while (i >= 0) {
		int j = m_raw.indexOf(end, pos);

		if (j > i + start.length())
			elements << m_raw.mid(i + start.length(), j - (i + start.length()));

		pos = j + end.length();

		i = m_raw.indexOf(start, pos);
	}

	return elements;
}
