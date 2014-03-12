#
#  Copyright (c) 2014 Scott Ellis and Richard Barnett
#	
#  This file is part of SyntroNet
#
#  SyntroNet is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  SyntroNet is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with SyntroNet.  If not, see <http://www.gnu.org/licenses/>.
#

TEMPLATE = app

TARGET = SyntroAlert

win32* {
    DESTDIR = Release
}
else {
    DESTDIR = Output
}

QT += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += release

# No debug in release builds 
unix:QMAKE_CXXFLAGS_RELEASE -= -g

unix {
	macx {
		target.path = /usr/local/bin
	}
	else {
		target.path = /usr/bin
	}

	INSTALLS += target

	INCLUDEPATH += ../SyntroLib \
			../SyntroGUI \
			../SyntroControlLib

	QMAKE_LIBDIR += ../SyntroLib \
			../SyntroGUI \
			../SyntroControlLib

	LIBS += -lSyntroLib -lSyntroGUI -lSyntroControlLib
}

DEFINES += QT_NETWORK_LIB

INCLUDEPATH += GeneratedFiles \
    GeneratedFiles/Release

win32-g++:LIBS += -L"$(SYNTRODIR)/bin"

win32-msvc*:LIBS += -L"$(SYNTRODIR)/lib"

win32 {
        DEFINES += _CRT_SECURE_NO_WARNINGS
        INCLUDEPATH += $(SYNTRODIR)/include
        LIBS += -lSyntroLib -lSyntroGUI
}

MOC_DIR += GeneratedFiles/release

OBJECTS_DIR += release

UI_DIR += GeneratedFiles

RCC_DIR += GeneratedFiles

include(SyntroAlert.pri)
