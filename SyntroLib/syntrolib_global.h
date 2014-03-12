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

#ifndef SYNTROLIB_GLOBAL_H
#define SYNTROLIB_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined Q_OS_WIN
	#if defined SYNTRO_STATIC_LIB
		#define SYNTROLIB_EXPORT
    #elif defined SYNTROLIB_LIB
        #define SYNTROLIB_EXPORT Q_DECL_EXPORT
    #else
        #define SYNTROLIB_EXPORT Q_DECL_IMPORT
    #endif
#else
    #define SYNTROLIB_EXPORT
#endif

#endif // SYNTROLIB_GLOBAL_H
