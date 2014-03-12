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

#ifndef _SYNTROAVDEFS_H
#define _SYNTROAVDEFS_H

#include "SyntroDefs.h"

//  Record header parameter field values

typedef enum
{
    SYNTRO_RECORDHEADER_PARAM_NOOP = 0,                     // indicates a filler record
	SYNTRO_RECORDHEADER_PARAM_REFRESH,						// indicates a refresh MJPEG frame
    SYNTRO_RECORDHEADER_PARAM_NORMAL,                       // indicates a normal record
    SYNTRO_RECORDHEADER_PARAM_PREROLL,                      // indicates a preroll frame
    SYNTRO_RECORDHEADER_PARAM_POSTROLL,                     // indicates a postroll frame
 } SYNTROAV_RECORDHEADER_PARAM;

//	AVMux subType codes

typedef enum
{
	SYNTRO_RECORD_TYPE_AVMUX_UNKNOWN = -1,					// unknown mux
	SYNTRO_RECORD_TYPE_AVMUX_MJPPCM,						// MJPEG + PCM interleaved
	SYNTRO_RECORD_TYPE_AVMUX_MP4,							// MP4 mux
	SYNTRO_RECORD_TYPE_AVMUX_OGG,							// Ogg mux
	SYNTRO_RECORD_TYPE_AVMUX_WEBM,							// Webm mux
	SYNTRO_RECORD_TYPE_AVMUX_RTP,							// RTP interleave format
	SYNTRO_RECORD_TYPE_AVMUX_RTPCAPS,						// RTP caps

	// This entry marks the end of the enum

	SYNTRO_RECORD_TYPE_AVMUX_END							// the end
} SYNTROAV_AVMUXSUBTYPE;


//	Video subType codes

typedef enum
{
	SYNTRO_RECORD_TYPE_VIDEO_UNKNOWN = -1,					// unknown format or not present
	SYNTRO_RECORD_TYPE_VIDEO_MJPEG,							// MJPEG compression
	SYNTRO_RECORD_TYPE_VIDEO_MPEG1,							// MPEG1 compression
	SYNTRO_RECORD_TYPE_VIDEO_MPEG2,							// MPEG2 compression
	SYNTRO_RECORD_TYPE_VIDEO_H264,							// H264 compression
	SYNTRO_RECORD_TYPE_VIDEO_VP8,							// VP8 compression
	SYNTRO_RECORD_TYPE_VIDEO_THEORA,						// theora compression
	SYNTRO_RECORD_TYPE_VIDEO_RTPMPEG4,						// mpeg 4 over RTP
	SYNTRO_RECORD_TYPE_VIDEO_RTPCAPS,						// RTP caps message

	// This entry marks the end of the enum

	SYNTRO_RECORD_TYPE_VIDEO_END							// the end
} SYNTROAV_VIDEOSUBTYPE;

//	Audio subType codes

typedef enum
{
	SYNTRO_RECORD_TYPE_AUDIO_UNKNOWN = -1,					// unknown format or not present
	SYNTRO_RECORD_TYPE_AUDIO_PCM,							// PCM/WAV uncompressed
	SYNTRO_RECORD_TYPE_AUDIO_MP3,							// MP3 compression
	SYNTRO_RECORD_TYPE_AUDIO_AC3,							// AC3 compression
	SYNTRO_RECORD_TYPE_AUDIO_AAC,							// AAC compression
	SYNTRO_RECORD_TYPE_AUDIO_VORBIS,						// Vorbis compression
	SYNTRO_RECORD_TYPE_AUDIO_RTPAAC,						// aac over RTP
	SYNTRO_RECORD_TYPE_AUDIO_RTPCAPS,						// RTP caps message

	// This entry marks the end of the enum

	SYNTRO_RECORD_TYPE_AUDIO_END							// the end
} SYNTROAV_AUDIOSUBTYPE;

//	SYNTRO_RECORD_AVMUX - used to send muxed video and audio data
//
//	The structure follows the SYNTRO_EHEAD structure. The lengths of the
//	three types of data (avmux, video and audio) describe what is present. The 
//	actual data is always in that order. So, if there's video and audio, the video would
//	be first and the audio second. The offset to the audio can be calculated from the
//	length of the video.
//
//	The header information is always set correctly regardless of what data is contained
//	so that any record can be used to determine the types of data that the stream carries.

typedef struct
{
	SYNTRO_RECORD_HEADER recordHeader;						// the record type header
	SYNTRO_UC2 spare0;										// unused at present - set to 0
	unsigned char videoSubtype;								// type of video stream
	unsigned char audioSubtype;								// type of audio stream
	SYNTRO_UC4 muxSize;										// size of the muxed data (if present)
	SYNTRO_UC4 videoSize;									// size of the video data (if present)
	SYNTRO_UC4 audioSize;									// size of the audio data (if present)
	SYNTRO_UC2 videoWidth;									// width of frame
	SYNTRO_UC2 videoHeight;									// height of frame
	SYNTRO_UC2 videoFramerate;								// framerate
	SYNTRO_UC2 videoSpare;									// unused at present - set to 0
	SYNTRO_UC4 audioSampleRate;								// sample rate of audio
	SYNTRO_UC2 audioChannels;								// number of channels
	SYNTRO_UC2 audioSampleSize;								// size of samples (in bits)
	SYNTRO_UC2 audioSpare;									// unused at present - set to 0
	SYNTRO_UC2 spare1;										// unused at present - set to 0
} SYNTRO_RECORD_AVMUX;

// A convenient version for use by code

typedef struct
{
	int avmuxSubtype;										// the mux subtype
	unsigned char videoSubtype;								// type of video stream
	unsigned char audioSubtype;								// type of audio stream
	int videoWidth;											// width of frame
	int videoHeight;										// height of frame
	int videoFramerate;										// framerate
	int audioSampleRate;									// sample rate of audio
	int audioChannels;										// number of channels
	int audioSampleSize;									// size of samples (in bits)
} SYNTRO_AVPARAMS;


//	SYNTRO_RECORD_VIDEO - used to send video frames
//
//	The structure follows the SYNTRO_EHEAD structure. Immediately following this structure
//	is the image data.

typedef struct
{
	SYNTRO_RECORD_HEADER recordHeader;						// the record type header
	SYNTRO_UC2 width;										// width of each image
	SYNTRO_UC2 height;										// height of each image
	SYNTRO_UC4 size;										// size of the image
} SYNTRO_RECORD_VIDEO;

//	SYNTRO_RECORD_AUDIO - used to send audio data
//
//	The structure follows the SYNTRO_EHEAD structure. Immediately following this structure
//	is the audio data.

typedef struct
{
	SYNTRO_RECORD_HEADER recordHeader;						// the record type header
	SYNTRO_UC4 size;										// size of the audio data
	SYNTRO_UC4 bitrate;										// bitrate of audio
	SYNTRO_UC4 sampleRate;									// sample rate of audio
	SYNTRO_UC2 channels;									// number of channels
	SYNTRO_UC2 sampleSize;									// size of samples (in bits)
} SYNTRO_RECORD_AUDIO;


//	SYNTRO_PTZ - used to send PTZ information
//
//	Data is sent in servo form, i.e. 1 (SYNTRO_SERVO_LEFT) to 0xffff (SYNTRO_SERVO_RIGHT).
//	SYNTRO_SERVER_CENTER means center camera in that axis. For zoom, it menas some default position.

typedef struct
{
	SYNTRO_UC2 pan;											// pan position
	SYNTRO_UC2 tilt;										// tilt position
	SYNTRO_UC2 zoom;										// zoom position
} SYNTRO_PTZ;



#endif	// _SYNTROAVDEFS_H

