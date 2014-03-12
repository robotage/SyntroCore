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

#ifndef _SYNTROCFSDEFS_H
#define _SYNTROCFSDEFS_H

//	SYNTRO_CFSHEADER is used on every SyntroCFS E2E message
//	cfsType is used to hold the message type code.
//	cfsParam depends on the cfsType field. If it is a response, this is a response code.
//	cfsIndex is the index of the record referred to in the req/res. Not always used.
//	cfsLength is the total length of data that follows the header. The type of data depends on cfsType.

//	Note about file handles and response codes
//
//	Since file handles and response codes can share the same field in some cases,
//	the top bit (bit 15) of the field is used to indicate which it is. Error response codes
//	have the top bit set. So, if the top bit is clear, this is a success response and the remaining
//	15 bits may contain the stream handle (on an open response). If the top bit is set, it is an error 
//	response code and the remaining 15 bits indicate the type.

typedef struct
{
	SYNTRO_UC2 cfsType;										// the message type
	SYNTRO_UC2 cfsParam;									// a parameter value used for response codes for example
	SYNTRO_UC2 cfsStoreHandle;								// the handle used to identify an open file at the SyntroCFS store
	SYNTRO_UC2 cfsClientHandle;								// the handle used to identify an open file at the client
	SYNTRO_UC4 cfsIndex;									// record index
	SYNTRO_UC4 cfsLength;									// length of data that follows this structure (record, stream name etc)
} SYNTRO_CFSHEADER;

typedef struct
{
	SYNTRO_CFSHEADER cfsHeader;
	SYNTRO_UC4 firstRow;
	SYNTRO_UC4 lastRow;
	SYNTRO_UC4 param1;
	SYNTRO_UC4 param2;
} SYNTRO_QUERYRESULT_HEADER;

//	SyntroCFS message type codes
//
//	Note: cfsLength is alsways used and must be set to zero if the message is just the SYNTRO_CFSHEADER

//	SYNTROCFS_TYPE_DIR_REQ is sent to a SyntroCFS store to request a list of available files.
//	cfsParam, cfsStoreHandle, cfsClientHandle, cfsIndex are not used. 

#define	SYNTROCFS_TYPE_DIR_REQ			0					// file directory request

//	SYNTROCFS_TYPE_DIR_RES is returned from a SyntroCFS store in response to a request.
//	cfsParam contains a response code. If it indicates success, the file names
//	follow the header as an XML record as a zero terminated string. 
//	cfsLength indicates the total length of the string including the terminating zero.
//	cfsIndex cfsStoreHandle, cfsClientHandle is not used.

#define	SYNTROCFS_TYPE_DIR_RES			1					// file directory response

//	SYNTROCFS_TYPE_OPEN_REQ is sent to a SyntroCFS to open a file.
//	cfsParam is used to hold the requested block size for flat files (up to 65535 bytes) or 0 for structured.
//	cfsClientHandle contains the handle to be used by the client for this stream.
//	cfsStoreHandle is unused.
//	cfsIndex is not used. 
//	The file path is a complete path relative to the store root (including subdirs and extension)
//	that follows the header. The total length of the file path (including the null) is in 
//	cfsLength.

#define	SYNTROCFS_TYPE_OPEN_REQ			2					// file open request

//	SYNTROCFS_TYPE_OPEN_RES is sent from the SyntroCFS in response to a request
//	cfsParam indicates the response code.
//	cfsClientHandle contains the handle to be used by the client for this file.
//	cfsStoreHandle contains the handle to be used by the store for this file.
//	cfsIndex is the number of records or blocks in the file.

#define	SYNTROCFS_TYPE_OPEN_RES			3					// file open response

//	SYNTROCFS_TYPE_CLOSE_REQ is sent to the SyntroCFS to close an open file
//	cfsParam is unused. 
//	cfsClientHandle contains the handle assigned to this file.
//	cfsStoreHandle contains the handle assigned to this file.
//	cfsIndex is unused.

//	cfsIndex and cfsLength are not used.

#define	SYNTROCFS_TYPE_CLOSE_REQ		4					// file close request

//	SYNTROCFS_TYPE_CLOSE_RES is send from the SyntroCFS in response to a close request
//	cfsParam contains the response code. If successful, this is the file handle that was closed
//	and no longer valid. If unsuccessful, this is the error code.
//	cfsClientHandle contains the handle assigned to this file.
//	cfsStoreHandle contains the handle assigned to this file.
//	cfsIndex is unused.

#define	SYNTROCFS_TYPE_CLOSE_RES		5					// file close response

//	SYNTROCFS_TYPE_KEEPALIVE_REQ is sent to the SyntroCFS to keep an open file alive
//	cfsParam contains the file handle.
//	cfsClientHandle contains the handle assigned to this file.
//	cfsStoreHandle contains the handle assigned to this file.
//	cfsIndex is not used.

#define	SYNTROCFS_TYPE_KEEPALIVE_REQ	6					// keep alive heartbeat request

//	SYNTROCFS_TYPE_KEEPALIVE_RES is sent from the SyntroCFS in response to a request
//	cfsParam contains the stream handle for an open file or an error code.
//	cfsClientHandle contains the handle assigned to this stream.
//	cfsStoreHandle contains the handle assigned to this stream.
//	cfsIndex is not used.

#define	SYNTROCFS_TYPE_KEEPALIVE_RES	6					// keep alive heartbeat response

//	SYNTROCFS_TYPE_READ_INDEX_REQ is sent to the SyntroCFS to request the record or block(s) at the index specified
//	cfsParam contains the number of blocks to be read (for flat files, ignored for structured).
//	cfsClientHandle contains the handle assigned to this file.
//	cfsStoreHandle contains the handle assigned to this file.
//	cfsIndex contains the record index to be read (structured) or the first block number to be read (flat).


#define	SYNTROCFS_TYPE_READ_INDEX_REQ	16					// requests a read of record or block starting at index n

//	SYNTROCFS_TYPE_READ_INDEX_RES is send from the SyntroCFS in response to a request.
//	cfsParam contains the file handle if successful, an error code otherwise.
//	cfsClientHandle contains the handle assigned to this file.
//	cfsStoreHandle contains the handle assigned to this file.
//	cfsIndex contains the record index (structured) or the first block number (flat file).
//	cfsLength indicates the total length of the record or block(s) that follows the header

#define	SYNTROCFS_TYPE_READ_INDEX_RES	17					// response to a read at index n - contains record or error code

//	SYNTROCFS_TYPE_WRITE_INDEX_REQ is sent to the SyntroCFS to write the record or block(s) at the appropriate place
//	cfsParam contains the number of blocks to be written (for flat files, ignored for structured).
//	cfsClientHandle contains the handle assigned to this file.
//	cfsStoreHandle contains the handle assigned to this file.
//	cfsIndex is 0 for reset file length to 0 before write. ANy other value means append.
//	cfsLength indicates the total length of the record or block(s) that follows the header.

#define	SYNTROCFS_TYPE_WRITE_INDEX_REQ	18					// requests a write of record or block(s) at end of file

//	SYNTROCFS_TYPE_WRITE_INDEX_RES is send from the SyntroCFS in response to a request.
//	cfsParam contains the stream handle if successful, an error code otherwise.
//	cfsClientHandle contains the handle assigned to this file.
//	cfsStoreHandle contains the handle assigned to this file.
//	cfsIndex contains the original index from the request.
//	cfsLength is unused.

#define	SYNTROCFS_TYPE_WRITE_INDEX_RES	19					// response to a write at index n - contains success or error code


#define SYNTROCFS_TYPE_QUERY_REQ		20
#define SYNTROCFS_TYPE_QUERY_RES		21
#define SYNTROCFS_TYPE_CANCEL_QUERY_REQ 22
#define SYNTROCFS_TYPE_CANCEL_QUERY_RES 23
#define SYNTROCFS_TYPE_FETCH_QUERY_REQ	24
#define SYNTROCFS_TYPE_FETCH_QUERY_RES	25

//	SyntroCFS Size Defines

#define	SYNTROCFS_MAX_CLIENT_FILES			32				// max files a client can have open at one time per EP

//	SyntroCFS Error Response codes

#define	SYNTROCFS_SUCCESS						0			// this is a success code as top bit is zero
#define	SYNTROCFS_ERROR_CODE					0x8000		// this is where error codes start

#define	SYNTROCFS_ERROR_SERVICE_UNAVAILABLE		(SYNTROCFS_ERROR_CODE + 0)	// service endpoint is not available
#define	SYNTROCFS_ERROR_REQUEST_ACTIVE			(SYNTROCFS_ERROR_CODE + 1)	// service request already active (usually directory)
#define	SYNTROCFS_ERROR_REQUEST_TIMEOUT 		(SYNTROCFS_ERROR_CODE + 2)	// service request timeout
#define	SYNTROCFS_ERROR_UNRECOGNIZED_COMMAND	(SYNTROCFS_ERROR_CODE + 3)	// cfsType wasn't recognized
#define	SYNTROCFS_ERROR_MAX_CLIENT_FILES		(SYNTROCFS_ERROR_CODE + 4)	// too many files open at client
#define	SYNTROCFS_ERROR_MAX_STORE_FILES			(SYNTROCFS_ERROR_CODE + 5)	// too many files open at store
#define	SYNTROCFS_ERROR_FILE_NOT_FOUND			(SYNTROCFS_ERROR_CODE + 6)	// file path wasn't in an open request
#define	SYNTROCFS_ERROR_INDEX_FILE_NOT_FOUND	(SYNTROCFS_ERROR_CODE + 7)	// couldn't find index file
#define	SYNTROCFS_ERROR_FILE_INVALID_FORMAT		(SYNTROCFS_ERROR_CODE + 8)	// file isn't a structured type
#define	SYNTROCFS_ERROR_INVALID_HANDLE			(SYNTROCFS_ERROR_CODE + 9)	// file handle not valid
#define	SYNTROCFS_ERROR_INVALID_RECORD_INDEX	(SYNTROCFS_ERROR_CODE + 10)	// record index beyond end of file
#define	SYNTROCFS_ERROR_READING_INDEX_FILE		(SYNTROCFS_ERROR_CODE + 11)	// read of index file failed
#define	SYNTROCFS_ERROR_RECORD_SEEK				(SYNTROCFS_ERROR_CODE + 12)	// record file seek failed
#define	SYNTROCFS_ERROR_RECORD_READ				(SYNTROCFS_ERROR_CODE + 13)	// record file read failed
#define	SYNTROCFS_ERROR_INVALID_HEADER			(SYNTROCFS_ERROR_CODE + 14)	// record header is invalid
#define	SYNTROCFS_ERROR_INDEX_WRITE				(SYNTROCFS_ERROR_CODE + 15) // index file write failed
#define	SYNTROCFS_ERROR_WRITE					(SYNTROCFS_ERROR_CODE + 16) // flat file write failed
#define	SYNTROCFS_ERROR_TRANSFER_TOO_LONG		(SYNTROCFS_ERROR_CODE + 17) // if message would be too long
#define	SYNTROCFS_ERROR_READ					(SYNTROCFS_ERROR_CODE + 18) // flat file read failed
#define SYNTROCFS_ERROR_BAD_BLOCKSIZE_REQUEST	(SYNTROCFS_ERROR_CODE + 19) // flat file access cannot request a block size < zero
#define SYNTROCFS_ERROR_INVALID_REQUEST_TYPE	(SYNTROCFS_ERROR_CODE + 20) // request type not valid for this cfs mode
#define SYNTROCFS_ERROR_DB_OPEN_FAILED			(SYNTROCFS_ERROR_CODE + 21)
#define SYNTROCFS_ERROR_MEMORY_ALLOC_FAIL		(SYNTROCFS_ERROR_CODE + 22)
#define SYNTROCFS_ERROR_INVALID_QUERY			(SYNTROCFS_ERROR_CODE + 23)
#define SYNTROCFS_ERROR_QUERY_FAILED			(SYNTROCFS_ERROR_CODE + 24)
#define SYNTROCFS_ERROR_QUERY_NOT_ACTIVE		(SYNTROCFS_ERROR_CODE + 25)
#define SYNTROCFS_ERROR_TYPE_NOT_SUPPORTED		(SYNTROCFS_ERROR_CODE + 26)
#define SYNTROCFS_ERROR_QUERY_RESULT_TYPE_NOT_SUPPORTED		(SYNTROCFS_ERROR_CODE + 27)


//	SyntroCFS Timer Values

#define	SYNTROCFS_DIRREQ_TIMEOUT				(SYNTRO_CLOCKS_PER_SEC * 5)	// how long to wait for a directory response
#define	SYNTROCFS_OPENREQ_TIMEOUT				(SYNTRO_CLOCKS_PER_SEC * 5)	// how long to wait for a file open response
#define	SYNTROCFS_READREQ_TIMEOUT				(SYNTRO_CLOCKS_PER_SEC * 5)	// how long to wait for a file read response
#define	SYNTROCFS_WRITEREQ_TIMEOUT				(SYNTRO_CLOCKS_PER_SEC * 5)	// how long to wait for a file write response
#define SYNTROCFS_QUERYREQ_TIMEOUT				(SYNTRO_CLOCKS_PER_SEC * 5)	
#define	SYNTROCFS_CLOSEREQ_TIMEOUT				(SYNTRO_CLOCKS_PER_SEC * 5)	// how long to wait for a file close response
#define	SYNTROCFS_KEEPALIVE_INTERVAL			(SYNTRO_CLOCKS_PER_SEC * 5)	// interval between keep alives
#define	SYNTROCFS_KEEPALIVE_TIMEOUT				(SYNTROCFS_KEEPALIVE_INTERVAL * 3)	// at which point the file is considered closed

//	SyntroCFS E2E Message Priority
//	Always use this priority for SyntroCFS messages. Random selection could lead to re-ordering

#define	SYNTROCFS_E2E_PRIORITY					(SYNTROLINK_MEDHIGHPRI)

// SyntroCFS DIRREQ optional parameters
#define SYNTROCFS_DIR_PARAM_LIST_ALL			0
#define SYNTROCFS_DIR_PARAM_LIST_DB_ONLY		1

// SyntroCFS OPENREQ parameters for cfsType
#define SYNTROCFS_TYPE_STRUCTURED_FILE          0
#define SYNTROCFS_TYPE_RAW_FILE                 1
#define SYNTROCFS_TYPE_DATABASE					2

#define SYNTROCFS_QUERY_RESULT_TYPE_ROW_DATA	0
#define SYNTROCFS_QUERY_RESULT_TYPE_AV_DATA		1

#define SYNTROCFS_QUERY_RESULTS_NO_MORE_DATA		0x01
#define SYNTROCFS_QUERY_RESULTS_HAVE_COLUMN_NAMES	0x02
#define SYNTROCFS_QUERY_RESULTS_SIZE_LIMITED		0x04

#endif	// _SYNTRODEFS_H
