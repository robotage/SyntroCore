#
#  Copyright (c) 2014 Scott Ellis and Richard Barnett
#	
#  This file is part of SyntroLib
#
#  SyntroLib is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  SyntroLib is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with SyntroLib.  If not, see <http://www.gnu.org/licenses/>.
#

INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

HEADERS += $$PWD/Endpoint.h \
    $$PWD/HelloDefs.h \
    $$PWD/SyntroDefs.h \
    $$PWD/SyntroLib.h \
    $$PWD/syntrolib_global.h \
    $$PWD/SyntroLink.h \
    $$PWD/SyntroUtils.h \
    $$PWD/Hello.h \
    $$PWD/SyntroThread.h \
    $$PWD/SyntroSocket.h \
    $$PWD/SyntroClock.h \
    $$PWD/LogWrapper.h \
    $$PWD/Logger.h \
    $$PWD/SyntroComponentData.h \
    $$PWD/DirectoryEntry.h \
    $$PWD/ChangeDetector.h

SOURCES += $$PWD/Endpoint.cpp \
    $$PWD/Hello.cpp \
    $$PWD/SyntroLink.cpp \
    $$PWD/SyntroSocket.cpp \
    $$PWD/SyntroThread.cpp \
    $$PWD/SyntroUtils.cpp \
    $$PWD/SyntroClock.cpp \
    $$PWD/LogWrapper.cpp \
    $$PWD/Logger.cpp \
    $$PWD/SyntroComponentData.cpp \
    $$PWD/DirectoryEntry.cpp \
    $$PWD/ChangeDetector.cpp

