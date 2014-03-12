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

#ifndef _SYNTRODEFS_H
#define _SYNTRODEFS_H

//	Version

#define	SYNTROLIB_VERSION	"RT1.0.0"

//	Timer definition
//
//	SYNTRO_CLOCKS_PER_SEC deifnes how many clock ticks are in one second

#define SYNTRO_CLOCKS_PER_SEC		1000					// 1000 ticks of mS clock equals one second

//-------------------------------------------------------------------------------------------
//	Reserved characters and their functions
//

#define	SYNTRO_SERVICEPATH_SEP		'/'						// the path component separator character
#define	SYNTRO_STREAM_TYPE_SEP		':'						// used to delimit a stream type ina path
#define	SYNTROCFS_FILENAME_SEP		';'						// used to separate file paths in a directory string
#define	SYNTRO_LOG_COMPONENT_SEP	'|'						// used to separate parts of log messages

//-------------------------------------------------------------------------------------------
//	The defines below are the most critical Syntro system definitions.
//

//	Syntro constants

#define	SYNTRO_HEARTBEAT_INTERVAL		2					// default heartbeatinterval in seconds 
#define	SYNTRO_HEARTBEAT_TIMEOUT		3					// default number of missed heartbeats before timeout
#define	SYNTRO_LOG_HEARTBEAT_INTERVAL	5					// default log heartbeatinterval in seconds 
#define	SYNTRO_LOG_HEARTBEAT_TIMEOUT	3					// default number of missed heartbeats before timeout for log

#define	SYNTRO_SERVICE_LOOKUP_INTERVAL		(5 * SYNTRO_CLOCKS_PER_SEC)	// 5 seconds interval between service lookup requests

#define	SYNTRO_SOCKET_LOCAL		1661						// socket for the SyntroControl

#define	SYNTRO_PRIMARY_SOCKET_STATICTUNNEL	1808			// socket for primary static SyntroControl tunnels
#define	SYNTRO_BACKUP_SOCKET_STATICTUNNEL	1809			// socket for backup static SyntroControl tunnels

#define	SYNTRO_MAX_CONNECTEDCOMPONENTS	512					// maximum directly connected components to a component
#define	SYNTRO_MAX_CONNECTIONIDS		(SYNTRO_MAX_CONNECTEDCOMPONENTS * 2) // used to uniquely identify sockets
#define	SYNTRO_MAX_COMPONENTSPERDEVICE	32					// maximum number of components assigned to a single device
#define	SYNTRO_MAX_SERVICESPERCOMPONENT	128					// max number of services in a component	
															// max possible multicast maps
#define	SYNTRO_MAX_DELENGTH				4096				// maximum possible directory entry length

//	Syntro message size maximums

#define SYNTRO_MESSAGE_MAX			0x80000

//-------------------------------------------------------------------------------------------
//	IP related definitions

#define	SYNTRO_IPSTR_LEN		16							// size to use for IP string buffers (xxx.xxx.xxx.xxx plus 0)

#define	SYNTRO_IPADDR_LEN		4							// 4 byte IP address
typedef unsigned char SYNTRO_IPADDR[SYNTRO_IPADDR_LEN] ;	// a convenient type for IP addresses

//-------------------------------------------------------------------------------------------
//	Some general purpose typedefs - used esepcially for transferring values greater than
//	8 bits across the network. 
//

typedef	unsigned char SYNTRO_UC2[2];						// an array of two unsigned chars (short)
typedef	unsigned char SYNTRO_UC4[4];						// an array of four unsigned chars (int)
typedef	unsigned char SYNTRO_UC8[8];						// an array of eight unsigned chars (int64)

//-------------------------------------------------------------------------------------------
//	Some useful defines for MAC addresses
//

#define	SYNTRO_MACADDR_LEN	6								// length of a MAC address

typedef unsigned char SYNTRO_MACADDR[SYNTRO_MACADDR_LEN];	// a convenient type for MAC addresses
#define	SYNTRO_MACADDRSTR_LEN		(SYNTRO_MACADDR_LEN*2+1)// the hex string version
typedef	char SYNTRO_MACADDRSTR[SYNTRO_MACADDRSTR_LEN];

//-------------------------------------------------------------------------------------------
//	The Syntro UID (Unique Identifier) consists of a 6 byte MAC address (or equivalent)
//	along with a two byte instance number to ensure uniqueness on a device.
//

typedef	struct
{
	SYNTRO_MACADDR macAddr;									// the MAC address
	SYNTRO_UC2 instance;									// the instance number
} SYNTRO_UID;

//	These defs are for a string version of the SYNTRO_UID

#define	SYNTRO_UIDSTR_LEN	(sizeof(SYNTRO_UID) * 2 + 1)	// length of string version as hex pairs plus 0
typedef char SYNTRO_UIDSTR[SYNTRO_UIDSTR_LEN];				// and for the string version

//-------------------------------------------------------------------------------------------
//	Service Path Syntax
//
//	When a component wishes to communicate with a service on another component, it needs to locate it using
//	a service path string. This is mapped by SyntroControl into a UID and port number. 
//	The service string can take various forms (note that case is important):
//
//	Form 1 -	Service Name. In this case, the service path is simply the service name. SyntroControl will
//				look up the name and stop at the first service with a matching name. for example, if the
//				service name is "video", the service path would be "video".
//
//	Form 2 -	Component name and service name. The service path is the component name, then "/" and
//				then the service name. SyntroControl will only match the service name against
//				the component with the specified component name. For example, if the service name is "video"
//				and the component name is "WebCam", the service path would be "WebCam/video". "*" is a wildcard
//				so that "*/video" is equivalent to "video".
//
//	Form 3 -	Region name, component name and service name. The service path consists of a region name, then
//				a "/", then the cmoponent name, then a "/" and then the sevice name. As an example, if the 
//				region name is "Robot1", the component name "WebCam" and the service name "video", the
//				service path would be "Robot1/WebCam/video". Again, "*" is a wildcard for regions and
//				components so that "Robot1/*/video" would match the first service called "video" found in region
//				"Robot1".

//	Config and Directory related definitions
//

#define	SYNTRO_MAX_TAG			256							// maximum length of tag string (including 0 at end)
#define	SYNTRO_MAX_NONTAG		1024						// max length of value (including 0 at end)

#define	SYNTRO_MAX_NAME			32							// general max name length

#define	SYNTRO_MAX_APPNAME		SYNTRO_MAX_NAME				// max length of a zero-terminated app name
#define	SYNTRO_MAX_APPTYPE		SYNTRO_MAX_NAME				// max length of a zero-terminated app type
#define	SYNTRO_MAX_COMPTYPE		SYNTRO_MAX_NAME				// max length of a zero-terminated component type
#define	SYNTRO_MAX_SERVNAME		SYNTRO_MAX_NAME				// max length of a service name
#define	SYNTRO_MAX_REGIONNAME	SYNTRO_MAX_NAME				// max length of a region name
#define	SYNTRO_MAX_SERVPATH		128							// this is max size of the NULL terminated path for service paths

typedef	char	SYNTRO_SERVNAME[SYNTRO_MAX_SERVNAME];		// the service type
typedef	char	SYNTRO_REGIONNAME[SYNTRO_MAX_REGIONNAME];	// the region name type
typedef	char	SYNTRO_APPNAME[SYNTRO_MAX_APPNAME];			// the app name
typedef	char	SYNTRO_APPTYPE[SYNTRO_MAX_APPNAME];			// the app type
typedef	char	SYNTRO_COMPTYPE[SYNTRO_MAX_COMPTYPE];		// the component type type

typedef char	SYNTRO_SERVPATH[SYNTRO_MAX_SERVPATH];		// the service path type


//-------------------------------------------------------------------------------------------
//	SyntroCore App Type defs

#define	APPTYPE_CONTROL			"SyntroControl"								
#define APPTYPE_DB				"SyntroDB"
#define APPTYPE_LOG				"SyntroLog"
#define APPTYPE_FT				"SyntroFT"

//-------------------------------------------------------------------------------------------
//	SyntroCore Component Type defs

#define	COMPTYPE_CONTROL		"Control"							
#define COMPTYPE_STORE			"Store"
#define COMPTYPE_CFS			"CFS"
#define COMPTYPE_DB				"DB"
#define COMPTYPE_FT				"FT"
#define COMPTYPE_VIEW			"View"
#define COMPTYPE_REVIEW			"Review"
#define COMPTYPE_LOGSOURCE		"LogSource"
#define COMPTYPE_LOGSINK		"LogSink"
#define COMPTYPE_CAMERA			"Camera"

//-------------------------------------------------------------------------------------------
//	SyntroControl Directory Entry Tag Defs
//
//	A directory record as transmitted betweeen components and SyntroControls looks like:
//
//<CMP>
//<UID>xxxxxxxxxxxxxxxxxx</UID>
//<NAM>name</NAME>											// component name 
//<TYP>type</NAME>											// component type 
//...
//</CMP>
//<CMP>
//...
//</CMP>
//...


//	These tags are the component container tags

#define	DETAG_COMP				"CMP"
#define	DETAG_COMP_END			"/CMP"

//	Component Directory Entry Tag Defs

#define	DETAG_APPNAME			"NAM"						// the app name
#define	DETAG_COMPTYPE			"TYP"						// a string identifying the component
#define	DETAG_UID				"UID"						// the UID
#define	DETAG_MSERVICE			"MSV"						// a string identifying a multicast service
#define	DETAG_ESERVICE			"ESV"						// a string identifying an E2E service
#define	DETAG_NOSERVICE			"NSV"						// a string identifying an empty service slot

//	E2E service code standard endpoint service names

#define	DE_E2ESERVICE_PARAMS	"Params"					// parameter deployment service
#define	DE_E2ESERVICE_SYNTROCFS	"SyntroCFS"					// SyntroCFS service

//	Service type codes

#define	SERVICETYPE_MULTICAST	0							// a multicast service
#define	SERVICETYPE_E2E			1							// an end to end service
#define	SERVICETYPE_NOSERVICE	2							// a code indicating no service

//-------------------------------------------------------------------------------------------
//	Syntro message types
//

//	HEARTBEAT
//	This message which is just the SYNTROMSG itself is sent regular by both parties
//	in a SyntroLink. It's used to ensure correct operation of the link and to allow
//	the link to be re-setup if necessary. The message itself is a Hello data structure -
//	the same as is sent on the Hello system.
//
//	The HELLO structure that forms the message may also be followed by a properly
//	formatted directory entry (DE) as described above. If there is nothing present,
//	this means that DE for the component hasn't changed. Otherwise, the DE is used by the
//	receiving SyntroControl as the new DE for the component.

#define	SYNTROMSG_HEARTBEAT					1

//	DE_UPDATE
//	This message is sent by a Component to the SyntroControl in order to transfer a DE.
//	Note that the message is sent as a null terminated string.

#define	SYNTROMSG_SERVICE_LOOKUP_REQUEST	2

//	SERVICE_LOOKUP_RESPONSE
//	This message is sent back to a component with the results of the lookup.
//	The relevant fields are filled in the SYNTRO_SERVICE_LOOKUP structure.

#define	SYNTROMSG_SERVICE_LOOKUP_RESPONSE	3

//	DIRECTORY_REQUEST
//	An appliation can request a copy of the directory using this message.
//	There are no parameters or data - the message is just a SYNTRO_MESSAGE

#define SYNTROMSG_DIRECTORY_REQUEST			4

//	DIRECTORY_RESPONSE
//	This message is sent to an application in response to a request.
//	The message consists of a SYNTRO_DIRECTORY_RESPONSE structure.

#define SYNTROMSG_DIRECTORY_RESPONSE		5

//	SERVICE_ACTIVATE
//	This message is sent by a SyntroControl to an Endpoint multicast service when the Endpoint
//	multicast should start generating data as someone has subscribed to its service.

#define	SYNTROMSG_SERVICE_ACTIVATE			6

//	MULTICAST_FRAME
//	Multicast frames are sent using this message. The data is the parameter

#define	SYNTROMSG_MULTICAST_MESSAGE			16

//	MULTICAST_ACK
//	This message is sent to acknowledge a multicast and request the next

#define	SYNTROMSG_MULTICAST_ACK				17

//	E2E - Endpoint to Endpoint message

#define	SYNTROMSG_E2E						18

#define	SYNTROMSG_MAX						18				// highest legal message value

//-------------------------------------------------------------------------------------------
//	SYNTRO_MESSAGE - the structure that defines the object transferred across
//	the SyntroLink - the component to component header. This structure must 
//	be the first entry in every message header.
//

//	The structure itself

typedef struct
{
	unsigned char cmd;										// the type of message
	SYNTRO_UC4 len;											// message length (includes the SYNTRO_MESSAGE itself and everything that follows)
	unsigned char flags;									// used to send priority
	unsigned char spare;									// to put on 32 bit boundary
	unsigned char cksm;										// checksum = 256 - sum of previous bytes as chars
} SYNTRO_MESSAGE;

//	SYNTRO_EHEAD - Endpoint header
//	
//	This is used to send messages between specific services within components.
//	seq is used to control the acknowledgement window. It starts off at zero
//	and increments with each new message. Acknowledgements indicate the next acceptable send
//	seq and so open the window again.

#define	SYNTRO_MAX_WINDOW	4								// the maximum number of outstanding messages

typedef struct
{
	SYNTRO_MESSAGE syntroMessage;							// the SyntroLink header
	SYNTRO_UID sourceUID;									// source component of the endpoint message
	SYNTRO_UID destUID;										// dest component of the message
	SYNTRO_UC2 sourcePort;									// the source port number
	SYNTRO_UC2 destPort;									// the destination port number
	unsigned char seq;										// seq number of message
	unsigned char par0;										// an application-specific parameter (for E2E use)				
	unsigned char par1;
	unsigned char par2;										// make a multiple of 32 bits
} SYNTRO_EHEAD;

//-------------------------------------------------------------------------------------------
//	The SYNTRO_SERVICE_LOOKUP structure

#define	SERVICE_LOOKUP_INTERVAL	(SYNTRO_CLOCKS_PER_SEC * 2)	 // while waiting
#define	SERVICE_REFRESH_INTERVAL (SYNTRO_CLOCKS_PER_SEC * 4) // when registered
#define	SERVICE_REFRESH_TIMEOUT	(SERVICE_REFRESH_INTERVAL * 3) // Refresh timeout period
#define	SERVICE_REMOVING_INTERVAL (SYNTRO_CLOCKS_PER_SEC * 4) // when closing
#define	SERVICE_REMOVING_MAX_RETRIES		2				// number of times an endpoint retries closing a remote service

#define	SERVICE_LOOKUP_FAIL		0							// not found
#define	SERVICE_LOOKUP_SUCCEED	1							// found and response fields filled in
#define	SERVICE_LOOKUP_REMOVE	2							// this is used to remove a multicast registration

typedef struct
{
	SYNTRO_MESSAGE syntroMessage;							// the SyntroLink header
	SYNTRO_UID lookupUID;									// the returned UID of the service
	SYNTRO_UC4 ID;											// the returned ID for this entry (to detect restarts requiring re-regs)
	SYNTRO_SERVPATH servicePath;							// the service path string to be looked up
	SYNTRO_UC2 remotePort;									// the returned port to use for the service - the remote index for the service
	SYNTRO_UC2 componentIndex;								// the returned component index on SyntroControl (for fast refreshes)
	SYNTRO_UC2 localPort;									// the port number of the requestor - the local index for the service
	unsigned char serviceType;								// the service type requested
	unsigned char response;									// the response code
} SYNTRO_SERVICE_LOOKUP;

typedef struct
{
	SYNTRO_MESSAGE syntroMessage;							// the message header
	SYNTRO_UC2 endpointPort;								// the endpoint service port to which this is directed
	SYNTRO_UC2 componentIndex;								// the returned component index on SyntroControl (for fast refreshes)
	SYNTRO_UC2 syntroControlPort;							// the SyntroControl port to send the messages to
	unsigned char response;									// the response code
} SYNTRO_SERVICE_ACTIVATE;


//	SYNTROMESSAGE nFlags masks

#define	SYNTROLINK_PRI			0x03						// bits 0 and 1 are priority bits

#define	SYNTROLINK_PRIORITIES	4							// four priority levels

#define	SYNTROLINK_HIGHPRI		0							// highest priority - typically for real time control data
#define	SYNTROLINK_MEDHIGHPRI	1
#define	SYNTROLINK_MEDPRI		2
#define	SYNTROLINK_LOWPRI		3							// lowest priority - typically for multicast information

//-------------------------------------------------------------------------------------------
//	SYNTRO_DIRECTORY_RESPONSE - the directory response message structure
//

typedef struct
{
	SYNTRO_MESSAGE syntroMessage;							// the message header
															// the directory string follows
} SYNTRO_DIRECTORY_RESPONSE;

//	Standard multicast stream names

#define SYNTRO_STREAMNAME_AVMUX				"avmux"
#define	SYNTRO_STREAMNAME_AVMUXLR			"avmux:lr"
#define	SYNTRO_STREAMNAME_AVMUXMOBILE		"avmuxmobile"
#define SYNTRO_STREAMNAME_VIDEO				"video"
#define	SYNTRO_STREAMNAME_VIDEOLR			"video:lr"
#define SYNTRO_STREAMNAME_AUDIO				"audio"
#define SYNTRO_STREAMNAME_NAV				"nav"
#define SYNTRO_STREAMNAME_LOG				"log"
#define SYNTRO_STREAMNAME_SENSOR			"sensor"
#define SYNTRO_STREAMNAME_TEMPERATURE		"temperature"
#define SYNTRO_STREAMNAME_HUMIDITY			"humidity"
#define SYNTRO_STREAMNAME_LIGHT				"light"
#define SYNTRO_STREAMNAME_MOTION			"motion"
#define SYNTRO_STREAMNAME_AIRQUALITY		"airquality"
#define SYNTRO_STREAMNAME_PRESSURE			"pressure"
#define SYNTRO_STREAMNAME_ACCELEROMETER		"accelerometer"
#define SYNTRO_STREAMNAME_ZIGBEE_MULTICAST  "zbmc"
#define SYNTRO_STREAMNAME_HOMEAUTOMATION	"ha"
#define SYNTRO_STREAMNAME_THUMBNAIL         "thumbnail"

//	Standard E2E stream names

#define SYNTRO_STREAMNAME_ZIGBEE_E2E	    "zbe2e"
#define SYNTRO_STREAMNAME_CONTROL			"control"
#define SYNTRO_STREAMNAME_FILETRANSFER		"filetransfer"
#define SYNTRO_STREAMNAME_PTZ				"ptz"
#define	SYNTRO_STREAMNAME_CFS				"cfs"


//-------------------------------------------------------------------------------------------
//	Syntro Record Headers
//
//	Record headers must always have the record type as the first field.
//

typedef struct
{
	SYNTRO_UC2 type;										// the record type code
	SYNTRO_UC2 subType;										// the sub type code
	SYNTRO_UC2 headerLength;								// total length of specific record header
	SYNTRO_UC2 param;										// type specific parameter
	SYNTRO_UC2 param1;										// another one
	SYNTRO_UC2 param2;										// and another one
	SYNTRO_UC4 recordIndex;									// a monotonically incerasing index number
	SYNTRO_UC8 timestamp;									// timestamp for the sample
} SYNTRO_RECORD_HEADER;

//	Major type codes

#define	SYNTRO_RECORD_TYPE_VIDEO		0					// a video record
#define	SYNTRO_RECORD_TYPE_AUDIO		1					// an audio record
#define	SYNTRO_RECORD_TYPE_NAV			2					// navigation data
#define SYNTRO_RECORD_TYPE_LOG			3					// log data
#define SYNTRO_RECORD_TYPE_SENSOR		4					// multiplexed sensor data
#define SYNTRO_RECORD_TYPE_TEMPERATURE	5					// temperature sensor
#define SYNTRO_RECORD_TYPE_HUMIDITY		6					// humidity sensor
#define SYNTRO_RECORD_TYPE_LIGHT		7					// light sensor
#define SYNTRO_RECORD_TYPE_MOTION		8					// motion detection events
#define SYNTRO_RECORD_TYPE_AIRQUALITY	9					// air quality sensor
#define SYNTRO_RECORD_TYPE_PRESSURE		10					// air pressure sensor
#define SYNTRO_RECORD_TYPE_ZIGBEE		11					// zigbee multicast data
#define	SYNTRO_RECORD_TYPE_AVMUX		12					// an avmux stream record

#define	SYNTRO_RECORD_TYPE_USER		(0x8000)				// user defined codes start here

//	Navigation subType codes

#define	SYNTRO_RECORD_TYPE_NAV_IMU			0				// IMU data
#define	SYNTRO_RECORD_TYPE_NAV_GPS			1				// GPS data
#define	SYNTRO_RECORD_TYPE_NAV_ODOMETRY		2				// odometry data

//----------------------------------------------------------
//
//	Defs for servo actuators

#define	SYNTRO_SERVO_CENTER			0x8000					// middle of servo range
#define	SYNTRO_SERVO_RANGE			0x7fff					// servo value range from center

#define	SYNTRO_SERVO_LEFT_END		(SYNTRO_SERVO_CENTER - SYNTRO_SERVO_RANGE)	// left end of servo range
#define	SYNTRO_SERVO_RIGHT_END		(SYNTRO_SERVO_CENTER + SYNTRO_SERVO_RANGE)	// right end of servo range

#endif	// _SYNTRODEFS_H

