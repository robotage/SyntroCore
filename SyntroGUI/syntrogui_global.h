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

#ifndef SYNTROGUI_GLOBAL_H
#define SYNTROGUI_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined Q_OS_WIN
	#if defined SYNTRO_STATIC_LIB
		#define SYNTROGUI_EXPORT
	#elif defined SYNTROGUI_LIB
		#define SYNTROGUI_EXPORT Q_DECL_EXPORT
	#else
		#define SYNTROGUI_EXPORT Q_DECL_IMPORT
	#endif
#else
	#define SYNTROGUI_EXPORT
#endif

#endif // SYNTROGUI_GLOBAL_H
