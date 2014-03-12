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

#ifndef SYNTRORECORD_H
#define SYNTRORECORD_H

//		Store format options

#define	SYNTRO_RECORD_STORE_FORMAT_SRF		"srf"			// structured record file
#define	SYNTRO_RECORD_STORE_FORMAT_RAW		"raw"			// raw file format (just data - no record headers on any sort)

//	Flat file defs

#define	SYNTRO_RECORD_FLAT_EXT				"dat"
#define	SYNTRO_RECORD_FLAT_FILTER			"*.dat"

//	SRF defs

#define	SYNC_LENGTH		8									// 8 bytes in sync sequence

#define	SYNC_STRINGV0	"SpRSHdV0"							// for version 0

#define	SYNTRO_RECORD_SRF_RECORD_EXT	"srf"				// file extension for record files
#define	SYNTRO_RECORD_SRF_INDEX_EXT		"srx"				// file extension for index files
#define	SYNTRO_RECORD_SRF_RECORD_DOTEXT	".srf"				// file extension for record files with .
#define	SYNTRO_RECORD_SRF_INDEX_DOTEXT	".srx"				// file extension for index files with .
#define	SYNTRO_RECORD_SRF_RECORD_FILTER	"*.srf"				// filter for record files
#define	SYNTRO_RECORD_SRF_INDEX_FILTER	"*.srx"				// filter extension for index files

//	The record header that's stored with a record in an srf file

typedef struct	
{
	char sync[SYNC_LENGTH];									// the sync sequence
	SYNTRO_UC4 size;										// size of the record that follows
	SYNTRO_UC4 data;										// unused at this time
} SYNTRO_STORE_RECORD_HEADER;

// SQL defs

#define SYNTRO_RECORD_SQL_VIDEO_FILE_DOTEXT	".dbv"

#endif // SYNTRORECORD_H

