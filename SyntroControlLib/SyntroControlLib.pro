#
#  Copyright (c) 2014 Scott Ellis and Richard Barnett
#	
#  This file is part of SyntroControlLib
#
#  SyntroControlLib is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  SyntroControlLib is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with SyntroControlLib.  If not, see <http://www.gnu.org/licenses/>.
#

TEMPLATE = lib
TARGET = SyntroControlLib

win32* {
	DESTDIR = Release 
}

include(../version.pri)


# common mac or linux
unix {
	macx {
		target.path = /usr/local/lib
		headerfiles.path = /usr/local/include/syntro/SyntroControlLib
	}
	else {
        	target.path = /usr/lib
		headerfiles.path = /usr/include/syntro/SyntroControlLib
	}

	headerfiles.files += *.h
	INSTALLS += headerfiles target
}

QT += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += shared

# No debug info in release builds
unix:QMAKE_CXXFLAGS_RELEASE -= -g
QMAKE_CXXFLAGS += $$(QT_CXXFLAGS)

DEFINES += QT_NETWORK_LIB

win32:DEFINES += _CRT_SECURE_NO_WARNINGS SYNTROCONTROLLIB_LIB

INCLUDEPATH += GeneratedFiles \
    GeneratedFiles/release \
    ../SyntroLib

win32* {
    LIBS += -L../SyntroLib/Release -lSyntroLib
} else {
    LIBS += -L../SyntroLib -lSyntroLib
}

MOC_DIR += GeneratedFiles/release

OBJECTS_DIR += release

UI_DIR += GeneratedFiles

RCC_DIR += GeneratedFiles

include(SyntroControlLib.pri)
