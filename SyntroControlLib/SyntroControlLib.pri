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

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

HEADERS += $$PWD/SyntroTunnel.h \
    $$PWD/FastUIDLookup.h \
    $$PWD/SyntroControlLib.h \
    $$PWD/syntrocontrollib_global.h \
    $$PWD/SyntroServer.h \
    $$PWD/DirectoryManager.h \
    $$PWD/MulticastManager.h
SOURCES += $$PWD/DirectoryManager.cpp \
    $$PWD/FastUIDLookup.cpp \
    $$PWD/MulticastManager.cpp \
    $$PWD/SyntroControLlib.cpp \
    $$PWD/SyntroServer.cpp \
    $$PWD/SyntroTunnel.cpp
