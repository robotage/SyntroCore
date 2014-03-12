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

HEADERS += CFSClient.h \
    CFSThread.h \
    ConfigurationDlg.h \
    DirThread.h \
    StoreClient.h \
    StoreManager.h \
    StoreStream.h \
    StoreStreamDlg.h \
    SyntroCFS.h \
    SyntroCFSRaw.h \
    SyntroCFSStructured.h \
    SyntroDB.h \
    SyntroDBConsole.h \
    SyntroStore.h \
    SyntroStoreBlocksRaw.h \
    SyntroStoreBlocksStructured.h

SOURCES += CFSClient.cpp \
    CFSThread.cpp \
    ConfigurationDlg.cpp \
    DirThread.cpp \
    main.cpp \
    StoreClient.cpp \
    StoreManager.cpp \
    StoreStream.cpp \
    StoreStreamDlg.cpp \    
    SyntroCFS.cpp \
    SyntroCFSRaw.cpp \
    SyntroCFSStructured.cpp \
    SyntroDB.cpp \
    SyntroDBConsole.cpp \
    SyntroStore.cpp \
    SyntroStoreBlocksRaw.cpp \
    SyntroStoreBlocksStructured.cpp

FORMS += SyntroDB.ui
