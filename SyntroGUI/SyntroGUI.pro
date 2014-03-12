#
#  Copyright (c) 2014 Scott Ellis and Richard Barnett
#	
#  This file is part of SyntroGUI
#
#  SyntroGUI is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  SyntroGUI is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with SyntroGUI.  If not, see <http://www.gnu.org/licenses/>.
#

TEMPLATE = lib
TARGET = SyntroGUI

win32* {
	DESTDIR = Release
}

include(../version.pri)

# common mac or linux
unix {
	macx {
		target.path = /usr/local/lib
		headerfiles.path = /usr/local/include/syntro
	}
	else {
		target.path = /usr/lib
		headerfiles.path = /usr/include/syntro
	}

	headerfiles.files += *.h
	INSTALLS += headerfiles target
}

QT += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += shared

# No debug in release builds
unix:QMAKE_CXXFLAGS_RELEASE -= -g
QMAKE_CXXFLAGS += $$(QT_CXXFLAGS)

win32:DEFINES += _CRT_SECURE_NO_WARNINGS SYNTROGUI_LIB

INCLUDEPATH += GeneratedFiles \
    GeneratedFiles/Release \
    ../SyntroLib

win32* {
    LIBS += -L../SyntroLib/Release -lSyntroLib
}
else {
    LIBS += -L../SyntroLib -lSyntroLib
}

MOC_DIR += GeneratedFiles/release

OBJECTS_DIR += release

UI_DIR += GeneratedFiles

RCC_DIR += GeneratedFiles

include(SyntroGUI.pri)
