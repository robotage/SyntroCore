//
//  Copyright (c) 2014 Scott Ellis and Richard Barnett
//	
//  This file is part of SyntroControlLib
//
//  SyntroControlLib is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  SyntroControlLib is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with SyntroControlLib.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef SYNTROCONTROLLIB_H
#define SYNTROCONTROLLIB_H

#include "syntrocontrollib_global.h"

//		Timer intervals

#define	SYNTROSERVER_INTERVAL			(SYNTRO_CLOCKS_PER_SEC / 100)	// SyntroServer runs 100 times per second

#define	EXCHANGE_TIMEOUT				(5 * SYNTRO_CLOCKS_PER_SEC)	// multicast without ack timeout
#define	MULTICAST_REFRESH_TIMEOUT		(3 * SYNTRO_SERVICE_LOOKUP_INTERVAL) // time before stop passing back lookup refreshes


class SYNTROCONTROLLIB_EXPORT SyntroControlLib
{
public:
	SyntroControlLib();
	~SyntroControlLib();

private:

};

#endif // SYNTROCONTROLLIB_H
